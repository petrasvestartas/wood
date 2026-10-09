#pragma once
#include "wood_session.h"
#include "intersection.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_cross {

// ═══════════════════════════════════════════════════════════════════════════
// VdaMesh
// ═══════════════════════════════════════════════════════════════════════════

/// The plates and connectors of a mesh, a session: a mitred plate per face and layer, connector plates across every interior edge.
///
/// Fields: the welded mesh and the plate and connector sizes.
/// Every plate (`plate_<face>_<layer>`) and connector (`connector_<face0>_<face1>_<k>`) is found in the session by name.
class VdaMesh : public WoodSession {
public:
    const Mesh mesh; // The input mesh, welded at 0.01.
    const double face_thickness; // Plate thickness along the face normal.
    const std::vector<double> face_positions; // Offsets of the plate layers along the face normal, sorted.
    const std::vector<int> edge_divisions; // Connectors per interior edge: one value for all, or one per edge.
    const std::vector<double> edge_division_length; // Connector spacing along an edge, used instead of edge_divisions when given.
    const std::vector<Line> insertion_lines; // Lines that turn the connectors of the edge they touch.
    const double connector_width; // Connector size across the edge, in the plane of the faces.
    const double connector_height; // Connector size along the average face normal.
    const double connector_thickness; // Connector size along the edge.

    /// The plates and connectors of the mesh as the session named name.
    explicit VdaMesh(
        const Mesh& mesh = default_mesh(),
        double face_thickness = 20.0,
        const std::vector<double>& face_positions = {0.0},
        const std::vector<int>& edge_divisions = {2},
        const std::vector<double>& edge_division_length = {},
        const std::vector<Line>& insertion_lines = {},
        double connector_width = 200.0,
        double connector_height = 200.0,
        double connector_thickness = 20.0,
        const std::string& name = "vda_mesh"
    );

    /// Not copied, as a session is not.
    VdaMesh(const VdaMesh&) = delete;

    /// Not assigned, as it is not copied.
    VdaMesh& operator=(const VdaMesh&) = delete;

    /// A fifteen-face dome of hexagons and pentagons.
    static Mesh default_mesh();

    /// The faces either side of every edge, by index in Mesh::faces order, edges in Mesh::edges order.
    const std::vector<std::vector<size_t>>& edge_faces() const;

    /// The bisector plane of every face corner, corner j between edges j and j + 1.
    const std::vector<std::vector<Plane>>& bisector_planes() const;

    /// The bottom and top outline of every face per layer.
    const std::vector<std::vector<std::array<Polyline, 2>>>& face_outlines() const;

    /// The top plane of every face per layer.
    const std::vector<std::vector<Plane>>& face_outline_planes() const;

    /// The frames of every edge's connectors, none on a naked edge.
    const std::vector<std::vector<Plane>>& connector_frames() const;

    /// The bottom and top rectangle of every edge's connectors.
    const std::vector<std::vector<std::array<Polyline, 2>>>& connector_outlines() const;

private:
    std::vector<std::vector<size_t>> _edge_faces;
    std::vector<Plane> _face_planes;
    std::vector<std::vector<Plane>> _edge_planes;
    std::vector<std::vector<Plane>> _bisector_planes;
    std::vector<std::vector<std::array<Polyline, 2>>> _face_outlines;
    std::vector<std::vector<Plane>> _face_outline_planes;
    std::vector<std::vector<Plane>> _connector_frames;
    std::vector<std::vector<std::array<Polyline, 2>>> _connector_outlines;

    /// The faces either side of every edge.
    std::vector<std::vector<size_t>> compute_edge_faces() const;

    /// The plane of every face, at its centroid along its normal.
    std::vector<Plane> compute_face_planes() const;

    /// Per face edge, the plane through the edge leaning on the average normal of the faces that share it.
    std::vector<std::vector<Plane>> compute_edge_planes() const;

    /// Per face corner, the plane through the corner vertex halving the angle between the edge planes that meet there.
    std::vector<std::vector<Plane>> compute_bisector_planes() const;

