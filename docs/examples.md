# Examples {#examples}

[TOC]

Twelve standalone C++ workflow targets covering contact detection, plate and beam joinery solvers, graph queries, spatial assignment, and boolean cutting. For single-element constructors and joint libraries see [Elements](@ref elements); for parametric building structures see [Templates](@ref templates).

```bash
cmake --build build --target 1_session_contacts && ./build/1_session_contacts
```

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#ex_1_session_contacts">
    <img src="ex_1_session_contacts.webp" alt="Session Contacts">
    <span class="wood-card-title">Session Contacts</span>
  </a>
  <a class="wood-card" href="#ex_2_contacts">
    <img src="ex_2_contacts.webp" alt="Contacts">
    <span class="wood-card-title">Contacts</span>
  </a>
  <a class="wood-card" href="#ex_3_features">
    <img src="ex_3_features.webp" alt="Plate Joinery">
    <span class="wood-card-title">Plate Joinery</span>
  </a>
  <a class="wood-card" href="#ex_4_traversal">
    <img src="ex_4_traversal.webp" alt="Graph Traversal">
    <span class="wood-card-title">Graph Traversal</span>
  </a>
  <a class="wood-card" href="#ex_5_settings">
    <img src="ex_5_settings.webp" alt="Solver Settings">
    <span class="wood-card-title">Solver Settings</span>
  </a>
  <a class="wood-card" href="#ex_6_custom_joint">
    <img src="ex_6_custom_joint.webp" alt="Custom Joint">
    <span class="wood-card-title">Custom Joint</span>
  </a>
  <a class="wood-card" href="#ex_7_assignment">
    <img src="ex_7_assignment.webp" alt="Joint Assignment">
    <span class="wood-card-title">Joint Assignment</span>
  </a>
  <a class="wood-card" href="#ex_8_beams">
    <img src="ex_8_beams.webp" alt="Beam Contacts">
    <span class="wood-card-title">Beam Contacts</span>
  </a>
  <a class="wood-card" href="#ex_9_cross_joints">
    <img src="ex_9_cross_joints.webp" alt="Cross Joints">
    <span class="wood-card-title">Cross Joints</span>
  </a>
  <a class="wood-card" href="#ex_10_profile_cuts">
    <img src="ex_10_profile_cuts.webp" alt="Profile Cuts">
    <span class="wood-card-title">Profile Cuts</span>
  </a>
  <a class="wood-card" href="#ex_11_drill_solids">
    <img src="ex_11_drill_solids.webp" alt="Drill Solids">
    <span class="wood-card-title">Drill Solids</span>
  </a>
  <a class="wood-card" href="#ex_12_solid_features">
    <img src="ex_12_solid_features.webp" alt="Solid Booleans">
    <span class="wood-card-title">Solid Booleans</span>
  </a>
</div>

---

## Session Contacts {#ex_1_session_contacts}

Computes face contacts between plates, a beam, and a column in WoodSession, orients a parametric tenon joint to the contact zone, and attaches cutting interactions to both plates.

