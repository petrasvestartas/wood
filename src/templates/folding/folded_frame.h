#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"
#include "src/templates/folding/fold_strips.h"

using namespace session_cpp;
using namespace wood_session;

/// A linear structure system of folded surfaces after Engel, Structure Systems, Folded plate structures (pp. 225-227): a frame or an
/// arch of triangular plates, bay by bay along y. Half a bay is one fold line zigzagging from the foot to the crown between two rails:
/// the bay's edge y 0, where the valleys lie, the frame's inner line, and the bay's centre y bay / 2, where the ridges lie, bulging
/// out at the knees. Every point of the zigzag makes a planar triangle with the last point on each rail, so the plates are the
/// triangles of Engel's plans: the pointed knee of the columns with its vertical plates, the diagonals from the eaves to the ridge or
/// from the knee to the valleys, the star at the waist of the A-frame, the kites of the elevations. The crown is the zigzag's last
/// two points, one on each rail, at mid-span; the other half of the span mirrors the zigzag, the other half of the bay mirrors the
/// rails, and the bays repeat along y.
///
/// Fields: the zigzag, the bay, the bay count and the thickness; the zigzag, the mesh, the folds and the plates are in the session.
class FoldedFrame : public WoodSession {
public:
    const Polyline zigzag; // The fold of half a bay from the foot to the crown at mid-span, its points on the rails y 0 and y bay / 2.
    const double bay; // mm, every bay along y.
    const size_t bays; // Bays along y.
    const double thickness; // mm, every plate.

    /// The folded frame of bays bays of zigzag as the session named name.
    explicit FoldedFrame(
        const Polyline& zigzag = two_hinged_frame(30000.0, 4500.0, 5800.0, 6600.0, 8750.0, 2900.0, 2200.0, Folding::ridge_to_ridge),
        double bay = 4500.0,
        size_t bays = 4,
        double thickness = 120.0,
        const std::string& name = "folded_frame"
    );

    /// Not copied, as a session is not.
    FoldedFrame(const FoldedFrame&) = delete;