    /// Per face and layer, the bottom and top outline where the edge planes cut the offset face plane.
    std::vector<std::vector<std::array<Polyline, 2>>> compute_face_outlines() const;

    /// Per face and layer, the top plane of its plate.
    std::vector<std::vector<Plane>> compute_face_outline_planes() const;

    /// Per interior edge, a frame at every division: z along the edge, y the average normal, x across into the first face.
    std::vector<std::vector<Plane>> compute_connector_frames() const;

    /// Per connector frame, the bottom and top rectangle connector_thickness apart along the edge.
    std::vector<std::vector<std::array<Polyline, 2>>> compute_connector_outlines() const;

    /// The closed outline where consecutive edge planes cross the base plane, nearly parallel neighbours merged.
    static Polyline outline(const Plane& base, const std::vector<Plane>& edge_planes, const std::vector<Plane>& bisector_planes);
};

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════

inline VdaMesh::VdaMesh(
    const Mesh& mesh,
    double face_thickness,
    const std::vector<double>& face_positions,
    const std::vector<int>& edge_divisions,
    const std::vector<double>& edge_division_length,
    const std::vector<Line>& insertion_lines,
    double connector_width,
    double connector_height,
    double connector_thickness,
    const std::string& name
)
    : WoodSession(name),
      mesh(mesh.weld(0.01)),
      face_thickness(face_thickness),
      face_positions(face_positions),
      edge_divisions(edge_divisions),
      edge_division_length(edge_division_length),
      insertion_lines(insertion_lines),
      connector_width(connector_width),
      connector_height(connector_height),
      connector_thickness(connector_thickness) {

    // mesh: the welded input mesh and the faces either side of every edge
    add_mesh(this->mesh, group_named("mesh"));
    const std::vector<std::vector<size_t>> edge_faces = compute_edge_faces();
    _edge_faces = edge_faces;

    // face_planes: a plane per face at its centroid
    const std::vector<Plane> face_planes = compute_face_planes();
    _face_planes = face_planes;
    const std::shared_ptr<TreeNode> face_plane_group = group_named("face_planes");

    for (const Plane& plane : _face_planes)
        add_plane(plane, face_plane_group);

    // edge_planes: per face edge a plane through the edge, leaning on the average normal of its faces
    const std::vector<std::vector<Plane>> edge_planes = compute_edge_planes();
    _edge_planes = edge_planes;
    const std::shared_ptr<TreeNode> edge_plane_group = group_named("edge_planes");

    for (const std::vector<Plane>& planes : _edge_planes)
        for (const Plane& plane : planes)
            add_plane(plane, edge_plane_group);

    // bisector_planes: per face corner the plane through its vertex halving its two edge planes
    const std::vector<std::vector<Plane>> bisector_planes = compute_bisector_planes();
    _bisector_planes = bisector_planes;
    const std::shared_ptr<TreeNode> bisector_plane_group = group_named("bisector_planes");

    for (const std::vector<Plane>& planes : _bisector_planes)
        for (const Plane& plane : planes)
            add_plane(plane, bisector_plane_group);

    // plates: per face and layer a plate between the outlines the edge planes cut from the offset face plane
    const std::vector<std::vector<std::array<Polyline, 2>>> face_outlines = compute_face_outlines();
    _face_outlines = face_outlines;
    const std::vector<std::vector<Plane>> face_outline_planes = compute_face_outline_planes();
    _face_outline_planes = face_outline_planes;
    const std::shared_ptr<TreeNode> plates = group_named("plates");

    for (size_t f = 0; f < _face_outlines.size(); f++)
        for (size_t layer = 0; layer < _face_outlines[f].size(); layer++) {
            const std::array<Polyline, 2>& loops = _face_outlines[f][layer];
            add(std::make_shared<Plate>(loops[0], loops[1], fmt::format("plate_{}_{}", f, layer)), plates);
        }

    // connector_frames: per interior edge a frame at every division, x across the edge into its first face
    const std::vector<std::vector<Plane>> connector_frames = compute_connector_frames();
    _connector_frames = connector_frames;
    const std::shared_ptr<TreeNode> frame_group = group_named("connector_frames");

    for (const std::vector<Plane>& frames : _connector_frames)
        for (const Plane& frame : frames)
            add_plane(frame, frame_group);

    // connectors: per frame a rectangular plate, connector_thickness along the edge
    const std::vector<std::vector<std::array<Polyline, 2>>> connector_outlines = compute_connector_outlines();
    _connector_outlines = connector_outlines;
    const std::shared_ptr<TreeNode> connectors = group_named("connectors");

    for (size_t e = 0; e < _connector_outlines.size(); e++)
        for (size_t k = 0; k < _connector_outlines[e].size(); k++) {
            const std::array<Polyline, 2>& loops = _connector_outlines[e][k];
            const std::string connector_name = fmt::format("connector_{}_{}_{}", _edge_faces[e][0], _edge_faces[e][1], k);
            add(std::make_shared<Plate>(loops[0], loops[1], connector_name), connectors);
        }
}

