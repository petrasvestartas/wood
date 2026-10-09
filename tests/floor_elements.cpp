#include <optional>
#include "wood_session.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include "src/templates/floor/floor.h"

using namespace session_cpp;
using namespace wood_session;

/// The guide of the rectangle of half spans half_x and half_y about the origin, corner 0 at (-half_x, -half_y).
wood_floor::FloorGuide rectangle_guide(double half_x, double half_y) {
    return wood_floor::FloorGuide({
        Point(-half_x, -half_y, 0.0),
        Point(half_x, -half_y, 0.0),
        Point(half_x, half_y, 0.0),
        Point(-half_x, half_y, 0.0),
    });
}

/// The square floor every check reads, the seams through the ribs by default, built on first use.
const wood_floor::FloorGuide& square_guide() {

    static const wood_floor::FloorGuide guide = rectangle_guide(3000.0, 3000.0);

    return guide;
}

const double EXACT_SUPPORT = 500671.261678; // the support's exact BRep volume, cylinders and hexagons
const double CARVED_OUTER_RIB = 98407909.203913; // an outer rib of the square carved by every connector of the floor: its rectangle plate pocket and pins, and the block pins
const double HEAD_CUT = 34771221.351479; // what the six head cuts take from the column
const double PLATE_POCKETS = 3888727.411870; // what the two column plates' pockets and pin bores take from the column in the floor

/// The exact bores of a BRep: its rational surfaces, cylinders.
size_t count_bores(const BRep& brep) {

    size_t bores = 0;

    for (const NurbsSurface& surface : brep.m_surfaces)
        bores += surface.is_rational();

    return bores;
}

/// Prints the message and throws with it when the condition fails.
void check(bool ok, const std::string& message) {

    if (!ok) {
        std::cerr << "floor_elements FAILED: " << message << std::endl;
        throw std::runtime_error(message);
    }
}

/// The element of the floor named name as T; fails naming it when there is none.
template <class T>
std::shared_ptr<T> named(const WoodSession& scene, const std::string& name) {

    const std::shared_ptr<T> element = scene.get_element_by_name<T>(name);
    check(element != nullptr, "an element named " + name);
    return element;
}

/// The connectors of one kind, `connector_<kind>_<place>`.
std::vector<std::shared_ptr<JointBeam>> connectors_of(const WoodSession& scene, const std::string& kind) {

    return scene.get_elements_placed<JointBeam>("connector_" + kind);
}

/// Every pin connector of the floor: the outer rib, seam beam and inner rib butt joints and the ring corners.
std::vector<std::shared_ptr<JointBeam>> pins_of(const WoodSession& scene) {

    std::vector<std::shared_ptr<JointBeam>> pins;

    for (const std::string kind : {"pins_outer_rib", "pins_seam_beam", "pins_inner_rib", "pins_ring_corner"})
        for (const std::shared_ptr<JointBeam>& connector : connectors_of(scene, kind))
            pins.push_back(connector);

    return pins;
}

/// Every connector of the floor but the pins: each kind's, then the cross laps.
std::vector<std::shared_ptr<JointBeam>> all_connectors(const WoodSession& scene) {

    std::vector<std::shared_ptr<JointBeam>> connectors;

    for (const std::string kind : {"seam_wedge", "oculus_wedge", "column_plate", "block_pins"})
        for (const std::shared_ptr<JointBeam>& connector : connectors_of(scene, kind))
            connectors.push_back(connector);

    return connectors;
}

/// The beam is closed, has the face count, and encloses the volume of the plate lofted from the same outline.
void check_beam(
    const BeamVariable& beam,
    const std::array<Polyline, 2>& outline,
    size_t faces,
    const std::string& name
) {

    const Mesh& mesh = beam.element_geometry_mesh();
    const double volume = compute_volume(mesh);
    const Plate plate(outline[1], outline[0], name);
    const double reference = compute_volume(plate.element_geometry_mesh());

    check(mesh.is_closed(), name + " closed");
    check(mesh.number_of_faces() == faces, name + " faces " + std::to_string(mesh.number_of_faces()));
    check(std::abs(volume - reference) <= 1e-9 * reference, name + " volume " + std::to_string(volume) + " vs " + std::to_string(reference));
}

/// Ribs and beams as variable beams: closed, the volume of the plate from the same loops, through a round trip and a move.
void check_beams() {

    wood_floor::Floor floor(square_guide(), "beam_variable");
    const std::span<const std::array<Polyline, 2>> outer = square_guide().outer_ribs(0);
    const std::span<const std::array<Polyline, 2>> inner = square_guide().inner_ribs(0);
    const std::span<const std::array<Polyline, 2>> beam_loops = square_guide().inner_beams(0);
    const std::span<const std::array<Polyline, 2>> oculus = square_guide().oculus();
    std::vector<std::shared_ptr<BeamVariable>> beams;

    for (size_t i = 0; i < 2; i++) {
        check_beam(
            *named<BeamVariable>(floor, fmt::format("outer_ribs_{}_0", i)),
            outer[i],
            11,
            "outer rib " + std::to_string(i)
        );
        check_beam(
            *named<BeamVariable>(floor, fmt::format("inner_ribs_{}_0", i)),
            inner[i],
            11,
            "inner rib " + std::to_string(i)
        );
        beams.insert(beams.end(), {named<BeamVariable>(floor, fmt::format("outer_ribs_{}_0", i)), named<BeamVariable>(floor, fmt::format("inner_ribs_{}_0", i))});
    }

    // the guide's three inner beams: seam 0, the oculus edge, seam 1
    const std::array<std::string, 3> beam_names = {"inner_beams_0_0", "inner_beams_1_0", "inner_beams_2_0"};

    for (size_t i = 0; i < 3; i++) {
        check_beam(
            *named<BeamVariable>(floor, beam_names[i]),
            beam_loops[i],
            6,
            beam_names[i]
        );
        beams.push_back(named<BeamVariable>(floor, beam_names[i]));
    }

    for (size_t i = 0; i < 4; i++) {
        check_beam(
            *named<BeamVariable>(floor, fmt::format("oculus_{}", i)),
            oculus[i],
            6,
            "ring beam " + std::to_string(i)
        );
        beams.push_back(named<BeamVariable>(floor, fmt::format("oculus_{}", i)));
    }

    const WoodSession back = WoodSession::pb_loads(floor.pb_dumps());

    for (const std::shared_ptr<BeamVariable>& beam : beams) {
        const std::shared_ptr<BeamVariable> loaded = back.get_element<BeamVariable>(beam->guid());
        check(loaded && loaded->sections.size() == beam->sections.size(), "round trip sections");
        check(loaded->axis.start() == beam->axis.start() && loaded->axis.end() == beam->axis.end(), "round trip axis");
        const double volume = compute_volume(beam->model_geometry_mesh());
        check(std::abs(compute_volume(loaded->model_geometry_mesh()) - volume) <= 1e-9 * volume, "round trip volume");
    }

    const Xform move = Xform::translation(100.0, -50.0, 3500.0);
    const std::shared_ptr<BeamVariable> moved = beams.front()->transformed(move);
    const double volume = compute_volume(beams.front()->element_geometry_mesh());
    check(std::abs(compute_volume(moved->element_geometry_mesh()) - volume) <= 1e-9 * volume, "transformed volume");
    check(moved->axis.start() == beams.front()->axis.start().transformed(move), "transformed axis");

    std::cout << "floor_elements: " << beams.size() << " variable beams closed, plate volumes, round trip and transform pass" << std::endl;
}

/// The thickness of a member's loops: an outer rib as thick as the guide says, an inner beam about as thick, the tilted middle block at least its plane offset.
void check_thickness() {

    const double rib = wood_floor::FloorGuide::thickness(square_guide().outer_ribs(1)[0]);
    const double beam = wood_floor::FloorGuide::thickness(square_guide().inner_beams(1)[1]);
    const double block = wood_floor::FloorGuide::thickness(square_guide().wedges(1)[1]);

    check(std::abs(rib - square_guide().size_outer_ribs) < 1e-6, "an outer rib as thick as the sizes say, " + std::to_string(rib));
    check(beam > square_guide().size_inner_beams - 1e-9 && beam < 1.5 * square_guide().size_inner_beams, "an inner beam about as thick as the sizes say, " + std::to_string(beam));
    check(block > 1.25 * square_guide().size_wedge - 1e-9, "the tilted middle block at least its plane offset thick, " + std::to_string(block));
    check(square_guide().beds(1).size() == 3 && square_guide().tsections(1).size() == 6 && square_guide().inner_ribs(1).size() == 2, "a quarter of three bed rows, six t-sections and two inner ribs");

    std::cout << "floor_elements: a rib, a beam and a block as thick as their loops say" << std::endl;
}

/// The area of the polygon a circle of that radius is faceted into.
double faceted_area(double radius, double chord_tolerance) {

    const int n = circle_segments(radius, chord_tolerance);

    return 0.5 * n * radius * radius * std::sin(2.0 * M_PI / n);
}