![](ex_1_session_contacts.webp)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    // WoodSession:
    WoodSession wood_session("elements");

    const std::shared_ptr<Plate> plate0 =
        Plate::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 400, 300, 40);
    const std::shared_ptr<Plate> plate1 =
        Plate::from_rectangle({150, 0, 40}, {0, 1, 0}, {0, 0, 1}, 300, 400, 40);
    const std::shared_ptr<Beam> beam =
        std::make_shared<Beam>(Polyline({{500, 50, 900}, {900, 50, 900}}), 50.0);
    const std::shared_ptr<Column> column = Column::square(
        Line::from_points({950, 50, 0}, {950, 50, 950}),
        Plane({900, 0, 0}, {1, 0, 0}, {0, 1, 0}),
        100
    );
    const std::shared_ptr<Block> block = std::make_shared<Block>(std::vector<Polyline>{
        Polyline::rectangle({1200, 0, 0}, {1, 0, 0}, {0, 1, 0}, 200, 400),
        Polyline::rectangle({1170, 0, 250}, {1, 0, 0}, {0, 1, 0}, 260, 400)
    });
    const std::shared_ptr<JointPlate> joint = JointPlate::top_side_tenons(8, 0.5);
    joint->is_visible = false;
    plate0->is_locked = true;

    wood_session.add(plate0);
    wood_session.add(plate1);
    wood_session.add(beam);
    wood_session.add(column);
    wood_session.add(block);
    wood_session.add(joint);

    std::cout << wood_session << "\n";

    std::shared_ptr<InteractionContactFace> contact =
        wood_session.compute_face_contact(column, beam);
    wood_session.add_interaction(column, beam, contact);
    // std::cout << *contact;

    // Compute contact with volume and orient the joint to it, then add the joint features to the plates:
    std::shared_ptr<InteractionContactFace> contact_with_volume =
        wood_session.compute_face_contact(plate0, plate1);
    joint->orient(contact_with_volume);
    wood_session.add_interaction(plate1, plate0, contact_with_volume);
    wood_session.add_interaction(joint, plate1, joint->interaction_feature(0));
    wood_session.add_interaction(joint, plate0, joint->interaction_feature(1));
    // std::cout << *contact_with_volume << "\n";

    // Element and model geometry:
    for (const std::shared_ptr<Element>& element : wood_session.elements()) {
        std::cout << *element << "\n";
        std::cout << element->model_geometry_mesh() << "\n";
        std::cout << element->model_geometry_brep() << "\n";
        std::cout << plate0->element_geometry_mesh() << "\n";
        std::cout << plate0->element_geometry_brep() << "\n";
    }

    // Serialize:
    std::string path = pb_path("live");
    wood_session.pb_dump(path);
    std::cout << path << "\n";

    return 0;
}
```

---

## Contacts {#ex_2_contacts}

Detects face, axis, and cross contacts across an in-plane hexagonal shell dataset and inspects every contact edge in the session graph.

![](ex_2_contacts.webp)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_face_contacts();
    wood_session.compute_cross_contacts();
    wood_session.compute_line_contacts();

    for (const std::tuple<std::string, std::string>& pair : wood_session.graph.get_edges()) {

        const Edge& edge = wood_session.graph.edges.at(std::get<0>(pair)).at(std::get<1>(pair));
        const std::shared_ptr<Element> first = wood_session.get_element<Element>(edge.v0);
        const std::shared_ptr<Element> second = wood_session.get_element<Element>(edge.v1);
        const std::string a = edge.v0.substr(0, 8);
        const std::string b = edge.v1.substr(0, 8);

        for (const std::shared_ptr<Interaction>& interaction :
             wood_session.get_interaction(first, second)) {

            if (const InteractionContactFace* face =
                    dynamic_cast<const InteractionContactFace*>(interaction.get()))
                std::cout << fmt::format(
                    "face   {} f{} with {} f{} class {} points {}\n",
                    a,
                    face->face_a,
                    b,
                    face->face_b,
                    contact_type_name(face->type),
                    face->polygon.point_count()
                );

            if (const InteractionContactAxis* axis =
                    dynamic_cast<const InteractionContactAxis*>(interaction.get()))
                std::cout << fmt::format(
                    "axis   {} s{} with {} s{} gap {:.3f}\n",
                    a,
                    axis->segment_a,
                    b,
                    axis->segment_b,
                    axis->segment.length()
                );

            if (const InteractionContactCross* cross =
                    dynamic_cast<const InteractionContactCross*>(interaction.get()))
                std::cout << fmt::format(
                    "cross  {} f{},{} with {} f{},{}\n",
                    a,
                    cross->faces_a[0],
                    cross->faces_a[1],
                    b,
                    cross->faces_b[0],
                    cross->faces_b[1]
                );
        }
    }

    std::cout << wood_session.get_contacts().size() << " contacts on "
              << wood_session.interactions.size() << " interactions\n";

    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

---

## Plate Joinery {#ex_3_features}

Runs the plate joinery solver on a hexagonal shell, orienting finger joints across shared edges and evaluating boolean-cut plate meshes.

![](ex_3_features.webp)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_features();

    for (const InteractionFeaturePlate& joint : wood_session.get_plate_features())
        std::cout << fmt::format(
            "{} type {} between {} f{} and {} f{}: {} male, {} female outlines, {} divisions\n",
            joint.name,
            joint.joint_type,
            joint.element_a.substr(0, 8),
            joint.contact.face_a,
            joint.element_b.substr(0, 8),
            joint.contact.face_b,
            joint.male_outlines[0].size(),
            joint.female_outlines[0].size(),
            joint.divisions
        );

    const std::shared_ptr<Plate> plate = wood_session.plates().front();
    std::cout << fmt::format(
        "{}: {} faces alone, {} faces with joints, {} joint features\n",
        plate->name,
        plate->element_geometry_mesh().number_of_faces(),
        plate->model_geometry_mesh().number_of_faces(),
        plate->Element::features().size()
    );

    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

---

## Graph Traversal {#ex_4_traversal}

Traverses the session graph from a solved plate joint to its connected male and female plates, edge interactions, and underlying face contact.

![](ex_4_traversal.webp)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_face_contacts();
    wood_session.compute_features();

    const InteractionFeaturePlate joint = wood_session.get_plate_features().front();
    const std::shared_ptr<Plate> male = wood_session.get_element<Plate>(joint.element_a);
    const std::shared_ptr<Plate> female = wood_session.get_element<Plate>(joint.element_b);
    std::cout << "joint " << joint.name << " cuts " << male->name << " and " << female->name
              << "\n";

    const std::vector<std::shared_ptr<Interaction>> interactions =
        wood_session.get_interaction(female, male);
    std::cout << "their edge holds " << interactions.size() << " interactions\n";

    for (const std::shared_ptr<Interaction>& interaction : interactions)
        if (dynamic_cast<InteractionContact*>(interaction.get()))
            std::cout << "the joint was solved from " << *interaction << "\n";

    const Edge& edge = wood_session.graph.edges.at(male->guid()).at(female->guid());
    std::cout << "the edge joins " << wood_session.get_element<Element>(edge.v0)->name << " and "
              << wood_session.get_element<Element>(edge.v1)->name << "\n";

    Mesh male_mesh = male->model_geometry_mesh();
    male_mesh.set_objectcolor(Color(0.807f, 0.063f, 0.258f));
    male->is_visible = false;
    wood_session.add(std::make_shared<Element>(male_mesh, male->name + "_male"));

    Mesh female_mesh = female->model_geometry_mesh();
    female_mesh.set_objectcolor(Color(0.24f, 0.54f, 0.86f));
    female->is_visible = false;
    wood_session.add(std::make_shared<Element>(female_mesh, female->name + "_female"));

    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

---

## Solver Settings {#ex_5_settings}

Configures division counts, tolerances, and joint extensions through WoodSession settings on a folded plate box and round-trips them through protobuf bytes.

![](ex_5_settings.webp)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::annen_box};

int main() {

    WoodSession wood_session = WoodSession::obj_load(DATASET);

    Settings& s = wood_session.settings;
    s.joint_parameters[1 * 3 + 0] = 200;
    s.joint_parameters[1 * 3 + 2] = 10;
    s.joint_parameters[2 * 3 + 2] = 20;
    s.joint_volume_extension = {0, 0, 10};
    s.joint_scale = {1.0, 1.0, 1.0};
    s.distance = 0.1;
    s.angle = 0.11;

    wood_session.compute_features(face_to_face);
    for (const InteractionFeaturePlate& joint : wood_session.get_plate_features())
        std::cout << fmt::format(
            "{} divisions {} length {:.0f}\n", joint.name, joint.divisions, joint.length
        );

    const WoodSession back = WoodSession::pb_loads(wood_session.pb_dumps());
    std::cout << "the file carries the settings: division length "
              << back.settings.joint_parameters[3] << "\n";

    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

---

## Custom Joint {#ex_6_custom_joint}

Defines custom butterfly dovetail outlines in unit-box space and tiles them along the seam between two coplanar plates.

![](ex_6_custom_joint.webp)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const double SEAM_DEPTH = 1.5;
const double TOOTH_HALF = 0.06;
const double NECK_RATIO = 0.45;

/// Three butterfly half-mortise teeth in unit-cube space: y at the plate face, x signed across the seam, z in [-0.5, 0.5] along it.
static Polyline teeth(const double y, const double depth) {
    std::vector<Point> pts;
    for (const double cz : {0.28, 0.0, -0.28}) {
        pts.emplace_back(0.0, y, cz + TOOTH_HALF * NECK_RATIO);
        pts.emplace_back(depth, y, cz + TOOTH_HALF);
        pts.emplace_back(depth, y, cz - TOOTH_HALF);
        pts.emplace_back(0.0, y, cz - TOOTH_HALF * NECK_RATIO);
    }
    return Polyline(pts);
}

int main() {

    WoodSession wood_session("custom_joint");
    wood_session.add(
        Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, 40)
    );
    wood_session.add(
        Plate::from_rectangle(Point(1000, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, 40)
    );

    wood_session.settings.joint_parameters[0 * 3 + 2] = 9;
    wood_session.settings.custom_joints["side_side_inplane"] = {
        std::vector<Polyline>{teeth(-0.5, -SEAM_DEPTH), teeth(0.5, -SEAM_DEPTH)},
        std::vector<Polyline>{teeth(-0.5, SEAM_DEPTH), teeth(0.5, SEAM_DEPTH)},
    };

    wood_session.compute_features(face_to_face);
    for (const InteractionFeaturePlate& joint : wood_session.get_plate_features())
        std::cout << joint.name << " with " << joint.male_outlines[0].front().point_count()
                  << " points per male outline\n";

    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

---

## Joint Assignment {#ex_7_assignment}

Assigns in-plane finger joints and top-to-side tenon-mortise joints from points on shared plate edges, and alters both finger and tenon-mortise directions from lines placed on those edges.

![](ex_7_assignment.webp)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession wood_session("assignment");

    // In-plane plates: seam at x = 300 keeps default 90-degree fingers; seam at x = 600 is angled by line
    wood_session.add(
        Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 300, 420, 40)
    );
    wood_session.add(
        Plate::from_rectangle(Point(300, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 300, 420, 40)
    );
    wood_session.add(
        Plate::from_rectangle(Point(600, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 300, 420, 40)
    );

    // Top-to-side T-joint: horizontal base (50 mm) and vertical wall with angled tenons and mortises
    wood_session.add(
        Plate::from_rectangle(Point(1040, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 420, 420, 50)
    );
    wood_session.add(
        Plate::from_rectangle(Point(1230, 0, 50), Vector(0, 1, 0), Vector(0, 0, 1), 420, 280, 40)
    );

    wood_session.settings.joint_parameters[0 * 3 + 0] = 60;
    wood_session.settings.joint_parameters[2 * 3 + 0] = 100;
    wood_session.settings.joint_volume_extension = {0, 0, -50};

    const Point straight_seam(300, 210, 40.5);
    const Point angled_seam(600, 210, 40.5);
    const Point tenon_edge(1229.5, 230, 50.5);

    const Line finger_dir = Line::from_points(angled_seam, angled_seam + Vector(-125, 145, 0));
    const Line tenon_dir = Line::from_points(tenon_edge, tenon_edge + Vector(0, -145, 160));

    const int finger_id = JointPlate::library_id("side_side_inplane_dovetails");
    const int tenon_id = JointPlate::library_id("top_side_tenons");
    wood_session.assign_joint_types_by_points(
        {straight_seam, angled_seam, tenon_edge}, {finger_id, finger_id, tenon_id}, 30.0
    );
    wood_session.assign_insertion_vectors_by_lines({finger_dir, tenon_dir}, 30.0);

    wood_session.compute_features(face_to_face);
    std::cout << wood_session.get_plate_features().size() << " assigned joints solved\n";

    const Color pink(0.807f, 0.063f, 0.258f);
    for (const Point& pt : {straight_seam, angled_seam, tenon_edge}) {
        auto marker = std::make_shared<Point>(pt);
        marker->pointcolor = pink;
        wood_session.add_point(marker);
    }
    for (const Line& dir_line : {finger_dir, tenon_dir}) {
        const Vector u = dir_line.to_vector().normalized();
        const Vector n = std::abs(u[2]) < 0.1 ? Vector(-u[1], u[0], 0) : Vector(0, -u[2], u[1]);
        const Point tip = dir_line.end();
        auto arrow = std::make_shared<Polyline>(std::vector<Point>{
            dir_line.start(), tip, tip - u * 28 + n * 14, tip, tip - u * 28 - n * 14
        });
        arrow->linecolor = pink;
        arrow->width = 5.0;
        wood_session.add_polyline(arrow);
    }

    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

---

## Beam Contacts {#ex_8_beams}

Detects axis intersections between three orthogonal timber beams and solves their oriented crossing and end-to-side joint volumes.

![](ex_8_beams.webp)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession wood_session("beams");
    wood_session.add(std::make_shared<Beam>(Polyline({Point(-1000, 0, 0), Point(1000, 0, 0)}), 60.0)
    );
    wood_session.add(std::make_shared<Beam>(Polyline({Point(0, -1000, 0), Point(0, 1000, 0)}), 60.0)
    );
    wood_session.add(std::make_shared<Beam>(Polyline({Point(0, 0, 0), Point(0, 0, 1000)}), 60.0));

    wood_session.compute_axis_contacts(5.0);
    wood_session.compute_beam_features(400.0, 0.9, 1);

    for (const std::shared_ptr<JointBeam>& joint : wood_session.get_elements<JointBeam>()) {
        const std::shared_ptr<Beam> a = wood_session.get_element<Beam>(joint->targets[0]);
        const std::shared_ptr<Beam> b = wood_session.get_element<Beam>(joint->targets[1]);
        const int end = joint->feature.end_type;
        std::cout << fmt::format(
            "{} with {}: end type {} ({}), {} volume rectangles\n",
            a->name,
            b->name,
            end,
            end == 0   ? "crossing"
            : end == 1 ? "side to end"
                       : "end to end",
            joint->feature.volumes.size()
        );
    }

    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

---

## Cross Joints {#ex_9_cross_joints}

Solves half-lap cross joints between intersecting plates using cross-only and face-then-cross contact search modes.

![](ex_9_cross_joints.webp)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::cross_corners};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.settings.joint_volume_extension = {0, 2, 0};

    wood_session.compute_features(cross_joint);
    for (const InteractionFeaturePlate& joint : wood_session.get_plate_features())
        std::cout << fmt::format(
            "{} type {}: side faces ({},{}) and ({},{}), volumes {}\n",
            joint.name,
            joint.joint_type,
            joint.contact.face_a,
            joint.cross_faces[0],
            joint.contact.face_b,
            joint.cross_faces[1],
            joint.joint_volumes[0].has_value() + joint.joint_volumes[1].has_value()
        );

    wood_session.compute_features(face_to_face_then_cross);
    std::cout << wood_session.get_plate_features().size()
              << " joints with face-to-face first and cross as the fallback\n";

    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

---

## Profile Cuts {#ex_10_profile_cuts}

Cuts rectangular timber plates with extruded 2D polyline profiles including concave L-outlines, interior holes, through-slots, and blind pockets.

![](ex_10_profile_cuts.webp)

```cpp
#include "cutting_gallery.h"
using namespace cutting_gallery;

