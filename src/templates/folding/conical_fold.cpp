#include "src/templates/folding/conical_fold.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
ConicalFold::ConicalFold(const ConicalFoldGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // stations: the cut profile at both ends and the sharp one at mid-span
    const std::shared_ptr<TreeNode> profiles = add_group("stations");
    add_polyline(compute_profile(0.0, guide.end_depth), profiles);
    add_polyline(compute_profile(0.5 * guide.span, guide.depth), profiles);
    add_polyline(compute_profile(guide.span, guide.end_depth), profiles);

    // mesh: the flank pentagons and the cut triangles
    _mesh = compute_mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh, the valleys, the cut edges and the ridge points' creases
    const std::shared_ptr<TreeNode> fold_lines = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end)
            add_polyline(Polyline({*start, *end}), fold_lines);
    }

    // plates: one plate per face by the exact offset, the free edges along the span cut flush by the side planes y 0 and y width
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const double width = 2.0 * guide.fold_width * static_cast<double>(guide.folds);
    const std::vector<Plane> sides = {
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, width, 0.0), Vector(0.0, 1.0, 0.0)),
    };

    for (const std::shared_ptr<FacePlanarElement>& plate :
         offset_plates(_mesh, guide.thickness, sides))
        add(plate, plates);

    // assembly: the plates under every fold make one fold, a ridge with its two flanks, the folds put side by side
    const std::vector<std::shared_ptr<FacePlanarElement>> placed =
        get_elements_numbered<FacePlanarElement>("plate");
    std::vector<size_t> keys;
    for (const std::shared_ptr<FacePlanarElement>& plate : placed)
        keys.push_back(
            static_cast<size_t>(plate->polylines[0].center()[1] / (2.0 * guide.fold_width))
        );
    assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("fold", placed, keys))};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& ConicalFold::mesh() const {
    return _mesh;
}

Polyline ConicalFold::compute_profile(double x, double height) const {

    const double run = guide.fold_width * height / guide.depth;
    std::vector<Point> points;

    for (size_t k = 0; k < guide.folds; k++) {
        const double valley = 2.0 * guide.fold_width * static_cast<double>(k);
        points.push_back(Point(x, valley, 0.0));
        points.push_back(Point(x, valley + run, height));

        if (run < guide.fold_width)
            points.push_back(Point(x, valley + 2.0 * guide.fold_width - run, height));
    }

    points.push_back(Point(x, 2.0 * guide.fold_width * static_cast<double>(guide.folds), 0.0));
    return Polyline(points);
}

Mesh ConicalFold::compute_mesh() const {

    const double run = guide.fold_width * guide.end_depth / guide.depth;
    const double middle = 0.5 * guide.span;
    FoldMesh fold;

    for (size_t k = 0; k < guide.folds; k++) {
        const double valley = 2.0 * guide.fold_width * static_cast<double>(k);
        const double ridge = valley + guide.fold_width;
        const double next = valley + 2.0 * guide.fold_width;
        const Point peak(middle, ridge, guide.depth);

        // flanks: the two pentagons from the valley line up to the cut edge through the ridge point
        fold.face(facing_down({
            Point(0.0, valley, 0.0),
            Point(0.0, valley + run, guide.end_depth),
            peak,
            Point(guide.span, valley + run, guide.end_depth),
            Point(guide.span, valley, 0.0),
        }));
        fold.face(facing_down({
            Point(0.0, next, 0.0),
            Point(0.0, next - run, guide.end_depth),
            peak,
            Point(guide.span, next - run, guide.end_depth),
            Point(guide.span, next, 0.0),
        }));

        // cuts: the two triangles in the sloping planes, from the flat end of the fold to the ridge point
        fold.face(facing_down(
            {Point(0.0, valley + run, guide.end_depth),
             Point(0.0, next - run, guide.end_depth),
             peak}
        ));
        fold.face(facing_down(
            {Point(guide.span, valley + run, guide.end_depth),
             Point(guide.span, next - run, guide.end_depth),
             peak}
        ));
    }

    return Mesh::from_vertices_and_faces(fold.points(), fold.faces());
}

std::vector<Point> ConicalFold::facing_down(std::vector<Point> corners) {

    const Vector normal = (corners[1] - corners[0]).cross(corners[2] - corners[0]);

    if (normal[2] > 0.0)
        std::reverse(corners.begin(), corners.end());

    return corners;
}

}
