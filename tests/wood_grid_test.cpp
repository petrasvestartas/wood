#include "wood_session.h"
#include "src/templates/grid.h"
#include "src/templates/clash.h"
#include "src/templates/grid_plan.h"
using namespace session_cpp;
using namespace wood_session;

static int failures = 0;

static void check(bool condition, const std::string& name) {

    if (condition)
        return;

    std::cerr << "FAIL " << name << "\n";
    ++failures;
}

/// Element count per name over the world elements.
static std::map<std::string, size_t> counts(const WoodSession& session) {

    std::map<std::string, size_t> table;
    for (const std::shared_ptr<Element>& element : session.world_elements())
        table[element->name]++;

    return table;
}

/// A count table as one line.
static std::string to_string(const std::map<std::string, size_t>& table) {

    std::string line;
    for (const std::pair<const std::string, size_t>& entry : table)
        line += fmt::format("{} {} ", entry.first, entry.second);

    return line;
}

/// Guids of the elements that touch nothing after compute_contacts.
static std::vector<std::string> lonely(const WoodSession& session) {

    std::set<std::string> touched;
    for (const std::tuple<std::string, std::string>& pair : session.graph.get_edges()) {
        const Edge& edge = session.graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        if (!session.interactions.count(edge.guid()))
            continue;

        for (const std::shared_ptr<Interaction>& interaction : session.interactions.at(edge.guid()))
            if (dynamic_cast<const InteractionContact*>(interaction.get())) {
                touched.insert(edge.v0);
                touched.insert(edge.v1);
            }
    }

    std::vector<std::string> alone;
    for (const std::shared_ptr<Element>& element : session.world_elements())
        if (!touched.count(element->guid()))
            alone.push_back(element->name + " " + element->guid());

    return alone;
}

/// Names of the elements whose solid is open, inside out or empty.
static std::vector<std::string> open(const WoodSession& session) {

    std::vector<std::string> bad;
    for (const std::shared_ptr<Element>& element : session.world_elements()) {
        Mesh copy = element->model_geometry_mesh();
        if (copy.number_of_faces() == 0 || !copy.is_closed() || copy.orient_outward() || copy.volume() <= 0.0)
            bad.push_back(element->name + " " + element->guid());
    }

    return bad;
}

/// True when a column or core wall stands in the box at height z.
static bool occupied(const WoodSession& session, const AABB& box, double z) {

    for (const std::shared_ptr<Element>& element : session.world_elements()) {
        if (element->name != "column" && element->name != "core_wall")
            continue;

        const AABB other = AABB::from_mesh(element->model_geometry_mesh(), 1.0);
        if (other.intersects(box) && other.min_point()[2] <= z && other.max_point()[2] >= z)
            return true;
    }

    return false;
}

/// Names of the decks with a hole nothing rises through.
static std::vector<std::string> empty_holes(const WoodSession& session) {

    std::vector<std::string> bad;
    for (const std::shared_ptr<Element>& element : session.world_elements()) {
        const std::shared_ptr<Plate> deck = std::dynamic_pointer_cast<Plate>(element);
        if (!deck || deck->name != "deck")
            continue;

        for (size_t k = 1; k < deck->features.bottom.size(); k++) {
            const AABB box = AABB::from_points(deck->features.bottom[k].get_points(), 1.0);
            if (!occupied(session, box, (deck->features.bottom[k].get_point(0)[2] + deck->features.top[k].get_point(0)[2]) / 2.0))
                bad.push_back(fmt::format("deck hole at {:.0f},{:.0f}", box.min_point()[0], box.min_point()[1]));
        }
    }

    return bad;
}

/// Extent of a mesh along a direction.
static std::pair<double, double> extent(const Mesh& mesh, const Vector& direction) {

    std::pair<double, double> range(std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());
    for (const size_t vertex : mesh.vertices()) {
        range.first = std::min(range.first, (*mesh.vertex_point(vertex) - Point(0.0, 0.0, 0.0)).dot(direction));
        range.second = std::max(range.second, (*mesh.vertex_point(vertex) - Point(0.0, 0.0, 0.0)).dot(direction));
    }

    return range;
}

/// Ends of members no contact polygon comes within the member's section size of: an end hanging in the air.
static std::vector<std::string> loose(const WoodSession& session) {

    std::map<std::string, std::vector<Point>> touches;
    for (const std::tuple<std::string, std::string>& pair : session.graph.get_edges()) {
        const Edge& edge = session.graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        if (!session.interactions.count(edge.guid()))
            continue;

        for (const std::shared_ptr<Interaction>& interaction : session.interactions.at(edge.guid()))
            if (const InteractionContactFace* face = dynamic_cast<const InteractionContactFace*>(interaction.get()))
                for (const std::string& guid : {edge.v0, edge.v1})
                    for (const Point& point : face->polygon.get_points())
                        touches[guid].push_back(point);
    }

    std::vector<std::string> bad;
    for (const std::shared_ptr<Element>& element : session.world_elements()) {
        const std::shared_ptr<Beam> beam = std::dynamic_pointer_cast<Beam>(element);
        if (!beam)
            continue;

        const Vector along = (beam->axis.get_points().back() - beam->axis.get_points().front()).normalized();
        const std::pair<double, double> range = extent(beam->model_geometry_mesh(), along);
        const std::pair<double, double> size = compute_size(beam->profile);
        const double slack = std::max(size.first, size.second);
        bool start = false;
        bool end = false;
        for (const Point& point : touches[beam->guid()]) {
            start = start || (point - Point(0.0, 0.0, 0.0)).dot(along) <= range.first + slack;
            end = end || (point - Point(0.0, 0.0, 0.0)).dot(along) >= range.second - slack;
        }
        if (!start || !end)
            bad.push_back(fmt::format("{} at {:.0f},{:.0f},{:.0f}", beam->name, beam->axis.get_point(0)[0], beam->axis.get_point(0)[1], beam->axis.get_point(0)[2]));
    }

    return bad;
}

/// Area of a closed polygon.
static double area(const Polyline& polygon) {

    const std::vector<Point> points = polygon.get_points();
    Vector sum(0.0, 0.0, 0.0);
    for (size_t k = 1; k + 1 < points.size(); k++)
        sum += (points[k] - points[0]).cross(points[k + 1] - points[0]);

    return sum.magnitude() / 2.0;
}