static std::shared_ptr<Plate> stock() {

    return Plate::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 200, 160, 35);
}

int main() {
    WoodSession scene("10_profile_cuts");
    const Polyline concave(
        {{20, 20, -5},
         {180, 20, -5},
         {180, 65, -5},
         {80, 65, -5},
         {80, 140, -5},
         {20, 140, -5},
         {20, 20, -5}}
    );
    const std::shared_ptr<Joint> l_profile = std::make_shared<Joint>(concave, Vector(0, 0, 45));
    cut_case(scene, "Concave L profile / intersection", stock(), l_profile, 0);

    const Polyline outer = rectangle(15, 15, -5, 170, 130);
    const Polyline hole = rectangle(65, 55, -5, 70, 50);
    const std::shared_ptr<Joint> holed =
        std::make_shared<Joint>(std::vector<Polyline>{outer, hole}, Vector(0, 0, 45));
    cut_case(scene, "Profile with a hole / intersection", stock(), holed, 260);

    const Polyline slot_outline = rectangle(85, -10, -5, 30, 180);
    const std::shared_ptr<Joint> slot = std::make_shared<Joint>(
        std::vector<Polyline>{slot_outline}, Vector(0, 0, 45), SolidOperation::subtract
    );
    cut_case(scene, "Through slot / two separate pieces", stock(), slot, 520);

    const Polyline pocket_outline = concave.translated({0, 0, 25});
    const std::shared_ptr<Joint> pocket = std::make_shared<Joint>(
        std::vector<Polyline>{pocket_outline}, Vector(0, 0, 30), SolidOperation::subtract
    );
    cut_case(scene, "Concave pocket / solid split", stock(), pocket, 780);
    finish(scene, "10_profile_cuts");
}
```

---

## Drill Solids {#ex_11_drill_solids}

Compares chord-tolerance-controlled cylindrical drill meshes against exact rational NURBS bore BReps and cuts a tilted bore through a timber block.

![](ex_11_drill_solids.webp)

```cpp
#include "cutting_gallery.h"
using namespace cutting_gallery;