/// The support under the column: closed, near the exact solid, its joint cutting exactly the head plate pocket and the pins, the glued head blocks and the six inclined faces the column's own features removing their volume, and every dimension through a round trip.
void check_support() {

    WoodSession scene("support");
    const wood_floor::FloorGuide& guide = square_guide();
    const std::shared_ptr<Support> support = std::make_shared<Support>(guide.support_plane(0), "support");
    const Point foot = support->column_foot();
    const Line axis = Line::from_points(foot, Point(foot[0], foot[1], guide.bay_height));
    const std::shared_ptr<Column> column = Column::square(axis, guide.column_frame(0), guide.size_column_head);
    scene.add(support);
    scene.add(column);

    const Mesh& base = support->element_geometry_mesh();
    check(base.is_closed(), "support closed");
    check(std::abs(compute_volume(base) - EXACT_SUPPORT) <= 1e-3 * EXACT_SUPPORT, "support volume " + std::to_string(compute_volume(base)));
    check(std::abs(column->axis.start()[2] - (support->height - support->head_plate_recess)) <= 1e-9, "column foot on the support");

    const double stock = compute_volume(column->model_geometry_mesh());
    const std::shared_ptr<Joint> joint = Joint::support(*support, *column);
    scene.add(joint);
    scene.add_interaction(joint, column, joint->interaction(0));

    const double pocket = faceted_area(support->head_plate_diameter * 0.5, support->chord_tolerance) * support->head_plate_recess;
    const double pins = faceted_area(support->pin_diameter * 0.5, support->chord_tolerance) * support->pin_length * support->pin_count;
    const double removed = stock - compute_volume(column->model_geometry_mesh());
    check(std::abs(removed - pocket - pins) <= 1e-6 * removed, "support joint removes " + std::to_string(removed) + " not " + std::to_string(pocket + pins));

    // the floor's column: the shaft, two blocks glued on through add interactions, six cutter plates through subtract ones
    const wood_floor::Floor floor(guide);
    WoodSession carved("column_0");
    carved.graft(floor.get_branch("column_0"), nullptr);
    const std::shared_ptr<Column> shaft = carved.columns().front();
    const double side = guide.size_column_head;
    const double head_side = side + guide.size_column_head_chamfer;
    const double glued = side * side * (shaft->axis.length() - guide.column_head_depth) + head_side * head_side * guide.column_head_depth;
    const Mesh stock_with_head = shaft->stock_mesh();
    check(std::abs(compute_volume(stock_with_head) - glued) <= 1e-9 * glued && stock_with_head.is_closed(), fmt::format("the shaft and its two glued blocks one closed stock of {:.6f}, not {:.6f}", compute_volume(stock_with_head), glued));

    const double head = glued - removed - compute_volume(shaft->model_geometry_mesh());
    check(std::abs(head - HEAD_CUT - PLATE_POCKETS) <= 1e-6 * HEAD_CUT, fmt::format("head cuts and plate pockets remove {:.6f}", head));
    size_t adds = 0;
    size_t subtracts = 0;

    for (const std::shared_ptr<Block>& block : carved.get_elements<Block>())
        for (const std::shared_ptr<Interaction>& interaction : carved.get_interaction(block, shaft))
            if (const std::shared_ptr<InteractionFeatureSolid> feature = std::dynamic_pointer_cast<InteractionFeatureSolid>(interaction))
                adds += feature->operation == SolidOperation::add;

    for (const std::shared_ptr<Plate>& cutter : carved.plates())
        for (const std::shared_ptr<Interaction>& interaction : carved.get_interaction(cutter, shaft))
            if (const std::shared_ptr<InteractionFeatureSolid> feature = std::dynamic_pointer_cast<InteractionFeatureSolid>(interaction))
                subtracts += feature->operation == SolidOperation::subtract;

    check(adds == 2 && subtracts == 6 && shaft->solid_features.size() == 11 && carved.get_elements<Joint>().size() == 1, fmt::format("two head blocks add and six cutter plates subtract through interactions, the support and the two plate pockets three more: {} adds, {} subtracts, {} hosted", adds, subtracts, shaft->solid_features.size()));
    check(shaft->model_geometry_mesh().is_closed(), "carved column closed");

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::shared_ptr<Support> loaded = back.supports().front();
    check(loaded->plane.origin() == support->plane.origin() && loaded->height == support->height && loaded->head_plate_diameter == support->head_plate_diameter, "support round trip");
    check(loaded->pin_count == support->pin_count && loaded->pin_angle == support->pin_angle && loaded->base_plate_hole_spacing == support->base_plate_hole_spacing, "support round trip fasteners");

    std::cout << fmt::format("floor_elements: support {:.3f} mm3, joint removes {:.3f}, two glued blocks and six cutter plates removing {:.6f}, round trip pass", compute_volume(base), removed, head) << std::endl;
}

/// The lines of a list that equal a line within 1e-9 mm at both ends.
size_t matches(const std::vector<Line>& lines, const Line& line) {

    size_t count = 0;

    for (const Line& other : lines)
        count += (other.start() - line.start()).magnitude() <= 1e-9 && (other.end() - line.end()).magnitude() <= 1e-9;

    return count;
}

/// The children of a connector in the scene tree: its part and pin elements, in order.
std::vector<std::shared_ptr<Joint>> children_of(const WoodSession& scene, const JointBeam& connector) {

    std::vector<std::shared_ptr<Joint>> children;

    for (TreeNode* child : scene.tree.get_node_by_name(connector.guid())->children())
        children.push_back(scene.get_element<Joint>(child->name));

    return children;
}

/// The connector is nested: it draws nothing itself, its parts then its pins as Pin children follow it in the tree, each pin named <name>_pin_<i>, every one visible and exact.
void check_nested(
    const WoodSession& scene,
    const JointBeam& connector,
    size_t parts,
    size_t pins
) {

    const std::vector<std::shared_ptr<Joint>> children = children_of(scene, connector);
    check(connector.is_connector() && connector.model_geometry_brep().m_faces.empty() && connector.model_geometry_mesh().number_of_faces() == 0, connector.name + " draws nothing itself");
    check(children.size() == parts + pins, fmt::format("{} nests {} parts and {} pins, not {} children", connector.name, parts, pins, children.size()));

    for (size_t i = 0; i < children.size(); i++) {
        const std::shared_ptr<Joint>& child = children[i];
        check(child && child->is_visible, connector.name + " child visible");

        if (i < parts) {
            const std::string name = parts == 1 ? connector.name + "_part" : fmt::format("{}_part_{}", connector.name, i);
            check(std::dynamic_pointer_cast<ConnectorPart>(child) && child->name == name, connector.name + " part child " + child->name);
        } else {
            const double cylinder = M_PI * connector.line_radius * connector.line_radius * connector.drill_lines[i - parts].length();
            check(std::dynamic_pointer_cast<Pin>(child) && child->name == fmt::format("{}_pin_{}", connector.name, i - parts) && std::abs(child->model_geometry_brep().volume() - cylinder) < 1e-2 * cylinder, connector.name + " pin child " + child->name);
        }
    }
}

/// No pin of the connector protrudes: just inside every pin's end lies in one of its two members; and when the pins pass through, just outside every end lies in neither, the end flush with the outer face.
void check_flush(
    const JointBeam& joint,
    const WoodSession& scene,
    const std::string& name,
    bool through
) {

    const std::shared_ptr<Element> first = scene.get_element<Element>(joint.targets[0]);
    const std::vector<PlanarFace> a = planar_faces(first->element_geometry_mesh());
    const std::shared_ptr<Element> second = scene.get_element<Element>(joint.targets[1]);
    const std::vector<PlanarFace> b = planar_faces(second->element_geometry_mesh());

    for (const Line& pin : joint.drill_lines) {
        const Vector d = pin.to_vector().normalized();

        for (const Point& end : {pin.start(), pin.end()}) {
            const Vector out = end == pin.start() ? -d : d;
            check(is_inside(a, end - out * 0.5) || is_inside(b, end - out * 0.5), name + " pin does not protrude");

            if (through)
                check(!is_inside(a, end + out * 0.5) && !is_inside(b, end + out * 0.5), name + " pin ends flush with the outer face");
        }
    }
}

/// The outer ribs of every quarter, in quarter order.
std::vector<std::shared_ptr<BeamVariable>> outer_ribs(const wood_floor::Floor& floor) {

    std::vector<std::shared_ptr<BeamVariable>> ribs;

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++)
            ribs.push_back(named<BeamVariable>(floor, fmt::format("outer_ribs_{}_{}", k, q)));

    return ribs;
}

/// Whether a plan point lies inside a convex counter-clockwise polygon, within tolerance mm.
bool inside_plan(const std::vector<Point>& polygon, const Point& point, double tolerance) {

    for (size_t i = 0; i < polygon.size(); i++) {
        const Point& a = polygon[i];
        const Point& b = polygon[(i + 1) % polygon.size()];
        const Vector edge = (b - a).normalized();
        if (edge[0] * (point[1] - a[1]) - edge[1] * (point[0] - a[0]) < -tolerance)
            return false;
    }

    return true;
}

/// Short members on small and narrow bays: every rib outline of every quarter stays inside its quarter in plan, so the trim keeps the member and not the extension past it; the column level is the rib bottom, deeper than the seam depth.
void check_short_members() {

    for (const auto& [half_x, half_y] : std::vector<std::pair<double, double>>{{1200.0, 1200.0}, {3000.0, 1200.0}}) {
        const wood_floor::FloorGuide guide = rectangle_guide(half_x, half_y);

        for (size_t q = 0; q < 4; q++) {
            const std::vector<Point>& polygon = guide.quarter_polygon(q);

            for (std::span<const std::array<Polyline, 2>> family : std::initializer_list<std::span<const std::array<Polyline, 2>>>{guide.outer_ribs(q), guide.inner_ribs(q)})
                for (const std::array<Polyline, 2>& rib : family)
                    for (const Polyline* loop : {&rib[0], &rib[1]})
                        for (const Point& point : loop->get_points())
                            check(inside_plan(polygon, point, 1.0), fmt::format("{} x {}: quarter {} rib point ({:.1f}, {:.1f}) inside its quarter", 2 * half_x, 2 * half_y, q, point[0], point[1]));

            check(guide.column_levels(q)[1] < -guide.static_h(), fmt::format("{} x {}: column {} level {:.1f} below the seam depth", 2 * half_x, 2 * half_y, q, guide.column_levels(q)[1]));
        }
    }

    std::cout << "floor_elements: short ribs on 2400 x 2400 and 6000 x 2400 bays stay inside their quarters" << std::endl;
}