/// Columns above the ground whose foot is not fully carried: the contact faces at the foot cover less than the section, and no column stands beneath.
static std::vector<std::string> overhanging(const WoodSession& session) {

    std::vector<std::string> bad;
    for (const std::shared_ptr<Element>& element : session.world_elements()) {
        const std::shared_ptr<Column> column = std::dynamic_pointer_cast<Column>(element);
        if (!column || column->axis.start()[2] < 1.0)
            continue;

        const double foot = column->axis.start()[2];
        const double section = std::abs(wood_grid::plan::compute_area(wood_grid::plan::to_loop(column->section)));
        double covered = 0.0;
        bool stacked = false;
        for (const std::tuple<std::string, std::string>& pair : session.graph.get_edges()) {
            const Edge& edge = session.graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
            if ((edge.v0 != column->guid() && edge.v1 != column->guid()) || !session.interactions.count(edge.guid()))
                continue;

            const std::shared_ptr<Element> other = session.get_element<Element>(edge.v0 == column->guid() ? edge.v1 : edge.v0);
            for (const std::shared_ptr<Interaction>& interaction : session.interactions.at(edge.guid())) {
                const InteractionContactFace* face = dynamic_cast<const InteractionContactFace*>(interaction.get());
                if (!face || std::abs(Point::centroid(face->polygon.get_points())[2] - foot) > 0.5)
                    continue;

                stacked = stacked || other->name == "column";
                covered += area(face->polygon);
            }
        }
        if (!stacked && covered < section - 100.0)
            bad.push_back(fmt::format("column foot on {:.0f} of {:.0f} mm2 at {:.0f},{:.0f},{:.0f}", covered, section, column->axis.start()[0], column->axis.start()[1], foot));
    }

    return bad;
}

/// Decks narrower across one of their sides than the wall thickness: a sliver plate.
static std::vector<std::string> slivers(const WoodSession& session, double wall) {

    std::vector<std::string> bad;
    for (const std::shared_ptr<Element>& element : session.world_elements()) {
        const std::shared_ptr<Plate> deck = std::dynamic_pointer_cast<Plate>(element);
        if (!deck || deck->name != "deck")
            continue;

        const std::vector<Point> outline = wood_grid::plan::to_loop(deck->polylines[0]);
        double width = std::numeric_limits<double>::max();
        for (size_t i = 0; i < outline.size(); i++) {
            const Vector normal = Vector(0.0, 0.0, 1.0).cross(wood_grid::plan::compute_direction(outline[i], outline[(i + 1) % outline.size()]));
            double depth = 0.0;
            for (const Point& point : outline)
                depth = std::max(depth, std::abs((point - outline[i]).dot(normal)));
            width = std::min(width, depth);
        }
        if (width < wall)
            bad.push_back(fmt::format("deck {:.0f} wide at {:.0f},{:.0f}", width, outline[0][0], outline[0][1]));
    }

    return bad;
}

/// Rank of a member name at a support, highest through.
static int rank(const std::string& name) {

    if (name == "edge_girder")
        return 8;
    if (name == "edge_beam")
        return 7;
    if (name == "girder")
        return 6;
    if (name == "beam")
        return 5;

    return name == "purlin" ? 4 : 0;
}

/// True when a contact face joins two elements.
static bool touching(const WoodSession& session, const std::string& a, const std::string& b) {

    if (!session.graph.has_edge({a, b}) || !session.interactions.count(session.graph.edges.at(a).at(b).guid()))
        return false;

    for (const std::shared_ptr<Interaction>& interaction : session.interactions.at(session.graph.edges.at(a).at(b).guid()))
        if (dynamic_cast<const InteractionContactFace*>(interaction.get()))
            return true;

    return false;
}

/// True when a beam has a contact face with any head.
static bool on_head(const WoodSession& session, const std::shared_ptr<Beam>& beam) {

    for (const std::shared_ptr<Element>& element : session.world_elements())
        if (element->name == "head" && touching(session, beam->guid(), element->guid()))
            return true;

    return false;
}

/// Member ends that hang off the column or core wall they aim at: an end centre within 50 mm of the support at a shared height, of the highest rank arriving there, with no contact face on the support and none on a head.
static std::vector<std::string> unbearing(const WoodSession& session) {

    std::vector<std::string> bad;
    for (const std::shared_ptr<Element>& support : session.world_elements()) {
        if (support->name != "column" && support->name != "core_wall")
            continue;

        const AABB box = AABB::from_mesh(support->model_geometry_mesh(), 50.0);
        std::vector<std::pair<int, std::shared_ptr<Beam>>> aimed;
        for (const std::shared_ptr<Element>& element : session.world_elements()) {
            const std::shared_ptr<Beam> beam = std::dynamic_pointer_cast<Beam>(element);
            if (!beam || rank(beam->name) == 0)
                continue;

            const Point start = beam->axis.get_points().front();
            const Vector along = (beam->axis.get_points().back() - start).normalized();
            const std::pair<double, double> range = extent(beam->model_geometry_mesh(), along);
            for (const double at : {range.first, range.second})
                if (box.contains(start + along * (at - (start - Point(0.0, 0.0, 0.0)).dot(along))))
                    aimed.emplace_back(rank(beam->name), beam);
        }

        int top = 0;
        for (const std::pair<int, std::shared_ptr<Beam>>& end : aimed)
            top = std::max(top, end.first);
        for (const std::pair<int, std::shared_ptr<Beam>>& end : aimed)
            if (end.first == top && !touching(session, support->guid(), end.second->guid()) && !on_head(session, end.second))
                bad.push_back(fmt::format("{} at {:.0f},{:.0f},{:.0f} off {}", end.second->name, end.second->axis.get_point(0)[0], end.second->axis.get_point(0)[1], end.second->axis.get_point(0)[2], support->name));
    }

    return bad;
}

/// Heads whose plan extent exceeds limit: a head grown over a sliver cell.
static std::vector<std::string> wide_heads(const WoodSession& session, double limit) {

    std::vector<std::string> bad;
    for (const std::shared_ptr<Element>& element : session.world_elements()) {
        if (element->name != "head")
            continue;

        const AABB box = AABB::from_mesh(element->model_geometry_mesh(), 0.0);
        const Vector size = box.max_point() - box.min_point();
        if (std::max(size[0], size[1]) > limit)
            bad.push_back(fmt::format("head {:.0f} x {:.0f} at {:.0f},{:.0f}", size[0], size[1], box.min_point()[0], box.min_point()[1]));
    }

    return bad;
}

/// The box of the first element of a name whose box holds the plan point.
static std::optional<AABB> box_at(const WoodSession& session, const std::string& name, const Point& point) {

    for (const std::shared_ptr<Element>& element : session.world_elements()) {
        const AABB box = AABB::from_mesh(element->model_geometry_mesh(), 0.0);
        if (element->name == name && box.min_point()[0] <= point[0] && point[0] <= box.max_point()[0] && box.min_point()[1] <= point[1] && point[1] <= box.max_point()[1])
            return box;
    }

    return std::nullopt;
}