    /// Not assigned, as it is not copied.
    FoldedFrame& operator=(const FoldedFrame&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

    /// The two-hinged frame (p. 225): columns eaves high, a ceiling rising to ceiling at mid-span under a ridge ridge high, the knee of
    /// every column bulging out bulge at knee height. Ridge to ridge, the feet are lines along the bay edges, a vertical plate stands at
    /// every bay edge from the foot to the eaves, and the roof folds from the eaves corners to the ridge top; ridge to valley, the feet
    /// are points at the bay centres and the roof folds from the knee to the valleys at mid-span.
    static Polyline two_hinged_frame(
        double span = 30000.0,
        double bay = 4500.0,
        double eaves = 5800.0,
        double ceiling = 6600.0,
        double ridge = 8750.0,
        double knee = 2900.0,
        double bulge = 2200.0,
        Folding folding = Folding::ridge_to_ridge
    );

    /// The three-hinged frame with ridge folding (p. 226): the feet points at the bay centres, the knee bulging out bulge at knee height,
    /// a flat ceiling eaves high, the roof rising to its ridge ridge high over the quarter points of the span, where the diagonals from
    /// the eaves corners cross, and dipping to the hinge hinge high at mid-span.
    static Polyline three_hinged_frame(
        double span = 30000.0,
        double bay = 4250.0,
        double eaves = 6250.0,
        double ridge = 8250.0,
        double hinge = 6800.0,
        double knee = 3100.0,
        double bulge = 1900.0
    );

    /// The two-hinged A-frame with ridge-to-valley folding (p. 226): two legs from feet at the bay edges to an apex apex high, where
    /// the valleys meet, under a ridge ridge high; every leg bulges out bulge at knee height and its diagonals cross at the waist,
    /// waist_in from the foot and waist high at the bay centre, the star of six plates.
    static Polyline a_frame(
        double span = 21000.0,
        double bay = 3850.0,
        double apex = 12700.0,
        double ridge = 18100.0,
        double knee = 6000.0,
        double bulge = 1450.0,
        double waist_in = 6100.0,
        double waist = 4000.0
    );

    /// The arch with a top hinge (p. 227): legs from feet at the bay centres rising nearly vertically height high, at Engel's
    /// proportions: the trunk's valley at 0.32 of the height, the knee bulging out at 0.57, the haunch's valley at 0.885, the ridge
    /// peaking over 0.36 of the span with its valley straight below it, and the hinge at mid-span, where the ridge dips to 0.93 of the
    /// height onto the level valleys.
    static Polyline top_hinged_arch(double span = 17900.0, double bay = 4950.0, double height = 26100.0);

    /// The three-hinged arch (p. 227): an octagonal arch from feet at the bay centres, at Engel's proportions: the level line of the
    /// ankles at 0.29 of the height, the haunch's valley at 0.59, its ridge at 0.67, the crown's corners over 0.27 of the span and the
    /// hinge at mid-span, where the crown's ridge and valley meet in one line.
    static Polyline three_hinged_arch(double span = 21500.0, double bay = 5000.0, double height = 20500.0);

private:
    Mesh _mesh;

    /// The mesh of every bay: per half bay the triangles of the zigzag and of its mirror over mid-span, each facing away from the
    /// frame's inside.
    Mesh compute_mesh() const;
};

inline FoldedFrame::FoldedFrame(
    const Polyline& zigzag,
    double bay,
    size_t bays,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      zigzag(zigzag),
      bay(bay),
      bays(bays),
      thickness(thickness) {

    // zigzag: the fold line of half a bay from the foot to the crown
    add_polyline(zigzag, add_group("zigzag"));

    // mesh: one planar triangle per point of the zigzag with the last valley and the last ridge, mirrored and repeated bay by bay
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end)
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face, mitered at the folds; only at a node of three or more plates whose inner corners spread apart
    // the corners are cut back, by just that spread, so they do not pass through each other; the free edges cut flush by the side planes
    // y 0 and y width
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    const double width = bay * static_cast<double>(bays);
    const std::vector<Plane> sides = {
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, width, 0.0), Vector(0.0, 1.0, 0.0)),
    };

    for (const std::shared_ptr<Plate>& plate : node_cut_plates(_mesh, thickness, sides))
        add(plate, plates);
}

inline const Mesh& FoldedFrame::mesh() const {
    return _mesh;
}

inline Polyline FoldedFrame::two_hinged_frame(double span, double bay, double eaves, double ceiling, double ridge, double knee, double bulge, Folding folding) {

    const double centre = 0.5 * bay;
    const double middle = 0.5 * span;

    if (folding == Folding::ridge_to_valley)
        return Polyline({Point(0.0, centre, 0.0), Point(0.0, 0.0, eaves), Point(-bulge, centre, knee), Point(middle, 0.0, ceiling), Point(middle, centre, ridge)});

    return Polyline({Point(0.0, centre, 0.0), Point(0.0, 0.0, 0.0), Point(-bulge, centre, knee), Point(0.0, 0.0, eaves), Point(middle, centre, ridge), Point(middle, 0.0, ceiling)});
}

inline Polyline FoldedFrame::three_hinged_frame(double span, double bay, double eaves, double ridge, double hinge, double knee, double bulge) {

    const double centre = 0.5 * bay;
    return Polyline({Point(0.0, centre, 0.0), Point(0.0, 0.0, eaves), Point(-bulge, centre, knee), Point(0.25 * span, centre, ridge), Point(0.5 * span, 0.0, eaves), Point(0.5 * span, centre, hinge)});
}

inline Polyline FoldedFrame::a_frame(double span, double bay, double apex, double ridge, double knee, double bulge, double waist_in, double waist) {

    const double centre = 0.5 * bay;
    return Polyline({Point(0.0, 0.0, 0.0), Point(-bulge, centre, knee), Point(waist_in, centre, waist), Point(0.5 * span, 0.0, apex), Point(0.5 * span, centre, ridge)});
}

