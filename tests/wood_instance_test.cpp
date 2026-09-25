#include "wood_session.h"
#include "src/templates/grid.h"
using namespace session_cpp;
using namespace wood_session;

static int failures = 0;

static void check(bool condition, std::string_view name) {

    if (condition)
        return;

    std::cerr << "FAIL " << name << "\n";
    ++failures;
}

/// Whether two polylines have the same points within tolerance.
static bool same(const Polyline& a, const Polyline& b, double tolerance = 1e-6) {

    if (a.point_count() != b.point_count())
        return false;

    for (size_t i = 0; i < a.point_count(); i++)
        if (a.get_point(i).distance(b.get_point(i)) > tolerance)
            return false;

    return true;
}

/// Whether two closed polylines have the same corners within tolerance in the same order from any start: a contact polygon of a rotated copy starts where the geometry puts it.
static bool same_loop(const Polyline& a, const Polyline& b, double tolerance) {

    const size_t count = a.point_count() - 1;
    if (a.point_count() != b.point_count() || count == 0)
        return same(a, b, tolerance);

    for (size_t shift = 0; shift < count; shift++) {
        bool equal = true;
        for (size_t i = 0; i < count && equal; i++)
            equal = a.get_point(i).distance(b.get_point((i + shift) % count)) <= tolerance;

        if (equal)
            return true;
    }

    return false;
}

/// The contacts on the edge of two element or instance guids, in order; instances are not Elements, so the edge is read directly.
static std::vector<const InteractionContact*> contacts_between(const WoodSession& session, const std::string& a, const std::string& b) {

    std::vector<const InteractionContact*> contacts;

    if (!session.graph.has_edge({a, b}) || !session.interactions.count(session.graph.edges.at(a).at(b).guid()))
        return contacts;

    for (const std::shared_ptr<Interaction>& interaction : session.interactions.at(session.graph.edges.at(a).at(b).guid()))
        if (const InteractionContact* contact = dynamic_cast<const InteractionContact*>(interaction.get()))
            contacts.push_back(contact);

    return contacts;
}

/// A 200 x 100 column of height 3000 standing at origin.
static std::shared_ptr<Column> column_at(const Point& origin) {
    return std::make_shared<Column>(Line::from_points(origin, origin + Vector(0, 0, 3000)), Polyline::rectangle(origin + Vector(-100, -50, 0), Vector::x_axis(), Vector::y_axis(), 200, 100));
}

/// A 400 x 200 x 40 plate with its bottom corner at origin.
static std::shared_ptr<Plate> plate_at(const Point& origin) {
    return Plate::from_rectangle(origin, Vector::x_axis(), Vector::y_axis(), 400, 200, Vector(0, 0, 40));
}

/// The 1_elements_tree scene: three bays of the grid template side by side, each a branch of the root.
static WoodSession tree_scene() {

    const wood_grid::Framing framing{.system = 1, .span = 0, .node = 0, .deck = 200.0, .head = 300.0, .reach = 200.0, .profiles = {.column = profile_rectangle(200.0, 200.0), .girder = profile_rectangle(200.0, 200.0)}};
    WoodSession scene("tree");

    for (int i = 0; i < 3; i++) {
        const Xform shift = Xform::translation(i * 6400.0, 0.0, 0.0);
        const std::vector<Polyline> footprint = {Polyline::rectangle(Point(0.0, 0.0, 0.0), Vector::x_axis(), Vector::y_axis(), 4000.0, 3000.0).transformed(shift)};
        const wood_grid::Building building = wood_grid::Building::from_footprint(footprint, {0.0, 3700.0}, wood_grid::Pattern::orthogonal({4000.0}, {3000.0}).transformed(shift));
        const std::shared_ptr<TreeNode> branch = scene.add_group(fmt::format("bay_{}", i));

        for (const std::shared_ptr<Element>& element : building.to_elements(framing, 0))
            scene.add(element, branch);
    }

    return scene;
}

/// Whether two scenes holding the same guids found the same contacts: pairs, counts, orientation, faces and polygons.
static bool same_contacts(const WoodSession& a, const WoodSession& b) {

    if (a.interactions.size() != b.interactions.size())
        return false;

    for (const std::tuple<std::string, std::string>& pair : a.graph.get_edges()) {

        const Edge& edge = a.graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        const std::vector<const InteractionContact*> mine = contacts_between(a, edge.v0, edge.v1);
        const std::vector<const InteractionContact*> other = contacts_between(b, edge.v0, edge.v1);

        if (mine.size() != other.size() || (!mine.empty() && b.graph.edges.at(edge.v0).at(edge.v1).v0 != edge.v0))
            return false;

        for (size_t i = 0; i < mine.size(); i++) {

            const InteractionContactFace* face = dynamic_cast<const InteractionContactFace*>(mine[i]);
            const InteractionContactFace* twin = dynamic_cast<const InteractionContactFace*>(other[i]);

            if (!face || !twin || face->face_a != twin->face_a || face->face_b != twin->face_b || !same_loop(face->polygon, twin->polygon, 1e-3))
                return false;
        }
    }

    return true;
}