/// The checks every scene passes: the expected count table, every solid closed and outward, no clash, every element in contact, every member end supported, every top member end on the column or wall it aims at, no deck hole left empty, every column foot above the ground fully carried, no deck narrower than a wall, no head wider than six reaches (a direction polygon at reach spans at most 5.2).
static void verify(WoodSession& session, const std::string& name, const std::map<std::string, size_t>& expected, int level = 0, double wall = 200.0, double reach = 400.0) {

    const std::map<std::string, size_t> table = counts(session);
    check(table == expected, fmt::format("{} counts: {}", name, to_string(table)));

    const std::vector<std::string> bad = open(session);
    check(bad.empty(), fmt::format("{} open solids {}: {}", name, bad.size(), bad.empty() ? "" : bad[0]));

    std::vector<std::tuple<std::string, std::string, double>> clashes;
    try {
        clashes = wood_grid::compute_clashes(session, 1.0);
    } catch (const std::runtime_error& error) {
        check(false, fmt::format("{} {}", name, error.what()));
    }
    std::string worst;
    for (size_t k = 0; k < std::min<size_t>(3, clashes.size()); k++)
        worst += fmt::format("{} x {} {:.0f} at x {:.0f}; ", session.get_element<Element>(std::get<0>(clashes[k]))->name, session.get_element<Element>(std::get<1>(clashes[k]))->name, std::get<2>(clashes[k]), session.get_element<Element>(std::get<1>(clashes[k]))->point()[0]);
    check(clashes.empty(), fmt::format("{} clashes {}: {}", name, clashes.size(), worst));

    session.compute_contacts(level);
    const std::vector<std::string> alone = lonely(session);
    check(alone.empty(), fmt::format("{} lonely {}: {}", name, alone.size(), alone.empty() ? "" : alone[0]));

    const std::vector<std::string> ends = loose(session);
    check(ends.empty(), fmt::format("{} loose ends {}: {}", name, ends.size(), ends.empty() ? "" : ends[0]));

    const std::vector<std::string> hanging = unbearing(session);
    check(hanging.empty(), fmt::format("{} unbearing ends {}: {}", name, hanging.size(), hanging.empty() ? "" : hanging[0]));

    const std::vector<std::string> wide = wide_heads(session, 6.0 * reach);
    check(wide.empty(), fmt::format("{} wide heads {}: {}", name, wide.size(), wide.empty() ? "" : wide[0]));

    const std::vector<std::string> holes = empty_holes(session);
    check(holes.empty(), fmt::format("{} empty holes {}: {}", name, holes.size(), holes.empty() ? "" : holes[0]));

    const std::vector<std::string> feet = overhanging(session);
    check(feet.empty(), fmt::format("{} overhanging feet {}: {}", name, feet.size(), feet.empty() ? "" : feet[0]));

    const std::vector<std::string> narrow = slivers(session, wall);
    check(narrow.empty(), fmt::format("{} sliver decks {}: {}", name, narrow.size(), narrow.empty() ? "" : narrow[0]));
}

// ═══════════════════════════════════════════════════════════════════════════
// Cases
// ═══════════════════════════════════════════════════════════════════════════

const Vector X(1.0, 0.0, 0.0);
const Vector Y(0.0, 1.0, 0.0);
const wood_grid::Profiles GLULAM{.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0), .edge_girder = profile_rectangle(300.0, 720.0), .edge_beam = profile_rectangle(260.0, 600.0)};

static void clash_test() {

    WoodSession session("clash");
    session.add(std::make_shared<Column>(Line::from_points(Point(0.0, 0.0, 0.0), Point(0.0, 0.0, 3000.0)), profile_w(206.0, 210.0, 14.2, 10.2), 0.0, "column"));
    session.add(std::make_shared<Beam>(Polyline({Point(-1000.0, 0.0, 2875.0), Point(1000.0, 0.0, 2875.0)}), profile_w(250.0, 250.0, 15.0, 10.0), std::vector<Vector>{Vector(0.0, 0.0, 1.0)}, "girder"));
    const std::vector<std::tuple<std::string, std::string, double>> through = wood_grid::compute_clashes(session, 1.0);
    check(through.size() == 1 && std::get<2>(through[0]) > 1e5, fmt::format("clash W girder through W column {}", through.size()));

    WoodSession notched("notched");
    std::shared_ptr<Plate> deck = std::make_shared<Plate>(Polyline::rectangle(Point(0.0, 0.0, 3000.0), X, Y, 4000.0, 4000.0), Polyline::rectangle(Point(0.0, 0.0, 3200.0), X, Y, 4000.0, 4000.0), "deck");
    deck->features.bottom = {Polyline::rectangle(Point(0.0, 0.0, 3000.0), X, Y, 4000.0, 4000.0), Polyline::rectangle(Point(850.0, 850.0, 3000.0), X, Y, 300.0, 300.0).reversed()};
    deck->features.top = {Polyline::rectangle(Point(0.0, 0.0, 3200.0), X, Y, 4000.0, 4000.0), Polyline::rectangle(Point(850.0, 850.0, 3200.0), X, Y, 300.0, 300.0).reversed()};
    deck->invalidate_geometry();
    notched.add(deck);
    notched.add(std::make_shared<Column>(Line::from_points(Point(1000.0, 1000.0, 0.0), Point(1000.0, 1000.0, 3300.0)), profile_rectangle(300.0, 300.0), 0.0, "column"));
    check(wood_grid::compute_clashes(notched, 1.0).empty(), "clash column in the deck hole");

    notched.add(std::make_shared<Column>(Line::from_points(Point(2500.0, 2500.0, 0.0), Point(2500.0, 2500.0, 3300.0)), profile_rectangle(300.0, 300.0), 0.0, "column"));
    const std::vector<std::tuple<std::string, std::string, double>> solid = wood_grid::compute_clashes(notched, 1.0);
    check(solid.size() == 1 && std::get<2>(solid[0]) > 1.5e7, fmt::format("clash column through the deck {}", solid.size()));
}

static void flat_test() {

    const wood_grid::Framing framing{.system = 1, .span = 0, .node = 0, .deck = 200.0, .head = 300.0, .reach = 200.0, .profiles = {.column = profile_rectangle(200.0, 200.0), .girder = profile_rectangle(200.0, 200.0)}};
    WoodSession session("flat");
    wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 4000.0, 3000.0)}, {0.0, 3700.0}, wood_grid::Pattern::orthogonal({4000.0}, {3000.0})).to_session(session, framing);
    verify(session, "flat", {{"column", 4}, {"head", 4}, {"edge_girder", 2}, {"edge_beam", 2}, {"deck", 1}});
    check(session.get_contacts().size() == 20, fmt::format("flat contacts {}", session.get_contacts().size()));
}

