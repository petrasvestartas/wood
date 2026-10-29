#include "pch.h"
#include "wood_element_beam_curved.h"
#include "wood_element_geometry.h"
#include "primitives.h"
#include <map>

namespace wood_session {

using namespace session_cpp;

namespace {

/// The curve with its domain set to [0, 1] for ruled patches.
NurbsCurve unit_domain(const NurbsCurve& curve) {
    NurbsCurve unit = curve;
    unit.set_domain(0.0, 1.0);
    return unit;
}

/// The straight segment between two points as a degree-1 NURBS curve.
NurbsCurve segment_curve(const Point& from, const Point& to) {
    return NurbsCurve::create(false, 1, {from, to});
}

/// Nearest vertex index in solid to point.
int corner_vertex(const BRep& solid, const Point& point) {
    int nearest = 0;
    for (int i = 1; i < static_cast<int>(solid.m_vertices.size()); i++)
        if (solid.m_vertices[i].point.distance(point) <
            solid.m_vertices[nearest].point.distance(point))
            nearest = i;
    return nearest;
}

/// Edge index between two vertices.
int edge_between(const std::map<std::pair<int, int>, int>& edges, int a, int b) {
    return edges.at({std::min(a, b), std::max(a, b)});
}

/// Adds a bounded rectangular patch face to solid.
int patch_face(
    BRep& solid, const NurbsSurface& patch, const std::map<std::pair<int, int>, int>& edges
) {
    const std::pair<double, double> du = patch.domain(0);
    const std::pair<double, double> dv = patch.domain(1);
    const std::array<Point, 4> uv = {
        Point(du.first, dv.first, 0.0),
        Point(du.second, dv.first, 0.0),
        Point(du.second, dv.second, 0.0),
        Point(du.first, dv.second, 0.0),
    };
    const std::array<int, 4> corners = {
        corner_vertex(solid, patch.point_at_corner(0, 0)),
        corner_vertex(solid, patch.point_at_corner(1, 0)),
        corner_vertex(solid, patch.point_at_corner(1, 1)),
        corner_vertex(solid, patch.point_at_corner(0, 1)),
    };
    const int surface = solid.add_surface(patch);
    std::vector<BRepRef> wire;

    for (size_t k = 0; k < 4; k++) {
        const size_t next = (k + 1) % 4;
        const int edge = edge_between(edges, corners[k], corners[next]);
        const bool forward = solid.m_edges[edge].start_vertex == corners[k];
        solid.add_pcurve(
            edge,
            surface,
            solid.add_curve_2d(
                forward ? segment_curve(uv[k], uv[next]) : segment_curve(uv[next], uv[k])
            )
        );
        wire.push_back({edge, forward ? BRepOrientation::Forward : BRepOrientation::Reversed});
    }

    return solid.add_face(surface, {{solid.add_wire(wire), BRepOrientation::Forward}});
}

Point patch_centre(const NurbsSurface& patch) {
    const std::pair<double, double> du = patch.domain(0);
    const std::pair<double, double> dv = patch.domain(1);
    return patch.point_at(0.5 * (du.first + du.second), 0.5 * (dv.first + dv.second));
}

Vector patch_normal(const NurbsSurface& patch) {
    const std::pair<double, double> du = patch.domain(0);
    const std::pair<double, double> dv = patch.domain(1);
    return patch.normal_at(0.5 * (du.first + du.second), 0.5 * (dv.first + dv.second));
}

/// Builds a closed 6-patch NURBS BRep from 4 corner curves (c0: -x-y, c1: +x-y, c2: +x+y, c3: -x+y).
BRep solid_from_four_edges(
    const std::array<NurbsCurve, 4>& corners, const std::string& solid_name
) {
    // Map to PlateCurved meridian ordering: outer_a (top-left c3), outer_b (top-right c2), inner_a (bottom-left c0), inner_b (bottom-right c1)
    const NurbsCurve& outer_a = corners[3];
    const NurbsCurve& outer_b = corners[2];
    const NurbsCurve& inner_a = corners[0];
    const NurbsCurve& inner_b = corners[1];

    const NurbsCurve outer_start = segment_curve(
        outer_a.point_at(outer_a.domain_start()), outer_b.point_at(outer_b.domain_start())
    );
    const NurbsCurve inner_start = segment_curve(
        inner_a.point_at(inner_a.domain_start()), inner_b.point_at(inner_b.domain_start())
    );
    const NurbsCurve outer_end = segment_curve(
        outer_a.point_at(outer_a.domain_end()), outer_b.point_at(outer_b.domain_end())
    );
    const NurbsCurve inner_end = segment_curve(
        inner_a.point_at(inner_a.domain_end()), inner_b.point_at(inner_b.domain_end())
    );

    const std::array<NurbsSurface, 6> patches = {
        Primitives::create_ruled(outer_a, outer_b),
        Primitives::create_ruled(inner_a, inner_b),
        Primitives::create_ruled(outer_a, inner_a),
        Primitives::create_ruled(outer_b, inner_b),
        Primitives::create_ruled(outer_start, inner_start),
        Primitives::create_ruled(outer_end, inner_end),
    };

    BRep solid;
    solid.name = solid_name;

    const std::array<NurbsCurve, 4> meridians = {outer_a, outer_b, inner_a, inner_b};
    for (const NurbsCurve& meridian : meridians) {
        solid.add_vertex(meridian.point_at(meridian.domain_start()));
        solid.add_vertex(meridian.point_at(meridian.domain_end()));
    }

    const std::array<std::pair<int, int>, 4> rulings = {{{0, 2}, {4, 6}, {0, 4}, {2, 6}}};
    std::map<std::pair<int, int>, int> edges;

    for (int k = 0; k < 4; k++)
        edges[{2 * k, 2 * k + 1}] =
            solid.add_edge(solid.add_curve_3d(unit_domain(meridians[k])), 2 * k, 2 * k + 1);

    for (int end = 0; end < 2; end++)
        for (const std::pair<int, int>& ruling : rulings) {
            const int start = ruling.first + end;
            const int stop = ruling.second + end;
            const NurbsCurve line =
                segment_curve(solid.m_vertices[start].point, solid.m_vertices[stop].point);
            edges[{start, stop}] = solid.add_edge(solid.add_curve_3d(line), start, stop);
        }

    std::array<int, 6> faces;
    for (size_t i = 0; i < 6; i++)
        faces[i] = patch_face(solid, patches[i], edges);

    std::vector<BRepRef> shell;
    for (size_t i = 0; i < 6; i++) {
        const Vector away = patch_centre(patches[i]) - patch_centre(patches[i ^ 1]);
        const bool outward = patch_normal(patches[i]).dot(away) > 0.0;
        shell.push_back({faces[i], outward ? BRepOrientation::Forward : BRepOrientation::Reversed});
    }

    solid.add_solid({{solid.add_shell(shell), BRepOrientation::Forward}});
    return solid;
}

/// Cross-section frame at origin with z_axis along tangent and y_axis aligned with up.
Plane frame_from_tangent_and_up(const Point& origin, const Vector& tangent, const Vector& up) {
    const Vector z = tangent.normalized();
    Vector rise = up.magnitude() > Tolerance::ZERO_TOLERANCE && up.is_parallel_to(z) == 0
                      ? up.normalized()
                      : Vector::z_axis();
    if (rise.is_parallel_to(z) != 0)
        rise = Vector::y_axis();
    if (rise.is_parallel_to(z) != 0)
        rise = Vector::x_axis();

    const Vector x = rise.cross(z).normalized();
    const Vector y = z.cross(x).normalized();
    return Plane::from_frame(origin, x, y, z);
}

/// Closed rectangle in plane from (x0, y0) to (x1, y1), wound counter-clockwise in (x_axis, y_axis).
Polyline section_in_plane(const Plane& frame, double x0, double y0, double x1, double y1) {
    const Point& o = frame.origin();
    const Vector& x = frame.x_axis();
    const Vector& y = frame.y_axis();
    const Point p0 = o + x * x0 + y * y0;
    const Point p1 = o + x * x1 + y * y0;
    const Point p2 = o + x * x1 + y * y1;
    const Point p3 = o + x * x0 + y * y1;
    return Polyline({p0, p1, p2, p3, p0});
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Constructors
// ═══════════════════════════════════════════════════════════════════════════

BeamCurved::BeamCurved() : WoodElement("beam_curved") {
}

BeamCurved::BeamCurved(
    const NurbsCurve& centreline, double width, double height, const std::string& name
)
    : WoodElement(name), centreline(centreline), width(width), height(height) {
    if (!centreline.is_valid())
        throw std::invalid_argument("A BeamCurved requires a valid NurbsCurve centreline");
}

BeamCurved::BeamCurved(
    const NurbsCurve& centreline,
    double width,
    double height,
    const Vector& up,
    int num_width,
    int num_height,
    int samples,
    const std::string& name
)
    : WoodElement(name), centreline(centreline), width(width), height(height), directions({up}),
      num_width(std::max(1, num_width)), num_height(std::max(1, num_height)),
      samples(std::max(2, samples)) {
    if (!centreline.is_valid())
        throw std::invalid_argument("A BeamCurved requires a valid NurbsCurve centreline");
}

BeamCurved::BeamCurved(
    const NurbsCurve& centreline,
    double width,
    double height,
    const std::vector<Vector>& directions,
    int num_width,
    int num_height,
    int samples,
    const std::string& name
)
    : WoodElement(name), centreline(centreline), width(width), height(height),
      directions(directions), num_width(std::max(1, num_width)),
      num_height(std::max(1, num_height)), samples(std::max(2, samples)) {
    if (!centreline.is_valid())
        throw std::invalid_argument("A BeamCurved requires a valid NurbsCurve centreline");
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<BeamCurved> BeamCurved::single_curved(
    const NurbsCurve& centreline,
    double width,
    double height,
    int num_height,
    int samples,
    const std::string& name
) {
    return std::make_shared<BeamCurved>(
        centreline, width, height, std::vector<Vector>{}, 1, std::max(1, num_height), samples, name
    );
}

std::shared_ptr<BeamCurved> BeamCurved::double_curved(
    const NurbsCurve& centreline,
    double width,
    double height,
    const std::vector<Vector>& directions,
    const std::vector<double>& parameters,
    int num_width,
    int num_height,
    int samples,
    const std::string& name
) {
    std::shared_ptr<BeamCurved> beam = std::make_shared<BeamCurved>(
        centreline,
        width,
        height,
        directions,
        std::max(1, num_width),
        std::max(1, num_height),
        samples,
        name
    );
    beam->direction_parameters = parameters;
    return beam;
}

std::shared_ptr<BeamCurved> BeamCurved::from_surface(
    const NurbsCurve& centreline,
    const NurbsSurface& surface,
    double width,
    double height,
    int num_width,
    int num_height,
    int samples,
    const std::string& name
) {
    const int n = std::max(2, samples);
    std::vector<Point> pts;
    std::vector<double> params;
    centreline.divide_by_count(n, pts, &params, true);

    std::vector<Vector> normals;
    normals.reserve(params.size());
    for (const Point& pt : pts) {
        const std::pair<double, double> uv = surface.closest_parameters(pt);
        normals.push_back(surface.normal_at(uv.first, uv.second));
    }

    return double_curved(
        centreline, width, height, normals, params, num_width, num_height, n, name
    );
}

std::shared_ptr<BeamCurved> BeamCurved::from_element(Element e) {
    const std::string bytes = e.element_data_dumps();
    std::shared_ptr<BeamCurved> beam = std::make_shared<BeamCurved>();
    static_cast<Element&>(*beam) = std::move(e);

    if (bytes.empty())
        return beam;

    try {
        const nlohmann::json payload = nlohmann::json::parse(bytes);
        if (payload.contains("centreline") && !payload["centreline"].is_null())
            beam->centreline = NurbsCurve::jsonload(payload["centreline"]);
        beam->width = payload.value("width", 0.0);
        beam->height = payload.value("height", 0.0);
        beam->num_width = std::max(1, payload.value("num_width", 1));
        beam->num_height = std::max(1, payload.value("num_height", 4));
        beam->samples = std::max(2, payload.value("samples", 32));
        beam->extension = payload.value("extension", 0.0);

        if (payload.contains("directions"))
            for (const nlohmann::json& dir : payload["directions"])
                beam->directions.push_back(Vector::jsonload(dir));

        if (payload.contains("direction_parameters"))
            beam->direction_parameters = payload["direction_parameters"].get<std::vector<double>>();

        if (payload.contains("cuts"))
            for (const nlohmann::json& cut : payload["cuts"])
                beam->cuts.push_back(Plane::jsonload(cut));

        if (payload.contains("cross_laps"))
            for (const nlohmann::json& lap_j : payload["cross_laps"]) {
                CrossLap lap;
                if (lap_j.contains("plane_a"))
                    lap.plane_a = Plane::jsonload(lap_j["plane_a"]);
                if (lap_j.contains("plane_b"))
                    lap.plane_b = Plane::jsonload(lap_j["plane_b"]);
                lap.upper_notch = lap_j.value("upper_notch", false);
                beam->cross_laps.push_back(lap);
            }

        if (payload.contains("solid_features"))
            for (const nlohmann::json& raw : payload["solid_features"]) {
                const std::vector<uint8_t> vec = raw.get<std::vector<uint8_t>>();
                const std::string pb_bytes(vec.begin(), vec.end());
                beam->solid_features.push_back(InteractionFeatureSolid::pb_loads(pb_bytes));
            }

        if (payload.contains("plane_features"))
            for (const nlohmann::json& raw : payload["plane_features"]) {
                const std::vector<uint8_t> vec = raw.get<std::vector<uint8_t>>();
                const std::string pb_bytes(vec.begin(), vec.end());
                beam->plane_features.push_back(InteractionFeaturePlane::pb_loads(pb_bytes));
            }
    } catch (const std::exception&) {
    }

    return beam;
}

// ═══════════════════════════════════════════════════════════════════════════
// Glulam queries & lamellae
// ═══════════════════════════════════════════════════════════════════════════

double BeamCurved::lamella_width() const {
    return width / static_cast<double>(std::max(1, num_width));
}

double BeamCurved::lamella_height() const {
    return height / static_cast<double>(std::max(1, num_height));
}

Plane BeamCurved::get_plane(double t) const {
    if (!centreline.is_valid())
        return Plane::xy_plane();

    const double t0 = centreline.domain_start();
    const double t1 = centreline.domain_end();
    t = std::clamp(t, t0, t1);

    const Point origin = centreline.point_at(t);
    const Vector tangent = centreline.tangent_at(t);

    // 1. Single reference vector (GluLamb VectorOrientation)
    if (directions.size() == 1)
        return frame_from_tangent_and_up(origin, tangent, directions[0]);

    // 2. Multiple guide vectors interpolated as twist over RMF (GluLamb VectorListOrientation)
    if (directions.size() >= 2) {
        const size_t m_count = directions.size();
        std::vector<double> params = direction_parameters;
        if (params.size() != m_count) {
            params.resize(m_count);
            for (size_t i = 0; i < m_count; i++)
                params[i] =
                    t0 + (t1 - t0) * static_cast<double>(i) / static_cast<double>(m_count - 1);
        }

        std::vector<double> angles(m_count, 0.0);
        for (size_t i = 0; i < m_count; i++) {
            const double pi = std::clamp(params[i], t0, t1);
            const Plane rmf_i = centreline.perpendicular_plane_at(pi, false);
            const double dx = directions[i].dot(rmf_i.x_axis());
            const double dy = directions[i].dot(rmf_i.y_axis());
            angles[i] = std::atan2(dy, dx);
        }

        // Unwrap angle jumps across pi
        for (size_t i = 1; i < m_count; i++) {
            while (angles[i] - angles[i - 1] > Tolerance::PI)
                angles[i] -= Tolerance::TWO_PI;
            while (angles[i] - angles[i - 1] < -Tolerance::PI)
                angles[i] += Tolerance::TWO_PI;
        }

        double theta = angles.front();
        if (t <= params.front()) {
            theta = angles.front();
        } else if (t >= params.back()) {
            theta = angles.back();
        } else {
            for (size_t i = 0; i + 1 < m_count; i++) {
                if (t >= params[i] && t <= params[i + 1]) {
                    const double span = params[i + 1] - params[i];
                    const double u =
                        span > Tolerance::ZERO_TOLERANCE ? (t - params[i]) / span : 0.0;
                    theta = angles[i] + u * (angles[i + 1] - angles[i]);
                    break;
                }
            }
        }

        const Plane rmf = centreline.perpendicular_plane_at(t, false);
        const Vector up = rmf.x_axis() * std::cos(theta) + rmf.y_axis() * std::sin(theta);
        return frame_from_tangent_and_up(origin, tangent, up);
    }

    // 3. Default (directions.empty()): planar normal for single-curved glulam, RMF for 3D space curve
    Plane curve_plane;
    if (centreline.is_planar(&curve_plane, 1e-3)) {
        Vector binormal = curve_plane.z_axis().normalized();
        const Vector mid_tangent = centreline.tangent_at(centreline.domain_middle());
        Vector mid_up = binormal.cross(mid_tangent);
        if (mid_up[2] < -1e-6 || (std::abs(mid_up[2]) <= 1e-6 && mid_up[1] < 0.0))
            binormal = -binormal;
        const Vector up = binormal.cross(tangent);
        return frame_from_tangent_and_up(origin, tangent, up);
    }

    const Plane rmf = centreline.perpendicular_plane_at(t, false);
    return frame_from_tangent_and_up(origin, tangent, rmf.y_axis());
}

std::vector<Plane> BeamCurved::frames() const {
    std::vector<Plane> result;
    if (!centreline.is_valid())
        return result;

    const int n = std::max(2, samples);
    std::vector<Point> pts;
    std::vector<double> params;
    centreline.divide_by_count(n, pts, &params, true);

    result.reserve(params.size());
    for (size_t i = 0; i < params.size(); i++) {
        Plane frame = get_plane(params[i]);
        if (i == 0 && std::abs(extension) > Tolerance::ZERO_TOLERANCE)
            frame = Plane::from_frame(
                frame.origin() - frame.z_axis() * extension,
                frame.x_axis(),
                frame.y_axis(),
                frame.z_axis()
            );
        else if (i + 1 == params.size() && std::abs(extension) > Tolerance::ZERO_TOLERANCE)
            frame = Plane::from_frame(
                frame.origin() + frame.z_axis() * extension,
                frame.x_axis(),
                frame.y_axis(),
                frame.z_axis()
            );
        result.push_back(frame);
    }

    return result;
}

bool BeamCurved::is_single_curved(double tolerance) const {
    if (!centreline.is_valid() || !centreline.is_planar(nullptr, tolerance))
        return false;
    const std::vector<Plane> station_frames = frames();
    if (station_frames.size() < 2)
        return true;
    const Vector ref_x = station_frames.front().x_axis();
    for (size_t i = 1; i < station_frames.size(); i++)
        if ( (station_frames[i].x_axis() - ref_x).magnitude() > tolerance &&
             (station_frames[i].x_axis() + ref_x).magnitude() > tolerance )
            return false;
    return true;
}

bool BeamCurved::is_double_curved(double tolerance) const {
    return !is_single_curved(tolerance);
}

std::vector<Polyline> BeamCurved::sections() const {
    std::vector<Polyline> rings;
    if (!centreline.is_valid() || width <= 0.0 || height <= 0.0)
        return rings;

    const double hx = 0.5 * width;
    const double hy = 0.5 * height;
    const std::vector<Plane> station_frames = frames();
    rings.reserve(station_frames.size());
    for (const Plane& frame : station_frames)
        rings.push_back(section_in_plane(frame, -hx, -hy, hx, hy));

    return rings;
}

std::array<NurbsCurve, 4> BeamCurved::edge_curves() const {
    const std::vector<Polyline> rings = sections();
    std::array<std::vector<Point>, 4> corner_pts;
    for (const Polyline& ring : rings) {
        if (ring.point_count() < 4)
            continue;
        for (size_t k = 0; k < 4; k++)
            corner_pts[k].push_back(ring.get_point(k));
    }

    std::array<NurbsCurve, 4> edges;
    for (size_t k = 0; k < 4; k++)
        if (corner_pts[k].size() >= 2)
            edges[k] = NurbsCurve::create_interpolated(
                corner_pts[k], CurveNurbsKnotStyle::Uniform, CurveInterpStyle::Occt
            );

    return edges;
}

NurbsCurve BeamCurved::offset_curve(double x, double y) const {
    const std::vector<Plane> station_frames = frames();
    std::vector<Point> pts;
    pts.reserve(station_frames.size());
    for (const Plane& frame : station_frames)
        pts.push_back(frame.origin() + frame.x_axis() * x + frame.y_axis() * y);

    return pts.size() >= 2 ? NurbsCurve::create_interpolated(
                                 pts, CurveNurbsKnotStyle::Uniform, CurveInterpStyle::Occt
                             )
                           : NurbsCurve();
}

NurbsCurve BeamCurved::lamella_curve(int ix, int iy) const {
    const int nw = std::max(1, num_width);
    const int nh = std::max(1, num_height);
    const double lw = width / static_cast<double>(nw);
    const double lh = height / static_cast<double>(nh);
    const double x = -0.5 * width + (static_cast<double>(ix) + 0.5) * lw;
    const double y = -0.5 * height + (static_cast<double>(iy) + 0.5) * lh;
    return offset_curve(x, y);
}

std::vector<NurbsCurve> BeamCurved::lamella_curves() const {
    std::vector<NurbsCurve> curves;
    const int nw = std::max(1, num_width);
    const int nh = std::max(1, num_height);
    curves.reserve(nw * nh);
    for (int iy = 0; iy < nh; iy++)
        for (int ix = 0; ix < nw; ix++)
            curves.push_back(lamella_curve(ix, iy));
    return curves;
}

std::vector<Mesh> BeamCurved::lamella_meshes() const {
    std::vector<Mesh> meshes;
    const std::vector<Plane> station_frames = frames();
    if (station_frames.size() < 2 || width <= 0.0 || height <= 0.0)
        return meshes;

    const int nw = std::max(1, num_width);
    const int nh = std::max(1, num_height);
    const double lw = width / static_cast<double>(nw);
    const double lh = height / static_cast<double>(nh);
    meshes.reserve(nw * nh);

    for (int iy = 0; iy < nh; iy++) {
        const double y0 = -0.5 * height + static_cast<double>(iy) * lh;
        const double y1 = y0 + lh;
        for (int ix = 0; ix < nw; ix++) {
            const double x0 = -0.5 * width + static_cast<double>(ix) * lw;
            const double x1 = x0 + lw;
            std::vector<Polyline> lam_sections;
            lam_sections.reserve(station_frames.size());
            for (const Plane& frame : station_frames)
                lam_sections.push_back(section_in_plane(frame, x0, y0, x1, y1));
            meshes.push_back(sweep_sections(lam_sections));
        }
    }

    return meshes;
}

std::vector<BRep> BeamCurved::lamella_breps() const {
    std::vector<BRep> breps;
    const std::vector<Plane> station_frames = frames();
    if (station_frames.size() < 2 || width <= 0.0 || height <= 0.0)
        return breps;

    const int nw = std::max(1, num_width);
    const int nh = std::max(1, num_height);
    const double lw = width / static_cast<double>(nw);
    const double lh = height / static_cast<double>(nh);

    std::vector<std::vector<NurbsCurve>> grid_curves(
        nw + 1, std::vector<NurbsCurve>(nh + 1)
    );
    for (int ix = 0; ix <= nw; ix++) {
        const double x = -0.5 * width + static_cast<double>(ix) * lw;
        for (int iy = 0; iy <= nh; iy++) {
            const double y = -0.5 * height + static_cast<double>(iy) * lh;
            grid_curves[ix][iy] = offset_curve(x, y);
        }
    }

    breps.reserve(nw * nh);
    for (int iy = 0; iy < nh; iy++) {
        for (int ix = 0; ix < nw; ix++) {
            const std::array<NurbsCurve, 4> corners = {
                grid_curves[ix][iy],
                grid_curves[ix + 1][iy],
                grid_curves[ix + 1][iy + 1],
                grid_curves[ix][iy + 1],
            };
            breps.push_back(
                solid_from_four_edges(corners, fmt::format("{}_lamella_{}_{}", name, ix, iy))
            );
        }
    }

    return breps;
}

double BeamCurved::max_curvature(double& max_kw, double& max_kh) const {
    max_kw = 0.0;
    max_kh = 0.0;
    double max_k = 0.0;
    if (!centreline.is_valid())
        return 0.0;

    std::vector<Point> pts;
    std::vector<double> params;
    centreline.divide_by_count(std::max(16, samples), pts, &params, true);

    for (const double t : params) {
        const std::vector<Vector> derivs = centreline.evaluate(t, 2);
        if (derivs.size() < 3)
            continue;
        const Vector& d1 = derivs[1];
        const Vector& d2 = derivs[2];
        const double d1_sq = d1.magnitude_squared();
        if (d1_sq < Tolerance::ZERO_TOLERANCE)
            continue;
        const Vector kappa_vec = (d2 * d1_sq - d1 * d2.dot(d1)) / (d1_sq * d1_sq);
        const Plane frame = get_plane(t);
        max_kw = std::max(max_kw, std::abs(kappa_vec.dot(frame.x_axis())));
        max_kh = std::max(max_kh, std::abs(kappa_vec.dot(frame.y_axis())));
        max_k = std::max(max_k, kappa_vec.magnitude());
    }

    return max_k;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

const Mesh& BeamCurved::element_geometry_mesh() const {
    if (!_element_geometry_mesh)
        _element_geometry_mesh = sweep_sections(sections());
    return *_element_geometry_mesh;
}

const BRep& BeamCurved::element_geometry_brep() const {
    if (!_element_geometry_brep) {
        const std::array<NurbsCurve, 4> corners = edge_curves();
        if (corners[0].is_valid() && corners[1].is_valid() && corners[2].is_valid() &&
            corners[3].is_valid())
            _element_geometry_brep = solid_from_four_edges(corners, name);
        else
            _element_geometry_brep = BRep();
    }
    return *_element_geometry_brep;
}

Mesh BeamCurved::trimmed_mesh() const {
    Mesh m = cut_mesh(stock_mesh(), cuts);
    if (cross_laps.empty() || !centreline.is_valid() || width <= 0.0 || height <= 0.0)
        return m;

    const std::vector<Plane> station_frames = frames();
    if (station_frames.size() < 2)
        return m;

    for (const CrossLap& lap : cross_laps) {
        const double x0 = -1.5 * width;
        const double x1 = 1.5 * width;
        const double y0 = lap.upper_notch ? 0.0 : -1.5 * height;
        const double y1 = lap.upper_notch ? 1.5 * height : 0.0;
        std::vector<Polyline> cutter_rings;
        cutter_rings.reserve(station_frames.size());
        for (const Plane& fr : station_frames)
            cutter_rings.push_back(section_in_plane(fr, x0, y0, x1, y1));
        const std::vector<Plane> clip_planes = {
            Plane::from_point_normal(lap.plane_a.origin(), lap.plane_a.z_axis() * -1.0),
            Plane::from_point_normal(lap.plane_b.origin(), lap.plane_b.z_axis() * -1.0),
        };
        const Mesh cutter = cut_mesh(sweep_sections(cutter_rings), clip_planes);
        if (cutter.number_of_faces() > 0)
            m = solid_boolean(m, cutter, SolidOperation::subtract);
    }
    return m;
}

BRep BeamCurved::trimmed_brep() const {
    if (cuts.empty() && cross_laps.empty())
        return element_geometry_brep();

    if (!centreline.is_valid() || width <= 0.0 || height <= 0.0)
        return mesh_brep(trimmed_mesh());

    const double t0 = centreline.domain_start();
    const double t1 = centreline.domain_end();
    const double span = t1 - t0;
    if (span <= Tolerance::ZERO_TOLERANCE)
        return mesh_brep(trimmed_mesh());

    const auto extended_plane_at = [&](double u_norm) -> Plane {
        const double u_clamped = std::clamp(u_norm, 0.0, 1.0);
        const double t = t0 + u_clamped * span;
        const Plane base = get_plane(t);
        if (u_norm >= 0.0 && u_norm <= 1.0)
            return base;

        const double dt = (u_norm < 0.0 ? u_norm : (u_norm - 1.0)) * span;
        const std::vector<Vector> derivs = centreline.evaluate(t, 2);
        if (derivs.size() >= 3 && derivs[1].magnitude() > Tolerance::ZERO_TOLERANCE) {
            const Vector& d1 = derivs[1];
            const Vector& d2 = derivs[2];
            const Point origin = base.origin() + d1 * dt + d2 * (0.5 * dt * dt);
            const Vector tangent = d1 + d2 * dt;
            return frame_from_tangent_and_up(origin, tangent, base.y_axis());
        }
        return Plane::from_frame(
            base.origin() + base.z_axis() * dt, base.x_axis(), base.y_axis(), base.z_axis()
        );
    };

    const double hx = 0.5 * width;
    const double hy = 0.5 * height;
    const std::array<double, 4> fx = {-hx, hx, hx, -hx};
    const std::array<double, 4> fy = {-hy, -hy, hy, hy};

    const auto eval_pt = [&](double x_off, double y_off, double u_norm) -> Point {
        const Plane fr = extended_plane_at(u_norm);
        return fr.origin() + fr.x_axis() * x_off + fr.y_axis() * y_off;
    };

    const int n_scan = std::max(32, samples);
    const double u_lo_bound = -0.35;
    const double u_hi_bound = 1.35;

    const auto bisect_front = [&](const Plane& plane, double x_off, double y_off, double& out_u) -> bool {
        const auto sd = [&](double u) -> double {
            return (eval_pt(x_off, y_off, u) - plane.origin()).dot(plane.z_axis());
        };
        const double d_lo = sd(u_lo_bound);
        const double d_mid = sd(0.5);
        if (!(d_lo < -Tolerance::APPROXIMATION && d_mid >= -Tolerance::APPROXIMATION))
            return false;
        double a = u_lo_bound;
        double b = 0.5;
        for (int k = 1; k <= n_scan; k++) {
            const double u_k = u_lo_bound + (0.9 - u_lo_bound) * static_cast<double>(k) / n_scan;
            if (sd(u_k) >= 0.0) {
                b = u_k;
                a = u_lo_bound + (0.9 - u_lo_bound) * static_cast<double>(k - 1) / n_scan;
                break;
            }
        }
        for (int it = 0; it < 44; it++) {
            const double mid = 0.5 * (a + b);
            if (sd(mid) < 0.0)
                a = mid;
            else
                b = mid;
        }
        out_u = 0.5 * (a + b);
        return true;
    };

    const auto bisect_back = [&](const Plane& plane, double x_off, double y_off, double& out_u) -> bool {
        const auto sd = [&](double u) -> double {
            return (eval_pt(x_off, y_off, u) - plane.origin()).dot(plane.z_axis());
        };
        const double d_mid = sd(0.5);
        const double d_hi = sd(u_hi_bound);
        if (!(d_hi < -Tolerance::APPROXIMATION && d_mid >= -Tolerance::APPROXIMATION))
            return false;
        double a = 0.5;
        double b = u_hi_bound;
        for (int k = n_scan - 1; k >= 0; k--) {
            const double u_k = 0.1 + (u_hi_bound - 0.1) * static_cast<double>(k) / n_scan;
            if (sd(u_k) >= 0.0) {
                a = u_k;
                b = 0.1 + (u_hi_bound - 0.1) * static_cast<double>(k + 1) / n_scan;
                break;
            }
        }
        for (int it = 0; it < 44; it++) {
            const double mid = 0.5 * (a + b);
            if (sd(mid) >= 0.0)
                a = mid;
            else
                b = mid;
        }
        out_u = 0.5 * (a + b);
        return true;
    };

    const auto front_envelope = [&](double v_frac, double y_off, int& win_idx) -> double {
        const double x_off = (-0.5 + v_frac) * width;
        double best_u = 0.0;
        bool has = false;
        win_idx = -1;
        for (size_t p_idx = 0; p_idx < cuts.size(); p_idx++) {
            double cand = 0.0;
            if (bisect_front(cuts[p_idx], x_off, y_off, cand)) {
                if (!has || cand > best_u) {
                    best_u = cand;
                    win_idx = static_cast<int>(p_idx);
                    has = true;
                }
            }
        }
        return best_u;
    };

    const auto back_envelope = [&](double v_frac, double y_off, int& win_idx) -> double {
        const double x_off = (-0.5 + v_frac) * width;
        double best_u = 1.0;
        bool has = false;
        win_idx = -1;
        for (size_t p_idx = 0; p_idx < cuts.size(); p_idx++) {
            double cand = 1.0;
            if (bisect_back(cuts[p_idx], x_off, y_off, cand)) {
                if (!has || cand < best_u) {
                    best_u = cand;
                    win_idx = static_cast<int>(p_idx);
                    has = true;
                }
            }
        }
        return best_u;
    };

    for (size_t f = 0; f < 4; f++) {
        for (const Plane& plane : cuts) {
            const double d_lo = (eval_pt(fx[f], fy[f], u_lo_bound) - plane.origin()).dot(plane.z_axis());
            const double d_mid = (eval_pt(fx[f], fy[f], 0.5) - plane.origin()).dot(plane.z_axis());
            const double d_hi = (eval_pt(fx[f], fy[f], u_hi_bound) - plane.origin()).dot(plane.z_axis());
            if (d_lo < -Tolerance::APPROXIMATION && d_mid < -Tolerance::APPROXIMATION &&
                d_hi < -Tolerance::APPROXIMATION)
                return mesh_brep(trimmed_mesh());
        }
    }

    std::array<double, 4> s0_arr{};
    std::array<double, 4> s1_arr{};
    std::array<int, 4> front_cutter = {-1, -1, -1, -1};
    std::array<int, 4> back_cutter  = {-1, -1, -1, -1};

    s0_arr[0] = front_envelope(0.0, -hy, front_cutter[0]);
    s0_arr[1] = front_envelope(1.0, -hy, front_cutter[1]);
    s0_arr[2] = front_envelope(1.0,  hy, front_cutter[2]);
    s0_arr[3] = front_envelope(0.0,  hy, front_cutter[3]);

    s1_arr[0] = back_envelope(0.0, -hy, back_cutter[0]);
    s1_arr[1] = back_envelope(1.0, -hy, back_cutter[1]);
    s1_arr[2] = back_envelope(1.0,  hy, back_cutter[2]);
    s1_arr[3] = back_envelope(0.0,  hy, back_cutter[3]);

    for (size_t f = 0; f < 4; f++)
        if (s0_arr[f] >= s1_arr[f])
            return mesh_brep(trimmed_mesh());

    const auto sample_fibre_xy = [&](double x_off, double y_off, double sa, double sb) -> NurbsCurve {
        const int count = std::max(8, samples);
        std::vector<Point> sub_pts;
        sub_pts.reserve(count + 1);
        for (int j = 0; j <= count; j++) {
            const double u = sa + (sb - sa) * static_cast<double>(j) / static_cast<double>(count);
            sub_pts.push_back(eval_pt(x_off, y_off, u));
        }
        return unit_domain(NurbsCurve::create_interpolated(
            sub_pts, CurveNurbsKnotStyle::Uniform, CurveInterpStyle::Occt
        ));
    };

    const bool single_front =
        (front_cutter[0] == front_cutter[1] && front_cutter[2] == front_cutter[3] &&
         front_cutter[0] == front_cutter[2]);
    const bool single_back =
        (back_cutter[0] == back_cutter[1] && back_cutter[2] == back_cutter[3] &&
         back_cutter[0] == back_cutter[2]);

    if (cross_laps.empty() && single_front && single_back) {
        std::array<NurbsCurve, 4> trimmed_corners;
        for (size_t k = 0; k < 4; k++)
            trimmed_corners[k] = sample_fibre_xy(fx[k], fy[k], s0_arr[k], s1_arr[k]);
        if (trimmed_corners[0].is_valid() && trimmed_corners[1].is_valid() &&
            trimmed_corners[2].is_valid() && trimmed_corners[3].is_valid())
            return solid_from_four_edges(trimmed_corners, name);
        return mesh_brep(trimmed_mesh());
    }

    // Solve exact lateral v_frac in (0, 1) where left and right miter planes meet (not hardcoded 0.5)
    const auto solve_miter_v = [&](bool is_front, double y_off, int c_left, int c_right, double& out_v, double& out_u) {
        if (c_left == c_right) {
            out_v = 0.5;
            int dummy = -1;
            out_u = is_front ? front_envelope(0.5, y_off, dummy) : back_envelope(0.5, y_off, dummy);
            return;
        }
        double lo = 0.0;
        double hi = 1.0;
        for (int it = 0; it < 40; it++) {
            const double mid = 0.5 * (lo + hi);
            int win = -1;
            if (is_front)
                front_envelope(mid, y_off, win);
            else
                back_envelope(mid, y_off, win);
            if (win == c_left)
                lo = mid;
            else
                hi = mid;
        }
        out_v = std::clamp(0.5 * (lo + hi), 0.005, 0.995);
        int dummy = -1;
        out_u = is_front ? front_envelope(out_v, y_off, dummy) : back_envelope(out_v, y_off, dummy);
    };

    double vf_bot = 0.5, uf_bot = 0.0;
    double vf_top = 0.5, uf_top = 0.0;
    if (!single_front) {
        solve_miter_v(true, -hy, front_cutter[0], front_cutter[1], vf_bot, uf_bot);
        solve_miter_v(true,  hy, front_cutter[3], front_cutter[2], vf_top, uf_top);
    }

    double vb_bot = 0.5, ub_bot = 1.0;
    double vb_top = 0.5, ub_top = 1.0;
    if (!single_back) {
        solve_miter_v(false, -hy, back_cutter[0], back_cutter[1], vb_bot, ub_bot);
        solve_miter_v(false,  hy, back_cutter[3], back_cutter[2], vb_top, ub_top);
    }

    double s_min = std::min({s0_arr[0], s0_arr[1], s0_arr[2], s0_arr[3]});
    double s_max = std::max({s1_arr[0], s1_arr[1], s1_arr[2], s1_arr[3]});
    if (!single_front)
        s_min = std::min({s_min, uf_bot, uf_top});
    if (!single_back)
        s_max = std::max({s_max, ub_bot, ub_top});
    if (s_max <= s_min + 1e-6)
        return mesh_brep(trimmed_mesh());

    // Resolve any interior half-lap cross notches
    struct ResolvedLap {
        bool upper = false;
        double ua_botL = 0.0, ua_midL = 0.0, ua_topL = 0.0;
        double ua_botR = 0.0, ua_midR = 0.0, ua_topR = 0.0;
        double ub_botL = 0.0, ub_midL = 0.0, ub_topL = 0.0;
        double ub_botR = 0.0, ub_midR = 0.0, ub_topR = 0.0;
    };
    std::vector<ResolvedLap> laps;

    for (const CrossLap& lap : cross_laps) {
        const auto bisect_lap = [&](const Plane& pl, double x_off, double y_off, double& out_u) -> bool {
            const auto sd = [&](double u) -> double {
                return (eval_pt(x_off, y_off, u) - pl.origin()).dot(pl.z_axis());
            };
            const double f0 = sd(s_min);
            const double f1 = sd(s_max);
            if ((f0 >= 0.0 && f1 >= 0.0) || (f0 <= 0.0 && f1 <= 0.0))
                return false;
            double a = s_min;
            double b = s_max;
            for (int it = 0; it < 48; it++) {
                const double m = 0.5 * (a + b);
                if ((sd(m) >= 0.0) == (f0 >= 0.0))
                    a = m;
                else
                    b = m;
            }
            out_u = 0.5 * (a + b);
            return true;
        };

        ResolvedLap rl;
        rl.upper = lap.upper_notch;
        if (!bisect_lap(lap.plane_a, -hx, -hy, rl.ua_botL) ||
            !bisect_lap(lap.plane_a, -hx, 0.0, rl.ua_midL) ||
            !bisect_lap(lap.plane_a, -hx,  hy, rl.ua_topL) ||
            !bisect_lap(lap.plane_a,  hx, -hy, rl.ua_botR) ||
            !bisect_lap(lap.plane_a,  hx, 0.0, rl.ua_midR) ||
            !bisect_lap(lap.plane_a,  hx,  hy, rl.ua_topR) ||
            !bisect_lap(lap.plane_b, -hx, -hy, rl.ub_botL) ||
            !bisect_lap(lap.plane_b, -hx, 0.0, rl.ub_midL) ||
            !bisect_lap(lap.plane_b, -hx,  hy, rl.ub_topL) ||
            !bisect_lap(lap.plane_b,  hx, -hy, rl.ub_botR) ||
            !bisect_lap(lap.plane_b,  hx, 0.0, rl.ub_midR) ||
            !bisect_lap(lap.plane_b,  hx,  hy, rl.ub_topR))
            return mesh_brep(trimmed_mesh());
        if (rl.ua_botL >= rl.ub_botL || rl.ua_midL >= rl.ub_midL || rl.ua_topL >= rl.ub_topL ||
            rl.ua_botR >= rl.ub_botR || rl.ua_midR >= rl.ub_midR || rl.ua_topR >= rl.ub_topR)
            return mesh_brep(trimmed_mesh());
        laps.push_back(rl);
    }

    std::sort(laps.begin(), laps.end(), [](const ResolvedLap& a, const ResolvedLap& b) {
        return a.ua_midL < b.ua_midL;
    });

    const auto norm_u = [&](double s) -> double {
        return std::clamp((s - s_min) / (s_max - s_min), 0.0, 1.0);
    };

    const NurbsCurve rail_bot_L = sample_fibre_xy(-hx, -hy, s_min, s_max);
    const NurbsCurve rail_bot_R = sample_fibre_xy( hx, -hy, s_min, s_max);
    const NurbsCurve rail_mid_L = sample_fibre_xy(-hx, 0.0, s_min, s_max);
    const NurbsCurve rail_mid_R = sample_fibre_xy( hx, 0.0, s_min, s_max);
    const NurbsCurve rail_top_L = sample_fibre_xy(-hx,  hy, s_min, s_max);
    const NurbsCurve rail_top_R = sample_fibre_xy( hx,  hy, s_min, s_max);

    const NurbsSurface surf_above = Primitives::create_ruled(rail_top_L, rail_top_R);
    NurbsSurface surf_below = Primitives::create_ruled(rail_bot_L, rail_bot_R);
    surf_below.transpose();
    const NurbsSurface surf_left = Primitives::create_ruled(rail_bot_L, rail_top_L);
    NurbsSurface surf_right = Primitives::create_ruled(rail_bot_R, rail_top_R);
    surf_right.transpose();

    BRep brep;
    brep.name = name;
    const int sid_above = brep.add_surface(surf_above);
    const int sid_below = brep.add_surface(surf_below);
    const int sid_left  = brep.add_surface(surf_left);
    const int sid_right = brep.add_surface(surf_right);

    std::map<std::pair<int, int>, int> edge_map;
    const auto get_edge = [&](int va, int vb, const std::function<NurbsCurve()>& make_crv) -> int {
        const std::pair<int, int> key = {std::min(va, vb), std::max(va, vb)};
        const auto it = edge_map.find(key);
        if (it != edge_map.end())
            return it->second;
        NurbsCurve crv = make_crv();
        if (va != key.first)
            crv.reverse();
        const int eid = brep.add_edge(brep.add_curve_3d(unit_domain(crv)), key.first, key.second);
        edge_map[key] = eid;
        return eid;
    };

    struct LoopEntry {
        int vid = -1;
        Point uv;
        std::function<NurbsCurve()> crv_to_next;
    };

    std::vector<BRepRef> faces;
    const auto add_loop_face = [&](int sid, const std::vector<LoopEntry>& entries) {
        std::vector<BRepRef> wire;
        const size_t n = entries.size();
        wire.reserve(n);
        for (size_t i = 0; i < n; i++) {
            const size_t next = (i + 1) % n;
            const int va = entries[i].vid;
            const int vb = entries[next].vid;
            const auto default_seg = [&]() {
                return segment_curve(brep.m_vertices[va].point, brep.m_vertices[vb].point);
            };
            const int eid = get_edge(va, vb, entries[i].crv_to_next ? entries[i].crv_to_next : default_seg);
            const bool fwd = (brep.m_edges[eid].start_vertex == va);
            const int cid2 = brep.add_curve_2d(
                fwd ? segment_curve(entries[i].uv, entries[next].uv)
                    : segment_curve(entries[next].uv, entries[i].uv)
            );
            brep.add_pcurve(eid, sid, cid2);
            wire.push_back({eid, fwd ? BRepOrientation::Forward : BRepOrientation::Reversed});
        }
        faces.push_back(
            {brep.add_face(sid, {{brep.add_wire(wire), BRepOrientation::Forward}}), BRepOrientation::Forward}
        );
    };

    const auto add_planar_quad = [&](int v0, int v1, int v2, int v3) {
        const Point& p0 = brep.m_vertices[v0].point;
        const Point& p1 = brep.m_vertices[v1].point;
        const Point& p2 = brep.m_vertices[v2].point;
        const Point& p3 = brep.m_vertices[v3].point;
        const NurbsSurface qsurf = Primitives::create_ruled(segment_curve(p0, p1), segment_curve(p3, p2));
        const int qsid = brep.add_surface(qsurf);
        add_loop_face(
            qsid,
            {
                {v0, Point(0.0, 0.0, 0.0), nullptr},
                {v1, Point(1.0, 0.0, 0.0), nullptr},
                {v2, Point(1.0, 1.0, 0.0), nullptr},
                {v3, Point(0.0, 1.0, 0.0), nullptr},
            }
        );
    };

    // Add start & end vertices
    const int v_s0_botL = brep.add_vertex(surf_below.point_at(0.0, norm_u(s0_arr[0])));
    const int v_s0_botR = brep.add_vertex(surf_below.point_at(1.0, norm_u(s0_arr[1])));
    const int v_s0_topR = brep.add_vertex(surf_above.point_at(norm_u(s0_arr[2]), 1.0));
    const int v_s0_topL = brep.add_vertex(surf_above.point_at(norm_u(s0_arr[3]), 0.0));

    const int v_s1_botL = brep.add_vertex(surf_below.point_at(0.0, norm_u(s1_arr[0])));
    const int v_s1_botR = brep.add_vertex(surf_below.point_at(1.0, norm_u(s1_arr[1])));
    const int v_s1_topR = brep.add_vertex(surf_above.point_at(norm_u(s1_arr[2]), 1.0));
    const int v_s1_topL = brep.add_vertex(surf_above.point_at(norm_u(s1_arr[3]), 0.0));

    int v_sf_bot = -1, v_sf_top = -1;
    if (!single_front) {
        v_sf_bot = brep.add_vertex(surf_below.point_at(vf_bot, norm_u(uf_bot)));
        v_sf_top = brep.add_vertex(surf_above.point_at(norm_u(uf_top), vf_top));
    }
    int v_sb_bot = -1, v_sb_top = -1;
    if (!single_back) {
        v_sb_bot = brep.add_vertex(surf_below.point_at(vb_bot, norm_u(ub_bot)));
        v_sb_top = brep.add_vertex(surf_above.point_at(norm_u(ub_top), vb_top));
    }

    struct LapVerts {
        int va_extL = -1, va_extR = -1, va_midL = -1, va_midR = -1;
        int vb_extL = -1, vb_extR = -1, vb_midL = -1, vb_midR = -1;
    };
    std::vector<LapVerts> lv(laps.size());
    for (size_t i = 0; i < laps.size(); i++) {
        const ResolvedLap& rl = laps[i];
        lv[i].va_midL = brep.add_vertex(surf_left.point_at(norm_u(rl.ua_midL), 0.5));
        lv[i].va_midR = brep.add_vertex(surf_right.point_at(0.5, norm_u(rl.ua_midR)));
        lv[i].vb_midL = brep.add_vertex(surf_left.point_at(norm_u(rl.ub_midL), 0.5));
        lv[i].vb_midR = brep.add_vertex(surf_right.point_at(0.5, norm_u(rl.ub_midR)));
        if (rl.upper) {
            lv[i].va_extL = brep.add_vertex(surf_above.point_at(norm_u(rl.ua_topL), 0.0));
            lv[i].va_extR = brep.add_vertex(surf_above.point_at(norm_u(rl.ua_topR), 1.0));
            lv[i].vb_extL = brep.add_vertex(surf_above.point_at(norm_u(rl.ub_topL), 0.0));
            lv[i].vb_extR = brep.add_vertex(surf_above.point_at(norm_u(rl.ub_topR), 1.0));
        } else {
            lv[i].va_extL = brep.add_vertex(surf_below.point_at(0.0, norm_u(rl.ua_botL)));
            lv[i].va_extR = brep.add_vertex(surf_below.point_at(1.0, norm_u(rl.ua_botR)));
            lv[i].vb_extL = brep.add_vertex(surf_below.point_at(0.0, norm_u(rl.ub_botL)));
            lv[i].vb_extR = brep.add_vertex(surf_below.point_at(1.0, norm_u(rl.ub_botR)));
        }
    }

    // 1. Top spans on surf_above (CCW in (u, v))
    {
        std::vector<size_t> top_laps;
        for (size_t i = 0; i < laps.size(); i++)
            if (laps[i].upper)
                top_laps.push_back(i);

        const size_t n_spans = top_laps.size() + 1;
        for (size_t sp = 0; sp < n_spans; sp++) {
            const bool is_first = (sp == 0);
            const bool is_last  = (sp + 1 == n_spans);
            const int vL0 = is_first ? v_s0_topL : lv[top_laps[sp - 1]].vb_extL;
            const int vR0 = is_first ? v_s0_topR : lv[top_laps[sp - 1]].vb_extR;
            const double sL0 = is_first ? s0_arr[3] : laps[top_laps[sp - 1]].ub_topL;
            const double sR0 = is_first ? s0_arr[2] : laps[top_laps[sp - 1]].ub_topR;

            const int vL1 = is_last ? v_s1_topL : lv[top_laps[sp]].va_extL;
            const int vR1 = is_last ? v_s1_topR : lv[top_laps[sp]].va_extR;
            const double sL1 = is_last ? s1_arr[3] : laps[top_laps[sp]].ua_topL;
            const double sR1 = is_last ? s1_arr[2] : laps[top_laps[sp]].ua_topR;

            std::vector<LoopEntry> loop;
            loop.push_back({vL0, Point(norm_u(sL0), 0.0, 0.0), [&, sL0, sL1]() { return sample_fibre_xy(-hx, hy, sL0, sL1); }});
            loop.push_back({vL1, Point(norm_u(sL1), 0.0, 0.0), nullptr});
            if (is_last && !single_back)
                loop.push_back({v_sb_top, Point(norm_u(ub_top), vb_top, 0.0), nullptr});
            loop.push_back({vR1, Point(norm_u(sR1), 1.0, 0.0), [&, sR1, sR0]() { return sample_fibre_xy(hx, hy, sR1, sR0); }});
            loop.push_back({vR0, Point(norm_u(sR0), 1.0, 0.0), nullptr});
            if (is_first && !single_front)
                loop.push_back({v_sf_top, Point(norm_u(uf_top), vf_top, 0.0), nullptr});
            add_loop_face(sid_above, loop);
        }
    }

    // 2. Bottom spans on surf_below (CCW in (v, u))
    {
        std::vector<size_t> bot_laps;
        for (size_t i = 0; i < laps.size(); i++)
            if (!laps[i].upper)
                bot_laps.push_back(i);

        const size_t n_spans = bot_laps.size() + 1;
        for (size_t sp = 0; sp < n_spans; sp++) {
            const bool is_first = (sp == 0);
            const bool is_last  = (sp + 1 == n_spans);
            const int vL0 = is_first ? v_s0_botL : lv[bot_laps[sp - 1]].vb_extL;
            const int vR0 = is_first ? v_s0_botR : lv[bot_laps[sp - 1]].vb_extR;
            const double sL0 = is_first ? s0_arr[0] : laps[bot_laps[sp - 1]].ub_botL;
            const double sR0 = is_first ? s0_arr[1] : laps[bot_laps[sp - 1]].ub_botR;

            const int vL1 = is_last ? v_s1_botL : lv[bot_laps[sp]].va_extL;
            const int vR1 = is_last ? v_s1_botR : lv[bot_laps[sp]].va_extR;
            const double sL1 = is_last ? s1_arr[0] : laps[bot_laps[sp]].ua_botL;
            const double sR1 = is_last ? s1_arr[1] : laps[bot_laps[sp]].ua_botR;

            std::vector<LoopEntry> loop;
            loop.push_back({vL0, Point(0.0, norm_u(sL0), 0.0), nullptr});
            if (is_first && !single_front)
                loop.push_back({v_sf_bot, Point(vf_bot, norm_u(uf_bot), 0.0), nullptr});
            loop.push_back({vR0, Point(1.0, norm_u(sR0), 0.0), [&, sR0, sR1]() { return sample_fibre_xy(hx, -hy, sR0, sR1); }});
            loop.push_back({vR1, Point(1.0, norm_u(sR1), 0.0), nullptr});
            if (is_last && !single_back)
                loop.push_back({v_sb_bot, Point(vb_bot, norm_u(ub_bot), 0.0), nullptr});
            loop.push_back({vL1, Point(0.0, norm_u(sL1), 0.0), [&, sL1, sL0]() { return sample_fibre_xy(-hx, -hy, sL1, sL0); }});
            add_loop_face(sid_below, loop);
        }
    }

    // 3. Notch seats & planar cheeks
    for (size_t i = 0; i < laps.size(); i++) {
        const ResolvedLap& rl = laps[i];
        const LapVerts& c = lv[i];
        if (rl.upper) {
            const NurbsSurface surf_mid = Primitives::create_ruled(rail_mid_L, rail_mid_R);
            const int sid_mid = brep.add_surface(surf_mid);
            add_loop_face(
                sid_mid,
                {
                    {c.va_midL, Point(norm_u(rl.ua_midL), 0.0, 0.0), [&, rl]() { return sample_fibre_xy(-hx, 0.0, rl.ua_midL, rl.ub_midL); }},
                    {c.vb_midL, Point(norm_u(rl.ub_midL), 0.0, 0.0), nullptr},
                    {c.vb_midR, Point(norm_u(rl.ub_midR), 1.0, 0.0), [&, rl]() { return sample_fibre_xy(hx, 0.0, rl.ub_midR, rl.ua_midR); }},
                    {c.va_midR, Point(norm_u(rl.ua_midR), 1.0, 0.0), nullptr},
                }
            );
            add_planar_quad(c.va_midL, c.va_midR, c.va_extR, c.va_extL);
            add_planar_quad(c.vb_midL, c.vb_extL, c.vb_extR, c.vb_midR);
        } else {
            NurbsSurface surf_mid = Primitives::create_ruled(rail_mid_L, rail_mid_R);
            surf_mid.transpose();
            const int sid_mid = brep.add_surface(surf_mid);
            add_loop_face(
                sid_mid,
                {
                    {c.va_midL, Point(0.0, norm_u(rl.ua_midL), 0.0), nullptr},
                    {c.va_midR, Point(1.0, norm_u(rl.ua_midR), 0.0), [&, rl]() { return sample_fibre_xy(hx, 0.0, rl.ua_midR, rl.ub_midR); }},
                    {c.vb_midR, Point(1.0, norm_u(rl.ub_midR), 0.0), nullptr},
                    {c.vb_midL, Point(0.0, norm_u(rl.ub_midL), 0.0), [&, rl]() { return sample_fibre_xy(-hx, 0.0, rl.ub_midL, rl.ua_midL); }},
                }
            );
            add_planar_quad(c.va_extL, c.va_extR, c.va_midR, c.va_midL);
            add_planar_quad(c.vb_extL, c.vb_midL, c.vb_midR, c.vb_extR);
        }
    }

    // 4. Start and end caps
    if (single_front) {
        add_planar_quad(v_s0_botL, v_s0_topL, v_s0_topR, v_s0_botR);
    } else {
        add_planar_quad(v_s0_botL, v_s0_topL, v_sf_top, v_sf_bot);
        add_planar_quad(v_sf_bot, v_sf_top, v_s0_topR, v_s0_botR);
    }

    if (single_back) {
        add_planar_quad(v_s1_botL, v_s1_botR, v_s1_topR, v_s1_topL);
    } else {
        add_planar_quad(v_s1_botL, v_sb_bot, v_sb_top, v_s1_topL);
        add_planar_quad(v_sb_bot, v_s1_botR, v_s1_topR, v_sb_top);
    }

    // 5. Left side face on surf_left (CCW in (u, w))
    {
        std::vector<LoopEntry> lloop;
        int cur_v = v_s0_botL;
        double cur_s = s0_arr[0];
        for (size_t i = 0; i < laps.size(); i++) {
            if (!laps[i].upper) {
                const double sa = laps[i].ua_botL;
                lloop.push_back({cur_v, Point(norm_u(cur_s), 0.0, 0.0), [&, cur_s, sa]() { return sample_fibre_xy(-hx, -hy, cur_s, sa); }});
                lloop.push_back({lv[i].va_extL, Point(norm_u(sa), 0.0, 0.0), nullptr});
                const double sm0 = laps[i].ua_midL;
                const double sm1 = laps[i].ub_midL;
                lloop.push_back({lv[i].va_midL, Point(norm_u(sm0), 0.5, 0.0), [&, sm0, sm1]() { return sample_fibre_xy(-hx, 0.0, sm0, sm1); }});
                lloop.push_back({lv[i].vb_midL, Point(norm_u(sm1), 0.5, 0.0), nullptr});
                cur_v = lv[i].vb_extL;
                cur_s = laps[i].ub_botL;
            }
        }
        const double s_end_bot = s1_arr[0];
        lloop.push_back({cur_v, Point(norm_u(cur_s), 0.0, 0.0), [&, cur_s, s_end_bot]() { return sample_fibre_xy(-hx, -hy, cur_s, s_end_bot); }});
        lloop.push_back({v_s1_botL, Point(norm_u(s_end_bot), 0.0, 0.0), nullptr});

        cur_v = v_s1_topL;
        cur_s = s1_arr[3];
        for (size_t idx = laps.size(); idx > 0; idx--) {
            const size_t i = idx - 1;
            if (laps[i].upper) {
                const double sb = laps[i].ub_topL;
                lloop.push_back({cur_v, Point(norm_u(cur_s), 1.0, 0.0), [&, cur_s, sb]() { return sample_fibre_xy(-hx, hy, cur_s, sb); }});
                lloop.push_back({lv[i].vb_extL, Point(norm_u(sb), 1.0, 0.0), nullptr});
                const double sm1 = laps[i].ub_midL;
                const double sm0 = laps[i].ua_midL;
                lloop.push_back({lv[i].vb_midL, Point(norm_u(sm1), 0.5, 0.0), [&, sm1, sm0]() { return sample_fibre_xy(-hx, 0.0, sm1, sm0); }});
                lloop.push_back({lv[i].va_midL, Point(norm_u(sm0), 0.5, 0.0), nullptr});
                cur_v = lv[i].va_extL;
                cur_s = laps[i].ua_topL;
            }
        }
        const double s_start_top = s0_arr[3];
        lloop.push_back({cur_v, Point(norm_u(cur_s), 1.0, 0.0), [&, cur_s, s_start_top]() { return sample_fibre_xy(-hx, hy, cur_s, s_start_top); }});
        lloop.push_back({v_s0_topL, Point(norm_u(s_start_top), 1.0, 0.0), nullptr});
        add_loop_face(sid_left, lloop);
    }

    // 6. Right side face on surf_right (CCW in (w, u))
    {
        std::vector<LoopEntry> rloop;
        rloop.push_back({v_s0_botR, Point(0.0, norm_u(s0_arr[1]), 0.0), nullptr});

        int cur_v = v_s0_topR;
        double cur_s = s0_arr[2];
        for (size_t i = 0; i < laps.size(); i++) {
            if (laps[i].upper) {
                const double sa = laps[i].ua_topR;
                rloop.push_back({cur_v, Point(1.0, norm_u(cur_s), 0.0), [&, cur_s, sa]() { return sample_fibre_xy(hx, hy, cur_s, sa); }});
                rloop.push_back({lv[i].va_extR, Point(1.0, norm_u(sa), 0.0), nullptr});
                const double sm0 = laps[i].ua_midR;
                const double sm1 = laps[i].ub_midR;
                rloop.push_back({lv[i].va_midR, Point(0.5, norm_u(sm0), 0.0), [&, sm0, sm1]() { return sample_fibre_xy(hx, 0.0, sm0, sm1); }});
                rloop.push_back({lv[i].vb_midR, Point(0.5, norm_u(sm1), 0.0), nullptr});
                cur_v = lv[i].vb_extR;
                cur_s = laps[i].ub_topR;
            }
        }
        const double s_end_top = s1_arr[2];
        rloop.push_back({cur_v, Point(1.0, norm_u(cur_s), 0.0), [&, cur_s, s_end_top]() { return sample_fibre_xy(hx, hy, cur_s, s_end_top); }});
        rloop.push_back({v_s1_topR, Point(1.0, norm_u(s_end_top), 0.0), nullptr});

        cur_v = v_s1_botR;
        cur_s = s1_arr[1];
        for (size_t idx = laps.size(); idx > 0; idx--) {
            const size_t i = idx - 1;
            if (!laps[i].upper) {
                const double sb = laps[i].ub_botR;
                rloop.push_back({cur_v, Point(0.0, norm_u(cur_s), 0.0), [&, cur_s, sb]() { return sample_fibre_xy(hx, -hy, cur_s, sb); }});
                rloop.push_back({lv[i].vb_extR, Point(0.0, norm_u(sb), 0.0), nullptr});
                const double sm1 = laps[i].ub_midR;
                const double sm0 = laps[i].ua_midR;
                rloop.push_back({lv[i].vb_midR, Point(0.5, norm_u(sm1), 0.0), [&, sm1, sm0]() { return sample_fibre_xy(hx, 0.0, sm1, sm0); }});
                rloop.push_back({lv[i].va_midR, Point(0.5, norm_u(sm0), 0.0), nullptr});
                cur_v = lv[i].va_extR;
                cur_s = laps[i].ua_botR;
            }
        }
        const double s_start_bot = s0_arr[1];
        rloop.push_back({cur_v, Point(0.0, norm_u(cur_s), 0.0), [&, cur_s, s_start_bot]() { return sample_fibre_xy(hx, -hy, cur_s, s_start_bot); }});
        add_loop_face(sid_right, rloop);
    }

    brep.add_solid({{brep.add_shell(faces), BRepOrientation::Forward}});
    brep.update_tolerances();
    return brep;
}

std::vector<Plane> BeamCurved::compute_planes() const {
    return face_planes(model_geometry_mesh());
}

std::optional<Plane> BeamCurved::base_plane() const {
    if (!centreline.is_valid())
        return std::nullopt;
    return get_plane(centreline.domain_start());
}

std::shared_ptr<BeamCurved> BeamCurved::transformed(const Xform& xform) const {
    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<BeamCurved> beam = std::make_shared<BeamCurved>(
        centreline.transformed(xform),
        width,
        height,
        transformed_list(directions, xform),
        num_width,
        num_height,
        samples,
        name
    );
    beam->guid() = guid();
    beam->direction_parameters = direction_parameters;
    beam->extension = extension;
    beam->cuts = transformed_list(cuts, xform);
    for (const CrossLap& lap : cross_laps)
        beam->cross_laps.push_back({lap.plane_a.transformed(xform), lap.plane_b.transformed(xform), lap.upper_notch});
    for (const InteractionFeatureSolid& cut : solid_features)
        beam->solid_features.push_back(cut.transformed(xform));
    for (const InteractionFeaturePlane& feature : plane_features)
        beam->plane_features.push_back(feature.transformed(xform));
    beam->set_features(transformed_features(_features, xform));
    beam->set_insertion_vectors(transformed_list(_insertion_vectors, xform));
    return beam;
}

void BeamCurved::place(const Xform& xform) {
    Element::place(xform);
    centreline.transform(xform);
    directions = transformed_list(directions, xform);
    cuts = transformed_list(cuts, xform);
    for (CrossLap& lap : cross_laps) {
        lap.plane_a = lap.plane_a.transformed(xform);
        lap.plane_b = lap.plane_b.transformed(xform);
    }
    for (InteractionFeatureSolid& cut : solid_features)
        cut = cut.transformed(xform);
    for (InteractionFeaturePlane& feature : plane_features)
        feature = feature.transformed(xform);

    _element_geometry_mesh.reset();
    _element_geometry_brep.reset();
    _model_mesh_cache.reset();
    _model_brep_cache.reset();
}

void BeamCurved::compute_geometry_mesh_impl() {
    const std::vector<Polyline> rings = sections();
    if (!rings.empty())
        set_geometry(model_geometry_mesh());
    compute_geometry_features();
}

void BeamCurved::compute_geometry_brep_impl() {
    const std::vector<Polyline> rings = sections();
    if (!rings.empty())
        set_geometry(model_geometry_brep());
    compute_geometry_features();
}

void BeamCurved::compute_geometry_features() {
    const std::vector<Plane> station_frames = frames();
    std::vector<Point> axis_pts;
    axis_pts.reserve(station_frames.size());
    for (const Plane& frame : station_frames)
        axis_pts.push_back(frame.origin());

    std::vector<Plane> all_cuts = cuts;
    const std::vector<Plane> f_planes = feature_planes();
    all_cuts.insert(all_cuts.end(), f_planes.begin(), f_planes.end());

    const std::pair<Polyline, std::vector<Polyline>> trimmed =
        trim_to_cuts(Polyline(axis_pts), sections(), all_cuts);

    std::vector<ElementFeature> next;
    if (trimmed.first.point_count() > 1)
        next.push_back(polyline_feature("axis", trimmed.first));

    for (const Polyline& ring : trimmed.second)
        if (ring.point_count() > 0)
            next.push_back(polyline_feature("section", ring));

    for (ElementFeature& feature : session_features(*this))
        next.push_back(std::move(feature));

    set_features(std::move(next));
}

AABB BeamCurved::aabb(double inflate) const {
    const Mesh& solid = geometry_mesh();
    if (solid.number_of_vertices() > 0)
        return AABB::from_mesh(solid, inflate);

    std::vector<Point> points;
    for (const Polyline& section : sections()) {
        const std::vector<Point> corners = section.get_points();
        points.insert(points.end(), corners.begin(), corners.end());
    }
    return AABB::from_points(points, inflate);
}

// ═══════════════════════════════════════════════════════════════════════════
// JSON & Protobuf
// ═══════════════════════════════════════════════════════════════════════════

nlohmann::ordered_json BeamCurved::element_data_jsondump() const {
    nlohmann::ordered_json data;
    data["centreline"] = centreline.jsondump();
    data["width"] = width;
    data["height"] = height;
    data["num_width"] = num_width;
    data["num_height"] = num_height;
    data["samples"] = samples;
    data["extension"] = extension;

    nlohmann::ordered_json dirs = nlohmann::ordered_json::array();
    for (const Vector& dir : directions)
        dirs.push_back(dir.jsondump());
    data["directions"] = std::move(dirs);
    data["direction_parameters"] = direction_parameters;

    nlohmann::ordered_json cut_list = nlohmann::ordered_json::array();
    for (const Plane& cut : cuts)
        cut_list.push_back(cut.jsondump());
    data["cuts"] = std::move(cut_list);

    nlohmann::ordered_json lap_list = nlohmann::ordered_json::array();
    for (const CrossLap& lap : cross_laps) {
        nlohmann::ordered_json lj;
        lj["plane_a"] = lap.plane_a.jsondump();
        lj["plane_b"] = lap.plane_b.jsondump();
        lj["upper_notch"] = lap.upper_notch;
        lap_list.push_back(std::move(lj));
    }
    data["cross_laps"] = std::move(lap_list);

    nlohmann::ordered_json solids = nlohmann::ordered_json::array();
    for (const InteractionFeatureSolid& cut : solid_features) {
        const std::string raw = cut.pb_dumps();
        solids.push_back(std::vector<uint8_t>(raw.begin(), raw.end()));
    }
    data["solid_features"] = std::move(solids);

    nlohmann::ordered_json planes = nlohmann::ordered_json::array();
    for (const InteractionFeaturePlane& feature : plane_features) {
        const std::string raw = feature.pb_dumps();
        planes.push_back(std::vector<uint8_t>(raw.begin(), raw.end()));
    }
    data["plane_features"] = std::move(planes);

    return data;
}

std::string BeamCurved::element_data_dumps() const {
    return element_data_jsondump().dump();
}

static std::shared_ptr<Element> beam_curved_from_protobuf(const std::string& data) {
    return BeamCurved::from_element(Element::pb_loads(data));
}

void BeamCurved::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), beam_curved_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string BeamCurved::str() const {
    std::ostringstream os;
    os << "BeamCurved(name=" << name << ", width=" << width << ", height=" << height
       << ", lamellae=" << num_width << "x" << num_height << ", samples=" << samples
       << ", is_visible=" << std::boolalpha << is_visible << ", is_locked=" << is_locked << ")";
    return os.str();
}

}
