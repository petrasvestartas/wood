#include "src/templates/folding/truncated_pyramid.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
TruncatedPyramid::TruncatedPyramid(const TruncatedPyramidGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // points: the base corners on the ground, the top corners, per corner line its fold point, per side its edge fold point
    const std::vector<Point> base = regular_polygon(4, guide.half * std::sqrt(2.0), 0.0);
    const std::vector<Point> tops = regular_polygon(4, guide.top * std::sqrt(2.0), guide.height);
    std::vector<Point> bends;
    std::vector<Point> middles;

    for (size_t i = 0; i < 4; i++) {
        const Point& start = base[i];
        const Point& end = base[(i + 1) % 4];
        const double along = guide.corner_fold ? guide.corner_fold->along : 0.0;
        const double bend_x = start[0] + (tops[i][0] - start[0]) * along;
        const double bend_y = start[1] + (tops[i][1] - start[1]) * along;
        const double bend_z = guide.corner_fold ? guide.corner_fold->rise * guide.height : 0.0;
        bends.push_back(Point(bend_x, bend_y, bend_z));

        const Vector inward = Vector(start[1] - end[1], end[0] - start[0], 0.0).normalized();
        const double in = guide.edge_fold ? guide.edge_fold->along * (guide.half - guide.top) : 0.0;
        const double middle_x = 0.5 * (start[0] + end[0]) + inward[0] * in;
        const double middle_y = 0.5 * (start[1] + end[1]) + inward[1] * in;
        const double middle_z = guide.edge_fold ? guide.edge_fold->rise * guide.height : 0.0;
        middles.push_back(Point(middle_x, middle_y, middle_z));
    }

    // mesh: per side its chain over the corner folds and the top edge, one face or a fan from the edge fold point; the flat top
    FoldMesh fold;

    for (size_t i = 0; i < 4; i++) {
        const size_t next = (i + 1) % 4;
        std::vector<Point> chain = {base[i]};

        if (guide.corner_fold)
            chain.push_back(bends[i]);

        chain.push_back(tops[i]);

        if (guide.top > 0.0)
            chain.push_back(tops[next]);

        if (guide.corner_fold)
            chain.push_back(bends[next]);

        chain.push_back(base[next]);

        if (!guide.edge_fold) {
            fold.face(chain);
            continue;
        }

        for (size_t k = 0; k + 1 < chain.size(); k++)
            fold.face({chain[k], chain[k + 1], middles[i]});

        if (middles[i][2] > 1.0)
            fold.face({base[next], base[i], middles[i]});
    }

    if (guide.top > 0.0)
        fold.face({tops[3], tops[2], tops[1], tops[0]});

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

    // assembly: the plates of every side of the plan make one side, the sides leaned on each other and the top put on them
    const std::vector<std::shared_ptr<FacePlanarElement>> placed =
        get_elements_numbered<FacePlanarElement>("plate");
    const size_t per_side = placed.size() / 4;
    std::vector<AssemblyModule> whole;
    for (size_t i = 0; i < 4; i++) {
        const std::vector<std::shared_ptr<FacePlanarElement>>::const_iterator from =
            placed.begin() + static_cast<std::ptrdiff_t>(i * per_side);
        const std::vector<AssemblyModule> parts = AssemblyModule::parts_of(
            std::vector<std::shared_ptr<FacePlanarElement>>(
                from, from + static_cast<std::ptrdiff_t>(per_side)
            )
        );
        whole.push_back(
            per_side == 1 ? parts.front() : AssemblyModule::raise(fmt::format("side_{}", i), parts)
        );
    }
    for (size_t i = 4 * per_side; i < placed.size(); i++)
        whole.push_back(AssemblyModule::part(placed[i]));
    assemblies = {AssemblyModule::module(name, whole)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& TruncatedPyramid::mesh() const {
    return _mesh;
}

}
