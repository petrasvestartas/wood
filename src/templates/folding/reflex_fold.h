#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A folded plate shell: a profile carried along a cross section, folded at every cross-section point, one plate per fold.
///
/// Fields: the two input polylines and the plate sizes; the curves, the mesh and the plates are in the session.
class ReflexFold : public WoodSession {
public:
    const Polyline cross_section; // The polyline the profile is carried along.
    const Polyline profile; // The polyline folded at every cross-section point.
    const double thickness; // mm, every plate.
    const double chamfer_bottom; // mm cut back at the bottom outline's corners.
    const double chamfer_top; // mm cut back at the top outline's corners.
    const double chamfer_angle; // Degrees, corners sharper than this are chamfered.

    /// The fold of profile along cross_section as the session named name.
    explicit ReflexFold(
        const Polyline& cross_section = default_cross_section(),
        const Polyline& profile = default_profile(),
        double thickness = 10.0,
        double chamfer_bottom = 20.0,
        double chamfer_top = 20.0,
        double chamfer_angle = 180.0,
        const std::string& name = "reflex_fold"
    );

    /// Not copied, as a session is not.
    ReflexFold(const ReflexFold&) = delete;

    /// Not assigned, as it is not copied.
    ReflexFold& operator=(const ReflexFold&) = delete;

    /// The folded quad mesh, one face per plate.
    const Mesh& mesh() const;

    /// A 2 m arch of five points in the xz plane.
    static Polyline default_cross_section();

    /// A 1.6 m zigzag along -y in the xy plane.
    static Polyline default_profile();

private:
    Mesh _mesh;

    /// The bisector plane at every cross-section point, the end planes normal to z.
    std::vector<Plane> compute_fold_planes() const;

    /// Each profile row moved along the cross section onto the next fold plane, the rows joined by quads.
    Mesh compute_mesh(const std::vector<Plane>& fold_planes) const;
};

inline ReflexFold::ReflexFold(
    const Polyline& cross_section,
    const Polyline& profile,
    double thickness,
    double chamfer_bottom,
    double chamfer_top,
    double chamfer_angle,
    const std::string& name
)
    : WoodSession(name),
      cross_section(cross_section),
      profile(profile),
      thickness(thickness),
      chamfer_bottom(chamfer_bottom),
      chamfer_top(chamfer_top),
      chamfer_angle(chamfer_angle) {

    // curves: the cross section and the profile
    const std::shared_ptr<TreeNode> curves = add_group("curves");
    add_polyline(cross_section, curves);
    add_polyline(profile, curves);

    // fold_planes: the plane that halves the cross section's angle at every point
    const std::vector<Plane> fold_planes = compute_fold_planes();
    const std::shared_ptr<TreeNode> planes = add_group("fold_planes");

    for (const Plane& plane : fold_planes)
        add_plane(plane, planes);

    // mesh: the profile carried from fold plane to fold plane, one quad strip per segment
    _mesh = compute_mesh(fold_planes);
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per quad, mitred at the folds, its corners chamfered
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const std::vector<std::shared_ptr<Plate>> folds = mitred_plates(
        _mesh,
        thickness,
        chamfer_bottom,
        chamfer_top,
        chamfer_angle
    );

    for (const std::shared_ptr<Plate>& plate : folds)
        add(plate, plates);
}

inline const Mesh& ReflexFold::mesh() const {
    return _mesh;
}

inline Polyline ReflexFold::default_cross_section() {
    return Polyline({
        {0.0, 0.0, 0.0},
        {232.466867, 0.0, 578.230273},
        {966.431048, 0.0, 738.362403},
        {1714.771039, 0.0, 604.197646},
        {2037.199246, 0.0, 4.784134},
    });
}

inline Polyline ReflexFold::default_profile() {
    return Polyline({
        {0.0, 0.0, 0.0},
        {-77.091582, -89.779688, 0.0},
        {-19.195901, -179.559376, 0.0},
        {-89.503039, -269.339064, 0.0},
        {-25.912208, -359.118752, 0.0},
        {-89.664508, -448.898440, 0.0},
        {-19.472054, -538.678128, 0.0},
        {-77.485143, -628.457816, 0.0},
        {-0.817868, -718.237504, 0.0},
        {-54.785307, -808.017192, 0.0},
        {26.377605, -897.796881, 0.0},
        {-25.921340, -987.576569, 0.0},
        {55.558217, -1077.356257, 0.0},
        {1.964204, -1167.135945, 0.0},
        {79.167116, -1256.915633, 0.0},
        {21.364954, -1346.695321, 0.0},
        {91.462653, -1436.475009, 0.0},
        {27.192231, -1526.254697, 0.0},
        {89.830774, -1616.034385, 0.0},
    });
}

inline std::vector<Plane> ReflexFold::compute_fold_planes() const {

    const size_t count = cross_section.point_count();
    std::vector<Plane> planes;

    for (size_t i = 0; i < count; i++) {
        Vector normal(0.0, 0.0, 1.0);

        if (i > 0 && i + 1 < count) {
            const Vector previous = (cross_section[i - 1] - cross_section[i]).normalized();
            const Vector next = (cross_section[i + 1] - cross_section[i]).normalized();
            normal = previous + next;

            if (!normal.normalize_self()) {
                normal = cross_section[i + 1] - cross_section[i - 1];
                normal.normalize_self();
            }
        }

        planes.push_back(Plane::from_point_normal(cross_section[i], normal));
    }

    return planes;
}

inline Mesh ReflexFold::compute_mesh(const std::vector<Plane>& fold_planes) const {

    const size_t row = profile.point_count();
    std::vector<Point> points = profile.get_points();
    std::vector<std::vector<size_t>> faces;

    for (size_t i = 1; i < fold_planes.size(); i++) {
        const Vector step = fold_planes[i].origin() - fold_planes[i - 1].origin();
        const size_t start = points.size();

        for (size_t j = 0; j < row; j++) {
            const Point previous = points[start - row + j];
            const Line along = Line::from_points(previous, previous + step);
            const std::optional<Point> moved = Intersection::line_plane(along, fold_planes[i], false);
            points.push_back(moved.value_or(previous));
        }

        for (size_t j = 0; j + 1 < row; j++)
            faces.push_back({start + j, start - row + j, start - row + j + 1, start + j + 1});
    }

    return Mesh::from_vertices_and_faces(points, faces);
}