/// A bay that is not a rectangle, corner 2 at 84.3 degrees and seams not square to each other, builds with every connector. On a smaller skewed bay, too small for the corner pins, the bed rows still trim into quad pairs where the end planes cross their layers on different segments.
void check_skewed_bays() {

    const wood_floor::FloorGuide guide({Point(0.0, 0.0, 0.0), Point(6000.0, 0.0, 0.0), Point(6600.0, 6000.0, 0.0), Point(0.0, 6000.0, 0.0)});
    const wood_floor::Floor scene(guide, "skewed");
    const size_t connectors = all_connectors(scene).size();
    check(connectors == 40, "the skewed bay's 40 connectors, not " + std::to_string(connectors));

    const wood_floor::FloorGuide small({Point(0.0, 0.0, 0.0), Point(4000.0, 0.0, 0.0), Point(4400.0, 3000.0, 0.0), Point(0.0, 3000.0, 0.0)});
    size_t beds = 0;

    for (size_t q = 0; q < 4; q++)
        for (const std::vector<std::array<Polyline, 2>>& row : small.beds(q))
            for (const std::array<Polyline, 2>& bed : row) {
                check(bed[0].point_count() == 5 && bed[1].point_count() == 5, fmt::format("a bed of quarter {} on the 4000 x 3000 skewed bay a quad pair", q));
                beds++;
            }

    std::cout << "floor_elements: a skewed bay with its " << connectors << " connectors; the bed rows of a 4000 x 3000 skewed bay trimmed alike, " << beds << " quad pairs" << std::endl;
}

/// The connectors and pins of a Floor: the columns' plates and cross laps, every connector named apart; a 6000 x 3400 bay with all its pins; on a skewed bay every seam pin starts on its seam plane.
void check_connector_calls() {

    const wood_floor::Floor floor(square_guide());
    check(connectors_of(floor, "column_plate").size() == 8 && floor.get_elements_placed<JointPlate>("connector_cross_lap").size() == 4, "the columns' eight plate joints and four cross laps");

    const std::vector<std::shared_ptr<JointBeam>> connectors = all_connectors(floor);
    std::set<std::string> names;
    for (const std::shared_ptr<JointBeam>& connector : connectors)
        names.insert(connector->name);

    check(names.size() == connectors.size() && names.count("connector_oculus_wedge_3"), fmt::format("{} connectors named apart, up to connector_oculus_wedge_3", names.size()));

    const wood_floor::Floor narrow_floor(rectangle_guide(3000.0, 1700.0), "narrow");
    const std::vector<std::shared_ptr<JointBeam>> narrow_pins = pins_of(narrow_floor);
    size_t lines = 0;
    for (const std::shared_ptr<JointBeam>& pins : narrow_pins)
        lines += pins->drill_lines.size();

    check(narrow_pins.size() == 28 && lines == 56, fmt::format("the floor on a 6000 x 3400 bay with {} pin sets, {} pins, each inside its contact", narrow_pins.size(), lines));

    const wood_floor::FloorGuide skewed({Point(-3000.0, -3000.0, 0.0), Point(3000.0, -3000.0, 0.0), Point(2000.0, 3000.0, 0.0), Point(-2000.0, 3000.0, 0.0)});
    double off = 0.0;

    const wood_floor::Floor skewed_floor(skewed);

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++) {
            const wood_floor::ConstructionPlanes& planes = skewed.construction_planes(q);
            const Xform lift = Xform::translation(0.0, 0.0, skewed.bay_height);
            const Plane far = planes.inner_beams[k == 0 ? 0 : 2][1].transformed(lift);

            const std::shared_ptr<JointBeam> set = skewed_floor.get_element_by_name<JointBeam>(fmt::format("connector_pins_outer_rib_{}_{}", q, k));

            for (const Line& pin : set->drill_lines)
                off = std::max(off, std::abs(std::abs(far.signed_distance(pin.start())) - skewed.size_inner_beams));
        }

    check(off <= 1e-6, fmt::format("every seam pin of the skewed bay starts on the seam beam's outer face, {:.3e} mm off", off));

    std::cout << "floor_elements: column plates and cross laps, connectors named apart, a 6000 x 3400 bay with all its pins, skewed seam pins on their seam planes" << std::endl;
}

/// Every contact interaction of a session: its name and the polygon two members share, read from the graph's edges.
std::vector<std::pair<std::string, Polyline>> contact_interactions(const WoodSession& scene) {

    std::vector<std::pair<std::string, Polyline>> contacts;

    for (const auto& [u, w] : scene.graph.get_edges()) {
        const std::shared_ptr<Element> a = scene.get_element<Element>(u);
        const std::shared_ptr<Element> b = scene.get_element<Element>(w);

        if (!a || !b)
            continue;

        for (const std::shared_ptr<Interaction>& interaction : scene.get_interaction(a, b))
            if (const std::shared_ptr<InteractionContactFace> contact = std::dynamic_pointer_cast<InteractionContactFace>(interaction))
                contacts.push_back({contact->name, contact->polygon});
    }

    return contacts;
}

/// The contacts of the default floor: one face interaction between every two members the design joins, 4 seam wedges, 4 oculus wedges, 8 column plates and 24 pin sets, each with its own area.
void check_contacts() {

    const wood_floor::Floor scene(square_guide(), "contacts");
    std::map<std::string, size_t> counts;
    double smallest = 1e300;

    for (const auto& [name, polygon] : contact_interactions(scene)) {
        counts[name.substr(0, name.find_first_of("0123456789") - 1)]++;
        smallest = std::min(smallest, polygon.area());
    }

    check(counts["seam_wedge"] == 4 && counts["oculus_wedge"] == 4 && counts["column_plate"] == 8 && counts["block_pins"] == 24, fmt::format("4 seam wedges, 4 oculus wedges, 8 column plates and 24 pin sets, not {} / {} / {} / {}", counts["seam_wedge"], counts["oculus_wedge"], counts["column_plate"], counts["block_pins"]));
    check(smallest > 100.0, fmt::format("every contact has its own area, the smallest {:.3f} mm2", smallest));

    std::cout << fmt::format("floor_elements: 40 contact interactions on the square, the smallest {:.3f} mm2", smallest) << std::endl;
}

/// The wedges between the inner beams and the oculus: eight visible connectors with their parts and cutters, every one cut flush with the floor top, their pins flush with the beams, exact cylinders in their BReps and exact bores through the wedge parts, and the carved beams, through a round trip.
void check_wedges() {

    wood_floor::Floor scene(square_guide(), "wedges");
    std::vector<std::shared_ptr<JointBeam>> wedges = connectors_of(scene, "seam_wedge");
    const std::vector<std::shared_ptr<JointBeam>> oculus_wedges = connectors_of(scene, "oculus_wedge");
    wedges.insert(wedges.end(), oculus_wedges.begin(), oculus_wedges.end());
    check(wedges.size() == 8, "eight wedges, not " + std::to_string(wedges.size()));

    for (const std::shared_ptr<JointBeam>& wedge : wedges) {
        check_flush(
            *wedge,
            scene,
            wedge->name,
            true
        );
        check(count_bores(wedge->part_brep(0)) == wedge->drill_lines.size(), "every wedge pin an exact bore through the wedge's part");
        check_nested(
            scene,
            *wedge,
            1,
            wedge->drill_lines.size()
        );
        check(count_bores(children_of(scene, *wedge).front()->model_geometry_brep()) == wedge->drill_lines.size(), "the wedge child carries the bores");
        double top = -1e300;

        for (const Polyline& loop : wedge->parts[0])
            for (const Point& point : loop.get_points())
                top = std::max(top, point[2]);

        check(std::abs(top - square_guide().bay_height) <= 1e-6, fmt::format("{} cut flush with the floor top, {:.3e} above it", wedge->name, top - square_guide().bay_height));
    }

    std::map<std::string, double> volumes;

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables())
        volumes[beam->guid()] = compute_volume(beam->model_geometry_mesh());

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<JointBeam>& joint : back.get_elements<JointBeam>()) {
        if (std::dynamic_pointer_cast<ConnectorPart>(joint) || !(joint->name.starts_with("connector_seam_wedge") || joint->name.starts_with("connector_oculus_wedge")))
            continue;

        check(joint->parts.size() == 1 && joint->cutters.size() == 2 && !joint->drill_lines.empty(), "wedge round trip");
        check_nested(
            back,
            *joint,
            1,
            joint->drill_lines.size()
        );
        loaded++;
    }

    check(loaded == wedges.size(), "wedge round trip count");

    for (const std::shared_ptr<BeamVariable>& beam : back.beam_variables())
        check(std::abs(compute_volume(beam->model_geometry_mesh()) - volumes.at(beam->guid())) <= 1e-9 * volumes.at(beam->guid()), "carved beam round trip " + beam->name);

    for (const std::shared_ptr<BeamVariable>& beam : scene.beam_variables()) {
        if (beam->solid_features.empty())
            continue;

        const BRep& brep = beam->model_geometry_brep();
        const double volume = compute_volume(beam->model_geometry_mesh());
        check(count_bores(brep) > 0 && brep.is_solid() && std::abs(brep.volume() - volume) <= 1e-3 * volume, "exact pin bores in " + beam->name);
    }

    std::cout << "floor_elements: " << wedges.size() << " wedges, visible, pins flush and exact, joints and carved beams through a round trip, every pin bore exact in the BReps, pass" << std::endl;
}

/// The farthest a loop's points lie from a plane on its negative side, and the nearest any lies to it.
std::array<double, 2> plane_reach(const Polyline& loop, const Plane& plane) {

    std::array<double, 2> reach = {0.0, 1e300};

    for (const Point& point : loop.get_points()) {
        const double d = (point - plane.origin()).dot(plane.z_axis());
        reach = {std::max(reach[0], -d), std::min(reach[1], std::abs(d))};
    }

    return reach;
}

/// The lowest corner of a loop on a plane.
double lowest_on(const Polyline& loop, const Plane& plane) {

    double level = 0.0;

    for (const Point& point : loop.get_points())
        if (std::abs((point - plane.origin()).dot(plane.z_axis())) <= 1e-6)
            level = std::min(level, point[2]);

    return level;
}