int main() {
    WoodSession scene("11_drill_solids");
    int row = 0;
    for (double tolerance : {2.0, 0.2, 0.02}) {
        const Line axis = Line::from_points({0, double(row * 180), 0}, {0, double(row * 180), 100});
        const std::shared_ptr<Joint> drill = Joint::drill(axis, 35, tolerance);
        drill->is_visible = true;
        drill->name = fmt::format(
            "Mesh: tolerance {}, {} segments", tolerance, circle_segments(35, tolerance)
        );
        drill->compute_geometry_mesh();
        Mesh drill_mesh = drill->geometry_mesh();
        drill_mesh.set_objectcolor(CUTTER_COLOR);
        drill->set_geometry(drill_mesh);
        scene.add(drill);
        BRep exact =
            drill->element_geometry_brep().transformed(Xform::translation(180, 0, 0));
        exact.surfacecolor = RESULT_COLOR;
        scene.add(std::make_shared<Element>(exact, "Exact cylindrical BRep"));
        ++row;
    }
    const std::shared_ptr<Block> stock = std::make_shared<Block>(
        std::vector<Polyline>{rectangle(0, 0, 0, 200, 160), rectangle(0, 0, 80, 200, 160)}
    );
    cut_case(
        scene,
        "Tilted drill through block",
        stock,
        Joint::drill(Line::from_points({60, 80, -30}, {145, 80, 130}), 28, 0.2),
        640
    );
    finish(scene, "11_drill_solids");
}
```

---

## Solid Booleans {#ex_12_solid_features}

Applies 3D polyhedral boolean difference, intersection, union, and tetrahedral mesh cutters to sloped and lofted timber blocks.

![](ex_12_solid_features.webp)

```cpp
#include "cutting_gallery.h"
using namespace cutting_gallery;