static void tree_test() {

    const wood_grid::Framing framing{.system = 1, .span = 0, .node = 0, .deck = 200.0, .head = 300.0, .reach = 200.0, .profiles = {.column = profile_rectangle(200.0, 200.0), .girder = profile_rectangle(200.0, 200.0)}};
    WoodSession session("tree");
    for (int i = 0; i < 3; i++) {
        const Xform shift = Xform::translation(i * 6400.0, 0.0, 0.0);
        const wood_grid::Building building = wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 4000.0, 3000.0).transformed(shift)}, {0.0, 3700.0}, wood_grid::Pattern::orthogonal({4000.0}, {3000.0}).transformed(shift));
        const std::shared_ptr<TreeNode> branch = session.add_group(fmt::format("bay_{}", i));
        for (const std::shared_ptr<Element>& element : building.to_elements(framing, 0))
            session.add(element, branch);
    }
    verify(session, "tree", {{"column", 12}, {"head", 12}, {"edge_girder", 6}, {"edge_beam", 6}, {"deck", 3}}, 1);

    WoodSession instanced = session;
    instanced.instance_by_key();
    check(instanced.definitions.elements->size() == 5, fmt::format("tree definitions {}", instanced.definitions.elements->size()));
}

static void l_test() {

    const std::vector<Polyline> footprint = {Polyline({Point(0.0, 0.0, 0.0), Point(26810.0, 0.0, 0.0), Point(26810.0, 7600.0, 0.0), Point(17670.0, 7600.0, 0.0), Point(17670.0, 15200.0, 0.0), Point(0.0, 15200.0, 0.0), Point(0.0, 0.0, 0.0)})};
    const wood_grid::Framing framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 0, .deck = 175.0, .wall = 175.0, .facade = true, .profiles = {.column = profile_rectangle(365.0, 365.0), .girder = profile_rectangle(365.0, 365.0), .purlin = profile_rectangle(265.0, 265.0)}};
    WoodSession session("l");
    wood_grid::Building::from_footprint(footprint, {0.0, 4500.0, 8300.0, 12100.0}, wood_grid::Pattern::orthogonal({9140.0, 8530.0, 9140.0}, {7600.0, 7600.0})).to_session(session, framing);
    verify(session, "l", {{"column", 33}, {"head", 33}, {"girder", 6}, {"edge_girder", 18}, {"edge_beam", 12}, {"purlin", 48}, {"deck", 15}, {"wall", 30}});
}

static void radial_test() {

    const wood_grid::Framing framing{.system = 1, .span = 0, .node = 0, .profiles = {.column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 240.0)}};
    WoodSession session("radial");
    wood_grid::Building::from_footprint({}, {0.0, 4000.0, 8000.0}, wood_grid::Pattern::radial({4000.0, 8000.0, 12000.0}, 12)).to_session(session, framing);
    verify(session, "radial", {{"column", 72}, {"head", 72}, {"girder", 48}, {"edge_beam", 48}, {"deck", 48}});
}

static void hex_test() {

    const wood_grid::Framing framing{.span = -1, .node = 0, .profiles = {.column = profile_rectangle(240.0, 240.0), .girder = profile_rectangle(240.0, 400.0)}};
    WoodSession session("hex");
    wood_grid::Building::from_footprint({}, {0.0, 4000.0, 7600.0}, wood_grid::Pattern::hexagonal(4000.0, 3, 2)).to_session(session, framing);
    verify(session, "hex", {{"column", 44}, {"head", 44}, {"beam", 18}, {"edge_beam", 36}, {"deck", 12}});
}

static void residential_test() {

    const std::vector<Polyline> footprint = {Polyline({Point(0.0, 0.0, 0.0), Point(21945.6, 0.0, 0.0), Point(21945.6, 45720.0, 0.0), Point(43891.2, 45720.0, 0.0), Point(43891.2, 67056.0, 0.0), Point(0.0, 67056.0, 0.0), Point(0.0, 0.0, 0.0)})};
    const wood_grid::Framing framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .deck = 189.8, .wall = 250.0, .profiles = GLULAM};
    WoodSession session("residential");
    wood_grid::Building::from_footprint(footprint, {0.0, 3657.6, 7315.2}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(43891.2, 9144.0), wood_grid::compute_bays(67056.0, 9144.0)), {Polyline::rectangle(Point(7467.6, 19354.8, 0.0), X, Y, 7010.4, 7010.4)}).to_session(session, framing);
    verify(session, "residential", {{"column", 90}, {"girder", 46}, {"edge_girder", 32}, {"edge_beam", 22}, {"purlin", 150}, {"core_wall", 8}, {"deck", 60}}, 0, 250.0);
}

static void residential_t2_test() {

    const std::vector<Polyline> footprint = {Polyline({Point(0.0, 0.0, 0.0), Point(21945.6, 0.0, 0.0), Point(21945.6, 45720.0, 0.0), Point(43891.2, 45720.0, 0.0), Point(43891.2, 67056.0, 0.0), Point(0.0, 67056.0, 0.0), Point(0.0, 0.0, 0.0)})};
    const std::vector<Polyline> cores = {Polyline::rectangle(Point(15849.6, 45720.0, 0.0), X, Y, 6096.0, 9144.0), Polyline::rectangle(Point(12496.8, 9144.0, 0.0), X, Y, 3048.0, 6096.0)};
    const wood_grid::Framing framing{.system = 1, .span = 0, .node = 2, .deck = 189.8, .wall = 250.0, .profiles = GLULAM};
    WoodSession session("residential_t2");
    wood_grid::Building::from_footprint(footprint, {0.0, 3657.6, 7315.2}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(43891.2, 7620.0), wood_grid::compute_bays(67056.0, 4572.0)), cores).to_session(session, framing);
    verify(session, "residential_t2", {{"column", 160}, {"girder", 112}, {"edge_girder", 26}, {"edge_beam", 60}, {"core_wall", 16}, {"deck", 124}}, 0, 250.0);

    const std::optional<AABB> beam = box_at(session, "edge_beam", Point(21945.6, 43000.0, 0.0));
    const std::optional<AABB> girder = box_at(session, "edge_girder", Point(22300.0, 45720.0, 0.0));
    check(beam && std::abs(beam->max_point()[1] - 45595.0) < 0.01, "residential_t2 edge beam ends on the core wall face");
    check(girder && std::abs(girder->min_point()[0] - 22070.6) < 0.01, "residential_t2 edge girder starts on the core wall face");
}

