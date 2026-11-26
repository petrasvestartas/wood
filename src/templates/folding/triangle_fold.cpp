#include "src/templates/folding/triangle_fold.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
TriangleFold::TriangleFold(const TriangleFoldGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // points: the corners on the ground, the edges' middles raised, the centre
    const std::vector<Point> corners = regular_polygon(guide.sides, guide.radius, 0.0);
    std::vector<Point> middles;

    for (size_t i = 0; i < guide.sides; i++) {
        const Point& start = corners[i];
        const Point& end = corners[(i + 1) % guide.sides];
        const double x = 0.5 * (start[0] + end[0]);
        const double y = 0.5 * (start[1] + end[1]);
        middles.push_back(Point(x, y, guide.middle));
    }

    // mesh: a gable per edge, a triangle per corner, the middles' polygon one flat face or a triangle per side to the centre
    FoldMesh fold;

    for (size_t i = 0; i < guide.sides; i++) {
        fold.face({corners[i], corners[(i + 1) % guide.sides], middles[i]});
        fold.face({corners[i], middles[i], middles[(i + guide.sides - 1) % guide.sides]});
    }

    const Point top(0.0, 0.0, guide.centre);

    if (std::abs(guide.centre - guide.middle) < 1.0)
        fold.face(middles);
    else
        for (size_t i = 0; i < guide.sides; i++)
            fold.face({middles[i], middles[(i + 1) % guide.sides], top});

    _mesh = fold.mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh off the ground
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

    // assembly: the gables and the corners between them raised, the plates over the middle one module put on them last
    const std::vector<std::shared_ptr<FacePlanarElement>> placed =
        get_elements_numbered<FacePlanarElement>("plate");
    std::vector<AssemblyModule> whole = AssemblyModule::parts_of(
        std::vector<std::shared_ptr<FacePlanarElement>>(
            placed.begin(), placed.begin() + static_cast<std::ptrdiff_t>(2 * guide.sides)
        )
    );
    whole.push_back(
        AssemblyModule::module(
            "top",
            AssemblyModule::parts_of(
                std::vector<std::shared_ptr<FacePlanarElement>>(
                    placed.begin() + static_cast<std::ptrdiff_t>(2 * guide.sides), placed.end()
                )
            )
        )
    );
    assemblies = {AssemblyModule::module(name, whole)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& TriangleFold::mesh() const {
    return _mesh;
}

}
