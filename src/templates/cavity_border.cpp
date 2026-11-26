#include "src/templates/cavity_border.h"

// ═══════════════════════════════════════════════════════════════════════════
// Cavity border boards
// ═══════════════════════════════════════════════════════════════════════════

std::optional<Plane> compute_cavity_bisector_plane(
    const Point& corner_vertex, const Vector& facet_normal, const Vector& other_normal
) {

    const Vector diff = other_normal - facet_normal;

    if (diff.magnitude() < 1e-6)
        return std::nullopt;

    return Plane::from_point_normal(corner_vertex, diff.normalized());
}

Plane tighten_cavity_cap(
    const Plane& cap,
    const std::optional<Plane>& bisector,
    double setback,
    const Plane& cavity_inner,
    const Plane& cavity_outer,
    const Plane& board_inner,
    const Plane& board_outer,
    const Vector& u_out
) {

    if (!bisector)
        return cap;

    const Plane bis_cap = bisector->translate_by_normal(-setback);
    const std::array<Plane, 2> b_planes = {board_inner, board_outer};
    const std::array<Plane, 2> c_planes = {cavity_inner, cavity_outer};
    Point r0[2][2];
    double m[2][2] = {{0.0, 0.0}, {0.0, 0.0}};
    bool cap_all = true;
    bool bis_all = true;

    for (size_t ui = 0; ui < 2; ui++)
        for (size_t vi = 0; vi < 2; vi++) {
            const std::optional<Point> qc =
                Intersection::plane_plane_plane(c_planes[vi], b_planes[ui], cap);
            const std::optional<Point> qb =
                Intersection::plane_plane_plane(c_planes[vi], b_planes[ui], bis_cap);

            if (!qc || !qb)
                return cap;

            const double tc = Vector{(*qc)[0], (*qc)[1], (*qc)[2]}.dot(u_out);
            const double tb = Vector{(*qb)[0], (*qb)[1], (*qb)[2]}.dot(u_out);
            cap_all = cap_all && (tc <= tb + 1e-6);
            bis_all = bis_all && (tb <= tc + 1e-6);
            r0[ui][vi] = *qc - u_out * tc;
            m[ui][vi] = std::min(tc, tb);
        }

    if (cap_all)
        return cap;

    if (bis_all)
        return bis_cap;

    const double delta = (m[1][0] + m[0][1]) - (m[0][0] + m[1][1]);
    const Point p00 = r0[0][0] + u_out * (delta >= 0.0 ? m[0][0] : m[0][0] + 0.5 * delta);
    const Point p10 = r0[1][0] + u_out * (delta >= 0.0 ? m[1][0] - 0.5 * delta : m[1][0]);
    const Point p01 = r0[0][1] + u_out * (delta >= 0.0 ? m[0][1] - 0.5 * delta : m[0][1]);
    Vector normal = (p10 - p00).cross(p01 - p00);

    if (normal.magnitude() < 1e-6)
        return cap;

    normal = normal.normalized();

    if (normal.dot(u_out) < 0.0)
        normal = -normal;

    return Plane::from_point_normal(p00, normal);
}

std::optional<Plane> compute_cavity_border_cap(
    const Plane& cavity_mid,
    const Plane& boundary_plane,
    double outer_gap,
    double thickness,
    const Plane& neighbor_plane,
    bool miter,
    double neighbor_gap,
    double neighbor_thickness,
    double setback
) {

    if (!miter)
        return neighbor_plane.translate_by_normal(-setback);

    const Plane board_outer = boundary_plane.translate_by_normal(-outer_gap);
    const Plane board_inner = boundary_plane.translate_by_normal(-(outer_gap + thickness));
    const Plane nb_outer = neighbor_plane.translate_by_normal(-neighbor_gap);
    const Plane nb_inner = neighbor_plane.translate_by_normal(-(neighbor_gap + neighbor_thickness));
    const std::optional<Point> p_out =
        Intersection::plane_plane_plane(board_outer, nb_outer, cavity_mid);
    const std::optional<Point> p_in =
        Intersection::plane_plane_plane(board_inner, nb_inner, cavity_mid);
    const Vector axis = boundary_plane.z_axis().cross(neighbor_plane.z_axis());

    if (!p_out || !p_in || axis.magnitude() < 1e-6)
        return std::nullopt;

    Vector normal = axis.cross(*p_in - *p_out);

    if (normal.magnitude() < 1e-6)
        return std::nullopt;

    normal = normal.normalized();
    const Vector toward_corner =
        neighbor_plane.z_axis() -
        boundary_plane.z_axis() * boundary_plane.z_axis().dot(neighbor_plane.z_axis());

    if (normal.dot(toward_corner) < 0.0)
        normal = -normal;

    return Plane::from_point_normal(*p_out, normal).translate_by_normal(-setback);
}

