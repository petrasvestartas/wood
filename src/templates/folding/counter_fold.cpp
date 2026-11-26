#include "src/templates/folding/counter_fold.h"
#include "src/templates/folding/fold_strips.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CounterFold::CounterFold(const CounterFoldGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // stations: the fold profile at every station along the span
    const std::shared_ptr<TreeNode> profiles = add_group("stations");

    for (const Polyline& station : guide.stations)
        add_polyline(station, profiles);

    // mesh: every strip one planar quad or two triangles on the folding's diagonal, two faces in one plane joined
    std::vector<std::vector<double>> heights;

    for (const Polyline& station : guide.stations) {
        heights.push_back({});

        for (size_t i = 0; i < station.point_count(); i++)
            heights.back().push_back(station[i][2]);
    }

    _mesh = merge_coplanar(fold_strips(guide.stations, heights, guide.folding));
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh, the fold lines between stations and the diagonals
    const std::shared_ptr<TreeNode> folds = add_group("folds");

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (start && end)
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face by the exact offset, the free edges cut flush by the side planes and the end stations' planes
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const double width = guide.stations[0][guide.stations[0].point_count() - 1][1];
    const std::vector<Plane> sides = {
        Plane::from_point_normal(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, width, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(guide.stations.front()[0], Vector(1.0, 0.0, 0.0)),
        Plane::from_point_normal(guide.stations.back()[0], Vector(1.0, 0.0, 0.0)),
    };

    for (const std::shared_ptr<FacePlanarElement>& plate :
         offset_plates(_mesh, guide.thickness, sides))
        add(plate, plates);

    // assembly: the plates under every fold make one fold, a ridge with its two flanks, the folds put side by side
    const std::vector<std::shared_ptr<FacePlanarElement>> placed =
        get_elements_numbered<FacePlanarElement>("plate");
    const double fold_width =
        2.0 * width / static_cast<double>(guide.stations[0].point_count() - 1);
    std::vector<size_t> keys;
    for (const std::shared_ptr<FacePlanarElement>& plate : placed)
        keys.push_back(static_cast<size_t>(plate->polylines[0].center()[1] / fold_width));
    assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("fold", placed, keys))};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& CounterFold::mesh() const {
    return _mesh;
}

}