/// The seam beams run through the rib band: every seam beam reaching the bay's outer face, every outer rib ending on its beam's far face, no rib end below the beams' soffit, the seam wedges flush with the outer face between the beams, horizontal pins from the beam's seam face along the rib 20 mm below its top and above its bottom.
void check_seam_beams() {

    const wood_floor::FloorGuide guide = rectangle_guide(3000.0, 3000.0);

    for (size_t q = 0; q < 4; q++) {
        const wood_floor::ConstructionPlanes& cp = guide.construction_planes(q);
        const std::span<const std::array<Polyline, 2>> beams = guide.inner_beams(q);
        const std::span<const std::array<Polyline, 2>> ribs = guide.outer_ribs(q);

        for (size_t k = 0; k < 2; k++) {
            const std::array<double, 2> beam = plane_reach(beams[k == 0 ? 0 : 2][1], cp.outer_ribs[k][0]);
            const Plane end = guide.rib_seam_ends(q)[k];
            const std::array<double, 2> rib = plane_reach(ribs[k][1], end);
            check(beam[0] <= 1e-9 && beam[1] <= 1e-9, fmt::format("quarter {} seam beam {} reaches the outer face, {:.3e} off", q, k, beam[1]));
            check(rib[1] <= 1e-9 && std::abs((end.origin() - cp.inner_beams[k == 0 ? 0 : 2][0].origin()).dot(end.z_axis())) > 1.0, fmt::format("quarter {} outer rib {} ends on its beam's far face, {:.3e} off", q, k, rib[1]));
            check(lowest_on(ribs[k][1], end) >= guide.soffit - 1e-9 && lowest_on(guide.inner_ribs(q)[k][1], cp.inner_beams[1][1]) >= guide.soffit - 1e-9, fmt::format("quarter {} rib ends {} within the beams' soffit {:.3f}", q, k, guide.soffit));
            check(std::abs(lowest_on(beams[k == 0 ? 0 : 2][1], cp.inner_beams[k == 0 ? 0 : 2][0]) - guide.soffit) <= 1e-9, fmt::format("quarter {} seam beam {} down to the soffit", q, k));
        }
    }

    wood_floor::Floor scene(guide, "seam_beams");
    const std::vector<std::shared_ptr<JointBeam>> wedges = connectors_of(scene, "seam_wedge");

    for (size_t i = 0; i < wedges.size(); i++) {
        const wood_floor::ConstructionPlanes& planes = guide.construction_planes(i);
        const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
        const Plane face = planes.outer_ribs[0][0].transformed(lift);
        const std::array<double, 2> near = plane_reach(wedges[i]->parts[0][0], face);
        const std::array<double, 2> far = plane_reach(wedges[i]->parts[0][1], face);
        const double off = std::min(std::max(near[0], near[1]), std::max(far[0], far[1]));
        check(wedges[i]->targets.size() == 2 && off <= 1e-9, fmt::format("{} between its two seam beams, flush with the outer face, {:.3e} off", wedges[i]->name, off));
    }


    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++) {
            const wood_floor::ConstructionPlanes& planes = guide.construction_planes(q);
            const std::array<Polyline, 2> rib = guide.outer_ribs(q)[k];
            const Plane end = guide.rib_seam_ends(q)[k];
            const Xform lift = Xform::translation(0.0, 0.0, guide.bay_height);
            const Plane far = planes.inner_beams[k == 0 ? 0 : 2][1].transformed(lift);
            const double bottom = guide.bay_height + std::min(lowest_on(rib[0], end), lowest_on(rib[1], end));
            const std::vector<Line> pins = scene.get_element_by_name<JointBeam>(fmt::format("connector_pins_outer_rib_{}_{}", q, k))->drill_lines;
            const std::string label = fmt::format("quarter {} outer rib {}", q, k);
            const double high = std::max(pins[0].start()[2], pins[1].start()[2]);
            const double low = std::min(pins[0].start()[2], pins[1].start()[2]);
            check(pins.size() == 2 && std::abs(high - (guide.bay_height - 20.0)) <= 1e-3 && std::abs(low - (bottom + 20.0)) <= 1e-3, label + " pins 20 mm below the rib top and above its bottom");

            for (const Line& pin : pins)
            for (const Line& pin : pins)
                check(std::abs(pin.to_direction()[2]) <= 1e-9 && std::abs(std::abs(far.signed_distance(pin.start())) - guide.size_inner_beams) <= 1e-6, label + " pins horizontal from the beam's seam face");
        }

    std::cout << fmt::format("floor_elements: seam beams through the rib band to the outer face, ribs ending on them within the beams' soffit {:.3f}, wedges flush with the outer face, horizontal pins from the seam face", guide.soffit) << std::endl;
}

/// The drill features of every member: one per stretch of a drill line the joint gave the member inside its stock, its glued blocks included, centred, headed and support pins alike, each the two circles of its radius where the hole enters and leaves, and every one through a round trip.
void check_drill_features() {

    wood_floor::Floor scene(square_guide(), "drills");
    std::map<std::string, size_t> expected;
    std::map<std::string, double> radius;
    std::map<std::string, std::string> names;

    for (const std::shared_ptr<Joint>& joint : scene.get_elements<Joint>()) {
        if (std::dynamic_pointer_cast<ConnectorPart>(joint) || std::dynamic_pointer_cast<Pin>(joint))
            continue;

        radius[joint->guid()] = joint->line_radius;
        names[joint->guid()] = joint->name;

        for (const std::string& guid : joint->targets) {
            const std::shared_ptr<Element> target = scene.get_element<Element>(guid);
            const std::shared_ptr<WoodElement> member = std::dynamic_pointer_cast<WoodElement>(target);
            const Mesh solid = member ? member->stock_mesh() : target->element_geometry_mesh();

            // the lines the joint gave this target: a connector's cut names them, any other joint drills all its axes
            std::vector<Line> lines = joint->drill_axes();

            for (const std::shared_ptr<Interaction>& interaction : scene.get_interaction(joint, target))
                if (const std::shared_ptr<InteractionFeatureSolid> cut = std::dynamic_pointer_cast<InteractionFeatureSolid>(interaction); cut && std::dynamic_pointer_cast<JointBeam>(joint))
                    lines = cut->drills;

            for (const Line& line : lines)
                for (const std::array<double, 2>& inside : inside_stretches(solid, line))
                    if (std::min(inside[1], line.length()) - std::max(inside[0], 0.0) >= 1e-6)
                        expected[guid]++;
        }
    }

    size_t total = 0;
    std::map<std::string, std::set<std::string>> sizes;

    for (const std::shared_ptr<Element>& element : *scene.objects.elements) {
        if (std::dynamic_pointer_cast<Joint>(element))
            continue;

        size_t drills = 0;

        for (const ElementFeature& feature : element->features()) {
            if (feature.feature_type != "drill")
                continue;

            drills++;
            check(feature.outlines.size() == 2 && feature.outlines[0].is_closed() && feature.outlines[1].is_closed(), element->name + " drill feature: two closed circles");
            std::vector<Point> circle = feature.outlines[0].get_points();
            circle.pop_back();
            const Point centre = Point::centroid(circle);
            const std::string joint = feature.guid().substr(0, feature.guid().find('/'));
            const double r = (feature.outlines[0].get_point(0) - centre).magnitude();
            check(radius.count(joint) && std::abs(r - radius.at(joint)) <= 1e-6, fmt::format("{} drill of {} radius {:.3f}", element->name, names[joint], r));
            sizes[names[joint].substr(0, names[joint].find_last_of('_'))].insert(fmt::format("d{:g}", std::round(2.0 * r * 1000.0) / 1000.0));
        }

        check(drills == expected[element->guid()], fmt::format("{}: {} drill features for {} holes", element->name, drills, expected[element->guid()]));
        total += drills;
    }

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<Element>& element : *back.objects.elements)
        for (const ElementFeature& feature : element->features())
            loaded += feature.feature_type == "drill";

    check(total > 0 && loaded == total, fmt::format("{} drill features, {} after a round trip", total, loaded));
    std::string radii;

    for (const std::pair<const std::string, std::set<std::string>>& kind : sizes)
        for (const std::string& size : kind.second)
            radii += fmt::format(" {} {},", kind.first, size);

    std::cout << fmt::format("floor_elements: every member carries a drill feature per hole its pins make, {} in all, the same after a round trip;{}", total, radii) << std::endl;
}

/// The drill features, joints and interactions of a session: the number of each.
std::array<size_t, 3> scene_counts(const WoodSession& scene) {

    std::array<size_t, 3> counts = {0, scene.get_elements<Joint>().size(), 0};

    for (const std::shared_ptr<Element>& element : *scene.objects.elements)
        for (const ElementFeature& feature : element->features())
            counts[0] += feature.feature_type == "drill";

    for (const std::pair<const std::string, std::vector<std::shared_ptr<Interaction>>>& edge : scene.interactions)
        counts[2] += edge.second.size();

    return counts;
}

/// Two floors as two templates in one scene: each a session of its own, grafted under its level, every element, joint, interaction and drill of both kept, the same floor refused a second time, and the scene through a round trip.
void check_floors_in_scene() {

    wood_floor::Floor square(rectangle_guide(3000.0, 3000.0), "square");
    wood_floor::Floor rectangle(rectangle_guide(3000.0, 2400.0), "rectangle");

    WoodSession scene("building");
    scene.graft(square, scene.add_group("level_1"));
    scene.graft(rectangle, scene.add_group("level_2"));
    const std::array<size_t, 3> a = scene_counts(square);
    const std::array<size_t, 3> b = scene_counts(rectangle);
    const std::array<size_t, 3> both = scene_counts(scene);

    check(scene.objects.elements->size() == square.objects.elements->size() + rectangle.objects.elements->size(), "every element of both floors in the scene");
    check(both[0] == a[0] + b[0] && both[1] == a[1] + b[1] && both[2] == a[2] + b[2], fmt::format("drills {} / {}, joints {} / {}, interactions {} / {} in the scene", both[0], a[0] + b[0], both[1], a[1] + b[1], both[2], a[2] + b[2]));
    check(scene.tree.root()->children().size() == 2 && scene.tree.root()->children()[0]->children()[0]->name == "quarter_0" && scene.tree.root()->children()[0]->children().size() == 5, "each floor under its level, its four quarters and the oculus");

    bool refused = false;

    try {
        scene.merge(square);
    } catch (const std::invalid_argument&) {
        refused = true;
    }

    check(refused, "the same floor refused a second time");
    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    check(back.objects.elements->size() == scene.objects.elements->size() && scene_counts(back) == both, "the scene of two floors through a round trip");
    std::cout << fmt::format("floor_elements: two floors grafted into one scene, {} elements, {} joints, {} interactions, {} drills, the same after a round trip", scene.objects.elements->size(), both[1], both[2], both[0]) << std::endl;
}

