#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

/// One building of the row: a closed massing, the pattern and framing it is filled with.
struct Case {
    std::string name; // Group name.
    Mesh massing; // Closed solid.
    wood_grid::Pattern pattern; // Plan lines.
    std::vector<double> elevations; // Level datums.
    wood_grid::Framing framing; // Method, joints and sizes.
};

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const double GAP = 8000.0; // clear distance between the buildings
const Polyline PENTAGON({Point(0.0, 0.0, 0.0), Point(13716.0, 13716.0, 0.0), Point(36576.0, 13716.0, 0.0), Point(36576.0, 27432.0, 0.0), Point(0.0, 27432.0, 0.0), Point(0.0, 0.0, 0.0)});
const Polyline PODIUM = Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 40000.0, 30000.0);
const Polyline TOWER = Polyline::rectangle(Point(10000.0, 5000.0, 8000.0), X, Y, 20000.0, 20000.0); // stands on the podium roof, its sides on the pattern lines
const Polyline OUTER = Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 36000.0, 36000.0);
const Polyline ATRIUM = Polyline::rectangle(Point(12000.0, 12000.0, 0.0), X, Y, 12000.0, 12000.0);
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

/// A closed shell through the faces of several open lofts and their caps.
Mesh compute_shell(const std::vector<Mesh>& parts, const std::vector<Polyline>& caps) {

    std::vector<Polyline> faces = caps;
    for (const Mesh& part : parts)
        for (const Polyline& face : part.face_outlines())
            faces.push_back(face);

    return Mesh::from_polylines(faces, 1.0);
}

const std::vector<Case> CASES = {
    {"box", Mesh::create_box(30000.0, 18000.0, 12000.0).transformed(Xform::translation(15000.0, 9000.0, 6000.0)), wood_grid::Pattern::orthogonal(wood_grid::compute_bays(30000.0, 6000.0), wood_grid::compute_bays(18000.0, 6000.0)), {0.0, 4000.0, 8000.0, 12000.0}, wood_grid::Framing{.system = 1, .span = 1, .node = 1}},
    {"prism", Mesh::loft({PENTAGON}, {PENTAGON.transformed(Xform::translation(0.0, 0.0, 10972.8))}), wood_grid::Pattern::orthogonal(wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0)), {0.0, 3657.6, 7315.2, 10972.8}, wood_grid::Framing{.system = 2, .span = 0, .spacing = 3048.0, .node = 0}},
    {"taper", Mesh::loft({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 30000.0, 20000.0)}, {Polyline::rectangle(Point(1500.0, 1500.0, 14400.0), X, Y, 27000.0, 17000.0)}), wood_grid::Pattern::orthogonal(wood_grid::compute_bays(30000.0, 6000.0), wood_grid::compute_bays(20000.0, 5000.0)), {0.0, 3600.0, 7200.0, 10800.0, 14400.0}, wood_grid::Framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 0, .taper = 30.0}},
    {"setback", compute_shell({Mesh::loft({PODIUM}, {PODIUM.transformed(Xform::translation(0.0, 0.0, 8000.0))}, false), Mesh::loft({PODIUM.transformed(Xform::translation(0.0, 0.0, 8000.0))}, {TOWER}, false), Mesh::loft({TOWER}, {TOWER.transformed(Xform::translation(0.0, 0.0, 14400.0))}, false)}, {PODIUM, TOWER.transformed(Xform::translation(0.0, 0.0, 14400.0))}), wood_grid::Pattern::orthogonal(wood_grid::compute_bays(40000.0, 5000.0), wood_grid::compute_bays(30000.0, 5000.0)), {0.0, 4000.0, 8000.0, 11600.0, 15200.0, 18800.0, 22400.0}, wood_grid::Framing{.system = 1, .span = 1, .node = 1}},
    {"atrium", Mesh::loft({OUTER, ATRIUM}, {OUTER.transformed(Xform::translation(0.0, 0.0, 20000.0)), ATRIUM.transformed(Xform::translation(0.0, 0.0, 20000.0))}), wood_grid::Pattern::orthogonal(wood_grid::compute_bays(36000.0, 6000.0), wood_grid::compute_bays(36000.0, 6000.0)), {0.0, 4000.0, 8000.0, 12000.0, 16000.0, 20000.0}, wood_grid::Framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 2}},
    {"curved", BRep::create_cylinder(15000.0, 15200.0).mesh(), wood_grid::Pattern::radial({5000.0, 10000.0, 15000.0}, 16), {0.0, 3800.0, 7600.0, 11400.0, 15200.0}, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2500.0, .node = 0}},
}; // a box, a pentagonal prism, a tapered loft, a podium with a tower, a block with an atrium, a cylinder

/// The x range of a massing.
std::pair<double, double> compute_extent(const Mesh& massing) {

    std::pair<double, double> extent(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
    for (const size_t vertex : massing.vertices()) {
        extent.first = std::min(extent.first, (*massing.vertex_point(vertex))[0]);
        extent.second = std::max(extent.second, (*massing.vertex_point(vertex))[0]);
    }

    return extent;
}

int main() {

    WoodSession wood_session("templates_grid_solid");
    double offset = 0.0;

    for (const Case& item : CASES) {
        const std::pair<double, double> extent = compute_extent(item.massing);
        const Xform shift = Xform::translation(offset - extent.first, 0.0, 0.0);
        const wood_grid::Building building = wood_grid::Building::from_solid(item.massing.transformed(shift), item.elevations, item.pattern.transformed(shift));
        const std::shared_ptr<TreeNode> group = wood_session.add_group(item.name);
        for (size_t storey = 0; storey + 1 < building.levels.size(); storey++)
            for (const std::shared_ptr<Element>& element : building.to_elements(item.framing, storey))
                wood_session.add(element, group);
        offset += extent.second - extent.first + GAP;
    }

    if constexpr (INSTANCES)
        wood_session.instance_by_key();

    wood_session.compute_contacts(1);
    wood_session.pb_dump(pb_path("live"));

    return 0;
}

/*
|||||||| DESCRIPTION ||||||||
Workflow A, a massing sliced at its levels, six buildings side by side, one group each: a box whose every section is the same rectangle; a pentagonal prism whose diagonal side cuts every girder, purlin and deck obliquely; a tapered loft whose perimeter columns lean to follow the moving section; a podium with a tower, the tower ring added to the roof plan so every tower column stands on a podium column or girder; a block with an atrium through every level, the pattern crossing in the hole getting no column; a cylinder whose facet corners fall on the sixteen rays. compute_contacts(1) pairs elements inside each building only. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_solid --parallel 4 && ./build/templates_grid_solid && ../bash/publish-scene.sh --target templates_grid_solid

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