/// Whether every interaction list sits on a graph edge whose two stored copies share its guid.
static bool attached(const WoodSession& session) {

    std::unordered_set<std::string> edges;

    for (const std::tuple<std::string, std::string>& pair : session.graph.get_edges()) {

        const std::string& id = session.graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair)).guid();

        if (session.graph.edges.at(std::get<1>(pair)).at(std::get<0>(pair)).guid() != id)
            return false;

        edges.insert(id);
    }

    for (const std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& entry : session.interactions)
        if (!edges.count(entry.first))
            return false;

    return !session.interactions.empty();
}

static void element_key_test() {

    const std::shared_ptr<Column> column = column_at(Point(0, 0, 0));
    const std::optional<std::pair<std::string, Xform>> key = element_key(*column);
    check(key.has_value(), "Element Key");

    std::shared_ptr<Column> moved = column->transformed(Xform::translation(500, 700, 0) * Xform::rotation_z(90.0, true));
    check(element_key(*moved)->first == key->first, "Element Key Rotated Copy");

    const Xform mirror = Xform::translation(500, 0, 0) * Xform::scale_xyz(-1, 1, 1) * Xform::translation(-500, 0, 0);
    moved->section = moved->section.transformed(mirror);
    check(element_key(*moved)->first != key->first, "Element Key Mirror");

    const Polyline bottom = Polyline::rectangle(Point(-100, -100, 0), Vector::x_axis(), Vector::y_axis(), 200, 200);
    const Block head({bottom, Polyline::rectangle(Point(-300, -300, 300), Vector::x_axis(), Vector::y_axis(), 600, 600)}, "head");
    const std::shared_ptr<Block> turned = head.transformed(Xform::rotation_z(90.0, true));
    check(element_key(*turned)->first == element_key(head)->first, "Element Key Rotated Head");
    check(!element_key(Element()).has_value(), "Element Key Plain Element");
}

static void transformed_test() {

    const Xform xform = Xform::translation(10, 20, 30) * Xform::rotation_x(30.0, true);
    const Beam beam(Polyline({Point(0, 0, 0), Point(1000, 0, 0)}), 50.0);
    const std::shared_ptr<Beam> light = beam.transformed(xform);
    const std::shared_ptr<Element> placed = beam.clone();
    placed->place(xform);
    light->geometry_mesh();
    check(light->directions.size() == 1 && light->directions[0] == Vector::z_axis().transformed(xform), "Transformed Fills Directions");
    check(same(light->polylines()[2], placed->polylines()[2]), "Transformed Equals Place");
    check(light->guid() == beam.guid() && light->name == beam.name, "Transformed Keeps Guid");

    const std::shared_ptr<Column> column = column_at(Point(0, 0, 0));
    ElementFeature feature("contact", 0, {Polyline({Point(0, 0, 0), Point(100, 0, 0)})}, "touch");
    const std::string guid = feature.guid();
    column->add_feature(std::move(feature));
    const std::shared_ptr<Column> column_copy = column->transformed(xform);
    column_copy->geometry_mesh();
    column->place(xform);
    check(same(column_copy->polylines()[0], column->polylines()[0]) && column_copy->axis == column->axis, "Transformed Column Equals Place");
    check(column->features().back().guid() == guid && column->features().back().outlines[0].get_point(1) == Point(100, 0, 0).transformed(xform), "Place Moves Features");

    const std::shared_ptr<Block> block = std::make_shared<Block>(std::vector<Polyline>{Polyline::rectangle(Point(0, 0, 0), Vector::x_axis(), Vector::y_axis(), 200, 200), Polyline::rectangle(Point(-100, -100, 300), Vector::x_axis(), Vector::y_axis(), 400, 400)});
    const std::shared_ptr<Block> block_copy = block->transformed(xform);
    block_copy->geometry_mesh();
    block->place(xform);
    check(same(block_copy->polylines()[1], block->polylines()[1]) && same(block_copy->loops[1], block->loops[1]), "Transformed Block Equals Place");

    const std::shared_ptr<Plate> plate = plate_at(Point(0, 0, 0));
    const std::shared_ptr<Plate> moved = plate->transformed(xform);
    plate->place(xform);
    check(same(moved->polylines[1], plate->polylines[1]) && moved->planes[3] == plate->planes[3] && moved->thickness == plate->thickness, "Transformed Plate Members");
    check(plate->transformed(Xform::scale_xyz(1, 1, -1)) == nullptr, "Transformed Refuses Mirror");
}