/// The elements, drill features and connectors under a node of a session, the node's own descendants alone.
std::array<size_t, 3> subtree_counts(const Session& session, const TreeNode& node) {

    std::array<size_t, 3> counts = {0, 0, 0};

    for (TreeNode* below : node.traverse()) {
        const std::shared_ptr<const Element> element = session.lookup.count(below->name) ? session.get_object<Element>(below->name) : nullptr;

        if (!element)
            continue;

        counts[0]++;
        counts[2] += std::dynamic_pointer_cast<const JointBeam>(element) != nullptr;

        for (const ElementFeature& feature : element->features())
            counts[1] += feature.feature_type == "drill";
    }

    return counts;
}

/// One quarter's branch taken out of the guide and out of the whole floor with WoodSession::get_branch: the guide's quarter its 33 drawn curves, the floor's quarter a WoodSession with every element, drill and connector under quarter_0, its connectors read back through get_elements<JointBeam>(), no pin of another quarter drilling its members now the oculus has none, every member reading its pre-drill lines, both sessions unchanged.
void check_extract_quarter() {

    const wood_floor::FloorGuide& guide = square_guide();
    const WoodSession drawn = guide.get_branch("quarter_0");
    check(drawn.name == "quarter_0" && drawn.lookup.size() == 63 && drawn.tree.root()->children().size() == 6, fmt::format("the guide's quarter 0: {} curves, planes and points in {} groups", drawn.lookup.size(), drawn.tree.root()->children().size()));

    wood_floor::Floor floor(guide);
    const size_t before = floor.lookup.size();
    std::set<std::string> elements;
    std::array<size_t, 2> members = {0, 0};

    for (const std::shared_ptr<Element>& element : *floor.objects.elements)
        elements.insert(element->name);

    for (TreeNode* family : drawn.tree.root()->children())
        for (TreeNode* member : family->children())
            if (!member->children().empty()) {
                members[0]++;
                members[1] += elements.count(member->name);
            }

    check(members[0] == 16 && members[1] == 16, fmt::format("every member the guide draws named as its element in the floor, {} of {}", members[1], members[0]));
    const WoodSession part = floor.get_branch("quarter_0");
    const std::array<size_t, 3> expected = subtree_counts(floor, *floor.tree.get_node_by_name("quarter_0"));
    const std::array<size_t, 3> found = subtree_counts(part, *part.tree.root());

    const size_t borrowed = found[2] - expected[2];
    check(found[0] - expected[0] == borrowed && found[1] == expected[1] && expected[1] > 0 && borrowed == 0, fmt::format("the floor's quarter 0: {} of {} elements, {} of {} drills, {} of {} connectors, {} pre-drill connectors from the next quarter", found[0], expected[0], found[1], expected[1], found[2], expected[2], borrowed));
    check(part.get_elements<JointBeam>().size() == found[2] && found[2] > 0 && part.settings.distance == floor.settings.distance, "the branch a WoodSession: its connectors as JointBeam, the floor's settings");
    size_t lines = 0;

    for (const std::shared_ptr<Element>& element : part.world_elements()) {
        const std::vector<Line> branch = part.pre_drill_lines(element->guid());
        const std::vector<Line> whole = floor.pre_drill_lines(element->guid());
        check(branch.size() == whole.size(), fmt::format("{} reads {} pre-drill lines in the branch, {} in the floor", element->name, branch.size(), whole.size()));

        for (const Line& line : branch)
            check(matches(whole, line) > 0, element->name + " reads its pre-drill lines in place in the branch");

        lines += branch.size();
    }

    check(floor.lookup.size() == before && guide.lookup.size() == 4 * 63, "the floor and the guide unchanged by the extraction");
    std::cout << fmt::format("floor_elements: quarter 0's branch from the guide, {} curves, and from the floor, a WoodSession of {} elements with {} drills and {} connectors, {} of them pre-drill connectors of the next quarter, every member's {} pre-drill lines as in the floor, both sessions unchanged", drawn.lookup.size(), found[0], found[1], found[2], borrowed, lines) << std::endl;
}

/// The centred_pins factory on two plates face to face: four Ø8 pins 30 long at the corners of the 600 x 200 contact inset by 50, 15 deep into both 60 plates, cut as exact bores into both, through a round trip.
void check_centred_pins() {

    WoodSession scene("pins");
    const std::shared_ptr<Plate> lower = Plate::from_rectangle(
        Point(0.0, 0.0, 0.0),
        Vector(1.0, 0.0, 0.0),
        Vector(0.0, 1.0, 0.0),
        600.0,
        200.0,
        60.0,
        "lower"
    );
    const std::shared_ptr<Plate> upper = Plate::from_rectangle(
        Point(0.0, 0.0, 60.0),
        Vector(1.0, 0.0, 0.0),
        Vector(0.0, 1.0, 0.0),
        600.0,
        200.0,
        60.0,
        "upper"
    );
    scene.add(lower);
    scene.add(upper);

    const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
    check(contact != nullptr, "the plates touch face to face");

    const std::shared_ptr<JointBeam> joint = JointBeam::centred_pins(*lower, *upper, *contact);
    check(joint && joint->drill_lines.size() == 4 && joint->is_visible, "four pins at the corners of the contact");
    std::set<std::pair<int, int>> corners;

    for (const Line& pin : joint->drill_lines) {
        check(std::abs(pin.length() - 30.0) < 1e-9, "a pin 15 deep into each plate, " + std::to_string(pin.length()));
        check(std::abs(pin.center()[2] - 60.0) < 1e-9, "a pin centred on the contact");
        corners.insert({static_cast<int>(std::lround(pin.center()[0])), static_cast<int>(std::lround(pin.center()[1]))});
    }

    check(corners == std::set<std::pair<int, int>>{{50, 50}, {550, 50}, {550, 150}, {50, 150}}, "the pins 50 in from the contact's corners");

    scene.add(joint);
    scene.add_interaction(joint, lower, joint->interaction(0));
    scene.add_interaction(joint, upper, joint->interaction(1));
    check_nested(
        scene,
        *joint,
        0,
        4
    );
    const double bore = faceted_area(joint->line_radius, joint->chord_tolerance) * 15.0 * 4.0;

    for (const std::shared_ptr<Plate>& plate : {lower, upper}) {
        const double volume = compute_volume(plate->model_geometry_mesh());
        check(std::abs(600.0 * 200.0 * 60.0 - bore - volume) <= 1e-6 * volume, "four blind holes out of " + plate->name);
        check(count_bores(plate->model_geometry_brep()) == 4 && plate->model_geometry_brep().is_solid(), "four exact bores in " + plate->name);
    }

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    const std::shared_ptr<JointBeam> loaded = back.get_elements<JointBeam>().front();
    check(loaded->drill_lines.size() == 4 && loaded->cutters.size() == 2 && loaded->drill_overshoot == joint->drill_overshoot, "pins round trip");
    check_nested(
        back,
        *loaded,
        0,
        4
    );

    std::cout << "floor_elements: four pins at the corners of a plate contact, 30 long, exact bores, nested as four pin children and a round trip pass" << std::endl;
}

/// Every inner rib is exact, bored once per pin of the sets that join it.
void check_inner_rib_bores(const WoodSession& scene, const std::vector<std::shared_ptr<JointBeam>>& sets) {

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++) {
            const std::shared_ptr<BeamVariable> rib = named<BeamVariable>(scene, fmt::format("inner_ribs_{}_{}", k, q));
            size_t crossing = 0;

            for (const std::shared_ptr<JointBeam>& set : sets)
                if (std::find(set->targets.begin(), set->targets.end(), rib->guid()) != set->targets.end())
                    crossing += set->drill_lines.size();

            const BRep& brep = rib->model_geometry_brep();
            check(brep.is_solid() && count_bores(brep) == crossing, fmt::format("an inner rib bored once per pin, one exact cylinder each: {} pins, {} bores in {}", crossing, count_bores(brep), rib->name));
        }
}

/// Every rib and column block with a cut is an exact solid with at least one bore: seven per quarter.
void check_drilled_members(const WoodSession& scene) {

    size_t drilled = 0;

    for (const std::shared_ptr<Element>& element : scene.world_elements()) {
        const std::vector<InteractionFeatureSolid>* cuts = solid_features_of(*element);
        const bool member = element->name.starts_with("outer_ribs") || element->name.starts_with("inner_ribs") || element->name.starts_with("wedges");

        if (!member || !cuts || cuts->empty())
            continue;

        drilled++;
        check(element->model_geometry_brep().is_solid() && count_bores(element->model_geometry_brep()) > 0, fmt::format("exact pin bores in {}: solid {}, {} bores", element->name, element->model_geometry_brep().is_solid(), count_bores(element->model_geometry_brep())));
    }

    check(drilled == 28, "seven drilled members per quarter, the ribs and the blocks, not " + std::to_string(drilled));
}