// ═══════════════════════════════════════════════════════════════════════════
// Default mesh
// ═══════════════════════════════════════════════════════════════════════════

inline Mesh VdaMesh::default_mesh() {

    const std::vector<std::vector<Point>> faces = {
        {{760.01, -2621.81, -1344.23}, {678.55, -2638.25, -1344.47}, {312.96, -2711.90, -1344.47}, {268.58, -2647.07, -802.86}, {785.47, -2537.90, -765.84}},
        {{1029.30, -1734.25, 502.84}, {484.63, -1736.48, 651.50}, {406.90, -527.14, 1226.27}, {1366.88, -621.66, 919.21}},
        {{1796.71, 794.99, 551.90}, {941.91, 1284.49, 761.48}, {983.61, 2177.67, -275.86}, {2053.18, 1640.26, -622.17}},
        {{785.47, -2537.90, -765.84}, {268.58, -2647.07, -802.86}, {-230.08, -2574.84, -543.21}, {-202.02, -1930.01, 521.65}, {484.63, -1736.48, 651.50}, {1029.30, -1734.25, 502.84}},
        {{1366.88, -621.66, 919.21}, {406.90, -527.14, 1226.27}, {-87.79, -224.28, 1344.47}, {-28.04, 991.06, 1133.00}, {941.91, 1284.49, 761.48}, {1796.71, 794.99, 551.90}},
        {{2053.18, 1640.26, -622.17}, {983.61, 2177.67, -275.86}, {75.35, 2683.99, -833.95}, {79.22, 2711.90, -1344.47}, {2064.27, 1676.82, -1344.47}},
        {{-782.27, -2617.50, -1344.47}, {-732.62, -2561.48, -809.19}, {-230.08, -2574.84, -543.21}, {268.58, -2647.07, -802.86}, {312.96, -2711.90, -1344.47}},
        {{484.63, -1736.48, 651.50}, {-202.02, -1930.01, 521.65}, {-834.72, -1644.25, 624.19}, {-735.12, -527.94, 1164.03}, {-87.79, -224.28, 1344.47}, {406.90, -527.14, 1226.27}},
        {{941.91, 1284.49, 761.48}, {-28.04, 991.06, 1133.00}, {-940.95, 1369.34, 723.57}, {-907.43, 2222.88, -267.53}, {75.35, 2683.99, -833.95}, {983.61, 2177.67, -275.86}},
        {{-1403.20, -1577.93, 406.59}, {-834.72, -1644.25, 624.19}, {-202.02, -1930.01, 521.65}, {-230.08, -2574.84, -543.21}, {-732.62, -2561.48, -809.19}, {-1247.95, -2364.14, -783.26}},
        {{-1890.38, 889.09, 465.62}, {-940.95, 1369.34, 723.57}, {-28.04, 991.06, 1133.00}, {-87.79, -224.28, 1344.47}, {-735.12, -527.94, 1164.03}, {-1628.78, -435.63, 816.65}},
        {{79.22, 2711.90, -1344.47}, {75.35, 2683.99, -833.95}, {-907.43, 2222.88, -267.53}, {-2058.23, 1739.09, -697.73}, {-2064.27, 1769.65, -1344.47}},
        {{-1101.44, -2497.51, -1344.47}, {-1231.26, -2448.62, -1343.85}, {-1247.95, -2364.14, -783.26}, {-732.62, -2561.48, -809.19}, {-782.27, -2617.50, -1344.47}},
        {{-1628.78, -435.63, 816.65}, {-735.12, -527.94, 1164.03}, {-834.72, -1644.25, 624.19}, {-1403.20, -1577.93, 406.59}},
        {{-2058.23, 1739.09, -697.73}, {-907.43, 2222.88, -267.53}, {-940.95, 1369.34, 723.57}, {-1890.38, 889.09, 465.62}},
    };

    return Mesh::from_polylines(faces, 10.0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Accessors
// ═══════════════════════════════════════════════════════════════════════════

inline const std::vector<std::vector<size_t>>& VdaMesh::edge_faces() const {
    return _edge_faces;
}

inline const std::vector<std::vector<Plane>>& VdaMesh::bisector_planes() const {
    return _bisector_planes;
}

inline const std::vector<std::vector<std::array<Polyline, 2>>>& VdaMesh::face_outlines() const {
    return _face_outlines;
}

inline const std::vector<std::vector<Plane>>& VdaMesh::face_outline_planes() const {
    return _face_outline_planes;
}

inline const std::vector<std::vector<Plane>>& VdaMesh::connector_frames() const {
    return _connector_frames;
}

inline const std::vector<std::vector<std::array<Polyline, 2>>>& VdaMesh::connector_outlines() const {
    return _connector_outlines;
}

// ═══════════════════════════════════════════════════════════════════════════
// Steps
// ═══════════════════════════════════════════════════════════════════════════

inline std::vector<std::vector<size_t>> VdaMesh::compute_edge_faces() const {

    const std::vector<size_t> faces = mesh.faces();
    const std::vector<std::pair<size_t, size_t>> edges = mesh.edges();
    const std::map<std::pair<size_t, size_t>, size_t> halfedge_faces = mesh.edge_face_map();
    std::map<size_t, size_t> face_index;

    for (size_t f = 0; f < faces.size(); f++)
        face_index[faces[f]] = f;

    std::vector<std::vector<size_t>> edge_faces(edges.size());

    for (size_t e = 0; e < edges.size(); e++) {
        const std::pair<size_t, size_t> forward = edges[e];
        const std::pair<size_t, size_t> backward = {edges[e].second, edges[e].first};

        for (const std::pair<size_t, size_t>& halfedge : {forward, backward})
            if (halfedge_faces.count(halfedge))
                edge_faces[e].push_back(face_index.at(halfedge_faces.at(halfedge)));

        std::sort(edge_faces[e].begin(), edge_faces[e].end());
    }

    return edge_faces;
}

inline std::vector<Plane> VdaMesh::compute_face_planes() const {

    std::vector<Plane> planes;

    for (const size_t face : mesh.faces()) {
        const Point centroid = mesh.face_centroid(face).value();
        const Vector normal = mesh.face_normal(face).value();
        planes.push_back(Plane::from_point_normal(centroid, normal));
    }

    return planes;
}

inline std::vector<std::vector<Plane>> VdaMesh::compute_edge_planes() const {

    const std::vector<size_t> faces = mesh.faces();
    const std::vector<std::pair<size_t, size_t>> edges = mesh.edges();
    std::map<std::pair<size_t, size_t>, size_t> edge_index;

    for (size_t e = 0; e < edges.size(); e++)
        edge_index[edges[e]] = e;

    std::vector<std::vector<Plane>> planes(faces.size());

    for (size_t f = 0; f < faces.size(); f++) {
        const std::vector<std::pair<size_t, size_t>> face_edges = mesh.face_edges(faces[f]).value();

        for (const std::pair<size_t, size_t>& edge : face_edges) {
            const Point start = mesh.vertex_point(edge.first).value();
            const Point end = mesh.vertex_point(edge.second).value();
            Vector x_axis = Vector::from_points(end, start);
            x_axis.normalize_self();
            Vector y_axis(0.0, 0.0, 0.0);

            for (const size_t neighbour : _edge_faces[edge_index.at(std::minmax(edge.first, edge.second))])
                y_axis += _face_planes[neighbour].z_axis();

            y_axis.normalize_self();
            planes[f].push_back(Plane(Point::mid_point(start, end), x_axis, y_axis));
        }
    }

    return planes;
}

inline std::vector<std::vector<Plane>> VdaMesh::compute_bisector_planes() const {

    const std::vector<size_t> faces = mesh.faces();
    std::vector<std::vector<Plane>> planes(faces.size());

    for (size_t f = 0; f < faces.size(); f++) {
        const std::vector<std::pair<size_t, size_t>> face_edges = mesh.face_edges(faces[f]).value();
        const size_t n = face_edges.size();

        for (size_t j = 0; j < n; j++) {
            const Plane& side = _edge_planes[f][j];
            const Plane& next = _edge_planes[f][(j + 1) % n];
            const Point corner = mesh.vertex_point(face_edges[j].second).value();
            const Vector seam = side.z_axis().cross(next.z_axis());

            // edges in line: the plane across the edge at the corner; else the plane along the seam between the inward normals
            if (seam.magnitude() < 1e-9)
                planes[f].push_back(Plane(corner, side.z_axis(), side.y_axis()));
            else
                planes[f].push_back(Plane(corner, seam, side.z_axis() + next.z_axis()));
        }
    }

    return planes;
}

inline std::vector<std::vector<std::array<Polyline, 2>>> VdaMesh::compute_face_outlines() const {

    std::vector<double> layers = face_positions;
    std::sort(layers.begin(), layers.end());
    std::vector<std::vector<std::array<Polyline, 2>>> outlines(_face_planes.size());

    for (size_t f = 0; f < _face_planes.size(); f++)
        for (const double layer : layers) {
            const Plane bottom = _face_planes[f].translate_by_normal(layer - 0.5 * face_thickness);
            const Plane top = _face_planes[f].translate_by_normal(layer + 0.5 * face_thickness);
            const Polyline bottom_loop = outline(bottom, _edge_planes[f], _bisector_planes[f]);
            const Polyline top_loop = outline(top, _edge_planes[f], _bisector_planes[f]);
            outlines[f].push_back({bottom_loop, top_loop});
        }

    return outlines;
}

inline std::vector<std::vector<Plane>> VdaMesh::compute_face_outline_planes() const {

    std::vector<double> layers = face_positions;
    std::sort(layers.begin(), layers.end());
    std::vector<std::vector<Plane>> planes(_face_planes.size());

    for (size_t f = 0; f < _face_planes.size(); f++)
        for (const double layer : layers)
            planes[f].push_back(_face_planes[f].translate_by_normal(layer + 0.5 * face_thickness));

    return planes;
}

inline std::vector<std::vector<Plane>> VdaMesh::compute_connector_frames() const {

    const std::vector<std::pair<size_t, size_t>> edges = mesh.edges();
    std::vector<Line> lines;

    for (const std::pair<size_t, size_t>& edge : edges) {
        const Point start = mesh.vertex_point(edge.first).value();
        const Point end = mesh.vertex_point(edge.second).value();
        lines.push_back(Line::from_points(start, end));
    }

    // an insertion line turns the connectors of the first edge its start or end lies on
    std::vector<Vector> insertions(edges.size(), Vector(0.0, 0.0, 0.0));

    for (const Line& insertion : insertion_lines)
        for (size_t e = 0; e < edges.size(); e++) {
            const Point on_start = lines[e].closest_point(insertion.start(), true).second;
            const Point on_end = lines[e].closest_point(insertion.end(), true).second;

            if (on_start.distance(insertion.start()) < 0.01 || on_end.distance(insertion.end()) < 0.01) {
                insertions[e] = insertion.to_direction();
                insertions[e].normalize_self();
                break;
            }
        }

    std::vector<std::vector<Plane>> frames(edges.size());

    for (size_t e = 0; e < edges.size(); e++) {
        if (_edge_faces[e].size() < 2)
            continue;

        const Plane& face0 = _face_planes[_edge_faces[e][0]];
        const Plane& face1 = _face_planes[_edge_faces[e][1]];
        const Point origin = lines[e].center();
        Vector z_axis = lines[e].to_direction();
        z_axis.normalize_self();
        Vector y_axis = face0.z_axis() + face1.z_axis();
        y_axis.normalize_self();
        Vector x_axis = z_axis.cross(y_axis);
        x_axis.normalize_self();

        if ((origin + x_axis).distance(face0.origin()) > (origin - x_axis).distance(face0.origin()))
            x_axis = -x_axis;

        if (!insertions[e].is_zero()) {
            Vector across = insertions[e] - y_axis * insertions[e].dot(y_axis);

            if (across.normalize_self()) {
                if ((origin + across).distance(face0.origin()) > (origin - across).distance(face0.origin()))
                    across = -across;

                x_axis = across;
            }
        }

        // divisions: by spacing when given, one per edge or one for all, else by count
        int divisions = 1;

        if (!edge_division_length.empty() && edge_division_length[0] >= 0.01) {
            const double spacing = edge_division_length.size() == edges.size() ? edge_division_length[e] : edge_division_length[0];

            if (spacing > 0.01)
                divisions = std::max(1, std::min(10, static_cast<int>(lines[e].length() / spacing)));
        } else if (!edge_divisions.empty()) {
            const int count = edge_divisions.size() == edges.size() ? edge_divisions[e] : edge_divisions[0];
            divisions = std::max(1, count);
        }

        for (int k = 1; k <= divisions; k++) {
            const Point station = lines[e].point_at(static_cast<double>(k) / (1.0 + divisions));
            frames[e].push_back(Plane(station, x_axis, y_axis));
        }
    }

    return frames;
}

inline std::vector<std::vector<std::array<Polyline, 2>>> VdaMesh::compute_connector_outlines() const {

    std::vector<std::vector<std::array<Polyline, 2>>> outlines(_connector_frames.size());

    for (size_t e = 0; e < _connector_frames.size(); e++)
        for (const Plane& frame : _connector_frames[e]) {
            const Vector to_corner = frame.x_axis() * (-0.5 * connector_width) + frame.y_axis() * (-0.5 * connector_height);
            const Vector half = frame.z_axis() * (0.5 * connector_thickness);
            const Polyline bottom = Polyline::rectangle(
                frame.origin() - half + to_corner,
                frame.x_axis(),
                frame.y_axis(),
                connector_width,
                connector_height
            );
            const Polyline top = Polyline::rectangle(
                frame.origin() + half + to_corner,
                frame.x_axis(),
                frame.y_axis(),
                connector_width,
                connector_height
            );
            outlines[e].push_back({bottom, top});
        }

    return outlines;
}

// ═══════════════════════════════════════════════════════════════════════════
// Outline
// ═══════════════════════════════════════════════════════════════════════════

inline Polyline VdaMesh::outline(const Plane& base, const std::vector<Plane>& edge_planes, const std::vector<Plane>& bisector_planes) {

    const size_t n = edge_planes.size();
    std::vector<Plane> sides = edge_planes;
    std::vector<Point> corners;

    for (size_t j = 0; j < n; j++) {
        const size_t next = (j + 1) % n;

        // a next side parallel to this one is this one: no corner between them
        const double normals_angle = sides[j].z_axis().angle(sides[next].z_axis(), false, true);

        if (normals_angle < 0.57 || std::abs(normals_angle - 180.0) < 0.57) {
            sides[next] = sides[j];
            continue;
        }

        // a next side nearly along this one crosses it too flat: the bisector through the vertex cuts the corner instead
        const double edges_angle = sides[j].x_axis().angle(sides[next].x_axis(), false, true);
        const Plane& cutter = edges_angle < 5.7 ? bisector_planes[j] : sides[next];
        Line seam;
        Point corner;

        if (!Intersection::plane_plane(sides[j], cutter, seam))
            continue;

        if (!Intersection::line_plane(seam, base, corner, false))
            continue;

        corners.push_back(corner);
    }

    if (!corners.empty())
        corners.push_back(corners[0]);

    return Polyline(corners);
}

}