static void add_definition_test() {

    WoodSession session("definitions");
    const std::string guid = session.add_definition(column_at(Point(0, 0, 0)), "column");
    check(!guid.empty() && session.add_definition(column_at(Point(5, 0, 0)), "column") == guid, "Add Definition Key Seen");
    check(session.definitions.elements->size() == 1 && session.order().empty() && session.xforms.empty(), "Add Definition Not Placed");
}

static void add_instance_test() {

    WoodSession session("instances");
    const std::string definition = session.add_definition(column_at(Point(0, 0, 0)), "column");
    const std::shared_ptr<TreeNode> node = session.add_instance(definition, Xform::translation(1000, 0, 0));
    const std::string guid = session.objects.instances->front()->guid();
    check(node && node->name == guid && session.graph.node_label(guid) == "instance_column", "Add Instance Node");
    check(session.xform(guid) == Xform::translation(1000, 0, 0), "Add Instance Placement");
    check(!session.add_instance(definition, Xform::scale_xyz(-1, 1, 1)) && !session.add_instance("missing", Xform()), "Add Instance Refused");
}

static void world_view_test() {

    WoodSession session("views");
    const std::shared_ptr<Column> column = column_at(Point(0, 0, 0));
    session.add_instance(session.add_definition(column, "column"), Xform::translation(1000, 0, 0), "post");
    const std::string guid = session.objects.instances->front()->guid();
    const std::shared_ptr<Column> view = std::dynamic_pointer_cast<Column>(session.world_view(guid, Xform::translation(1000, 0, 0)));
    check(view && view->guid() == guid && view->name == "post" && view->axis.start() == Point(1000, 0, 0), "World View Moved");
    check(!view->is_dirty() && same(view->polylines()[0], column->polylines()[0].transformed(Xform::translation(1000, 0, 0))), "World View Seeded");
    check(!view->geometry_synced(), "World View Not Lofted");
}

static void world_elements_test() {

    WoodSession session("worlds");
    const std::shared_ptr<Plate> plate = plate_at(Point(0, 0, 0));
    session.add(plate);
    const std::string definition = session.add_definition(column_at(Point(0, 0, 0)), "column");
    session.add_instance(definition, Xform::translation(1000, 0, 0));
    session.add_instance(definition, Xform::translation(2000, 0, 0));
    const std::vector<std::shared_ptr<Element>> elements = session.world_elements();
    check(elements.size() == 3 && elements[0] == plate, "World Elements Live First");
    check(session.world_elements<Column>().size() == 2 && session.world_elements<Plate>().size() == 1, "World Elements Typed");
    check(session.element_guids().size() == 3, "World Elements Guids");
}

static void host_feature_test() {

    WoodSession session("hosts");
    session.add_instance(session.add_definition(column_at(Point(0, 0, 0)), "column"), Xform::translation(1000, 0, 0));
    const std::shared_ptr<InstanceRef> instance = session.objects.instances->front();
    ElementFeature feature("contact", 0, {Polyline({Point(1000, 0, 0), Point(1100, 0, 0)})}, "touch");
    const std::string guid = feature.guid();
    session.host_feature(instance->guid(), std::move(feature));
    check(instance->features.size() == 1 && instance->features[0].guid() == guid, "Host Feature Guid");
    check(instance->features[0].outlines[0].get_point(0) == Point(0, 0, 0), "Host Feature Local");
}

static void instance_by_key_test() {

    WoodSession world = tree_scene();
    WoodSession instanced = world;
    WoodSession carried = world;
    check(instanced.instance_by_key() == 39, "Instance By Key Count");
    check(!instanced.history.can_undo(), "Instance By Key Unrecorded");
    check(instanced.definitions.elements->size() == 5 && instanced.objects.instances->size() == 39 && instanced.objects.elements->empty(), "Instance By Key Definitions");
    check(instanced.element_guids() == world.element_guids(), "Instance By Key Order");

    world.compute_contacts(1);
    instanced.compute_contacts(1);
    check(same_contacts(world, instanced), "Instance By Key Contacts");

    carried.compute_contacts(1);
    carried.instance_by_key();
    carried.compute_contacts(1);
    size_t features = 0;
    for (const std::shared_ptr<InstanceRef>& instance : *carried.objects.instances)
        features += instance->features.size();
    check(same_contacts(world, carried) && features == world.get_contacts().size(), "Instance By Key Carries Contacts");
}