static void office_ps_test() {

    const std::vector<Polyline> cores = {Polyline::rectangle(Point(15240.0, 27432.0, 0.0), X, Y, 9144.0, 9144.0), Polyline::rectangle(Point(18288.0, 14020.8, 0.0), X, Y, 6096.0, 3048.0)};
    const wood_grid::Framing framing{.system = 0, .span = 1, .edge = false, .node = 0, .deck = 295.8, .wall = 250.0, .head = 300.0, .reach = 600.0, .panel = 3505.2};
    WoodSession session("office_ps");
    wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 45567.6, 54864.0)}, {0.0, 4267.2, 8534.4}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(45567.6, 3505.2), wood_grid::compute_bays(54864.0, 4572.0)), cores).to_session(session, framing);
    verify(session, "office_ps", {{"column", 346}, {"head", 346}, {"core_wall", 16}, {"deck", 302}}, 0, 250.0);
}

static void institutional_ps_test() {

    const std::vector<Polyline> footprint = {Polyline({Point(0.0, 0.0, 0.0), Point(45567.6, 0.0, 0.0), Point(45567.6, 21336.0, 0.0), Point(17526.0, 21336.0, 0.0), Point(17526.0, 48768.0, 0.0), Point(45567.6, 48768.0, 0.0), Point(45567.6, 70104.0, 0.0), Point(0.0, 70104.0, 0.0), Point(0.0, 0.0, 0.0)})};
    const std::vector<Polyline> cores = {Polyline::rectangle(Point(11430.0, 48768.0, 0.0), X, Y, 6096.0, 9144.0), Polyline::rectangle(Point(14478.0, 15240.0, 0.0), X, Y, 3048.0, 6096.0)};
    const wood_grid::Framing framing{.system = 0, .span = 1, .edge = false, .node = 0, .deck = 294.3, .wall = 250.0, .head = 300.0, .reach = 600.0, .panel = 3505.2};
    WoodSession session("institutional_ps");
    wood_grid::Building::from_footprint(footprint, {0.0, 4876.8, 9753.6}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(45567.6, 3505.2), wood_grid::compute_bays(70104.0, 4572.0)), cores).to_session(session, framing);
    verify(session, "institutional_ps", {{"column", 402}, {"head", 402}, {"core_wall", 16}, {"deck", 334}}, 0, 250.0);
}

static void taper_test() {

    const wood_grid::Framing framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 0, .taper = 30.0};
    WoodSession session("taper");
    const Mesh massing = Mesh::loft({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 30000.0, 20000.0)}, {Polyline::rectangle(Point(1500.0, 1500.0, 14400.0), X, Y, 27000.0, 17000.0)});
    wood_grid::Building::from_solid(massing, {0.0, 3600.0, 7200.0, 10800.0, 14400.0}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(30000.0, 6000.0), wood_grid::compute_bays(20000.0, 5000.0))).to_session(session, framing);
    verify(session, "taper", {{"column", 120}, {"head", 120}, {"girder", 60}, {"edge_girder", 40}, {"edge_beam", 32}, {"purlin", 144}, {"deck", 80}});
}

static void braced_test() {

    std::vector<Line> lines;
    std::vector<Polyline> surfaces;
    for (int k = 0; k <= 2; k++)
        for (int i = 0; i <= 2; i++)
            for (int j = 0; j <= 2; j++) {
                const Point node(i * 6000.0, j * 6000.0, k * 3800.0);
                if (k > 0 && i < 2)
                    lines.push_back(Line::from_points(node, node + X * 6000.0));
                if (k > 0 && j < 2)
                    lines.push_back(Line::from_points(node, node + Y * 6000.0));
                if (k < 2)
                    lines.push_back(Line::from_points(node, node + Vector(0.0, 0.0, 3800.0)));
                if (k > 0 && i < 2 && j < 2)
                    surfaces.push_back(Polyline::rectangle(node, X, Y, 6000.0, 6000.0));
                if (k < 2 && j < 2 && (i == 0 || i == 2))
                    lines.push_back(Line::from_points(node, node + Y * 6000.0 + Vector(0.0, 0.0, 3800.0)));
            }

    const wood_grid::Framing framing{.span = -1, .node = 1, .profiles = {.column = profile_rectangle(300.0, 300.0), .girder = profile_rectangle(200.0, 500.0), .brace = profile_rectangle(150.0, 150.0)}};
    WoodSession session("braced");
    wood_grid::Building::from_lines(lines, surfaces).to_session(session, framing);
    verify(session, "braced", {{"column", 18}, {"beam", 8}, {"edge_beam", 16}, {"deck", 8}, {"brace", 8}});
}

static void profiles_test() {

    const std::vector<wood_grid::Profiles> profiles = {
        {.column = profile_rectangle(315.0, 342.0), .girder = profile_rectangle(265.0, 608.0)},
        {.column = profile_round(360.0), .girder = profile_round(300.0)},
        {.column = profile_w(206.0, 210.0, 14.2, 10.2), .girder = profile_w(250.0, 250.0, 15.0, 10.0)},
        {.column = profile_hss(178.0, 178.0, 12.7), .girder = profile_hss(250.0, 250.0, 10.0)},
        {.column = profile_rectangle(315.0, 342.0), .girder = profile_double(120.0, 600.0, 60.0)},
        {.column = profile_rectangle(315.0, 342.0), .girder = profile_slab_band(1200.0, 300.0)},
        {.column = profile_rectangle(315.0, 342.0), .girder = profile_t(300.0, 500.0, 100.0, 80.0)}
    };
    WoodSession session("profiles");
    const std::shared_ptr<TreeNode> group = session.add_group("storey_0");

    for (size_t bay = 0; bay < profiles.size(); bay++) {
        wood_grid::Framing framing{.system = 2, .span = 0, .spacing = 2000.0, .node = 1, .deck = 87.0, .profiles = profiles[bay]};
        framing.profiles.purlin = profile_rectangle(215.0, 456.0);
        const wood_grid::Pattern pattern = wood_grid::Pattern::orthogonal({6000.0}, {6000.0}).transformed(Xform::translation(8000.0 * bay, 0.0, 0.0));
        for (const std::shared_ptr<Element>& element : wood_grid::Building::from_footprint({}, {0.0, 4500.0}, pattern).to_elements(framing, 0))
            session.add(element, group);
    }

    for (const std::shared_ptr<Element>& element : session.world_elements())
        check(element->model_geometry_mesh().is_closed(), "profiles closed " + element->name);
    verify(session, "profiles", {{"column", 28}, {"edge_girder", 16}, {"edge_beam", 14}, {"purlin", 14}, {"deck", 7}});

    const std::optional<AABB> beam = box_at(session, "edge_beam", Point(16000.0, 3000.0, 0.0));
    const std::optional<AABB> girder = box_at(session, "edge_girder", Point(16500.0, 0.0, 0.0));
    check(beam && std::abs(beam->min_point()[1] - 105.0) < 0.01, "profiles W edge beam ends on the column flange");
    check(girder && std::abs(girder->min_point()[0] - 16103.0) < 0.01, "profiles W girder starts on the column flange");
}

