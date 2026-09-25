#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

/// One building of the row: its footprint rings (empty for every bounded cell), pattern, cores and framing.
struct Case {
    std::string name; // Group name.
    std::vector<Polyline> footprint; // Outer rings counter-clockwise, holes clockwise.
    wood_grid::Pattern pattern; // Plan lines.
    std::vector<Polyline> cores; // Core rings.
    wood_grid::Framing framing; // Method, joints and sizes.
};

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const double GAP = 8000.0; // clear distance between the buildings
const std::vector<double> ELEVATIONS = {0.0, 4000.0, 8000.0, 12000.0};
const wood_grid::Profiles GLULAM{.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0)}; // Branch3D's reference sections
const std::vector<Case> CASES = {
    {"orthogonal", {Polyline({Point(0.0, 0.0, 0.0), Point(26810.0, 0.0, 0.0), Point(26810.0, 7600.0, 0.0), Point(17670.0, 7600.0, 0.0), Point(17670.0, 15200.0, 0.0), Point(0.0, 15200.0, 0.0), Point(0.0, 0.0, 0.0)})}, wood_grid::Pattern::orthogonal({9140.0, 8530.0, 9140.0}, {7600.0, 7600.0}), {}, wood_grid::Framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 0, .deck = 175.0, .wall = 175.0, .facade = true, .profiles = {.column = profile_rectangle(365.0, 365.0), .girder = profile_rectangle(365.0, 365.0), .purlin = profile_rectangle(265.0, 265.0)}}},
    {"skewed", {}, wood_grid::Pattern::orthogonal(std::vector<double>(4, 6000.0), std::vector<double>(3, 6000.0), 30.0), {}, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2500.0, .node = 1}},
    {"radial", {}, wood_grid::Pattern::radial({4000.0, 8000.0, 12000.0}, 12), {}, wood_grid::Framing{.system = 1, .span = 0, .node = 0, .profiles = {.column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 240.0)}}},
    {"triangular", {}, wood_grid::Pattern::triangular(6000.0, 4, 3), {}, wood_grid::Framing{.system = 1, .span = -1, .node = 0}},
    {"hexagonal", {}, wood_grid::Pattern::hexagonal(4000.0, 3, 2), {}, wood_grid::Framing{.system = 1, .span = -1, .node = 0, .profiles = {.column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 400.0)}}},
    {"irregular", {Polyline({Point(0.0, 0.0, 0.0), Point(28000.0, 1500.0, 0.0), Point(30000.0, 14000.0, 0.0), Point(15000.0, 19000.0, 0.0), Point(-1000.0, 12000.0, 0.0), Point(0.0, 0.0, 0.0)})}, wood_grid::Pattern::from_lines({Line::from_points(Point(-2000.0, 5000.0, 0.0), Point(32000.0, 7000.0, 0.0)), Line::from_points(Point(-2000.0, 11000.0, 0.0), Point(32000.0, 12500.0, 0.0)), Line::from_points(Point(7000.0, -2000.0, 0.0), Point(5000.0, 21000.0, 0.0)), Line::from_points(Point(15000.0, -2000.0, 0.0), Point(16000.0, 21000.0, 0.0)), Line::from_points(Point(22000.0, -2000.0, 0.0), Point(24000.0, 21000.0, 0.0))}), {}, wood_grid::Framing{.system = 1, .span = -1, .node = 0}},
    {"courtyard", {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 36576.0, 27432.0), Polyline({Point(12192.0, 9144.0, 0.0), Point(12192.0, 18288.0, 0.0), Point(24384.0, 18288.0, 0.0), Point(24384.0, 9144.0, 0.0), Point(12192.0, 9144.0, 0.0)})}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0)), {}, wood_grid::Framing{.system = 1, .span = 0, .node = 2, .facade = true}},
    {"pentagon", {Polyline({Point(0.0, 0.0, 0.0), Point(13716.0, 13716.0, 0.0), Point(36576.0, 13716.0, 0.0), Point(36576.0, 27432.0, 0.0), Point(0.0, 27432.0, 0.0), Point(0.0, 0.0, 0.0)})}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0)), {}, wood_grid::Framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .profiles = GLULAM}},
    {"cores", {Polyline({Point(0.0, 0.0, 0.0), Point(45720.0, 0.0, 0.0), Point(45720.0, 21336.0, 0.0), Point(18288.0, 21336.0, 0.0), Point(18288.0, 48768.0, 0.0), Point(45720.0, 48768.0, 0.0), Point(45720.0, 70104.0, 0.0), Point(0.0, 70104.0, 0.0), Point(0.0, 0.0, 0.0)})}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(45720.0, 9144.0), wood_grid::compute_bays(70104.0, 9144.0)), {Polyline::rectangle(Point(12192.0, 48768.0, 0.0), X, Y, 6096.0, 9144.0), Polyline::rectangle(Point(15240.0, 15240.0, 0.0), X, Y, 3048.0, 6096.0)}, wood_grid::Framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .wall = 250.0, .profiles = GLULAM}},
}; // an L with uneven bays and a facade, a skewed grid, radial, triangular and hexagonal cells, hand-drawn axes, a courtyard, Branch3D's pentagon and its institutional U with two cores
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

/// The x range of a case: its pattern lines and footprint corners.
std::pair<double, double> compute_extent(const Case& item) {

    std::pair<double, double> extent(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
    for (const Line& line : item.pattern.lines)
        for (const Point& point : {line.start(), line.end()}) {
            extent.first = std::min(extent.first, point[0]);
            extent.second = std::max(extent.second, point[0]);
        }
    for (const Polyline& ring : item.footprint)
        for (const Point& point : ring.get_points()) {
            extent.first = std::min(extent.first, point[0]);
            extent.second = std::max(extent.second, point[0]);
        }

    return extent;
}

int main() {

    WoodSession wood_session("templates_grid_footprint");
    double offset = 0.0;

    for (const Case& item : CASES) {
        const std::pair<double, double> extent = compute_extent(item);
        const Xform shift = Xform::translation(offset - extent.first, 0.0, 0.0);
        std::vector<Polyline> footprint;
        std::vector<Polyline> cores;
        for (const Polyline& ring : item.footprint)
            footprint.push_back(ring.transformed(shift));
        for (const Polyline& ring : item.cores)
            cores.push_back(ring.transformed(shift));

        const wood_grid::Building building = wood_grid::Building::from_footprint(footprint, ELEVATIONS, item.pattern.transformed(shift), cores);
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
Workflow B, a footprint and a pattern, nine buildings side by side over three storeys, one group each: an L with uneven bays, purlins on girders and facade walls; a grid whose y lines lean 30 degrees with flush columns; a radial plan with girders on the rays; triangular cells and hexagonal cells with every line a beam mitred at the nodes; five hand-drawn axes clipped to a five-sided footprint; a courtyard ring with columns through the levels; Branch3D's pentagon with girders hung 8 in and purlins at 10 ft; its institutional U with two cores as pinwheel walls. compute_contacts(1) pairs elements inside each building only. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_footprint --parallel 4 && ./build/templates_grid_footprint && ../bash/publish-scene.sh --target templates_grid_footprint

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