static void promote_test() {

    config::reset_defaults();
    WoodSession world("promote");
    world.add(plate_at(Point(0, 0, 0)));
    world.add(plate_at(Point(400, 0, 0)));
    const std::vector<std::string> guids = world.element_guids();
    WoodSession instanced = world;
    instanced.instance_by_key();
    check(instanced.definitions.elements->size() == 1 && instanced.objects.instances->size() == 2, "Promote Instances");

    const std::vector<InteractionFeaturePlate> joints = world.compute_features();
    check(!joints.empty() && instanced.compute_features().size() == joints.size(), "Promote Joints");
    check(instanced.objects.instances->empty() && instanced.get_element<Plate>(guids[0]) && instanced.get_element<Plate>(guids[1]), "Promote Keeps Guids");

    const std::vector<std::shared_ptr<Plate>> plates = instanced.world_elements<Plate>();
    for (size_t i = 0; i < plates.size(); i++)
        check(!plates[i]->features.top.empty() && same(plates[i]->features.top[0], world.plates()[i]->features.top[0], 1e-6), "Promote Merged Outlines");

    WoodSession placed("placed");
    const std::shared_ptr<Plate> plate = plate_at(Point(0, 0, 0));
    placed.add(plate_at(Point(0, 0, 0)));
    placed.add(plate);
    placed.set_xform(plate->guid(), Xform::translation(400, 0, 0));
    check(placed.compute_features().size() == joints.size() && same(placed.world_elements<Plate>()[1]->features.top[0], world.plates()[1]->features.top[0], 1e-6), "Promote Placed Element");

    WoodSession ordered("ordered");
    ordered.add(plate_at(Point(-5000, 0, 0)));
    ordered.add(plate_at(Point(0, 0, 0)));
    ordered.add(plate_at(Point(400, 0, 0)));
    const std::vector<std::string> order = ordered.element_guids();
    ordered.adjacency = {{1, 2}};
    ordered.instance_by_key();
    check(ordered.compute_features().size() == 1 && ordered.objects.instances->size() == 1 && ordered.element_guids() == order, "Promote Keeps Order");
    check(ordered.compute_features().size() == 1 && ordered.get_element<Plate>(order[2])->features.top.size() == world.plates()[1]->features.top.size(), "Promote Keeps Sidecar Pairs");
}

static void round_trip_test() {

    WoodSession instanced = tree_scene();
    instanced.instance_by_key();
    instanced.compute_contacts(1);
    const WoodSession loaded = WoodSession::pb_loads(instanced.pb_dumps());
    check(loaded.definitions.elements->size() == 5 && loaded.objects.instances->size() == 39, "Round Trip Counts");
    check(std::dynamic_pointer_cast<Column>(loaded.definitions.elements->front()) != nullptr, "Round Trip Definitions Typed");
    check(loaded.interactions.size() == instanced.interactions.size() && same_contacts(instanced, loaded), "Round Trip Interactions");
}

static void undo_test() {

    WoodSession session = tree_scene();
    session.compute_contacts(1);
    const std::vector<std::string> guids = session.element_guids();
    session.begin("instance by key");
    session.instance_by_key();
    session.commit();
    check(session.undo(), "Undo Instance By Key");
    check(session.objects.instances->empty() && session.definitions.elements->empty() && session.element_guids() == guids, "Undo Restores Elements");
    check(session.columns().size() == 12 && session.plates().size() == 3 && session.beams().size() == 12 && session.blocks().size() == 12, "Undo Restores Types");
    check(attached(session), "Undo Keeps Interactions");
    check(session.redo() && session.objects.instances->size() == 39 && attached(session), "Redo Keeps Interactions");
}

static void file_size_test() {

    WoodSession world = tree_scene();
    WoodSession instanced = world;
    instanced.instance_by_key();
    world.compute_contacts(1);
    instanced.compute_contacts(1);
    check(instanced.pb_dumps().size() * 3 < world.pb_dumps().size() * 2, "File Size Instanced");
}

int main() {

    element_key_test();
    transformed_test();
    add_definition_test();
    add_instance_test();
    world_view_test();
    world_elements_test();
    host_feature_test();
    instance_by_key_test();
    promote_test();
    round_trip_test();
    undo_test();
    file_size_test();

    return failures ? 1 : 0;
}