static void skewed_test() {

    const wood_grid::Framing framing{.system = 2, .span = 0, .spacing = 2500.0, .node = 1};
    WoodSession session("skewed");
    wood_grid::Building::from_footprint({}, {0.0, 4000.0, 8000.0}, wood_grid::Pattern::orthogonal(std::vector<double>(4, 6000.0), std::vector<double>(3, 6000.0), 30.0)).to_session(session, framing);
    verify(session, "skewed", {{"column", 40}, {"girder", 16}, {"edge_girder", 16}, {"edge_beam", 12}, {"purlin", 66}, {"deck", 24}});
}

static void triangular_test() {

    const wood_grid::Framing framing{.system = 1, .span = 0, .node = 0};
    WoodSession session("triangular");
    wood_grid::Building::from_footprint({}, {0.0, 4000.0}, wood_grid::Pattern::triangular(6000.0, 4, 3)).to_session(session, framing);
    verify(session, "triangular", {{"column", 20}, {"head", 20}, {"girder", 8}, {"edge_girder", 8}, {"edge_beam", 6}, {"deck", 24}});
}

static void irregular_test() {

    const std::vector<Line> lines = {Line::from_points(Point(-2000.0, 5000.0, 0.0), Point(32000.0, 7000.0, 0.0)), Line::from_points(Point(-2000.0, 11000.0, 0.0), Point(32000.0, 12500.0, 0.0)), Line::from_points(Point(7000.0, -2000.0, 0.0), Point(5000.0, 21000.0, 0.0)), Line::from_points(Point(15000.0, -2000.0, 0.0), Point(16000.0, 21000.0, 0.0)), Line::from_points(Point(22000.0, -2000.0, 0.0), Point(24000.0, 21000.0, 0.0))};
    const std::vector<Polyline> footprint = {Polyline({Point(0.0, 0.0, 0.0), Point(28000.0, 1500.0, 0.0), Point(30000.0, 14000.0, 0.0), Point(15000.0, 19000.0, 0.0), Point(-1000.0, 12000.0, 0.0), Point(0.0, 0.0, 0.0)})};
    const wood_grid::Framing framing{.system = 1, .span = -1, .node = 0};
    WoodSession session("irregular");
    wood_grid::Building::from_footprint(footprint, {0.0, 4000.0, 7600.0}, wood_grid::Pattern::from_lines(lines)).to_session(session, framing);
    verify(session, "irregular", {{"column", 38}, {"head", 38}, {"beam", 34}, {"edge_beam", 26}, {"deck", 24}});
}

static void courtyard_test() {

    const std::vector<Polyline> footprint = {Polyline({Point(0.0, 0.0, 0.0), Point(36576.0, 0.0, 0.0), Point(36576.0, 27432.0, 0.0), Point(0.0, 27432.0, 0.0), Point(0.0, 0.0, 0.0)}), Polyline({Point(12192.0, 9144.0, 0.0), Point(12192.0, 18288.0, 0.0), Point(24384.0, 18288.0, 0.0), Point(24384.0, 9144.0, 0.0), Point(12192.0, 9144.0, 0.0)})};
    const wood_grid::Framing framing{.system = 1, .span = 0, .node = 2, .facade = true};
    WoodSession session("courtyard");
    wood_grid::Building::from_footprint(footprint, {0.0, 3657.6, 7315.2, 10972.8}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0))).to_session(session, framing);
    verify(session, "courtyard", {{"column", 72}, {"girder", 24}, {"edge_girder", 36}, {"edge_beam", 24}, {"deck", 36}, {"wall", 60}});
}

static void pentagon_test() {

    const std::vector<Polyline> footprint = {Polyline({Point(0.0, 0.0, 0.0), Point(13716.0, 13716.0, 0.0), Point(36576.0, 13716.0, 0.0), Point(36576.0, 27432.0, 0.0), Point(0.0, 27432.0, 0.0), Point(0.0, 0.0, 0.0)})};
    const wood_grid::Framing framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .profiles = GLULAM};
    WoodSession session("pentagon");
    wood_grid::Building::from_footprint(footprint, {0.0, 3657.6, 7315.2, 10972.8}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0))).to_session(session, framing);
    verify(session, "pentagon", {{"column", 51}, {"girder", 18}, {"edge_girder", 15}, {"edge_beam", 27}, {"purlin", 63}, {"deck", 27}});
}

static void framings_test() {

    const wood_grid::Framing base{.system = 2, .span = 0, .spacing = 3000.0, .node = 1, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 800.0), .purlin = profile_rectangle(260.0, 640.0)}};
    WoodSession session("framings");
    const std::vector<double> drops = {0.0, 203.2, 640.0};
    for (size_t i = 0; i < drops.size(); i++) {
        const Xform shift = Xform::translation(i * 12000.0, 0.0, 0.0);
        const wood_grid::Building building = wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 9000.0, 9000.0).transformed(shift)}, {0.0, 4500.0}, wood_grid::Pattern::orthogonal({9000.0}, {9000.0}).transformed(shift));
        wood_grid::Framing framing = base;
        framing.drop = drops[i];
        const std::shared_ptr<TreeNode> branch = session.add_group(fmt::format("bay_{}", i));
        for (const std::shared_ptr<Element>& element : building.to_elements(framing, 0))
            session.add(element, branch);
    }
    verify(session, "framings", {{"column", 12}, {"edge_girder", 6}, {"edge_beam", 6}, {"purlin", 6}, {"deck", 3}}, 1);
}

static void fastepp_test() {

    const wood_grid::Framing framing{.system = 2, .span = 0, .spacing = 2250.0, .node = 1, .deck = 87.0, .panel = 3114.0, .profiles = {.column = profile_rectangle(315.0, 342.0), .girder = profile_rectangle(265.0, 608.0), .purlin = profile_rectangle(215.0, 456.0)}};
    WoodSession session("fastepp");
    wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 9000.0, 9000.0)}, {0.0, 4500.0}, wood_grid::Pattern::orthogonal({9000.0}, {9000.0})).to_session(session, framing);
    verify(session, "fastepp", {{"column", 4}, {"edge_girder", 2}, {"edge_beam", 2}, {"purlin", 3}, {"deck", 3}});
}

