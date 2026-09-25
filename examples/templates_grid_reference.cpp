#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

/// One building of the row: a reference configuration as its footprint, pattern, cores, levels and framing.
struct Case {
    std::string name; // Group name.
    std::vector<Polyline> footprint; // Outer rings counter-clockwise, holes clockwise.
    wood_grid::Pattern pattern; // Plan lines.
    std::vector<Polyline> cores; // Core rings.
    std::vector<double> elevations; // Level datums.
    wood_grid::Framing framing; // Method, joints and sizes.
};

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const double GAP = 8000.0; // clear distance between the buildings
const std::vector<Polyline> SQUARE = {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 18288.0, 18288.0)}; // Branch3D's 60 ft square
const wood_grid::Pattern SQUARE_BAYS = wood_grid::Pattern::orthogonal(wood_grid::compute_bays(18288.0, 4572.0), wood_grid::compute_bays(18288.0, 4572.0)); // 15 ft bays
const std::vector<double> BRANCH = {0.0, 3657.6, 7315.2, 10972.8}; // three of Branch3D's storeys at 12 ft
const wood_grid::Profiles GLULAM{.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0)}; // Branch3D's reference sections
const std::vector<Case> CASES = {
    {"branch_plate", SQUARE, SQUARE_BAYS, {}, BRANCH, wood_grid::Framing{.system = 0, .node = 0, .deck = 365.1, .head = 300.0, .reach = 600.0, .capital = 1, .panel = 3505.2}},
    {"branch_post_and_beam", SQUARE, SQUARE_BAYS, {}, BRANCH, wood_grid::Framing{.system = 1, .span = 0, .node = 2, .deck = 189.8, .profiles = {.girder = profile_rectangle(220.0, 520.0)}}},
    {"branch_purlin_on_girder", SQUARE, SQUARE_BAYS, {}, BRANCH, wood_grid::Framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .deck = 189.8, .profiles = {.girder = profile_rectangle(220.0, 520.0), .purlin = profile_rectangle(220.0, 400.0)}}},
    {"branch_residential", {Polyline({Point(0.0, 0.0, 0.0), Point(21945.6, 0.0, 0.0), Point(21945.6, 45720.0, 0.0), Point(43891.2, 45720.0, 0.0), Point(43891.2, 67056.0, 0.0), Point(0.0, 67056.0, 0.0), Point(0.0, 0.0, 0.0)})}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(43891.2, 9144.0), wood_grid::compute_bays(67056.0, 9144.0)), {Polyline::rectangle(Point(7467.6, 19354.8, 0.0), X, Y, 7010.4, 7010.4)}, BRANCH, wood_grid::Framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .deck = 189.8, .wall = 250.0, .profiles = GLULAM}},
    {"fastepp_v1", {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 9000.0, 9000.0)}, wood_grid::Pattern::orthogonal({9000.0}, {9000.0}), {}, {0.0, 4500.0}, wood_grid::Framing{.system = 2, .span = 0, .spacing = 2250.0, .node = 1, .deck = 87.0, .panel = 3114.0, .profiles = {.column = profile_rectangle(315.0, 342.0), .girder = profile_rectangle(265.0, 608.0), .purlin = profile_rectangle(215.0, 456.0)}}},
    {"fastepp_v2", {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 6000.0, 12000.0)}, wood_grid::Pattern::orthogonal({6000.0}, {12000.0}), {}, {0.0, 4500.0}, wood_grid::Framing{.system = 2, .span = 0, .spacing = 1500.0, .node = 1, .deck = 87.0, .panel = 3095.0, .profiles = {.column = profile_rectangle(265.0, 380.0), .girder = profile_rectangle(265.0, 532.0), .purlin = profile_rectangle(215.0, 532.0)}}},
    {"fastepp_v3", {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 6000.0, 9000.0)}, wood_grid::Pattern::orthogonal({6000.0}, {9000.0}), {}, {0.0, 4500.0}, wood_grid::Framing{.system = 1, .span = 1, .node = 1, .deck = 243.0, .panel = 3126.67, .profiles = {.column = profile_rectangle(265.0, 380.0), .girder = profile_rectangle(265.0, 836.0), .beam = profile_rectangle(215.0, 836.0)}}},
    {"fastepp_v4", {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 8000.0, 12000.0)}, wood_grid::Pattern::orthogonal({8000.0}, {12000.0}), {}, {0.0, 4500.0}, wood_grid::Framing{.system = 2, .span = 0, .spacing = 8000.0 / 6.0, .node = 1, .deck = 87.0, .panel = 3104.5, .profiles = {.column = profile_rectangle(365.0, 418.0), .girder = profile_rectangle(265.0, 684.0), .purlin = profile_rectangle(215.0, 494.0)}}},
}; // Branch3D's square in its three structural methods and its residential L with a core, then the four FAST+EPP timber bays at the tool's sections and spacings
const bool INSTANCES = false; // repeated elements as one definition each, placed by instances; off until the viewer draws instances

/// The x range of a footprint.
std::pair<double, double> compute_extent(const std::vector<Polyline>& footprint) {

    std::pair<double, double> extent(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
    for (const Polyline& ring : footprint)
        for (const Point& point : ring.get_points()) {
            extent.first = std::min(extent.first, point[0]);
            extent.second = std::max(extent.second, point[0]);
        }

    return extent;
}

int main() {

    WoodSession wood_session("templates_grid_reference");
    double offset = 0.0;

    for (const Case& item : CASES) {
        const std::pair<double, double> extent = compute_extent(item.footprint);
        const Xform shift = Xform::translation(offset - extent.first, 0.0, 0.0);
        std::vector<Polyline> footprint;
        std::vector<Polyline> cores;
        for (const Polyline& ring : item.footprint)
            footprint.push_back(ring.transformed(shift));
        for (const Polyline& ring : item.cores)
            cores.push_back(ring.transformed(shift));

        const wood_grid::Building building = wood_grid::Building::from_footprint(footprint, item.elevations, item.pattern.transformed(shift), cores);
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
The reference configurations side by side, one group each: Branch3D's 60 ft square in its three structural methods, plate on columns with stepped heads under CLT strips, post and beam with girders on the x lines, purlin on girder with the girders running y hung 8 in and purlin rows at 10 ft, the columns through the levels with the decks notched round them; Branch3D's residential L over 30 ft bays with its core as pinwheel walls, every girder and purlin reaching it cut at the wall face; the four FAST+EPP timber bay variants with the datum at the framing top, columns flush with the datum, girders cut by the column faces, purlins cut by the girder sides and CLT strips over the outer column faces. compute_contacts(1) pairs elements inside each building only. INSTANCES, off until the viewer draws instances, keeps one definition per repeated element, placed by instances.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE CONFIGURE ||||||||
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

|||||||| CMAKE BUILD && RUN && CLOUDFLARE ||||||||
cmake --build build --target templates_grid_reference --parallel 4 && ./build/templates_grid_reference && ../bash/publish-scene.sh --target templates_grid_reference

|||||||| VIEW ||||||||
https://petrasvestartas.github.io/session/
*/
