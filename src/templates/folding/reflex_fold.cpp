#include "src/templates/folding/reflex_fold.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
ReflexFold::ReflexFold(const ReflexFoldGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // curves: the cross section and the profile
    const std::shared_ptr<TreeNode> curves = add_group("curves");
    add_polyline(guide.cross_section, curves);
    add_polyline(guide.profile, curves);

    // fold_planes: the plane that halves the cross section's angle at every point
    const std::vector<Plane> fold_planes = compute_fold_planes();
    const std::shared_ptr<TreeNode> planes = add_group("fold_planes");

    for (const Plane& plane : fold_planes)
        add_plane(plane, planes);

    // mesh: the profile carried from fold plane to fold plane, one quad strip per segment
    _mesh = compute_mesh(fold_planes);
    add_mesh(_mesh, add_group("mesh"));

    // plates: one plate per quad, mitered at the folds, its corners chamfered
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const std::vector<std::shared_ptr<FacePlanarElement>> folds = mitered_plates(
        _mesh, guide.thickness, guide.chamfer_bottom, guide.chamfer_top, guide.chamfer_angle
    );

    for (const std::shared_ptr<FacePlanarElement>& plate : folds)
        add(plate, plates);

    // flat: the folded mesh unfolded into one flat crease pattern beside the model
    _unfolding = FoldUnfolding(_mesh);
    _unfolding.add_to(*this);

    // assembly: the plates of every cross section segment make one row along the profile, the rows put one after another from one foot over the crown to the other
    std::vector<size_t> keys;
    for (size_t i = 0; i < folds.size(); i++)
        keys.push_back(i / (guide.profile.point_count() - 1));
    assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("row", folds, keys))};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& ReflexFold::mesh() const {
    return _mesh;
}

const FoldUnfolding& ReflexFold::unfolding() const {
    return _unfolding;
}

std::vector<Plane> ReflexFold::compute_fold_planes() const {

    const size_t count = guide.cross_section.point_count();
    std::vector<Plane> planes;

    for (size_t i = 0; i < count; i++) {
        Vector normal(0.0, 0.0, 1.0);

        if (i > 0 && i + 1 < count) {
            const Vector previous =
                (guide.cross_section[i - 1] - guide.cross_section[i]).normalized();
            const Vector next = (guide.cross_section[i + 1] - guide.cross_section[i]).normalized();
            normal = previous + next;

            if (!normal.normalize_self()) {
                normal = guide.cross_section[i + 1] - guide.cross_section[i - 1];
                normal.normalize_self();
            }
        }

        planes.push_back(Plane::from_point_normal(guide.cross_section[i], normal));
    }

    return planes;
}

Mesh ReflexFold::compute_mesh(const std::vector<Plane>& fold_planes) const {

    const size_t row = guide.profile.point_count();
    std::vector<Point> points = guide.profile.get_points();
    std::vector<std::vector<size_t>> faces;

    for (size_t i = 1; i < fold_planes.size(); i++) {
        const Vector step = fold_planes[i].origin() - fold_planes[i - 1].origin();
        const size_t start = points.size();

        for (size_t j = 0; j < row; j++) {
            const Point previous = points[start - row + j];
            const Line along = Line::from_points(previous, previous + step);
            const std::optional<Point> moved =
                Intersection::line_plane(along, fold_planes[i], false);
            points.push_back(moved.value_or(previous));
        }

        for (size_t j = 0; j + 1 < row; j++)
            faces.push_back({start + j, start - row + j, start - row + j + 1, start + j + 1});
    }

    return Mesh::from_vertices_and_faces(points, faces);
}

}
