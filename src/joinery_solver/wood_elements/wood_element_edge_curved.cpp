#include "pch.h"
#include "wood_element_edge_curved.h"
#include "wood_element_geometry.h"
#include "primitives.h"
#include <map>

namespace wood_session {

using namespace laika_cpp;

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

/// How far a curve's control points may lie from its plane, in mm, and its guides' x axes from each other as unit vectors, and still count as planar.
constexpr double PLANAR_CURVE = 1e-3;
constexpr double PLANAR_GUIDES = 1e-6;

/// How close to a cut plane, in mm, a fibre's crossing is taken as found.
constexpr double CROSSING_TOLERANCE = 1e-8;

/// Intervals a fibre is scanned over from its end, enough to keep the first of two crossings on a strongly curved beam.
constexpr int CROSSING_SCAN = 8;

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
    // Map to FaceCurvedElement meridian ordering: outer_a (top-left c3), outer_b (top-right c2), inner_a (bottom-left c0), inner_b (bottom-right c1)
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

EdgeCurvedElement::EdgeCurvedElement() : EdgeElement("beam_curved") {
}

EdgeCurvedElement::EdgeCurvedElement(
    const NurbsCurve& centreline, double width, double height, const std::string& name
)
    : EdgeElement(name), centreline(centreline), width(width), height(height) {
    if (!centreline.is_valid())
        throw std::invalid_argument("An EdgeCurvedElement requires a valid NurbsCurve centreline");
}

EdgeCurvedElement::EdgeCurvedElement(
    const NurbsCurve& centreline,
    double width,
    double height,
    const Vector& up,
    int num_width,
    int num_height,
    int samples,
    const std::string& name
)
    : EdgeElement(name), centreline(centreline), width(width), height(height), directions({up}),
      num_width(std::max(1, num_width)), num_height(std::max(1, num_height)),
      samples(std::max(2, samples)) {
    if (!centreline.is_valid())
        throw std::invalid_argument("An EdgeCurvedElement requires a valid NurbsCurve centreline");
}

EdgeCurvedElement::EdgeCurvedElement(
    const NurbsCurve& centreline,
    double width,
    double height,
    const std::vector<Vector>& directions,
    int num_width,
    int num_height,
    int samples,
    const std::string& name
)
    : EdgeElement(name), centreline(centreline), width(width), height(height),
      directions(directions), num_width(std::max(1, num_width)),
      num_height(std::max(1, num_height)), samples(std::max(2, samples)) {
    if (!centreline.is_valid())
        throw std::invalid_argument("An EdgeCurvedElement requires a valid NurbsCurve centreline");
}

// ═══════════════════════════════════════════════════════════════════════════
// Static constructors
// ═══════════════════════════════════════════════════════════════════════════

std::shared_ptr<EdgeCurvedElement> EdgeCurvedElement::single_curved(
    const NurbsCurve& centreline,
    double width,
    double height,
    int num_height,
    int samples,
    const std::string& name
) {
    return std::make_shared<EdgeCurvedElement>(
        centreline, width, height, std::vector<Vector>{}, 1, std::max(1, num_height), samples, name
    );
}

std::shared_ptr<EdgeCurvedElement> EdgeCurvedElement::double_curved(
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
    std::shared_ptr<EdgeCurvedElement> beam = std::make_shared<EdgeCurvedElement>(
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

std::shared_ptr<EdgeCurvedElement> EdgeCurvedElement::from_surface(
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

std::shared_ptr<EdgeCurvedElement> EdgeCurvedElement::from_element(Element e) {
    const std::string bytes = e.element_data_dumps();
    std::shared_ptr<EdgeCurvedElement> beam = std::make_shared<EdgeCurvedElement>();
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

double EdgeCurvedElement::lamella_width() const {
    return width / static_cast<double>(std::max(1, num_width));
}

double EdgeCurvedElement::lamella_height() const {
    return height / static_cast<double>(std::max(1, num_height));
}

EdgeCurvedElement::Twist EdgeCurvedElement::compute_twist() const {
    Twist twist;
    const size_t m_count = directions.size();
    if (m_count < 2 || !centreline.is_valid())
        return twist;

    const double t0 = centreline.domain_start();
    const double t1 = centreline.domain_end();
    twist.parameters = direction_parameters;
    if (twist.parameters.size() != m_count) {
        twist.parameters.resize(m_count);
        for (size_t i = 0; i < m_count; i++)
            twist.parameters[i] =
                t0 + (t1 - t0) * static_cast<double>(i) / static_cast<double>(m_count - 1);
    }

    // the section's x at every guide: all the same on a curve lying in that plane, so nothing turns
    std::optional<Vector> x_axis;
    bool planar = true;
    for (size_t i = 0; i < m_count && planar; i++) {
        const Vector tangent = centreline.tangent_at(std::clamp(twist.parameters[i], t0, t1));
        const Vector x = directions[i].cross(tangent);
        planar = x.magnitude() > Tolerance::ZERO_TOLERANCE;
        if (planar && !x_axis)
            x_axis = x.normalized();
        planar = planar && (x.normalized() - *x_axis).magnitude() < PLANAR_GUIDES;
    }
    for (int i = 0; i < centreline.cv_count() && planar; i++)
        planar = std::abs((centreline.get_cv(i) - centreline.get_cv(0)).dot(*x_axis)) < PLANAR_CURVE;
    if (planar) {
        twist.x_axis = x_axis;
        return twist;
    }

    twist.angles.assign(m_count, 0.0);
    for (size_t i = 0; i < m_count; i++) {
        const double pi = std::clamp(twist.parameters[i], t0, t1);
        const Plane rmf_i = centreline.perpendicular_plane_at(pi, false);
        const double dx = directions[i].dot(rmf_i.x_axis());
        const double dy = directions[i].dot(rmf_i.y_axis());
        twist.angles[i] = std::atan2(dy, dx);
    }

    // Unwrap angle jumps across pi
    for (size_t i = 1; i < m_count; i++) {
        while (twist.angles[i] - twist.angles[i - 1] > Tolerance::PI)
            twist.angles[i] -= Tolerance::TWO_PI;
        while (twist.angles[i] - twist.angles[i - 1] < -Tolerance::PI)
            twist.angles[i] += Tolerance::TWO_PI;
    }

    return twist;
}

Plane EdgeCurvedElement::get_plane(double t) const {
    return get_plane(t, compute_twist());
}

Plane EdgeCurvedElement::get_plane(double t, const Twist& twist) const {
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
    if (twist.x_axis) {
        const Vector z = tangent.normalized();
        return Plane::from_frame(origin, *twist.x_axis, z.cross(*twist.x_axis).normalized(), z);
    }
    if (directions.size() >= 2) {
        const std::vector<double>& params = twist.parameters;
        const std::vector<double>& angles = twist.angles;
        const size_t m_count = angles.size();

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

std::vector<Plane> EdgeCurvedElement::frames() const {
    std::vector<Plane> result;
    if (!centreline.is_valid())
        return result;

    const int n = std::max(2, samples);
    std::vector<Point> pts;
    std::vector<double> params;
    centreline.divide_by_count(n, pts, &params, true);

    const Twist twist = compute_twist();
    result.reserve(params.size());
    for (size_t i = 0; i < params.size(); i++) {
        Plane frame = get_plane(params[i], twist);
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

bool EdgeCurvedElement::is_single_curved(double tolerance) const {
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

bool EdgeCurvedElement::is_double_curved(double tolerance) const {
    return !is_single_curved(tolerance);
}

std::vector<Polyline> EdgeCurvedElement::sections() const {
    return sections(frames());
}

std::vector<Polyline> EdgeCurvedElement::sections(const std::vector<Plane>& station_frames) const {
    std::vector<Polyline> rings;
    if (!centreline.is_valid() || width <= 0.0 || height <= 0.0)
        return rings;

    const double hx = 0.5 * width;
    const double hy = 0.5 * height;
    rings.reserve(station_frames.size());
    for (const Plane& frame : station_frames)
        rings.push_back(section_in_plane(frame, -hx, -hy, hx, hy));

    return rings;
}

std::array<NurbsCurve, 4> EdgeCurvedElement::edge_curves() const {
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

NurbsCurve EdgeCurvedElement::offset_curve(double x, double y) const {
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

NurbsCurve EdgeCurvedElement::lamella_curve(int ix, int iy) const {
    const int nw = std::max(1, num_width);
    const int nh = std::max(1, num_height);
    const double lw = width / static_cast<double>(nw);
    const double lh = height / static_cast<double>(nh);
    const double x = -0.5 * width + (static_cast<double>(ix) + 0.5) * lw;
    const double y = -0.5 * height + (static_cast<double>(iy) + 0.5) * lh;
    return offset_curve(x, y);
}

std::vector<NurbsCurve> EdgeCurvedElement::lamella_curves() const {
    std::vector<NurbsCurve> curves;
    const int nw = std::max(1, num_width);
    const int nh = std::max(1, num_height);
    curves.reserve(nw * nh);
    for (int iy = 0; iy < nh; iy++)
        for (int ix = 0; ix < nw; ix++)
            curves.push_back(lamella_curve(ix, iy));
    return curves;
}

std::vector<Mesh> EdgeCurvedElement::lamella_meshes() const {
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

std::vector<BRep> EdgeCurvedElement::lamella_breps() const {
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

double EdgeCurvedElement::max_curvature(double& max_kw, double& max_kh) const {
    max_kw = 0.0;
    max_kh = 0.0;
    double max_k = 0.0;
    if (!centreline.is_valid())
        return 0.0;

    std::vector<Point> pts;
    std::vector<double> params;
    centreline.divide_by_count(std::max(16, samples), pts, &params, true);

    const Twist twist = compute_twist();
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
        const Plane frame = get_plane(t, twist);
        max_kw = std::max(max_kw, std::abs(kappa_vec.dot(frame.x_axis())));
        max_kh = std::max(max_kh, std::abs(kappa_vec.dot(frame.y_axis())));
        max_k = std::max(max_k, kappa_vec.magnitude());
    }

    return max_k;
}

// ═══════════════════════════════════════════════════════════════════════════
// Geometry
// ═══════════════════════════════════════════════════════════════════════════

const Mesh& EdgeCurvedElement::element_geometry_mesh() const {
    if (!_element_geometry_mesh)
        _element_geometry_mesh = sweep_sections(sections());
    return *_element_geometry_mesh;
}

const BRep& EdgeCurvedElement::element_geometry_brep() const {
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

Mesh EdgeCurvedElement::trimmed_mesh() const {
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

namespace {

// ═══════════════════════════════════════════════════════════════════════════
// Exact BRep trimming
// ═══════════════════════════════════════════════════════════════════════════

/// Evaluates extended cross-section frames, fibre points, and cut-plane crossings on a curved beam.
struct CurvedTrimContext {
    const EdgeCurvedElement* beam = nullptr; // Source curved beam.
    EdgeCurvedElement::Twist twist; // Unwrapped guide twist along the centreline.
    double t0 = 0.0; // Start parameter of the centreline domain.
    double span = 0.0; // Parameter span of the centreline domain.
    double u_lo_bound = -0.35; // Lower normalized scan bound past start.
    double u_hi_bound = 1.35; // Upper normalized scan bound past end.
    double s_min = 0.0; // Minimum normalized parameter of the trimmed solid.
    double s_max = 1.0; // Maximum normalized parameter of the trimmed solid.

    /// Returns the cross-section plane at normalized parameter u_norm, extrapolated past [0, 1].
    Plane extended_plane_at(double u_norm) const {

        const double u_clamped = std::clamp(u_norm, 0.0, 1.0);
        const double t = t0 + u_clamped * span;
        const Plane base = beam->get_plane(t, twist);

        if (u_norm >= 0.0 && u_norm <= 1.0)
            return base;

        const double dt = (u_norm < 0.0 ? u_norm : (u_norm - 1.0)) * span;
        const std::vector<Vector> derivs = beam->centreline.evaluate(t, 2);

        if (derivs.size() >= 3 && derivs[1].magnitude() > Tolerance::ZERO_TOLERANCE) {
            const Vector& d1 = derivs[1];
            const Vector& d2 = derivs[2];
            const Point origin = base.origin() + d1 * dt + d2 * (0.5 * dt * dt);
            const Vector tangent = d1 + d2 * dt;
            return frame_from_tangent_and_up(origin, tangent, base.y_axis());
        }

        return Plane::from_frame(
            base.origin() + base.z_axis() * dt,
            base.x_axis(),
            base.y_axis(),
            base.z_axis()
        );
    }

    /// Evaluates the 3D point on fibre (x_off, y_off) at normalized parameter u_norm.
    Point eval_pt(double x_off, double y_off, double u_norm) const {

        const Plane fr = extended_plane_at(u_norm);

        return fr.origin() + fr.x_axis() * x_off + fr.y_axis() * y_off;
    }

    /// Signed distance from fibre (x_off, y_off) at u_norm to plane.
    double signed_distance(
        const Plane& plane,
        double x_off,
        double y_off,
        double u_norm
    ) const {
        return (eval_pt(x_off, y_off, u_norm) - plane.origin()).dot(plane.z_axis());
    }

    /// Finds the crossing of fibre (x_off, y_off) with plane on [a, b] by Illinois false position.
    double crossing_on_fibre(
        const Plane& plane,
        double x_off,
        double y_off,
        double a,
        double value_a,
        double b,
        double value_b
    ) const {

        if (value_a * value_b > 0.0)
            return b;

        for (int i = 0; i < 64 && std::abs(b - a) > 1e-13; i++) {
            const double c = (a * value_b - b * value_a) / (value_b - value_a);
            const double value = signed_distance(
                plane,
                x_off,
                y_off,
                c
            );

            if (std::abs(value) < CROSSING_TOLERANCE)
                return c;

            if (value * value_b < 0.0) {
                a = b;
                value_a = value_b;
            } else {
                value_a *= 0.5;
            }

            b = c;
            value_b = value;
        }

        return 0.5 * (a + b);
    }

    /// Finds a front-end cut-plane crossing on fibre (x_off, y_off), or nullopt when none trims it.
    std::optional<double> bisect_front(const Plane& plane, double x_off, double y_off) const {

        const double d_lo = signed_distance(
            plane,
            x_off,
            y_off,
            u_lo_bound
        );
        const double d_mid = signed_distance(
            plane,
            x_off,
            y_off,
            0.5
        );

        if (!(d_lo < -Tolerance::APPROXIMATION && d_mid >= -Tolerance::APPROXIMATION))
            return std::nullopt;

        double a = u_lo_bound;
        double value_a = d_lo;
        double b = 0.5;
        double value_b = d_mid;

        for (int k = 1; k <= CROSSING_SCAN; k++) {
            const double u_k =
                u_lo_bound + (0.9 - u_lo_bound) * static_cast<double>(k) / CROSSING_SCAN;
            const double value = signed_distance(
                plane,
                x_off,
                y_off,
                u_k
            );

            if (value >= 0.0) {
                b = u_k;
                value_b = value;
                break;
            }

            a = u_k;
            value_a = value;
        }

        return crossing_on_fibre(
            plane,
            x_off,
            y_off,
            a,
            value_a,
            b,
            value_b
        );
    }

    /// Finds a back-end cut-plane crossing on fibre (x_off, y_off), or nullopt when none trims it.
    std::optional<double> bisect_back(const Plane& plane, double x_off, double y_off) const {

        const double d_mid = signed_distance(
            plane,
            x_off,
            y_off,
            0.5
        );
        const double d_hi = signed_distance(
            plane,
            x_off,
            y_off,
            u_hi_bound
        );

        if (!(d_hi < -Tolerance::APPROXIMATION && d_mid >= -Tolerance::APPROXIMATION))
            return std::nullopt;

        double a = 0.5;
        double value_a = d_mid;
        double b = u_hi_bound;
        double value_b = d_hi;

        for (int k = CROSSING_SCAN - 1; k >= 0; k--) {
            const double u_k = 0.1 + (u_hi_bound - 0.1) * static_cast<double>(k) / CROSSING_SCAN;
            const double value = signed_distance(
                plane,
                x_off,
                y_off,
                u_k
            );

            if (value >= 0.0) {
                a = u_k;
                value_a = value;
                break;
            }

            b = u_k;
            value_b = value;
        }

        return crossing_on_fibre(
            plane,
            x_off,
            y_off,
            a,
            value_a,
            b,
            value_b
        );
    }

    /// Returns the furthest front-end cut parameter at lateral fraction v_frac and height y_off.
    double front_envelope(double v_frac, double y_off, int& win_idx) const {

        const double x_off = (-0.5 + v_frac) * beam->width;
        double best_u = 0.0;
        bool has = false;
        win_idx = -1;

        for (size_t p_idx = 0; p_idx < beam->cuts.size(); p_idx++) {
            const std::optional<double> cand = bisect_front(beam->cuts[p_idx], x_off, y_off);

            if (cand && (!has || *cand > best_u)) {
                best_u = *cand;
                win_idx = static_cast<int>(p_idx);
                has = true;
            }
        }

        return best_u;
    }

    /// Returns the nearest back-end cut parameter at lateral fraction v_frac and height y_off.
    double back_envelope(double v_frac, double y_off, int& win_idx) const {

        const double x_off = (-0.5 + v_frac) * beam->width;
        double best_u = 1.0;
        bool has = false;
        win_idx = -1;

        for (size_t p_idx = 0; p_idx < beam->cuts.size(); p_idx++) {
            const std::optional<double> cand = bisect_back(beam->cuts[p_idx], x_off, y_off);

            if (cand && (!has || *cand < best_u)) {
                best_u = *cand;
                win_idx = static_cast<int>(p_idx);
                has = true;
            }
        }

        return best_u;
    }

    /// True when any cut plane clips out fibre (x_off, y_off) across the entire scan range.
    bool fibre_clipped_out(double x_off, double y_off) const {

        for (const Plane& plane : beam->cuts) {
            const double d_lo = signed_distance(
                plane,
                x_off,
                y_off,
                u_lo_bound
            );
            const double d_mid = signed_distance(
                plane,
                x_off,
                y_off,
                0.5
            );
            const double d_hi = signed_distance(
                plane,
                x_off,
                y_off,
                u_hi_bound
            );

            if (d_lo < -Tolerance::APPROXIMATION && d_mid < -Tolerance::APPROXIMATION &&
                d_hi < -Tolerance::APPROXIMATION)
                return true;
        }

        return false;
    }

    /// Interpolates a unit-domain NURBS curve along fibre (x_off, y_off) from sa to sb.
    NurbsCurve sample_fibre_xy(
        double x_off,
        double y_off,
        double sa,
        double sb
    ) const {

        const int count = std::max(8, beam->samples);
        std::vector<Point> sub_pts;
        sub_pts.reserve(count + 1);

        for (int j = 0; j <= count; j++) {
            const double u = sa + (sb - sa) * static_cast<double>(j) / static_cast<double>(count);
            sub_pts.push_back(eval_pt(x_off, y_off, u));
        }

        return unit_domain(NurbsCurve::create_interpolated(
            sub_pts, CurveNurbsKnotStyle::Uniform, CurveInterpStyle::Occt
        ));
    }

    /// Solves the lateral fraction out_v and parameter out_u where two end miter planes meet.
    void solve_miter_v(
        bool is_front,
        double y_off,
        int c_left,
        int c_right,
        double& out_v,
        double& out_u
    ) const {

        if (c_left == c_right) {
            out_v = 0.5;
            int dummy = -1;
            out_u = is_front ? front_envelope(0.5, y_off, dummy) : back_envelope(0.5, y_off, dummy);
            return;
        }

        Line meet;

        if (twist.x_axis &&
            Intersection::plane_plane(beam->cuts[c_left], beam->cuts[c_right], meet) &&
            std::abs(meet.to_vector().normalized().dot(*twist.x_axis)) < PLANAR_GUIDES) {
            const double offset = (meet.start() - eval_pt(0.0, y_off, 0.5)).dot(*twist.x_axis);
            out_v = std::clamp(0.5 + offset / beam->width, 0.005, 0.995);
            int dummy = -1;
            out_u =
                is_front ? front_envelope(out_v, y_off, dummy) : back_envelope(out_v, y_off, dummy);
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
    }

    /// Finds the crossing of a cross-lap cheek plane on fibre (x_off, y_off) within [s_min, s_max].
    bool bisect_lap(const Plane& plane, double x_off, double y_off, double& out_u) const {

        const double f0 = signed_distance(
            plane,
            x_off,
            y_off,
            s_min
        );
        const double f1 = signed_distance(
            plane,
            x_off,
            y_off,
            s_max
        );

        if ((f0 >= 0.0 && f1 >= 0.0) || (f0 <= 0.0 && f1 <= 0.0))
            return false;

        out_u = crossing_on_fibre(
            plane,
            x_off,
            y_off,
            s_min,
            f0,
            s_max,
            f1
        );

        return true;
    }

    /// Maps normalized parameter s onto [0, 1] across [s_min, s_max].
    double norm_u(double s) const {
        return std::clamp((s - s_min) / (s_max - s_min), 0.0, 1.0);
    }
};

/// Normalized fibre crossing parameters of one interior cross-lap notch.
struct ResolvedLap {
    bool upper = false; // True when the notch removes the upper half.
    double ua_botL = 0.0; // Plane A crossing on bottom-left fibre.
    double ua_midL = 0.0; // Plane A crossing on mid-left fibre.
    double ua_topL = 0.0; // Plane A crossing on top-left fibre.
    double ua_botR = 0.0; // Plane A crossing on bottom-right fibre.
    double ua_midR = 0.0; // Plane A crossing on mid-right fibre.
    double ua_topR = 0.0; // Plane A crossing on top-right fibre.
    double ub_botL = 0.0; // Plane B crossing on bottom-left fibre.
    double ub_midL = 0.0; // Plane B crossing on mid-left fibre.
    double ub_topL = 0.0; // Plane B crossing on top-left fibre.
    double ub_botR = 0.0; // Plane B crossing on bottom-right fibre.
    double ub_midR = 0.0; // Plane B crossing on mid-right fibre.
    double ub_topR = 0.0; // Plane B crossing on top-right fibre.
};

/// Orders resolved cross-lap notches along the beam by their mid-left entry parameter.
bool resolved_lap_before(const ResolvedLap& a, const ResolvedLap& b) {
    return a.ua_midL < b.ua_midL;
}

/// Resolves the six fibre crossings of one cross-lap cheek plane on left and right sides.
bool resolve_lap_cheek(
    const CurvedTrimContext& ctx,
    const Plane& plane,
    double hx,
    double hy,
    double& u_botL,
    double& u_midL,
    double& u_topL,
    double& u_botR,
    double& u_midR,
    double& u_topR
) {
    return ctx.bisect_lap(
               plane,
               -hx,
               -hy,
               u_botL
           ) &&
           ctx.bisect_lap(
               plane,
               -hx,
               0.0,
               u_midL
           ) &&
           ctx.bisect_lap(
               plane,
               -hx,
               hy,
               u_topL
           ) &&
           ctx.bisect_lap(
               plane,
               hx,
               -hy,
               u_botR
           ) &&
           ctx.bisect_lap(
               plane,
               hx,
               0.0,
               u_midR
           ) &&
           ctx.bisect_lap(
               plane,
               hx,
               hy,
               u_topR
           );
}

/// Resolves and sorts all interior cross-lap notches along the beam; false when any notch misses.
bool resolve_cross_laps(
    const CurvedTrimContext& ctx,
    double hx,
    double hy,
    std::vector<ResolvedLap>& laps
) {

    for (const EdgeCurvedElement::CrossLap& lap : ctx.beam->cross_laps) {
        ResolvedLap rl;
        rl.upper = lap.upper_notch;

        if (!resolve_lap_cheek(
                ctx,
                lap.plane_a,
                hx,
                hy,
                rl.ua_botL,
                rl.ua_midL,
                rl.ua_topL,
                rl.ua_botR,
                rl.ua_midR,
                rl.ua_topR
            ) ||
            !resolve_lap_cheek(
                ctx,
                lap.plane_b,
                hx,
                hy,
                rl.ub_botL,
                rl.ub_midL,
                rl.ub_topL,
                rl.ub_botR,
                rl.ub_midR,
                rl.ub_topR
            ))
            return false;

        if (rl.ua_botL >= rl.ub_botL || rl.ua_midL >= rl.ub_midL || rl.ua_topL >= rl.ub_topL ||
            rl.ua_botR >= rl.ub_botR || rl.ua_midR >= rl.ub_midR || rl.ua_topR >= rl.ub_topR)
            return false;

        laps.push_back(rl);
    }

    std::sort(laps.begin(), laps.end(), resolved_lap_before);

    return true;
}

/// BRep vertex indices at the eight corners of one cross-lap notch.
struct LapVerts {
    int va_extL = -1; // Plane A outer-left vertex.
    int va_extR = -1; // Plane A outer-right vertex.
    int va_midL = -1; // Plane A mid-left vertex.
    int va_midR = -1; // Plane A mid-right vertex.
    int vb_extL = -1; // Plane B outer-left vertex.
    int vb_extR = -1; // Plane B outer-right vertex.
    int vb_midL = -1; // Plane B mid-left vertex.
    int vb_midR = -1; // Plane B mid-right vertex.
};

/// One corner and outgoing edge specification of a trimmed BRep face loop.
struct LoopEntry {
    int vid = -1; // Vertex index at this corner.
    Point uv; // Surface UV coordinates at this corner.
    bool has_fibre = false; // True when the outgoing edge samples a curved fibre.
    double x_off = 0.0; // Fibre cross-section X offset.
    double y_off = 0.0; // Fibre cross-section Y offset.
    double sa = 0.0; // Fibre start normalized parameter.
    double sb = 0.0; // Fibre end normalized parameter.

    /// Corner followed by a straight segment edge to the next vertex.
    static LoopEntry straight(int vertex, double u, double v) {

        LoopEntry entry;
        entry.vid = vertex;
        entry.uv = Point(u, v, 0.0);

        return entry;
    }

    /// Corner followed by a sampled curved fibre edge to the next vertex.
    static LoopEntry fibre(
        int vertex,
        const Point& uv_pt,
        double x,
        double y,
        double start_s,
        double end_s
    ) {

        LoopEntry entry;
        entry.vid = vertex;
        entry.uv = uv_pt;
        entry.has_fibre = true;
        entry.x_off = x;
        entry.y_off = y;
        entry.sa = start_s;
        entry.sb = end_s;

        return entry;
    }
};

/// Builds the multi-face NURBS BRep of a curved beam with mitered ends or cross-lap notches.
struct TrimmedBRepBuilder {
    const CurvedTrimContext* ctx = nullptr; // Curved beam trim context.
    double hx = 0.0; // Half cross-section width.
    double hy = 0.0; // Half cross-section height.
    std::array<double, 4> s0_arr{}; // Front corner normalized parameters.
    std::array<double, 4> s1_arr{}; // Back corner normalized parameters.
    bool single_front = true; // True when one plane trims the entire front end.
    bool single_back = true; // True when one plane trims the entire back end.
    double vf_bot = 0.5; // Front bottom miter fraction across width.
    double uf_bot = 0.0; // Front bottom miter normalized parameter.
    double vf_top = 0.5; // Front top miter fraction across width.
    double uf_top = 0.0; // Front top miter normalized parameter.
    double vb_bot = 0.5; // Back bottom miter fraction across width.
    double ub_bot = 1.0; // Back bottom miter normalized parameter.
    double vb_top = 0.5; // Back top miter fraction across width.
    double ub_top = 1.0; // Back top miter normalized parameter.
    NurbsCurve rail_mid_L; // Sampled mid-height left rail curve.
    NurbsCurve rail_mid_R; // Sampled mid-height right rail curve.
    int sid_above = -1; // Top ruled surface index in brep.
    int sid_below = -1; // Bottom ruled surface index in brep.
    int sid_left = -1; // Left ruled surface index in brep.
    int sid_right = -1; // Right ruled surface index in brep.
    int v_s0_botL = -1; // Start bottom-left vertex index.
    int v_s0_botR = -1; // Start bottom-right vertex index.
    int v_s0_topR = -1; // Start top-right vertex index.
    int v_s0_topL = -1; // Start top-left vertex index.
    int v_s1_botL = -1; // End bottom-left vertex index.
    int v_s1_botR = -1; // End bottom-right vertex index.
    int v_s1_topR = -1; // End top-right vertex index.
    int v_s1_topL = -1; // End top-left vertex index.
    int v_sf_bot = -1; // Front bottom miter ridge vertex index.
    int v_sf_top = -1; // Front top miter ridge vertex index.
    int v_sb_bot = -1; // Back bottom miter ridge vertex index.
    int v_sb_top = -1; // Back top miter ridge vertex index.
    std::vector<ResolvedLap> laps; // Sorted interior cross-lap notches.
    std::vector<LapVerts> lv; // BRep vertex indices per cross-lap notch.
    BRep brep; // Assembled BRep solid.
    std::map<std::pair<int, int>, int> edge_map; // Shared 3D edge lookup by vertex pair.
    std::vector<BRepRef> faces; // Oriented face references for the outer shell.

    /// Returns the shared edge between va and vb, creating its 3D curve on first use.
    int get_edge(int va, int vb, const LoopEntry& entry) {

        const std::pair<int, int> key = {std::min(va, vb), std::max(va, vb)};
        const auto it = edge_map.find(key);

        if (it != edge_map.end())
            return it->second;

        NurbsCurve crv =
            entry.has_fibre
                ? ctx->sample_fibre_xy(
                      entry.x_off,
                      entry.y_off,
                      entry.sa,
                      entry.sb
                  )
                : segment_curve(brep.m_vertices[va].point, brep.m_vertices[vb].point);

        if (va != key.first)
            crv.reverse();

        const int eid =
            brep.add_edge(brep.add_curve_3d(unit_domain(crv)), key.first, key.second);
        edge_map[key] = eid;

        return eid;
    }

    /// Adds a face on surface sid bounded by entries in counter-clockwise UV order.
    void add_loop_face(int sid, const std::vector<LoopEntry>& entries) {

        std::vector<BRepRef> wire;
        const size_t n = entries.size();
        wire.reserve(n);

        for (size_t i = 0; i < n; i++) {
            const size_t next = (i + 1) % n;
            const int va = entries[i].vid;
            const int vb = entries[next].vid;
            const int eid = get_edge(va, vb, entries[i]);
            const bool fwd = brep.m_edges[eid].start_vertex == va;
            const int cid2 = brep.add_curve_2d(
                fwd ? segment_curve(entries[i].uv, entries[next].uv)
                    : segment_curve(entries[next].uv, entries[i].uv)
            );
            brep.add_pcurve(eid, sid, cid2);
            wire.push_back({eid, fwd ? BRepOrientation::Forward : BRepOrientation::Reversed});
        }

        const int wire_id = brep.add_wire(wire);
        const int face_id = brep.add_face(sid, {{wire_id, BRepOrientation::Forward}});
        faces.push_back({face_id, BRepOrientation::Forward});
    }

    /// Adds a ruled planar quad face through four BRep vertex indices.
    void add_planar_quad(const std::array<int, 4>& quad) {

        const Point& p0 = brep.m_vertices[quad[0]].point;
        const Point& p1 = brep.m_vertices[quad[1]].point;
        const Point& p2 = brep.m_vertices[quad[2]].point;
        const Point& p3 = brep.m_vertices[quad[3]].point;
        const NurbsSurface qsurf =
            Primitives::create_ruled(segment_curve(p0, p1), segment_curve(p3, p2));
        const int qsid = brep.add_surface(qsurf);
        add_loop_face(
            qsid,
            {
                LoopEntry::straight(quad[0], 0.0, 0.0),
                LoopEntry::straight(quad[1], 1.0, 0.0),
                LoopEntry::straight(quad[2], 1.0, 1.0),
                LoopEntry::straight(quad[3], 0.0, 1.0),
            }
        );
    }

    /// Initializes the four main ruled surfaces and all corner, miter, and notch vertices.
    void init_surfaces_and_vertices() {

        const NurbsCurve rail_bot_L = ctx->sample_fibre_xy(
            -hx,
            -hy,
            ctx->s_min,
            ctx->s_max
        );
        const NurbsCurve rail_bot_R = ctx->sample_fibre_xy(
            hx,
            -hy,
            ctx->s_min,
            ctx->s_max
        );
        rail_mid_L = ctx->sample_fibre_xy(
            -hx,
            0.0,
            ctx->s_min,
            ctx->s_max
        );
        rail_mid_R = ctx->sample_fibre_xy(
            hx,
            0.0,
            ctx->s_min,
            ctx->s_max
        );
        const NurbsCurve rail_top_L = ctx->sample_fibre_xy(
            -hx,
            hy,
            ctx->s_min,
            ctx->s_max
        );
        const NurbsCurve rail_top_R = ctx->sample_fibre_xy(
            hx,
            hy,
            ctx->s_min,
            ctx->s_max
        );

        const NurbsSurface surf_above = Primitives::create_ruled(rail_top_L, rail_top_R);
        NurbsSurface surf_below = Primitives::create_ruled(rail_bot_L, rail_bot_R);
        surf_below.transpose();
        const NurbsSurface surf_left = Primitives::create_ruled(rail_bot_L, rail_top_L);
        NurbsSurface surf_right = Primitives::create_ruled(rail_bot_R, rail_top_R);
        surf_right.transpose();

        brep.name = ctx->beam->name;
        sid_above = brep.add_surface(surf_above);
        sid_below = brep.add_surface(surf_below);
        sid_left = brep.add_surface(surf_left);
        sid_right = brep.add_surface(surf_right);

        v_s0_botL = brep.add_vertex(surf_below.point_at(0.0, ctx->norm_u(s0_arr[0])));
        v_s0_botR = brep.add_vertex(surf_below.point_at(1.0, ctx->norm_u(s0_arr[1])));
        v_s0_topR = brep.add_vertex(surf_above.point_at(ctx->norm_u(s0_arr[2]), 1.0));
        v_s0_topL = brep.add_vertex(surf_above.point_at(ctx->norm_u(s0_arr[3]), 0.0));

        v_s1_botL = brep.add_vertex(surf_below.point_at(0.0, ctx->norm_u(s1_arr[0])));
        v_s1_botR = brep.add_vertex(surf_below.point_at(1.0, ctx->norm_u(s1_arr[1])));
        v_s1_topR = brep.add_vertex(surf_above.point_at(ctx->norm_u(s1_arr[2]), 1.0));
        v_s1_topL = brep.add_vertex(surf_above.point_at(ctx->norm_u(s1_arr[3]), 0.0));

        if (!single_front) {
            v_sf_bot = brep.add_vertex(surf_below.point_at(vf_bot, ctx->norm_u(uf_bot)));
            v_sf_top = brep.add_vertex(surf_above.point_at(ctx->norm_u(uf_top), vf_top));
        }

        if (!single_back) {
            v_sb_bot = brep.add_vertex(surf_below.point_at(vb_bot, ctx->norm_u(ub_bot)));
            v_sb_top = brep.add_vertex(surf_above.point_at(ctx->norm_u(ub_top), vb_top));
        }

        lv.resize(laps.size());

        for (size_t i = 0; i < laps.size(); i++) {
            const ResolvedLap& rl = laps[i];
            lv[i].va_midL = brep.add_vertex(surf_left.point_at(ctx->norm_u(rl.ua_midL), 0.5));
            lv[i].va_midR = brep.add_vertex(surf_right.point_at(0.5, ctx->norm_u(rl.ua_midR)));
            lv[i].vb_midL = brep.add_vertex(surf_left.point_at(ctx->norm_u(rl.ub_midL), 0.5));
            lv[i].vb_midR = brep.add_vertex(surf_right.point_at(0.5, ctx->norm_u(rl.ub_midR)));

            if (rl.upper) {
                lv[i].va_extL = brep.add_vertex(surf_above.point_at(ctx->norm_u(rl.ua_topL), 0.0));
                lv[i].va_extR = brep.add_vertex(surf_above.point_at(ctx->norm_u(rl.ua_topR), 1.0));
                lv[i].vb_extL = brep.add_vertex(surf_above.point_at(ctx->norm_u(rl.ub_topL), 0.0));
                lv[i].vb_extR = brep.add_vertex(surf_above.point_at(ctx->norm_u(rl.ub_topR), 1.0));
            } else {
                lv[i].va_extL = brep.add_vertex(surf_below.point_at(0.0, ctx->norm_u(rl.ua_botL)));
                lv[i].va_extR = brep.add_vertex(surf_below.point_at(1.0, ctx->norm_u(rl.ua_botR)));
                lv[i].vb_extL = brep.add_vertex(surf_below.point_at(0.0, ctx->norm_u(rl.ub_botL)));
                lv[i].vb_extR = brep.add_vertex(surf_below.point_at(1.0, ctx->norm_u(rl.ub_botR)));
            }
        }
    }

    /// Adds all top span faces on surf_above between upper cross-lap notches.
    void add_top_spans() {

        std::vector<size_t> top_laps;

        for (size_t i = 0; i < laps.size(); i++)
            if (laps[i].upper)
                top_laps.push_back(i);

        const size_t n_spans = top_laps.size() + 1;

        for (size_t sp = 0; sp < n_spans; sp++) {
            const bool is_first = sp == 0;
            const bool is_last = sp + 1 == n_spans;
            const int vL0 = is_first ? v_s0_topL : lv[top_laps[sp - 1]].vb_extL;
            const int vR0 = is_first ? v_s0_topR : lv[top_laps[sp - 1]].vb_extR;
            const double sL0 = is_first ? s0_arr[3] : laps[top_laps[sp - 1]].ub_topL;
            const double sR0 = is_first ? s0_arr[2] : laps[top_laps[sp - 1]].ub_topR;

            const int vL1 = is_last ? v_s1_topL : lv[top_laps[sp]].va_extL;
            const int vR1 = is_last ? v_s1_topR : lv[top_laps[sp]].va_extR;
            const double sL1 = is_last ? s1_arr[3] : laps[top_laps[sp]].ua_topL;
            const double sR1 = is_last ? s1_arr[2] : laps[top_laps[sp]].ua_topR;

            std::vector<LoopEntry> loop;
            loop.push_back(LoopEntry::fibre(
                vL0,
                Point(ctx->norm_u(sL0), 0.0, 0.0),
                -hx,
                hy,
                sL0,
                sL1
            ));
            loop.push_back(LoopEntry::straight(vL1, ctx->norm_u(sL1), 0.0));

            if (is_last && !single_back)
                loop.push_back(LoopEntry::straight(v_sb_top, ctx->norm_u(ub_top), vb_top));

            loop.push_back(LoopEntry::fibre(
                vR1,
                Point(ctx->norm_u(sR1), 1.0, 0.0),
                hx,
                hy,
                sR1,
                sR0
            ));
            loop.push_back(LoopEntry::straight(vR0, ctx->norm_u(sR0), 1.0));

            if (is_first && !single_front)
                loop.push_back(LoopEntry::straight(v_sf_top, ctx->norm_u(uf_top), vf_top));

            add_loop_face(sid_above, loop);
        }
    }

    /// Adds all bottom span faces on surf_below between lower cross-lap notches.
    void add_bottom_spans() {

        std::vector<size_t> bot_laps;

        for (size_t i = 0; i < laps.size(); i++)
            if (!laps[i].upper)
                bot_laps.push_back(i);

        const size_t n_spans = bot_laps.size() + 1;

        for (size_t sp = 0; sp < n_spans; sp++) {
            const bool is_first = sp == 0;
            const bool is_last = sp + 1 == n_spans;
            const int vL0 = is_first ? v_s0_botL : lv[bot_laps[sp - 1]].vb_extL;
            const int vR0 = is_first ? v_s0_botR : lv[bot_laps[sp - 1]].vb_extR;
            const double sL0 = is_first ? s0_arr[0] : laps[bot_laps[sp - 1]].ub_botL;
            const double sR0 = is_first ? s0_arr[1] : laps[bot_laps[sp - 1]].ub_botR;

            const int vL1 = is_last ? v_s1_botL : lv[bot_laps[sp]].va_extL;
            const int vR1 = is_last ? v_s1_botR : lv[bot_laps[sp]].va_extR;
            const double sL1 = is_last ? s1_arr[0] : laps[bot_laps[sp]].ua_botL;
            const double sR1 = is_last ? s1_arr[1] : laps[bot_laps[sp]].ua_botR;

            std::vector<LoopEntry> loop;
            loop.push_back(LoopEntry::straight(vL0, 0.0, ctx->norm_u(sL0)));

            if (is_first && !single_front)
                loop.push_back(LoopEntry::straight(v_sf_bot, vf_bot, ctx->norm_u(uf_bot)));

            loop.push_back(LoopEntry::fibre(
                vR0,
                Point(1.0, ctx->norm_u(sR0), 0.0),
                hx,
                -hy,
                sR0,
                sR1
            ));
            loop.push_back(LoopEntry::straight(vR1, 1.0, ctx->norm_u(sR1)));

            if (is_last && !single_back)
                loop.push_back(LoopEntry::straight(v_sb_bot, vb_bot, ctx->norm_u(ub_bot)));

            loop.push_back(LoopEntry::fibre(
                vL1,
                Point(0.0, ctx->norm_u(sL1), 0.0),
                -hx,
                -hy,
                sL1,
                sL0
            ));
            add_loop_face(sid_below, loop);
        }
    }

    /// Adds the ruled mid-height seat and two planar cheek faces for every cross-lap notch.
    void add_lap_notches() {

        for (size_t i = 0; i < laps.size(); i++) {
            const ResolvedLap& rl = laps[i];
            const LapVerts& c = lv[i];

            if (rl.upper) {
                const NurbsSurface surf_mid = Primitives::create_ruled(rail_mid_L, rail_mid_R);
                const int sid_mid = brep.add_surface(surf_mid);
                add_loop_face(
                    sid_mid,
                    {
                        LoopEntry::fibre(
                            c.va_midL,
                            Point(ctx->norm_u(rl.ua_midL), 0.0, 0.0),
                            -hx,
                            0.0,
                            rl.ua_midL,
                            rl.ub_midL
                        ),
                        LoopEntry::straight(c.vb_midL, ctx->norm_u(rl.ub_midL), 0.0),
                        LoopEntry::fibre(
                            c.vb_midR,
                            Point(ctx->norm_u(rl.ub_midR), 1.0, 0.0),
                            hx,
                            0.0,
                            rl.ub_midR,
                            rl.ua_midR
                        ),
                        LoopEntry::straight(c.va_midR, ctx->norm_u(rl.ua_midR), 1.0),
                    }
                );
                add_planar_quad({c.va_midL, c.va_midR, c.va_extR, c.va_extL});
                add_planar_quad({c.vb_midL, c.vb_extL, c.vb_extR, c.vb_midR});
            } else {
                NurbsSurface surf_mid = Primitives::create_ruled(rail_mid_L, rail_mid_R);
                surf_mid.transpose();
                const int sid_mid = brep.add_surface(surf_mid);
                add_loop_face(
                    sid_mid,
                    {
                        LoopEntry::straight(c.va_midL, 0.0, ctx->norm_u(rl.ua_midL)),
                        LoopEntry::fibre(
                            c.va_midR,
                            Point(1.0, ctx->norm_u(rl.ua_midR), 0.0),
                            hx,
                            0.0,
                            rl.ua_midR,
                            rl.ub_midR
                        ),
                        LoopEntry::straight(c.vb_midR, 1.0, ctx->norm_u(rl.ub_midR)),
                        LoopEntry::fibre(
                            c.vb_midL,
                            Point(0.0, ctx->norm_u(rl.ub_midL), 0.0),
                            -hx,
                            0.0,
                            rl.ub_midL,
                            rl.ua_midL
                        ),
                    }
                );
                add_planar_quad({c.va_extL, c.va_extR, c.va_midR, c.va_midL});
                add_planar_quad({c.vb_extL, c.vb_midL, c.vb_midR, c.vb_extR});
            }
        }
    }

    /// Adds the front and back planar cap faces (single quad or split miter pair).
    void add_end_caps() {

        if (single_front) {
            add_planar_quad({v_s0_botL, v_s0_topL, v_s0_topR, v_s0_botR});
        } else {
            add_planar_quad({v_s0_botL, v_s0_topL, v_sf_top, v_sf_bot});
            add_planar_quad({v_sf_bot, v_sf_top, v_s0_topR, v_s0_botR});
        }

        if (single_back) {
            add_planar_quad({v_s1_botL, v_s1_botR, v_s1_topR, v_s1_topL});
        } else {
            add_planar_quad({v_s1_botL, v_sb_bot, v_sb_top, v_s1_topL});
            add_planar_quad({v_sb_bot, v_s1_botR, v_s1_topR, v_sb_top});
        }
    }

    /// Adds the notched left side face on surf_left.
    void add_left_face() {

        std::vector<LoopEntry> lloop;
        int cur_v = v_s0_botL;
        double cur_s = s0_arr[0];

        for (size_t i = 0; i < laps.size(); i++) {
            if (laps[i].upper)
                continue;

            const double sa = laps[i].ua_botL;
            lloop.push_back(LoopEntry::fibre(
                cur_v,
                Point(ctx->norm_u(cur_s), 0.0, 0.0),
                -hx,
                -hy,
                cur_s,
                sa
            ));
            lloop.push_back(LoopEntry::straight(lv[i].va_extL, ctx->norm_u(sa), 0.0));
            const double sm0 = laps[i].ua_midL;
            const double sm1 = laps[i].ub_midL;
            lloop.push_back(LoopEntry::fibre(
                lv[i].va_midL,
                Point(ctx->norm_u(sm0), 0.5, 0.0),
                -hx,
                0.0,
                sm0,
                sm1
            ));
            lloop.push_back(LoopEntry::straight(lv[i].vb_midL, ctx->norm_u(sm1), 0.5));
            cur_v = lv[i].vb_extL;
            cur_s = laps[i].ub_botL;
        }

        const double s_end_bot = s1_arr[0];
        lloop.push_back(LoopEntry::fibre(
            cur_v,
            Point(ctx->norm_u(cur_s), 0.0, 0.0),
            -hx,
            -hy,
            cur_s,
            s_end_bot
        ));
        lloop.push_back(LoopEntry::straight(v_s1_botL, ctx->norm_u(s_end_bot), 0.0));

        cur_v = v_s1_topL;
        cur_s = s1_arr[3];

        for (size_t idx = laps.size(); idx > 0; idx--) {
            const size_t i = idx - 1;

            if (!laps[i].upper)
                continue;

            const double sb = laps[i].ub_topL;
            lloop.push_back(LoopEntry::fibre(
                cur_v,
                Point(ctx->norm_u(cur_s), 1.0, 0.0),
                -hx,
                hy,
                cur_s,
                sb
            ));
            lloop.push_back(LoopEntry::straight(lv[i].vb_extL, ctx->norm_u(sb), 1.0));
            const double sm1 = laps[i].ub_midL;
            const double sm0 = laps[i].ua_midL;
            lloop.push_back(LoopEntry::fibre(
                lv[i].vb_midL,
                Point(ctx->norm_u(sm1), 0.5, 0.0),
                -hx,
                0.0,
                sm1,
                sm0
            ));
            lloop.push_back(LoopEntry::straight(lv[i].va_midL, ctx->norm_u(sm0), 0.5));
            cur_v = lv[i].va_extL;
            cur_s = laps[i].ua_topL;
        }

        const double s_start_top = s0_arr[3];
        lloop.push_back(LoopEntry::fibre(
            cur_v,
            Point(ctx->norm_u(cur_s), 1.0, 0.0),
            -hx,
            hy,
            cur_s,
            s_start_top
        ));
        lloop.push_back(LoopEntry::straight(v_s0_topL, ctx->norm_u(s_start_top), 1.0));
        add_loop_face(sid_left, lloop);
    }

    /// Adds the notched right side face on surf_right.
    void add_right_face() {

        std::vector<LoopEntry> rloop;
        rloop.push_back(LoopEntry::straight(v_s0_botR, 0.0, ctx->norm_u(s0_arr[1])));

        int cur_v = v_s0_topR;
        double cur_s = s0_arr[2];

        for (size_t i = 0; i < laps.size(); i++) {
            if (!laps[i].upper)
                continue;

            const double sa = laps[i].ua_topR;
            rloop.push_back(LoopEntry::fibre(
                cur_v,
                Point(1.0, ctx->norm_u(cur_s), 0.0),
                hx,
                hy,
                cur_s,
                sa
            ));
            rloop.push_back(LoopEntry::straight(lv[i].va_extR, 1.0, ctx->norm_u(sa)));
            const double sm0 = laps[i].ua_midR;
            const double sm1 = laps[i].ub_midR;
            rloop.push_back(LoopEntry::fibre(
                lv[i].va_midR,
                Point(0.5, ctx->norm_u(sm0), 0.0),
                hx,
                0.0,
                sm0,
                sm1
            ));
            rloop.push_back(LoopEntry::straight(lv[i].vb_midR, 0.5, ctx->norm_u(sm1)));
            cur_v = lv[i].vb_extR;
            cur_s = laps[i].ub_topR;
        }

        const double s_end_top = s1_arr[2];
        rloop.push_back(LoopEntry::fibre(
            cur_v,
            Point(1.0, ctx->norm_u(cur_s), 0.0),
            hx,
            hy,
            cur_s,
            s_end_top
        ));
        rloop.push_back(LoopEntry::straight(v_s1_topR, 1.0, ctx->norm_u(s_end_top)));

        cur_v = v_s1_botR;
        cur_s = s1_arr[1];

        for (size_t idx = laps.size(); idx > 0; idx--) {
            const size_t i = idx - 1;

            if (laps[i].upper)
                continue;

            const double sb = laps[i].ub_botR;
            rloop.push_back(LoopEntry::fibre(
                cur_v,
                Point(0.0, ctx->norm_u(cur_s), 0.0),
                hx,
                -hy,
                cur_s,
                sb
            ));
            rloop.push_back(LoopEntry::straight(lv[i].vb_extR, 0.0, ctx->norm_u(sb)));
            const double sm1 = laps[i].ub_midR;
            const double sm0 = laps[i].ua_midR;
            rloop.push_back(LoopEntry::fibre(
                lv[i].vb_midR,
                Point(0.5, ctx->norm_u(sm1), 0.0),
                hx,
                0.0,
                sm1,
                sm0
            ));
            rloop.push_back(LoopEntry::straight(lv[i].va_midR, 0.5, ctx->norm_u(sm0)));
            cur_v = lv[i].va_extR;
            cur_s = laps[i].ua_botR;
        }

        const double s_start_bot = s0_arr[1];
        rloop.push_back(LoopEntry::fibre(
            cur_v,
            Point(0.0, ctx->norm_u(cur_s), 0.0),
            hx,
            -hy,
            cur_s,
            s_start_bot
        ));
        add_loop_face(sid_right, rloop);
    }

    /// Assembles and closes the trimmed BRep shell.
    BRep build() {

        init_surfaces_and_vertices();
        add_top_spans();
        add_bottom_spans();
        add_lap_notches();
        add_end_caps();
        add_left_face();
        add_right_face();
        brep.add_solid({{brep.add_shell(faces), BRepOrientation::Forward}});
        brep.update_tolerances();

        return brep;
    }
};

} // namespace

BRep EdgeCurvedElement::trimmed_brep() const {

    if (cuts.empty() && cross_laps.empty())
        return element_geometry_brep();

    if (!centreline.is_valid() || width <= 0.0 || height <= 0.0)
        return mesh_brep(trimmed_mesh());

    const double t0 = centreline.domain_start();
    const double span = centreline.domain_end() - t0;

    if (span <= Tolerance::ZERO_TOLERANCE)
        return mesh_brep(trimmed_mesh());

    CurvedTrimContext ctx;
    ctx.beam = this;
    ctx.twist = compute_twist();
    ctx.t0 = t0;
    ctx.span = span;

    const double hx = 0.5 * width;
    const double hy = 0.5 * height;
    const std::array<double, 4> fx = {-hx, hx, hx, -hx};
    const std::array<double, 4> fy = {-hy, -hy, hy, hy};

    for (size_t f = 0; f < 4; f++)
        if (ctx.fibre_clipped_out(fx[f], fy[f]))
            return mesh_brep(trimmed_mesh());

    TrimmedBRepBuilder builder;
    builder.ctx = &ctx;
    builder.hx = hx;
    builder.hy = hy;

    std::array<int, 4> front_cutter = {-1, -1, -1, -1};
    std::array<int, 4> back_cutter = {-1, -1, -1, -1};
    builder.s0_arr[0] = ctx.front_envelope(0.0, -hy, front_cutter[0]);
    builder.s0_arr[1] = ctx.front_envelope(1.0, -hy, front_cutter[1]);
    builder.s0_arr[2] = ctx.front_envelope(1.0, hy, front_cutter[2]);
    builder.s0_arr[3] = ctx.front_envelope(0.0, hy, front_cutter[3]);
    builder.s1_arr[0] = ctx.back_envelope(0.0, -hy, back_cutter[0]);
    builder.s1_arr[1] = ctx.back_envelope(1.0, -hy, back_cutter[1]);
    builder.s1_arr[2] = ctx.back_envelope(1.0, hy, back_cutter[2]);
    builder.s1_arr[3] = ctx.back_envelope(0.0, hy, back_cutter[3]);

    for (size_t f = 0; f < 4; f++)
        if (builder.s0_arr[f] >= builder.s1_arr[f])
            return mesh_brep(trimmed_mesh());

    builder.single_front = front_cutter[0] == front_cutter[1] &&
                           front_cutter[2] == front_cutter[3] && front_cutter[0] == front_cutter[2];
    builder.single_back = back_cutter[0] == back_cutter[1] && back_cutter[2] == back_cutter[3] &&
                          back_cutter[0] == back_cutter[2];

    if (cross_laps.empty() && builder.single_front && builder.single_back) {
        std::array<NurbsCurve, 4> trimmed_corners;

        for (size_t k = 0; k < 4; k++)
            trimmed_corners[k] = ctx.sample_fibre_xy(
                fx[k],
                fy[k],
                builder.s0_arr[k],
                builder.s1_arr[k]
            );

        if (trimmed_corners[0].is_valid() && trimmed_corners[1].is_valid() &&
            trimmed_corners[2].is_valid() && trimmed_corners[3].is_valid())
            return solid_from_four_edges(trimmed_corners, name);

        return mesh_brep(trimmed_mesh());
    }

    if (!builder.single_front) {
        ctx.solve_miter_v(
            true,
            -hy,
            front_cutter[0],
            front_cutter[1],
            builder.vf_bot,
            builder.uf_bot
        );
        ctx.solve_miter_v(
            true,
            hy,
            front_cutter[3],
            front_cutter[2],
            builder.vf_top,
            builder.uf_top
        );
    }

    if (!builder.single_back) {
        ctx.solve_miter_v(
            false,
            -hy,
            back_cutter[0],
            back_cutter[1],
            builder.vb_bot,
            builder.ub_bot
        );
        ctx.solve_miter_v(
            false,
            hy,
            back_cutter[3],
            back_cutter[2],
            builder.vb_top,
            builder.ub_top
        );
    }

    ctx.s_min =
        std::min({builder.s0_arr[0], builder.s0_arr[1], builder.s0_arr[2], builder.s0_arr[3]});
    ctx.s_max =
        std::max({builder.s1_arr[0], builder.s1_arr[1], builder.s1_arr[2], builder.s1_arr[3]});

    if (!builder.single_front)
        ctx.s_min = std::min({ctx.s_min, builder.uf_bot, builder.uf_top});

    if (!builder.single_back)
        ctx.s_max = std::max({ctx.s_max, builder.ub_bot, builder.ub_top});

    if (ctx.s_max <= ctx.s_min + 1e-6)
        return mesh_brep(trimmed_mesh());

    if (!resolve_cross_laps(
            ctx,
            hx,
            hy,
            builder.laps
        ))
        return mesh_brep(trimmed_mesh());

    return builder.build();
}

const BRep& EdgeCurvedElement::model_geometry_brep() const {

    if (plane_features.empty() && solid_features.empty())
        return WoodElement::model_geometry_brep();

    if (!_model_brep_cache) {
        EdgeCurvedElement trimmed = *this;
        const std::vector<Plane> planes = feature_planes();
        trimmed.cuts.insert(trimmed.cuts.end(), planes.begin(), planes.end());
        trimmed.plane_features.clear();
        trimmed.solid_features.clear();
        trimmed.invalidate_geometry();
        _model_brep_cache = exact_solid_features(trimmed.trimmed_brep(), solid_features);
    }

    return *_model_brep_cache;
}

Line EdgeCurvedElement::chord() const {

    if (!centreline.is_valid())
        return Line::from_points(Point(0, 0, 0), Point(0, 0, 0));

    return Line::from_points(centreline.point_at_start(), centreline.point_at_end());
}

std::optional<Plane> EdgeCurvedElement::base_plane() const {
    if (!centreline.is_valid())
        return std::nullopt;
    return get_plane(centreline.domain_start());
}

std::shared_ptr<EdgeCurvedElement> EdgeCurvedElement::transformed(const Xform& xform) const {
    if (is_mirror(xform))
        return nullptr;

    std::shared_ptr<EdgeCurvedElement> beam = std::make_shared<EdgeCurvedElement>(
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

void EdgeCurvedElement::place(const Xform& xform) {
    // nothing stored yet has nothing to carry: the solid comes from the moved curve when it is asked for
    if (_geometry_mesh || _geometry_brep || !_features.empty() || !_insertion_vectors.empty())
        Element::place(xform);
    else
        reset();
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

void EdgeCurvedElement::compute_geometry_mesh_impl() {
    if (centreline.is_valid() && width > 0.0 && height > 0.0)
        set_geometry(model_geometry_mesh());
    compute_geometry_features();
}

void EdgeCurvedElement::compute_geometry_brep_impl() {
    if (centreline.is_valid() && width > 0.0 && height > 0.0)
        set_geometry(model_geometry_brep());
    compute_geometry_features();
}

void EdgeCurvedElement::compute_geometry_features() {
    const std::vector<Plane> station_frames = frames();
    std::vector<Point> axis_pts;
    axis_pts.reserve(station_frames.size());
    for (const Plane& frame : station_frames)
        axis_pts.push_back(frame.origin());

    std::vector<Plane> all_cuts = cuts;
    const std::vector<Plane> f_planes = feature_planes();
    all_cuts.insert(all_cuts.end(), f_planes.begin(), f_planes.end());

    const std::pair<Polyline, std::vector<Polyline>> trimmed =
        trim_to_cuts(Polyline(axis_pts), sections(station_frames), all_cuts);

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

AABB EdgeCurvedElement::aabb(double inflate) const {
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

nlohmann::ordered_json EdgeCurvedElement::element_data_jsondump() const {
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

std::string EdgeCurvedElement::element_data_dumps() const {
    return element_data_jsondump().dump();
}

static std::shared_ptr<Element> beam_curved_from_protobuf(const std::string& data) {
    return EdgeCurvedElement::from_element(Element::pb_loads(data));
}

void EdgeCurvedElement::register_type() {
    Element::register_type(std::string(ELEMENT_TYPE), beam_curved_from_protobuf);
    Element::register_type(std::string(LEGACY_ELEMENT_TYPE), beam_curved_from_protobuf);
}

// ═══════════════════════════════════════════════════════════════════════════
// String
// ═══════════════════════════════════════════════════════════════════════════

std::string EdgeCurvedElement::str() const {
    std::ostringstream os;
    os << "EdgeCurvedElement(name=" << name << ", width=" << width << ", height=" << height
       << ", lamellae=" << num_width << "x" << num_height << ", samples=" << samples
       << ", is_visible=" << std::boolalpha << is_visible << ", is_locked=" << is_locked << ")";
    return os.str();
}

}
