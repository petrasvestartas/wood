#include "src/templates/folding/single_fold.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
SingleFold::SingleFold(const SingleFoldGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // points: the corners on the ground, every edge's middle stood off it and raised, the centre
    const std::vector<Point> corners = regular_polygon(guide.sides, guide.radius, 0.0);
    const Point centre(0.0, 0.0, guide.apex);
    std::vector<Point> middles;

    for (size_t i = 0; i < guide.sides; i++) {
        const Point& start = corners[i];
        const Point& end = corners[(i + 1) % guide.sides];
        const Vector outward = Vector(end[1] - start[1], start[0] - end[0], 0.0).normalized();
        const double x = 0.5 * (start[0] + end[0]) + outward[0] * guide.overhang;
        const double y = 0.5 * (start[1] + end[1]) + outward[1] * guide.overhang;
        middles.push_back(Point(x, y, guide.gable));
    }

    // mesh: per edge its eave triangle to its middle and two triangles folded on the ridge from the middle to the centre
    FoldMesh fold;

    for (size_t i = 0; i < guide.sides; i++) {
        const Point& start = corners[i];
        const Point& end = corners[(i + 1) % guide.sides];
        fold.face({start, end, middles[i]});
        fold.face({start, middles[i], centre});
        fold.face({middles[i], end, centre});
    }

    _mesh = fold.mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: the ridges and the valleys, every edge of the mesh off the ground
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end && ((*start)[2] > 1.0 || (*end)[2] > 1.0))
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face by the exact offset, the edges on the ground cut flush with it
    const std::shared_ptr<TreeNode> plates = add_group("plates");

    for (const std::shared_ptr<FacePlanarElement>& plate :
         offset_plates(_mesh, guide.thickness, {Plane::xy_plane()}))
        add(plate, plates);

    // assembly: the plates of every side of the plan make one side, the sides leaned on each other
    const std::vector<std::shared_ptr<FacePlanarElement>> placed =
        get_elements_numbered<FacePlanarElement>("plate");
    const size_t per_side = placed.size() / guide.sides;
    std::vector<size_t> keys;
    for (size_t i = 0; i < placed.size(); i++)
        keys.push_back(i / per_side);
    assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("side", placed, keys))};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& SingleFold::mesh() const {
    return _mesh;
}

}