inline Polyline FoldedFrame::top_hinged_arch(double span, double bay, double height) {

    const double centre = 0.5 * bay;
    return Polyline({
        Point(0.0, centre, 0.0),
        Point(-0.02 * height, 0.0, 0.32 * height),
        Point(-0.042 * height, centre, 0.57 * height),
        Point(0.19 * span, 0.0, 0.885 * height),
        Point(0.36 * span, centre, height),
        Point(0.36 * span, 0.0, 0.92 * height),
        Point(0.5 * span, centre, 0.93 * height),
        Point(0.5 * span, 0.0, 0.915 * height),
    });
}

inline Polyline FoldedFrame::three_hinged_arch(double span, double bay, double height) {

    const double centre = 0.5 * bay;
    return Polyline({
        Point(0.0, centre, 0.0),
        Point(-0.075 * span, 0.0, 0.29 * height),
        Point(-0.23 * span, centre, 0.29 * height),
        Point(-0.115 * span, 0.0, 0.59 * height),
        Point(-0.195 * span, centre, 0.67 * height),
        Point(0.27 * span, 0.0, 0.95 * height),
        Point(0.27 * span, centre, height),
        Point(0.5 * span, 0.0, height),
        Point(0.5 * span, centre, height),
    });
}

inline Mesh FoldedFrame::compute_mesh() const {

    // the zigzag's triangles: every point with the last point on each rail
    const size_t count = zigzag.point_count();
    const double middle = zigzag[count - 1][0];
    std::vector<std::array<Point, 3>> triangles;
    std::optional<Point> valley;
    std::optional<Point> ridge;
    double top = 0.0;

    for (size_t i = 0; i < count; i++) {
        const Point& point = zigzag[i];
        top = std::max(top, point[2]);

        if (valley && ridge)
            triangles.push_back({*valley, *ridge, point});

        if (std::abs(point[1] - 0.5 * bay) < std::abs(point[1]))
            ridge = point;
        else
            valley = point;
    }

    // vertices: shared where a point repeats, at the rails between the half bays and between the bays
    const std::function<std::array<long long, 3>(const Point&)> key = [](const Point& point) {
        return std::array<long long, 3>{std::llround(point[0] * 100.0), std::llround(point[1] * 100.0), std::llround(point[2] * 100.0)};
    };
    std::map<std::array<long long, 3>, size_t> indices;
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    // faces: the zigzag's triangles, mirrored over mid-span and over the bay's centre, bay by bay, each facing away from the
    // frame's inside, mid-span at the bay's centre at half the height
    for (size_t b = 0; b < bays; b++)
        for (const bool over_span : {false, true})
            for (const bool over_bay : {false, true}) {
                const double y0 = bay * static_cast<double>(b);
                const Point inside(middle, y0 + 0.5 * bay, 0.5 * top);

                for (const std::array<Point, 3>& triangle : triangles) {
                    std::array<Point, 3> placed;
                    std::vector<size_t> face;

                    for (size_t k = 0; k < 3; k++) {
                        placed[k] = Point(over_span ? 2.0 * middle - triangle[k][0] : triangle[k][0], over_bay ? y0 + bay - triangle[k][1] : y0 + triangle[k][1], triangle[k][2]);
                        const std::array<long long, 3> id = key(placed[k]);

                        if (indices.find(id) == indices.end()) {
                            indices[id] = points.size();
                            points.push_back(placed[k]);
                        }

                        face.push_back(indices[id]);
                    }

                    const Vector normal = (placed[1] - placed[0]).cross(placed[2] - placed[0]);
                    const Point centroid((placed[0][0] + placed[1][0] + placed[2][0]) / 3.0, (placed[0][1] + placed[1][1] + placed[2][1]) / 3.0, (placed[0][2] + placed[1][2] + placed[2][2]) / 3.0);

                    if (normal.dot(centroid - inside) < 0.0)
                        std::swap(face[1], face[2]);

                    faces.push_back(face);
                }
            }

    return Mesh::from_vertices_and_faces(points, faces);
}