static void square_test() {

    const wood_grid::Framing framing{.system = 1, .span = 0, .node = 2, .deck = 189.8, .profiles = {.girder = profile_rectangle(220.0, 520.0), .edge_girder = profile_rectangle(220.0, 440.0), .edge_beam = profile_rectangle(220.0, 280.0)}};
    WoodSession session("square");
    wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 18288.0, 18288.0)}, {0.0, 3657.6, 7315.2}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(18288.0, 4572.0), wood_grid::compute_bays(18288.0, 4572.0))).to_session(session, framing);
    verify(session, "square", {{"column", 50}, {"girder", 24}, {"edge_girder", 16}, {"edge_beam", 16}, {"deck", 32}});
}

static void office_test() {

    const std::vector<Polyline> cores = {Polyline::rectangle(Point(15240.0, 27432.0, 0.0), X, Y, 9144.0, 9144.0), Polyline::rectangle(Point(18288.0, 14020.8, 0.0), X, Y, 6096.0, 3048.0)};
    const wood_grid::Framing framing{.system = 2, .span = 0, .spacing = 3048.0, .node = 2, .drop = 203.2, .deck = 241.8, .wall = 250.0, .profiles = {.column = profile_rectangle(360.0, 360.0), .girder = profile_rectangle(320.0, 720.0), .purlin = profile_rectangle(240.0, 520.0)}};
    WoodSession session("office");
    wood_grid::Building::from_footprint({Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 45720.0, 54864.0)}, {0.0, 4267.2, 8534.4, 12801.6}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(45720.0, 9144.0), wood_grid::compute_bays(54864.0, 6096.0)), cores).to_session(session, framing);
    verify(session, "office", {{"column", 174}, {"girder", 120}, {"edge_girder", 30}, {"edge_beam", 54}, {"purlin", 375}, {"core_wall", 24}, {"deck", 135}}, 0, 250.0);
}

static void institutional_test() {

    const std::vector<Polyline> footprint = {Polyline({Point(0.0, 0.0, 0.0), Point(45720.0, 0.0, 0.0), Point(45720.0, 21336.0, 0.0), Point(18288.0, 21336.0, 0.0), Point(18288.0, 48768.0, 0.0), Point(45720.0, 48768.0, 0.0), Point(45720.0, 70104.0, 0.0), Point(0.0, 70104.0, 0.0), Point(0.0, 0.0, 0.0)})};
    const std::vector<Polyline> cores = {Polyline::rectangle(Point(12192.0, 48768.0, 0.0), X, Y, 6096.0, 9144.0), Polyline::rectangle(Point(15240.0, 15240.0, 0.0), X, Y, 3048.0, 6096.0)};
    const wood_grid::Framing framing{.system = 2, .span = 1, .spacing = 3048.0, .node = 2, .drop = 203.2, .wall = 250.0, .profiles = GLULAM};
    WoodSession session("institutional");
    wood_grid::Building::from_footprint(footprint, {0.0, 4876.8, 9753.6, 14630.4}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(45720.0, 9144.0), wood_grid::compute_bays(70104.0, 9144.0)), cores).to_session(session, framing);
    verify(session, "institutional", {{"column", 147}, {"girder", 72}, {"edge_girder", 54}, {"edge_beam", 48}, {"purlin", 240}, {"core_wall", 24}, {"deck", 102}}, 0, 250.0);
}

static void point_supported_test() {

    const std::vector<Polyline> footprint = {Polyline({Point(0.0, 0.0, 0.0), Point(21031.2, 0.0, 0.0), Point(21031.2, 45720.0, 0.0), Point(42062.4, 45720.0, 0.0), Point(42062.4, 67056.0, 0.0), Point(0.0, 67056.0, 0.0), Point(0.0, 0.0, 0.0)})};
    const std::vector<Polyline> cores = {Polyline::rectangle(Point(14935.2, 45720.0, 0.0), X, Y, 6096.0, 9144.0), Polyline::rectangle(Point(11582.4, 9144.0, 0.0), X, Y, 3048.0, 6096.0)};
    const wood_grid::Framing framing{.system = 0, .span = 1, .edge = false, .node = 0, .deck = 291.4, .wall = 250.0, .head = 300.0, .reach = 600.0, .capital = 1, .panel = 3505.2};
    WoodSession session("point_supported");
    wood_grid::Building::from_footprint(footprint, {0.0, 3657.6, 7315.2, 10972.8}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(42062.4, 3505.2), wood_grid::compute_bays(67056.0, 4572.0)), cores).to_session(session, framing);
    verify(session, "point_supported", {{"column", 420}, {"head", 420}, {"drop_panel", 420}, {"core_wall", 24}, {"deck", 354}}, 0, 250.0);
}

static void box_test() {

    const wood_grid::Framing framing{.system = 1, .span = 1, .node = 1};
    WoodSession session("box");
    const Mesh massing = Mesh::create_box(30000.0, 18000.0, 12000.0).transformed(Xform::translation(15000.0, 9000.0, 6000.0));
    wood_grid::Building::from_solid(massing, {0.0, 4000.0, 8000.0, 12000.0}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(30000.0, 6000.0), wood_grid::compute_bays(18000.0, 6000.0))).to_session(session, framing);
    verify(session, "box", {{"column", 72}, {"girder", 36}, {"edge_girder", 18}, {"edge_beam", 30}, {"deck", 45}});
}

static void prism_test() {

    const Polyline footprint({Point(0.0, 0.0, 0.0), Point(13716.0, 13716.0, 0.0), Point(36576.0, 13716.0, 0.0), Point(36576.0, 27432.0, 0.0), Point(0.0, 27432.0, 0.0), Point(0.0, 0.0, 0.0)});
    const wood_grid::Framing framing{.system = 2, .span = 0, .spacing = 3048.0, .node = 0};
    WoodSession session("prism");
    wood_grid::Building::from_solid(Mesh::loft({footprint}, {footprint.transformed(Xform::translation(0.0, 0.0, 10972.8))}), {0.0, 3657.6, 7315.2, 10972.8}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(36576.0, 9144.0), wood_grid::compute_bays(27432.0, 9144.0))).to_session(session, framing);
    verify(session, "prism", {{"column", 51}, {"head", 51}, {"girder", 15}, {"edge_girder", 21}, {"edge_beam", 21}, {"purlin", 72}, {"deck", 27}});
}

/// The podium and the tower of the setback example as one closed shell: the faces of three lofts and two caps.
static Mesh setback() {

    const Polyline podium = Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 40000.0, 30000.0);
    const Polyline roof = podium.transformed(Xform::translation(0.0, 0.0, 8000.0));
    const Polyline foot = Polyline::rectangle(Point(10000.0, 5000.0, 8000.0), X, Y, 20000.0, 20000.0);
    const Polyline top = foot.transformed(Xform::translation(0.0, 0.0, 14400.0));

    std::vector<Polyline> faces = {podium, top};
    for (const Mesh& part : {Mesh::loft({podium}, {roof}, false), Mesh::loft({roof}, {foot}, false), Mesh::loft({foot}, {top}, false)})
        for (const Polyline& face : part.face_outlines())
            faces.push_back(face);

    return Mesh::from_polylines(faces, 1.0);
}