/// The assembly pins of the quarters: a pin set on every rib-to-wedge-block contact, six per quarter, never across quarters, four 30 mm pins exactly at the corners of each contact inset 50, the contacts found on the uncut members; every pin half in each member, every drilled member exact; through a round trip.
void check_quarter_pins() {

    wood_floor::Floor scene(square_guide(), "quarter_pins");
    const std::vector<std::shared_ptr<JointBeam>> sets = connectors_of(scene, "block_pins");
    size_t pins = 0;

    for (const std::shared_ptr<JointBeam>& set : sets) {
        check_nested(
            scene,
            *set,
            0,
            set->drill_lines.size()
        );
        const std::shared_ptr<Element> rib = scene.get_element<Element>(set->targets[0]);
        const std::shared_ptr<Element> block = scene.get_element<Element>(set->targets[1]);
        check(rib->name.substr(rib->name.find_last_of('_')) == block->name.substr(block->name.find_last_of('_')), "a pin set stays within one quarter, not " + rib->name + " to " + block->name);
        check(rib->name.find("ribs_") != std::string::npos && block->name.starts_with("wedges_"), "a pin set joins a rib to a wedge block, not " + rib->name + " to " + block->name);
        check(set->drill_lines.size() == 4, "four pins per contact, not " + std::to_string(set->drill_lines.size()));

        for (const Line& pin : set->drill_lines) {
            check(std::abs(pin.length() - 30.0) < 1e-9, "a pin 30 long");
            const Vector half = pin.to_vector().normalized() * 0.5;
            check(is_inside(planar_faces(rib->element_geometry_mesh()), pin.center() - half) && is_inside(planar_faces(block->element_geometry_mesh()), pin.center() + half), "a pin crosses the contact at its middle, half in each member");
            pins++;
        }
    }

    check(sets.size() == 24, "six rib-to-block pin sets per quarter, not " + std::to_string(sets.size()));
    check_inner_rib_bores(scene, sets);
    check_drilled_members(scene);

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<JointBeam>& set : back.get_elements<JointBeam>())
        if (!std::dynamic_pointer_cast<ConnectorPart>(set) && set->name.starts_with("connector_block_pins"))
            loaded += set->drill_lines.size();

    check(loaded == pins, "quarter pins round trip");

    std::cout << fmt::format("floor_elements: {} pin sets of {} pins on the wedge blocks of the quarters, every pin half in each member, every drilled member exact, round trip pass", sets.size(), pins) << std::endl;
}

/// The two plates of every column half-lapped by a cr_c_ip cross lap merged into their outlines: each plate closed, the two touching without overlap, the same volume taken out of both.
void check_cross_laps(const WoodSession& scene, const std::vector<std::shared_ptr<JointPlate>>& laps) {

    for (const std::shared_ptr<JointPlate>& lap : laps) {
        const std::shared_ptr<Plate> a = scene.get_element<Plate>(lap->targets[0]);
        const std::shared_ptr<Plate> b = scene.get_element<Plate>(lap->targets[1]);
        const Mesh& lapped_a = a->model_geometry_mesh();
        const Mesh& lapped_b = b->model_geometry_mesh();
        check(lapped_a.is_closed() && lapped_b.is_closed(), lap->name + " both plates closed");
        const Mesh overlap = solid_boolean(
            lapped_a,
            lapped_b,
            SolidOperation::intersect,
            1e-7
        );
        check(!overlap.number_of_faces() || std::abs(compute_volume(overlap)) < 1e-2, lap->name + " the lapped plates do not overlap");
        const double taken_a = compute_volume(a->element_geometry_mesh()) - compute_volume(lapped_a);
        const double taken_b = compute_volume(b->element_geometry_mesh()) - compute_volume(lapped_b);
        check(taken_a > 0.0 && std::abs(taken_a - taken_b) < 1e-6 * taken_a, fmt::format("{} the same {:.3f} out of both plates", lap->name, taken_a));
    }
}

/// The rectangle plates between the columns and the outer ribs of the square: eight, half-lapped by four cross laps, every carved outer rib at its pinned volume, the column still exact, through a round trip.
void check_rectangle_plates() {

    wood_floor::Floor scene(square_guide(), "rectangle_plates");
    const std::vector<std::shared_ptr<BeamVariable>> ribs = outer_ribs(scene);
    const std::vector<std::shared_ptr<JointBeam>> plates = connectors_of(scene, "column_plate");
    const std::vector<std::shared_ptr<JointPlate>> laps = scene.get_elements_placed<JointPlate>("connector_cross_lap");

    for (const std::shared_ptr<BeamVariable>& rib : ribs)
        check(std::abs(compute_volume(rib->model_geometry_mesh()) - CARVED_OUTER_RIB) <= 1e-9 * CARVED_OUTER_RIB, fmt::format("carved outer rib {} {:.6f}", rib->name, compute_volume(rib->model_geometry_mesh())));

    check(plates.size() == 8 && laps.size() == 4 && laps[0]->name == "connector_cross_lap_0" && plates[7]->name == "connector_column_plate_3_1", "eight plate joints and four cross laps");
    check_cross_laps(scene, laps);

    for (const std::shared_ptr<Column>& column : scene.columns())
        check(column->model_geometry_brep().is_solid() && count_bores(column->model_geometry_brep()) == 11, "the column exact with its eight pin and three pin bores");

    // a column carries only the pins that pass through it: two per plate, never the plate's rib pins
    for (const std::shared_ptr<Column>& column : scene.columns()) {
        size_t plate_drills = 0;

        for (const InteractionFeatureSolid& feature : column->solid_features)
            if (feature.drills.size() != 3)
                plate_drills += feature.drills.size();

        check(plate_drills == 4, fmt::format("{} assigned its two pins of each plate, not {}", column->name, plate_drills));
    }

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++) {
            const std::string name = fmt::format("column_plate_{}_{}", q, k);
            const double before = compute_volume(named<Plate>(scene, name)->model_geometry_mesh());
            const double after = compute_volume(named<Plate>(back, name)->model_geometry_mesh());
            check(std::abs(before - after) < 1e-6 * before, name + " lapped and bored the same after a round trip");
        }

    const size_t parts = scene.get_elements<ConnectorPart>().size();
    const size_t pins = scene.get_elements<Pin>().size();
    check(pins >= 32 && back.get_elements<ConnectorPart>().size() == parts && back.get_elements<Pin>().size() == pins, "the plate parts and the pins, among every connector's, round trip as children");

    std::cout << "floor_elements: " << plates.size() << " column plates let in and half-lapped by " << laps.size() << " cross laps, drilled and exact, carved outer ribs at their pinned volume, round trip pass" << std::endl;
}

/// Whether two planes are one plane: unit normals parallel or opposite as flip says, the same offset, within 1e-9.
bool same_plane(const Plane& a, const Plane& b, bool opposite) {

    const Vector normal = opposite ? -b.z_axis() : b.z_axis();
    const double offset_a = a.z_axis().dot(Vector(a.origin()[0], a.origin()[1], a.origin()[2]));
    const double offset_b = normal.dot(Vector(b.origin()[0], b.origin()[1], b.origin()[2]));

    return (a.z_axis() - normal).magnitude() <= 1e-9 && std::abs(offset_a - offset_b) <= 1e-9;
}

/// Two neighbouring quarters read their shared seam and bay edge as one plane each, and on the square every quarter equals quarter 0 turned by its quarter turns within 1e-6.
void check_shared_entities() {

    const wood_floor::FloorGuide& guide = square_guide();

    for (size_t q = 0; q < 4; q++) {
        const wood_floor::ConstructionPlanes& mine = guide.construction_planes(q);
        const wood_floor::ConstructionPlanes& next = guide.construction_planes((q + 1) % 4);
        check(same_plane(mine.inner_beams[0][0], next.inner_beams[2][0], true), fmt::format("seam {} is the beam-0 plane of quarter {} and the beam-2 plane of quarter {}", q, q, (q + 1) % 4));
        check(same_plane(mine.outer_ribs[0][0], next.outer_ribs[1][0], false) && same_plane(mine.outer_ribs[0][1], next.outer_ribs[1][1], false), fmt::format("bay edge {} is the outer rib band of quarters {} and {}", q, q, (q + 1) % 4));


        const Xform turn = Xform::rotation_z(static_cast<double>(q) * 90.0, true);
        const std::span<const std::array<Polyline, 2>> turned = guide.outer_ribs(0);
        const std::span<const std::array<Polyline, 2>> built = guide.outer_ribs(q);

        for (size_t i = 0; i < 2; i++) {
            const std::vector<Point> a = turned[i][0].transformed(turn).get_points();
            const std::vector<Point> b = built[i][0].get_points();
            check(a.size() == b.size(), "the in-place rib has the turned rib's vertex count");

            for (size_t j = 0; j < a.size(); j++)
                check((a[j] - b[j]).magnitude() <= 1e-6, fmt::format("quarter {} outer rib {} vertex {} is quarter 0's turned: {:.3e} off", q, i, j, (a[j] - b[j]).magnitude()));
        }
    }

    std::cout << "floor_elements: every seam and bay edge read by its two quarters as one plane, every in-place quarter equal to the turned quarter 0 within 1e-6" << std::endl;
}

/// The ring built from the four oculus edges is four-fold symmetric on the square: every ring beam and bottom wedge equals the first turned by its quarter turns, and the plate equals itself turned, within 1e-9.
void check_ring() {

    const std::span<const std::array<Polyline, 2>> ring = square_guide().oculus();
    check(ring.size() == 9, "four ring beams, four wedges and the plate");

    for (size_t i = 0; i < 8; i++) {
        const Xform turn = Xform::rotation_z(static_cast<double>(i % 4) * 90.0, true);
        const std::array<Polyline, 2>& first = ring[i < 4 ? 0 : 4];

        for (const std::array<const Polyline*, 2>& loops : {std::array<const Polyline*, 2>{&first[0], &ring[i][0]}, std::array<const Polyline*, 2>{&first[1], &ring[i][1]}}) {
            const std::vector<Point> a = loops[0]->transformed(turn).get_points();
            const std::vector<Point> b = loops[1]->get_points();
            check(a.size() == b.size(), "a ring member has the first member's vertex count");

            for (size_t j = 0; j < a.size(); j++)
                check((a[j] - b[j]).magnitude() <= 1e-9, fmt::format("ring member {} vertex {} is member {} turned: {:.3e} off", i, j, i < 4 ? 0 : 4, (a[j] - b[j]).magnitude()));
        }
    }

    const std::vector<Point> plate = ring[8][0].get_points();
    const std::vector<Point> turned = ring[8][0].transformed(Xform::rotation_z(90.0, true)).get_points();

    for (size_t j = 0; j + 1 < plate.size(); j++)
        check((turned[j] - plate[(j + 1) % (plate.size() - 1)]).magnitude() <= 1e-9, "the ring plate is four-fold symmetric");

    std::cout << "floor_elements: the ring from the four oculus edges is the rotated ring within 1e-9 on the square" << std::endl;
}

