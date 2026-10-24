#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/folding/fold_strips.h"

using namespace session_cpp;
using namespace wood_session;

/// A frame or arch of triangular folded plates, half a bay one zigzag fold from the foot to the crown.
class FoldedFrame : public WoodSession {
public:
    const Polyline zigzag; // The fold of half a bay from the foot to the crown.
    const double bay; // mm, every bay along y.
    const size_t bays; // Bays along y.
    const double thickness; // mm, every plate.

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.

public:
    /// The folded frame of bays bays of zigzag as the session named name.
    explicit FoldedFrame(
        const Polyline& zigzag =
            two_hinged_frame(30000.0, 4500.0, 5800.0, 6600.0, 8750.0, 2900.0, 2200.0),
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

    /// The two-hinged frame with ridge-to-ridge folding: columns eaves high, the knees bulging out bulge.
    static Polyline two_hinged_frame(
        double span = 30000.0,
        double bay = 4500.0,
        double eaves = 5800.0,
        double ceiling = 6600.0,
        double ridge = 8750.0,
        double knee = 2900.0,
        double bulge = 2200.0
    );

    /// The three-hinged frame with ridge folding: a flat ceiling eaves high, the roof dipping to the hinge at mid-span.
    static Polyline three_hinged_frame(
        double span = 30000.0,
        double bay = 4250.0,
        double eaves = 6250.0,
        double ridge = 8250.0,
        double hinge = 6800.0,
        double knee = 3100.0,
        double bulge = 1900.0
    );

    /// The two-hinged A-frame with ridge-to-valley folding: two legs to an apex, their diagonals crossing at the waist.
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

    /// The arch with a top hinge at the proportions, its legs rising nearly vertically height high.
    static Polyline
    top_hinged_arch(double span = 17900.0, double bay = 4950.0, double height = 26100.0);

    /// The three-hinged octagonal arch at the proportions, height high.
    static Polyline
    three_hinged_arch(double span = 21500.0, double bay = 5000.0, double height = 20500.0);

private:
    /// The zigzag's triangles, every point with the last point on each rail.
    std::vector<std::array<Point, 3>> compute_triangles() const;

    /// The mesh of every bay: the zigzag's triangles mirrored over mid-span and the bay's centre, facing out.
    Mesh compute_mesh() const;
};

inline FoldedFrame::FoldedFrame(
    const Polyline& zigzag, double bay, size_t bays, double thickness, const std::string& name
)
    : WoodSession(name), zigzag(zigzag), bay(bay), bays(bays), thickness(thickness) {

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

    // plates: one plate per face by the exact offset, the free edges cut flush by the side planes, the feet by the floor
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const double width = bay * static_cast<double>(bays);
    const std::vector<Plane> sides = {
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, width, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0)),
    };

    for (const std::shared_ptr<Plate>& plate : offset_plates(_mesh, thickness, sides))
        add(plate, plates);
}

inline const Mesh& FoldedFrame::mesh() const {
    return _mesh;
}

inline Polyline FoldedFrame::two_hinged_frame(
    double span, double bay, double eaves, double ceiling, double ridge, double knee, double bulge
) {

    const double centre = 0.5 * bay;
    const double middle = 0.5 * span;

    return Polyline({
        Point(0.0, centre, 0.0),
        Point(0.0, 0.0, 0.0),
        Point(-bulge, centre, knee),
        Point(0.0, 0.0, eaves),
        Point(middle, centre, ridge),
        Point(middle, 0.0, ceiling),
    });
}

inline Polyline FoldedFrame::three_hinged_frame(
    double span, double bay, double eaves, double ridge, double hinge, double knee, double bulge
) {

    const double centre = 0.5 * bay;

    return Polyline({
        Point(0.0, centre, 0.0),
        Point(0.0, 0.0, eaves),
        Point(-bulge, centre, knee),
        Point(0.25 * span, centre, ridge),
        Point(0.5 * span, 0.0, eaves),
        Point(0.5 * span, centre, hinge),
    });
}

inline Polyline FoldedFrame::a_frame(
    double span,
    double bay,
    double apex,
    double ridge,
    double knee,
    double bulge,
    double waist_in,
    double waist
) {

    const double centre = 0.5 * bay;

    return Polyline({
        Point(0.0, 0.0, 0.0),
        Point(-bulge, centre, knee),
        Point(waist_in, centre, waist),
        Point(0.5 * span, 0.0, apex),
        Point(0.5 * span, centre, ridge),
    });
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

inline std::vector<std::array<Point, 3>> FoldedFrame::compute_triangles() const {

    std::vector<std::array<Point, 3>> triangles;
    std::optional<Point> valley;
    std::optional<Point> ridge;

    for (size_t i = 0; i < zigzag.point_count(); i++) {
        const Point& point = zigzag[i];

        if (valley && ridge)
            triangles.push_back({*valley, *ridge, point});

        if (std::abs(point[1] - 0.5 * bay) < std::abs(point[1]))
            ridge = point;
        else
            valley = point;
    }

    return triangles;
}

inline Mesh FoldedFrame::compute_mesh() const {

    const std::vector<std::array<Point, 3>> triangles = compute_triangles();
    const double middle = zigzag[zigzag.point_count() - 1][0];
    double top = 0.0;

    for (size_t i = 0; i < zigzag.point_count(); i++)
        top = std::max(top, zigzag[i][2]);

    // faces: the triangles mirrored over mid-span and the bay's centre, bay by bay, facing away from the inside
    FoldMesh fold;
    std::vector<std::vector<size_t>> faces;

    for (size_t b = 0; b < bays; b++)
        for (const bool over_span : {false, true})
            for (const bool over_bay : {false, true}) {
                const double y0 = bay * static_cast<double>(b);
                const Point inside(middle, y0 + 0.5 * bay, 0.5 * top);

                for (const std::array<Point, 3>& triangle : triangles) {
                    std::array<Point, 3> placed;
                    std::vector<size_t> face;

                    for (size_t k = 0; k < 3; k++) {
                        const double x = over_span ? 2.0 * middle - triangle[k][0] : triangle[k][0];
                        const double y = over_bay ? y0 + bay - triangle[k][1] : y0 + triangle[k][1];
                        placed[k] = Point(x, y, triangle[k][2]);
                        face.push_back(fold.add(placed[k]));
                    }

                    const Vector normal = (placed[1] - placed[0]).cross(placed[2] - placed[0]);
                    const double centroid_x = (placed[0][0] + placed[1][0] + placed[2][0]) / 3.0;
                    const double centroid_y = (placed[0][1] + placed[1][1] + placed[2][1]) / 3.0;
                    const double centroid_z = (placed[0][2] + placed[1][2] + placed[2][2]) / 3.0;
                    const Point centroid(centroid_x, centroid_y, centroid_z);

                    if (normal.dot(centroid - inside) < 0.0)
                        std::swap(face[1], face[2]);

                    faces.push_back(face);
                }
            }

    return Mesh::from_vertices_and_faces(fold.points(), faces);
}