static void setback_test() {

    const wood_grid::Framing framing{.system = 1, .span = 1, .node = 1};
    WoodSession session("setback");
    wood_grid::Building::from_solid(setback(), {0.0, 4000.0, 8000.0, 11600.0, 15200.0, 18800.0, 22400.0}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(40000.0, 5000.0), wood_grid::compute_bays(30000.0, 5000.0))).to_session(session, framing);
    verify(session, "setback", {{"column", 226}, {"girder", 132}, {"edge_girder", 56}, {"edge_beam", 64}, {"deck", 160}});
}

static void atrium_test() {

    const wood_grid::Framing framing{.system = 2, .span = 0, .spacing = 3000.0, .node = 2};
    WoodSession session("atrium");
    const std::vector<Polyline> bottom = {Polyline::rectangle(Point(0.0, 0.0, 0.0), X, Y, 36000.0, 36000.0), Polyline::rectangle(Point(12000.0, 12000.0, 0.0), X, Y, 12000.0, 12000.0)};
    const std::vector<Polyline> top = {bottom[0].transformed(Xform::translation(0.0, 0.0, 20000.0)), bottom[1].transformed(Xform::translation(0.0, 0.0, 20000.0))};
    wood_grid::Building::from_solid(Mesh::loft(bottom, top), {0.0, 4000.0, 8000.0, 12000.0, 16000.0, 20000.0}, wood_grid::Pattern::orthogonal(wood_grid::compute_bays(36000.0, 6000.0), wood_grid::compute_bays(36000.0, 6000.0))).to_session(session, framing);
    verify(session, "atrium", {{"column", 240}, {"girder", 120}, {"edge_girder", 80}, {"edge_beam", 80}, {"purlin", 280}, {"deck", 160}});
}

static void curved_test() {

    const wood_grid::Framing framing{.system = 2, .span = 0, .spacing = 2500.0, .node = 0};
    WoodSession session("curved");
    const std::map<std::string, size_t> expected = {{"column", 192}, {"head", 192}, {"girder", 128}, {"edge_beam", 64}, {"purlin", 256}, {"deck", 132}};
    const Polyline ring = Polyline::from_sides(16, 15000.0, true);
    wood_grid::Building::from_solid(Mesh::loft({ring}, {ring.transformed(Xform::translation(0.0, 0.0, 15200.0))}), {0.0, 3800.0, 7600.0, 11400.0, 15200.0}, wood_grid::Pattern::radial({5000.0, 10000.0, 15000.0}, 16)).to_session(session, framing);
    verify(session, "curved", expected);

    for (const double facet : {5.0, 22.5}) {
        WoodSession faceted(fmt::format("curved {}", facet));
        wood_grid::Building::from_solid(wood_grid::to_mesh(BRep::create_cylinder(15000.0, 15200.0), facet), {0.0, 3800.0, 7600.0, 11400.0, 15200.0}, wood_grid::Pattern::radial({5000.0, 10000.0, 15000.0}, 16)).to_session(faceted, framing);
        verify(faceted, fmt::format("curved brep facet {}", facet), expected);
    }
}

static void crea_test(const std::string& input_name, const std::map<std::string, size_t>& expected) {

    const Session input = Session::pb_load(config::dataset_path("crea/" + input_name + "_input", ".pb").string());
    std::vector<Line> lines;
    for (const std::vector<Polyline>& group : input.select_by_type<Polyline>())
        for (const Polyline& polyline : group)
            lines.push_back(Line::from_points(polyline.get_points().front(), polyline.get_points().back()));

    std::vector<Polyline> surfaces;
    for (const std::vector<Mesh>& group : input.select_by_type<Mesh>())
        for (const Mesh& mesh : group)
            for (const size_t face : mesh.faces()) {
                surfaces.push_back(*mesh.face_polygon(face));
                surfaces.back().name = mesh.name.find("Core") == std::string::npos ? "surface" : "core";
            }

    const wood_grid::Framing framing{.system = 1, .span = -1, .node = 0, .deck = 200.0, .wall = 200.0, .head = 300.0, .reach = 360.0, .profiles = {.column = profile_rectangle(300.0, 300.0), .girder = profile_rectangle(300.0, 300.0)}};
    WoodSession session(input_name);
    wood_grid::Building::from_lines(lines, surfaces).to_session(session, framing);
    verify(session, input_name, expected);
}

int main() {

    clash_test();
    flat_test();
    tree_test();
    l_test();
    radial_test();
    hex_test();
    residential_test();
    residential_t2_test();
    office_ps_test();
    institutional_ps_test();
    taper_test();
    braced_test();
    skewed_test();
    triangular_test();
    irregular_test();
    courtyard_test();
    pentagon_test();
    framings_test();
    fastepp_test();
    square_test();
    office_test();
    institutional_test();
    point_supported_test();
    box_test();
    prism_test();
    setback_test();
    atrium_test();
    curved_test();
    crea_test("crea_4x4", {{"column", 16}, {"head", 16}, {"beam", 4}, {"edge_beam", 16}, {"deck", 6}, {"wall", 8}, {"core_wall", 4}});
    crea_test("crea_4x4_ground", {{"column", 16}, {"head", 16}, {"beam", 4}, {"edge_beam", 16}, {"deck", 9}, {"wall", 8}, {"core_wall", 4}});
    crea_test("crea_full", {{"column", 340}, {"head", 340}, {"beam", 294}, {"edge_beam", 240}, {"deck", 248}, {"wall", 176}, {"core_wall", 80}});
    profiles_test();

    return failures == 0 ? 0 : 1;
}

/*
|||||||| DESCRIPTION ||||||||
grid template cases: the clash tool is proved on a W girder run through a W column and on a notched deck, then every grid example building and Branch's residential, office and institutional topologies are built, their element counts per role compared with the expected table, every solid checked closed and outward, zero pairwise solid overlap through compute_clashes, and after compute_contacts every element must touch another, every member end must have a contact near it and every deck hole must have a column or core wall through it; the tree scene also proves five instance definitions.

|||||||| DIRECTORY ||||||||
cd wood_research/wood

|||||||| CMAKE BUILD && RUN ||||||||
cmake --build build --target wood_grid_test --parallel 4 && tools/run_guarded.sh -t 3 -m 3 -- build/wood_grid_test
*/