/// Rule A on the square: the ruling and the rib sweep of every quarter along the column chamfer.
void check_rule_a() {

    const wood_floor::FloorGuide& guide = square_guide();

    for (size_t q = 0; q < 4; q++) {
        const wood_floor::CentralPanel& panel = guide.central_panel(q);
        const Vector chamfer = (guide.quarter_column_polygon(q)[3] - guide.quarter_column_polygon(q)[2]).normalized();
        check(std::abs(std::abs(panel.ruling.dot(chamfer)) - 1.0) <= 1e-12 && std::abs(std::abs(panel.rib_sweep.dot(chamfer)) - 1.0) <= 1e-12, fmt::format("rule A gives quarter {} the chamfer direction for the ruling and the sweep", q));
    }

    std::cout << "floor_elements: rule A on the square is the chamfer direction" << std::endl;
}

/// The thinnest and thickest central bed plate of a floor: the distance of each plate's top corners from its bottom face's plane.
std::array<double, 2> central_bed_thickness(const wood_floor::FloorGuide& guide) {

    std::array<double, 2> range = {1e300, 0.0};

    for (size_t q = 0; q < 4; q++) {
        const std::array<std::vector<std::array<Polyline, 2>>, 3>& rows = guide.beds(q);

        for (const std::array<Polyline, 2>& bed : rows[1]) {
            const std::vector<Point> bottom = bed[1].get_points();
            const Vector normal = (bottom[1] - bottom[0]).cross(bottom[3] - bottom[0]).normalized();
            const Plane plane = Plane::from_point_normal(bottom[0], normal);

            for (const Point& point : bed[0].get_points()) {
                const double thickness = std::abs((point - plane.origin()).dot(plane.z_axis()));
                range = {std::min(range[0], thickness), std::max(range[1], thickness)};
            }
        }
    }

    return range;
}

/// Every central bed plate on the square exactly tsections thick.
void check_section_layers() {

    const wood_floor::FloorGuide& guide = square_guide();
    const std::array<double, 2> section = central_bed_thickness(guide);
    check(std::abs(section[0] - guide.size_tsections) <= 1e-9 && std::abs(section[1] - guide.size_tsections) <= 1e-9, fmt::format("every central bed plate {} thick, not {:.12f} .. {:.12f}", guide.size_tsections, section[0], section[1]));

    std::cout << fmt::format("floor_elements: the central bed plates {:.9f} .. {:.9f} thick", section[0], section[1]) << std::endl;
}

/// The farthest point of a loop from the plane through its first point with its Newell normal, mm.
double loop_flatness(std::vector<Point> points) {

    if (points.size() > 1 && points.front() == points.back())
        points.pop_back();

    const Vector normal = compute_newell(points).normalized();
    double worst = 0.0;

    for (const Point& point : points)
        worst = std::max(worst, std::abs((point - points[0]).dot(normal)));

    return worst;
}

/// The least flat face of a member outline: its two loops and every side quad between them, mm.
double outline_flatness(const std::array<Polyline, 2>& outline) {

    const std::vector<Point> top = outline[0].get_points();
    const std::vector<Point> bottom = outline[1].get_points();
    double worst = std::max(loop_flatness(top), loop_flatness(bottom));

    for (size_t i = 0; i + 1 < top.size() && i + 1 < bottom.size(); i++)
        worst = std::max(worst, loop_flatness({top[i], top[i + 1], bottom[i + 1], bottom[i]}));

    return worst;
}

/// The least flat face over every member of the floor.
double floor_flatness(const wood_floor::FloorGuide& guide) {

    std::vector<std::array<Polyline, 2>> outlines(guide.oculus().begin(), guide.oculus().end());

    for (size_t q = 0; q < 4; q++) {

        for (std::span<const std::array<Polyline, 2>> family : std::initializer_list<std::span<const std::array<Polyline, 2>>>{guide.outer_ribs(q), guide.inner_ribs(q), guide.inner_beams(q), guide.wedges(q), guide.tsections(q)})
            outlines.insert(outlines.end(), family.begin(), family.end());

        for (const std::vector<std::array<Polyline, 2>>& row : guide.beds(q))
            outlines.insert(outlines.end(), row.begin(), row.end());
    }

    double worst = 0.0;

    for (const std::array<Polyline, 2>& outline : outlines)
        worst = std::max(worst, outline_flatness(outline));

    return worst;
}

/// The eight rib face bottoms of corner q at the column head: both faces of the two outer and the two inner ribs.
std::vector<double> rib_bottoms(const wood_floor::FloorGuide& guide, size_t q) {

    std::vector<double> levels;

    for (std::span<const std::array<Polyline, 2>> family : std::initializer_list<std::span<const std::array<Polyline, 2>>>{guide.outer_ribs(q), guide.inner_ribs(q)})
        for (const std::array<Polyline, 2>& rib : family)
            levels.insert(levels.end(), {rib[0].get_point(2)[2], rib[1].get_point(2)[2]});

    return levels;
}

/// One rib level per column head: on 3000 x 2400 both outer ribs of every corner end on their fan planes at the cutter level, the shallower end -689.979, the short rib's start solved to 187.667 and the long one's kept at the wedge, every inner rib face within 0.2 mm of it; the square keeps the wedge as both rib starts.
void check_rib_levels() {

    const wood_floor::FloorGuide guide = rectangle_guide(3000.0, 2400.0);

    for (size_t q = 0; q < 4; q++) {
        const double level = guide.column_levels(q)[1];
        const std::vector<double> bottoms = rib_bottoms(guide, q);
        const std::array<double, 2>& starts = guide.rib_starts(q);
        check(std::abs(level + 689.979) < 1e-3, fmt::format("corner {}'s level at the shallower outer rib end, {:.3f}", q, level));
        check(std::max(starts[0], starts[1]) == guide.size_wedge && std::abs(std::min(starts[0], starts[1]) - 187.667) < 1e-3, fmt::format("corner {}'s rib starts {:.3f} / {:.3f}: the long rib keeps the wedge, the short one 187.667", q, starts[0], starts[1]));

        for (size_t i = 0; i < 4; i++)
            check(std::abs(bottoms[i] - level) <= 1e-9, fmt::format("corner {}'s outer rib face {} ends {:.3e} mm off the level", q, i, bottoms[i] - level));

        for (size_t i = 4; i < 8; i++)
            check(std::abs(bottoms[i] - level) <= 0.2, fmt::format("corner {}'s inner rib face {} ends {:.3f} mm off the level", q, i - 4, bottoms[i] - level));
    }

    const wood_floor::FloorGuide& square = square_guide();

    for (size_t q = 0; q < 4; q++)
        check(square.rib_starts(q)[0] == square.size_wedge && square.rib_starts(q)[1] == square.size_wedge, "the square keeps the wedge as its rib start");

    std::cout << fmt::format("floor_elements: one rib level per column on 3000 x 2400, {:.3f}, the short rib start {:.3f}; the square at the wedge rib start", guide.column_levels(0)[1], std::min(guide.rib_starts(0)[0], guide.rib_starts(0)[1])) << std::endl;
}

/// The thickness of each column block of quarter q, side 0, middle and side 1: its far plane's distance from its fan plane.
std::array<double, 3> block_thickness(const wood_floor::FloorGuide& guide, size_t q) {

    std::array<double, 3> thickness;

    for (size_t i = 0; i < 3; i++) {
        const std::array<Plane, 2>& planes = guide.construction_planes(q).wedges[i];
        thickness[i] = (planes[1].origin() - planes[0].origin()).dot(planes[0].z_axis());
    }

    return thickness;
}

/// The lowest corner of a column block's far face.
double far_bottom(const std::array<Polyline, 2>& block) {

    double lowest = 1e300;

    for (const Point& point : block[0].get_points())
        lowest = std::min(lowest, point[2]);

    return lowest;
}

/// The column blocks span their ribs' rib starts: on 3000 x 2400 the side blocks 240 and 187.667 thick with their far ends within 1 mm, the middle one 1.25 times their mean, 267.292; 240 / 300 / 240 on the square.
void check_column_blocks() {

    const wood_floor::FloorGuide guide = rectangle_guide(3000.0, 2400.0);

    for (size_t q = 0; q < 4; q++) {
        const std::array<double, 3> thickness = block_thickness(guide, q);
        const std::array<double, 2>& starts = guide.rib_starts(q);
        const std::span<const std::array<Polyline, 2>> blocks = guide.wedges(q);
        check(std::abs(thickness[0] - starts[0]) <= 1e-9 && std::abs(thickness[2] - starts[1]) <= 1e-9 && std::abs(thickness[1] - 267.292) < 1e-3, fmt::format("quarter {}'s blocks {:.3f} / {:.3f} / {:.3f} thick over the rib starts {:.3f} / {:.3f}", q, thickness[0], thickness[1], thickness[2], starts[0], starts[1]));
        check(std::abs(far_bottom(blocks[0]) - far_bottom(blocks[2])) <= 1.0, fmt::format("quarter {}'s side blocks end {:.3f} mm apart", q, far_bottom(blocks[0]) - far_bottom(blocks[2])));
    }

    for (size_t q = 0; q < 4; q++) {
        const std::array<double, 3> thickness = block_thickness(square_guide(), q);
        check(std::abs(thickness[0] - 240.0) <= 1e-9 && std::abs(thickness[1] - 300.0) <= 1e-9 && std::abs(thickness[2] - 240.0) <= 1e-9, fmt::format("the square's blocks 240 / 300 / 240, not {:.12f} / {:.12f} / {:.12f}", thickness[0], thickness[1], thickness[2]));
    }

    const std::span<const std::array<Polyline, 2>> blocks = guide.wedges(0);
    std::cout << fmt::format("floor_elements: the column blocks over the rib starts on 3000 x 2400, {:.3f} / {:.3f} / {:.3f} thick, the side ends {:.3f} mm apart; 240 / 300 / 240 on the square", block_thickness(guide, 0)[0], block_thickness(guide, 0)[1], block_thickness(guide, 0)[2], std::abs(far_bottom(blocks[0]) - far_bottom(blocks[2]))) << std::endl;
}