std::optional<std::pair<Plane, Plane>> resolve_cavity_border_caps(
    const CavityBorderSpec& spec,
    const Plane& cavity_inner,
    const Plane& cavity_outer,
    const Plane& cavity_mid,
    const Plane& board_inner,
    const Plane& board_outer
) {

    const std::optional<Plane> sc = compute_cavity_border_cap(
        cavity_mid,
        spec.boundary_plane,
        spec.outer_gap,
        spec.thickness,
        spec.start_plane,
        spec.miter_start,
        spec.start_neighbor_gap,
        spec.start_neighbor_thickness,
        spec.start_setback
    );
    const std::optional<Plane> ec = compute_cavity_border_cap(
        cavity_mid,
        spec.boundary_plane,
        spec.outer_gap,
        spec.thickness,
        spec.end_plane,
        spec.miter_end,
        spec.end_neighbor_gap,
        spec.end_neighbor_thickness,
        spec.end_setback
    );

    if (!sc || !ec)
        return std::nullopt;

    const Polyline raw = Polyline::from_planes({cavity_inner, *ec, cavity_outer, *sc}, board_inner);

    if (raw.point_count() != 5 || raw[0].distance(raw[3]) < 1e-6)
        return std::nullopt;

    const Vector u_end = (raw[0] - raw[3]).normalized();

    return std::pair<Plane, Plane>{
        tighten_cavity_cap(
            *sc,
            spec.start_bisector,
            spec.start_setback,
            cavity_inner,
            cavity_outer,
            board_inner,
            board_outer,
            -u_end
        ),
        tighten_cavity_cap(
            *ec,
            spec.end_bisector,
            spec.end_setback,
            cavity_inner,
            cavity_outer,
            board_inner,
            board_outer,
            u_end
        )
    };
}

std::shared_ptr<FacePlanarElement>
compute_cavity_border_board(const CavityBorderSpec& spec, const std::string& name) {

    if (spec.outer_level - spec.inner_level < 5.0 || spec.thickness < 1.0)
        return nullptr;

    const Plane cavity_inner = spec.facet_plane.translate_by_normal(spec.inner_level);
    const Plane cavity_outer = spec.facet_plane.translate_by_normal(spec.outer_level);
    const Plane cavity_mid =
        spec.facet_plane.translate_by_normal(0.5 * (spec.inner_level + spec.outer_level));
    const Plane board_outer = spec.boundary_plane.translate_by_normal(-spec.outer_gap);
    const Plane board_inner =
        spec.boundary_plane.translate_by_normal(-(spec.outer_gap + spec.thickness));
    const std::optional<std::pair<Plane, Plane>> caps = resolve_cavity_border_caps(
        spec, cavity_inner, cavity_outer, cavity_mid, board_inner, board_outer
    );

    if (!caps)
        return nullptr;

    const std::vector<Plane> sides = {cavity_inner, caps->second, cavity_outer, caps->first};
    const Polyline bottom = Polyline::from_planes(sides, board_inner);
    const Polyline top = Polyline::from_planes(sides, board_outer);

    if (bottom.point_count() != 5 || top.point_count() != 5)
        return nullptr;

    const Vector span_in = bottom[0] - bottom[3];
    const Vector span_out = bottom[1] - bottom[2];

    if (span_in.dot(span_out) <= 0.0 ||
        std::max(span_in.magnitude(), span_out.magnitude()) < spec.min_length)
        return nullptr;

    return std::make_shared<FacePlanarElement>(bottom, top, name);
}