int main() {
    WoodSession scene("12_solid_features");
    int row = 0;
    for (const std::pair<SolidOperation, std::string>& design :
         std::vector<std::pair<SolidOperation, std::string>>{
             {SolidOperation::subtract, "Difference"},
             {SolidOperation::intersect, "Intersection"},
             {SolidOperation::add, "Union"}
         }) {
        const std::shared_ptr<Block> stock = std::make_shared<Block>(
            std::vector<Polyline>{rectangle(0, 0, 0, 200, 160), rectangle(30, 15, 100, 140, 130)}
        );
        const std::shared_ptr<Joint> cutter = std::make_shared<Joint>(std::vector<Polyline>{
            rectangle(90, -20, 20, 130, 190), rectangle(60, -5, 130, 130, 190)
        });
        cutter->operation = design.first;
        cut_case(scene, design.second + " / sloped solids", stock, cutter, row++ * 280);
    }
    const Mesh tetrahedron = Mesh::from_vertices_and_faces(
        {{40, 30, -10}, {190, 30, -10}, {115, 150, -10}, {115, 80, 130}},
        {{0, 2, 1}, {0, 1, 3}, {1, 2, 3}, {2, 0, 3}}
    );
    const Polyline block_bottom = rectangle(0, 0, 0, 200, 160);
    const Polyline block_top = rectangle(0, 0, 80, 200, 160);
    const std::shared_ptr<Block> block =
        std::make_shared<Block>(Mesh::loft({block_bottom}, {block_top}));
    cut_case(
        scene,
        "Custom mesh cutter / tetrahedron",
        block,
        std::make_shared<Joint>(tetrahedron),
        row * 280
    );
    finish(scene, "12_solid_features");
}
```