/// The 3000 x 2400 bay: every member face planar, and the floor builds with every connector.
void check_rectangle() {

    const wood_floor::FloorGuide guide = rectangle_guide(3000.0, 2400.0);
    check(floor_flatness(guide) <= 1e-9, fmt::format("every member face planar, {:.3e} off", floor_flatness(guide)));

    const wood_floor::Floor scene(guide, "rectangle");
    check(all_connectors(scene).size() == 40, "the rectangle's 40 connectors");
    std::cout << fmt::format("floor_elements: 3000 x 2400 faces planar within {:.1e}, 40 connectors", floor_flatness(guide)) << std::endl;
}

/// Whether the node carries the connector colour.
bool in_connector_color(const TreeNode& node) {

    const Color& color = JointBeam::CONNECTOR_COLOR;

    return node.color && node.color->r == color.r && node.color->g == color.g && node.color->b == color.b && node.color->a == color.a;
}

/// Every connector's node and every part and pin node nested under it carry the connector colour; returns the nodes checked.
size_t check_connector_colors(const WoodSession& scene, const std::vector<std::shared_ptr<JointBeam>>& connectors, const std::string& label) {

    size_t nodes = 0;

    for (const std::shared_ptr<JointBeam>& connector : connectors) {
        const std::shared_ptr<TreeNode> node = scene.tree.get_node_by_name(connector->guid());
        check(node && in_connector_color(*node), label + " " + connector->name + " in the connector colour");
        nodes++;

        for (TreeNode* child : node->descendants()) {
            check(in_connector_color(*child), label + " " + connector->name + " child " + child->name + " in the connector colour");
            nodes++;
        }
    }

    return nodes;
}

/// Every connector sits in connectors_q under quarter_q, the one quarter it belongs to, or in connectors under oculus; returns the connectors per group as parent/group.
std::map<std::string, size_t> check_connector_tree(const WoodSession& scene, const std::vector<std::shared_ptr<JointBeam>>& connectors, const std::string& label) {

    std::map<std::string, size_t> counts;

    for (const std::shared_ptr<JointBeam>& connector : connectors) {
        const std::shared_ptr<TreeNode> node = scene.tree.get_node_by_name(connector->guid());
        const std::shared_ptr<TreeNode> group = node ? node->parent() : nullptr;
        const std::shared_ptr<TreeNode> parent = group ? group->parent() : nullptr;
        const std::string path = parent ? parent->name + "/" + group->name : "";
        const bool quarter = path.starts_with("quarter_") && path.substr(path.find('/') + 1) == "connectors_" + path.substr(8, path.find('/') - 8);
        check(quarter || path == "oculus/connectors", fmt::format("{} {} under connectors_q of its quarter_q or connectors of oculus, not {}", label, connector->name, path));
        counts[path]++;
    }

    return counts;
}

/// The pins of one floor: per kind 16, 16 and 16, none at the oculus, every connector pre-drilled and naming the two members it joins first, every pin 200 long, radius 2; no member cut by them, every member reading its pre-drill lines from the one connector, and both through a round trip; every connector of the floor and every part and pin nested under it in the connector colour, every connector in the connectors group of its quarter, 15 per quarter, the four oculus wedges in the connectors group of the oculus, also after the round trip.
void check_floor_pins(const wood_floor::FloorGuide& guide, const std::string& label) {

    wood_floor::Floor scene(guide, "pins");
    std::vector<std::shared_ptr<JointBeam>> connectors = all_connectors(scene);

    // each pin connector by its place, and the two members it joins
    std::vector<std::shared_ptr<JointBeam>> pins;
    std::vector<std::pair<std::string, std::array<std::shared_ptr<Element>, 2>>> joined;

    for (size_t q = 0; q < 4; q++)
        for (size_t k = 0; k < 2; k++) {
            const std::shared_ptr<Element> seam_beam = named<Element>(scene, fmt::format("inner_beams_{}_{}", k == 0 ? 0 : 2, q));
            const std::shared_ptr<Element> oculus_beam = named<Element>(scene, fmt::format("inner_beams_1_{}", q));
            pins.push_back(named<JointBeam>(scene, fmt::format("connector_pins_outer_rib_{}_{}", q, k)));
            joined.push_back({"outer_rib_seam_beam", {seam_beam, named<Element>(scene, fmt::format("outer_ribs_{}_{}", k, q))}});
            pins.push_back(named<JointBeam>(scene, fmt::format("connector_pins_seam_beam_{}_{}", q, k)));
            joined.push_back({"seam_beam_oculus_beam", {seam_beam, oculus_beam}});
            pins.push_back(named<JointBeam>(scene, fmt::format("connector_pins_inner_rib_{}_{}", q, k)));
            joined.push_back({"oculus_beam_inner_rib", {oculus_beam, named<Element>(scene, fmt::format("inner_ribs_{}_{}", k, q))}});
        }

    for (size_t q = 0; q < 4; q++) {
        pins.push_back(named<JointBeam>(scene, fmt::format("connector_pins_ring_corner_{}", q)));
        joined.push_back({"ring_corner", {named<Element>(scene, fmt::format("oculus_{}", q)), named<Element>(scene, fmt::format("oculus_{}", (q + 1) % 4))}});
    }

    connectors.insert(connectors.end(), pins.begin(), pins.end());
    check(pins_of(scene).size() == pins.size(), label + " no pin connector beyond the butt joints and the ring corners");

    std::map<std::string, size_t> counts;

    for (size_t i = 0; i < pins.size(); i++) {
        const JointBeam& connector = *pins[i];
        const auto& [kind, pair] = joined[i];
        counts[kind] += connector.drill_lines.size();
        check(connector.pre_drill && connector.cutters.empty() && connector.parts.empty() && connector.targets.size() >= 2, label + " " + connector.name + " pre-drilled, no cutter, its members as targets");
        check(connector.targets[0] == pair[0]->guid() && connector.targets[1] == pair[1]->guid(), label + " " + connector.name + " names both members of its " + kind);
        check(std::abs(connector.line_radius - 2.0) <= 1e-12, label + " " + connector.name + " radius 2");
        check_nested(
            scene,
            connector,
            0,
            connector.drill_lines.size()
        );

        for (const Line& line : connector.drill_lines) {
            check(std::abs(line.length() - 200.0) <= 1e-9, label + " " + connector.name + " pins 200 long");
            const Vector along = line.to_direction().normalized();
            bool on_axis = false;

            for (const std::shared_ptr<Element>& member : pair)
                if (const std::shared_ptr<BeamVariable> beam = std::dynamic_pointer_cast<BeamVariable>(member)) {
                    const Vector axis = beam->axis.to_vector();
                    const Vector level = Vector(axis[0], axis[1], 0.0).normalized();
                    on_axis = on_axis || std::abs(std::abs(along.dot(level)) - 1.0) <= 1e-9;
                }

            check(std::abs(along[2]) <= 1e-9 && on_axis, label + " " + connector.name + " pins level along the axis of the member that ends on the contact");

            for (const std::string& guid : connector.targets)
                check(matches(scene.pre_drill_lines(guid), line) == 1, label + " " + connector.name + " every member reads each pin once");
        }
    }

    check(counts["outer_rib_seam_beam"] == 16 && counts["seam_beam_oculus_beam"] == 16 && counts["oculus_beam_inner_rib"] == 16 && counts["ring_corner"] == 8, label + " pins per kind 16, 16, 16 and two at each of the four ring corners");

    const WoodSession back = WoodSession::pb_loads(scene.pb_dumps());
    size_t loaded = 0;

    for (const std::shared_ptr<JointBeam>& connector : pins) {
        const std::shared_ptr<JointBeam> read = back.get_element<JointBeam>(connector->guid());
        check(read && read->pre_drill && read->targets == connector->targets && read->drill_lines.size() == connector->drill_lines.size(), label + " pin connector round trip " + connector->name);
        check_nested(
            back,
            *read,
            0,
            read->drill_lines.size()
        );

        for (const Line& line : connector->drill_lines)
            for (const std::string& guid : connector->targets)
                check(matches(back.pre_drill_lines(guid), line) == 1, label + " pre-drill lines round trip " + connector->name);

        loaded++;
    }

    check(loaded == pins.size(), label + " pin round trip count");
    const size_t painted = check_connector_colors(scene, connectors, label);
    const std::map<std::string, size_t> groups = check_connector_tree(scene, connectors, label);
    std::map<std::string, size_t> expected;

    for (size_t q = 0; q < 4; q++)
        expected[fmt::format("quarter_{}/connectors_{}", q, q)] = 15;

    expected["oculus/connectors"] = 8;
    check(groups == expected, label + " every connector in its quarter, 15 each, the four oculus wedges and the four ring corners in the oculus");
    check(check_connector_tree(back, connectors, label + " round trip") == expected, label + " the connector tree through a round trip");
    const size_t count = 68;
    check(connectors.size() == count && check_connector_colors(back, connectors, label + " round trip") == painted, fmt::format("{} {} connectors, {} nodes in the connector colour, the same after a round trip", label, count, painted));
    size_t total = 0;

    for (const std::pair<const std::string, size_t>& kind : counts)
        total += kind.second;

    std::cout << fmt::format("floor_elements: {} pins in {} connectors on {}, none at the oculus, both members named, 200 x d4, nothing cut, pre-drill lines through a round trip; {} connectors and their {} part and pin nodes in the connector colour, also after it", total, pins.size(), label, connectors.size(), painted - connectors.size()) << std::endl;
}

/// The assembly pins on the square and on 3000 x 2400.
void check_floor_pins() {

    check_floor_pins(square_guide(), "the square");
    check_floor_pins(rectangle_guide(3000.0, 2400.0), "3000 x 2400");
}

int main() {

    check_shared_entities();
    check_ring();
    check_rule_a();
    check_section_layers();
    check_rectangle();
    check_rib_levels();
    check_column_blocks();
    check_short_members();
    check_contacts();
    check_beams();
    check_thickness();
    check_support();
    check_wedges();
    check_seam_beams();
    check_drill_features();
    check_floors_in_scene();
    check_extract_quarter();
    check_centred_pins();
    check_quarter_pins();
    check_rectangle_plates();
    check_floor_pins();
    check_connector_calls();
    check_skewed_bays();

    return 0;
}
