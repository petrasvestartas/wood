# Examples {#examples}

[TOC]

Self-contained example targets in `examples/*.cpp`. Build and run any target with `cmake --build build --target <name> && ./build/<name>`.

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#ex_1_session_contacts">
    <img src="wood_joint_unit_box.webp" alt="Session and Joinery">
    <span class="wood-card-title">Session and Joinery</span>
  </a>
  <a class="wood-card" href="#ex_cutting_gallery">
    <img src="wood_boolean_cutting.webp" alt="Cutting Gallery">
    <span class="wood-card-title">Cutting Gallery</span>
  </a>
  <a class="wood-card" href="#ex_20_butt_frame">
    <img src="20_butt_frame.png" alt="Butt Frame and House">
    <span class="wood-card-title">Butt Frame and House</span>
  </a>
  <a class="wood-card" href="#ex_22_butt_pairs">
    <img src="butt_brace_frame.png" alt="Butt Pairs and Assemblies">
    <span class="wood-card-title">Butt Pairs and Assemblies</span>
  </a>
</div>
<div style="display:none">
![](diagrams/wood_joint_unit_box.webp)
![](diagrams/wood_boolean_cutting.webp)

- @subpage cutting_gallery
</div>

## Session Contacts {#ex_1_session_contacts}

Computes face contacts between timber elements in a `WoodSession`, orients a parametric `JointPlate` to the contact zone, and attaches cutting interactions to both plates.

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

## Hard-Coded Contacts

Constructs explicit `InteractionContactFace` records directly in code without running automatic contact detection.

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

    wood_session.add(plate0);
    wood_session.add(plate1);
    wood_session.add(beam);
    wood_session.add(column);
    wood_session.add(block);

    std::cout << wood_session << "\n";

    // Interaction - Simple Contact:
    std::shared_ptr<InteractionContactFace> contact = std::make_shared<InteractionContactFace>(
        5,
        5,
        ContactType::end_side,
        Polyline::rectangle({900, 0, 850}, {0, 1, 0}, {0, 0, 1}, 100, 100)
    );
    wood_session.add_interaction(column, beam, contact);

    // Interaction - Plate Contact:
    const Line alignment = Line::from_points({170, 300, 40}, {170, 0, 40});
    const Polyline volume0({{150, 0, 40}, {190, 0, 40}, {190, 0, 0}, {150, 0, 0}, {150, 0, 40}});
    const Polyline volume1(
        {{150, 300, 40}, {190, 300, 40}, {190, 300, 0}, {150, 300, 0}, {150, 300, 40}}
    );
    std::shared_ptr<InteractionContactFace> contact_with_volume =
        std::make_shared<InteractionContactFace>(
            1,
            5,
            ContactType::side_top,
            Polyline::rectangle({150, 0, 40}, {1, 0, 0}, {0, 1, 0}, 40, 300),
            std::array<Line, 2>{alignment, alignment},
            std::array<Polyline, 4>{volume0, volume1, volume0, volume1}
        );
    wood_session.add_interaction(plate0, plate1, contact_with_volume);

    contact_with_volume->flip();
    joint->orient(contact_with_volume);
    wood_session.add(joint);

    // Apply the joint features to each plate:
    wood_session.add_interaction(plate1, plate0, contact_with_volume);
    wood_session.add_interaction(joint, plate1, joint->interaction_feature(0));
    wood_session.add_interaction(joint, plate0, joint->interaction_feature(1));

    // Get element and model geometry:
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

## Elements Tree

Organizes repeated structural bays into separate scene-tree branches so contact detection stays scoped within each branch and shared definitions are instanced with `instance_by_key`.

```cpp
#include "wood_session.h"
#include "src/templates/grid/grid.h"

using namespace session_cpp;
using namespace wood_session;

const std::vector<double> XS = {4000.0, 4000.0, 4000.0};
const std::vector<double> YS = {3000.0, 3000.0};
const std::vector<Polyline> FOOTPRINT = {Polyline(
    {Point(0.0, 0.0, 0.0),
     Point(12000.0, 0.0, 0.0),
     Point(12000.0, 3000.0, 0.0),
     Point(8000.0, 3000.0, 0.0),
     Point(8000.0, 6000.0, 0.0),
     Point(0.0, 6000.0, 0.0),
     Point(0.0, 0.0, 0.0)}
)};
const std::vector<double> ELEVATIONS = {0.0, 3000.0, 6000.0};
const wood_grid::Framing FRAMING{
    .system = 1,
    .span = -1,
    .node = 0,
    .deck = 200.0,
    .reach = 200.0,
    .profiles =
        {.column = profile_rectangle(200.0, 200.0), .girder = profile_rectangle(200.0, 200.0)}
};
const bool INSTANCES = false;

/// The L of five bays, two storeys of post and beam, its tree a level per storey and a group per kind.
int main() {

    const wood_grid::Building building = wood_grid::Building::from_footprint(
        FOOTPRINT, ELEVATIONS, wood_grid::Pattern::orthogonal(XS, YS)
    );
    wood_grid::Grid grid(building, FRAMING, "elements_tree");

    if constexpr (INSTANCES)
        grid.instance_by_key();

    std::cout << grid;
    grid.pb_dump(pb_path("live"));

    return 0;
}
```

## Datasets

Loads timber plate and mesh datasets from YAML, OBJ, and `.pb` scene files into `WoodSession`.

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::hex_block_rossiniere};

int main() {

    WoodSession from_yaml = WoodSession::yaml_load(DATASET);
    std::cout << from_yaml << " distance " << from_yaml.settings.distance << " search "
              << from_yaml.settings.search_type << "\n";

    WoodSession from_obj = WoodSession::obj_load(DATASET);
    std::cout << from_obj << " plates from the obj alone, default settings\n";

    WoodSession from_pb = WoodSession::pb_load(config::session_pb(0));
    std::cout << from_pb << " from a session file\n";

    from_yaml.pb_dump(pb_path("live"));

    return 0;
}
```

## Contacts

Detects face, axis, and cross contacts across a hexagonal shell dataset and inspects the resulting contact edges in the session graph.

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

## Features

Runs the full plate joinery pipeline on a hexagonal shell, orienting finger joints across all shared edges and evaluating both uncut stock and boolean-cut plate meshes.

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

## Graph Traversal

Traverses `Session::graph` from a joint element to its connected plates, reading the feature interactions and underlying face contacts along each directed edge.

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

    return 0;
}
```

## Solver Settings

Configures solver tolerances, division counts, and joint parameters via `WoodSession::settings` and round-trips them through serialization.

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

## Custom Joint

Defines a custom joint profile as explicit male and female polyline outlines passed through the solver settings.

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const double SEAM_DEPTH = 2.0;
const double TOOTH_HALF = 1.6;
const double NECK_RATIO = 0.2917;

/// One butterfly tooth face in unit-cube space: y at the plate face, x signed across the seam, z along it.
static Polyline tooth(const double y, const double depth) {
    return Polyline(
        {Point(0.0, y, TOOTH_HALF * NECK_RATIO),
         Point(depth, y, TOOTH_HALF),
         Point(depth, y, -TOOTH_HALF),
         Point(0.0, y, -TOOTH_HALF * NECK_RATIO)}
    );
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
        std::vector<Polyline>{tooth(-0.5, -SEAM_DEPTH), tooth(0.5, -SEAM_DEPTH)},
        std::vector<Polyline>{tooth(-0.5, SEAM_DEPTH), tooth(0.5, SEAM_DEPTH)},
    };

    wood_session.compute_features(face_to_face);
    for (const InteractionFeaturePlate& joint : wood_session.get_plate_features())
        std::cout << joint.name << " with " << joint.male_outlines[0].front().point_count()
                  << " points per male outline\n";

    wood_session.pb_dump(pb_path("live"));

    return 0;
}
```

## Spatial Joint Assignment

Assigns joint types and insertion directions to plate faces from geometric marker points and direction lines placed on the elements.

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession wood_session("assignment");
    wood_session.add(
        Plate::from_rectangle(Point(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, 40)
    );
    wood_session.add(
        Plate::from_rectangle(Point(1000, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), 1000, 500, 40)
    );

    wood_session.assign_joint_types_by_points(
        {Point(1000, 250, 0), Point(500, 250, 40)}, {3, -40}, 1.0
    );
    wood_session.assign_insertion_vectors_by_lines(
        {Line::from_points(Point(1000, 250, 0), Point(1000, 250, 300))}, 1.0
    );

    for (const std::shared_ptr<Plate>& plate : wood_session.plates()) {
        std::cout << plate->name << " feature types:";
        for (int type : plate->feature_types)
            std::cout << " " << type;
        std::cout << " | insertion:";
        for (const Vector& v : plate->insertion_vectors())
            std::cout << " (" << v[0] << "," << v[1] << "," << v[2] << ")";
        std::cout << "\n";
    }

    wood_session.compute_features(face_to_face);
    std::cout << wood_session.get_plate_features().size() << " joints from the assigned types\n";

    return 0;
}
```

## Beam Contacts

Computes axis contacts between crossing and meeting timber beams and generates the oriented contact volume boxes for each beam pair.

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

## Serialization

Serializes elements, interactions, and complete `WoodSession` scenes to both JSON and protobuf bytes and reconstructs them identically.

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::inplane_hexshell};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_face_contacts();
    wood_session.compute_features();

    const std::string bytes = wood_session.pb_dumps();
    const WoodSession back = WoodSession::pb_loads(bytes);
    std::cout << back.interactions.size() << " interactions and " << back.get_features().size()
              << " features read back, consistent " << back.consistent() << "\n";

    const Session kernel = Session::pb_loads(bytes);
    std::cout << "the kernel opens the same bytes: " << kernel.objects.elements->size()
              << " elements, " << kernel.graph.number_of_edges() << " edges\n";

    const InteractionFeaturePlate joint = wood_session.get_plate_features().front();
    const nlohmann::ordered_json json = joint.jsondump();
    std::cout << "one joint as JSON: type " << json["interaction_type"] << ", name " << json["name"]
              << ", " << json.size() << " keys\n";
    std::cout << "and back: " << *Interaction::file_json_loads(joint.file_json_dumps()) << "\n";

    std::cout << "one contact as protobuf: "
              << wood_session.get_contacts().front()->pb_dumps().size() << " bytes\n";
    std::cout << "the settings as JSON: distance " << wood_session.settings.jsondump()["distance"]
              << "\n";

    return 0;
}
```

## Viewer Output

Arranges scene layers, color tables, and `.pb` outputs for interactive inspection in `session_viewer`.

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

const std::string DATASET{config::Dataset::annen_corner};

int main() {

    WoodSession wood_session = WoodSession::yaml_load(DATASET);
    wood_session.compute_face_contacts();
    wood_session.compute_features();

    const std::shared_ptr<Plate> plate = wood_session.plates().front();
    for (const ElementFeature& feature : plate->Element::features())
        std::cout << fmt::format(
            "{} {} on face {}: {} outlines\n",
            feature.feature_type,
            feature.name,
            feature.face_index,
            feature.outlines.size()
        );

    wood_session.pb_dump(pb_path("live"));
    wood_session.set_features_visible("contact", false);
    wood_session.pb_dump(pb_path(wood_session.name));

    return 0;
}
```

## Cross Joints

Solves cross-lap joints between intersecting plates using face and cross contact search modes.

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

## Cutting Gallery {#ex_cutting_gallery}

See [Cutting Gallery](@ref cutting_gallery) for cutter specifications.

### Profile Cuts {#ex_15_profile_cuts}

Cuts plates with extruded 2D polyline profiles including concave L-outlines, interior holes, through-slots, and blind pockets.

```cpp
#include "cutting_gallery.h"
using namespace cutting_gallery;

static std::shared_ptr<Plate> stock() {

    return Plate::from_rectangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, 200, 160, 35);
}

int main() {
    WoodSession scene("15_profile_cuts");
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
    finish(scene, "15_profile_cuts");
}
```

### Drill Solids {#ex_16_drill_solids}

Compares chord-tolerance-controlled cylindrical drill meshes against exact rational NURBS bore BReps and cuts a tilted bore through a solid block.

```cpp
#include "cutting_gallery.h"
using namespace cutting_gallery;

int main() {
    WoodSession scene("16_drill_solids");
    int row = 0;
    for (double tolerance : {2.0, 0.2, 0.02}) {
        const Line axis = Line::from_points({0, double(row * 180), 0}, {0, double(row * 180), 100});
        const std::shared_ptr<Joint> drill = Joint::drill(axis, 35, tolerance);
        drill->name = fmt::format(
            "Mesh: tolerance {}, {} segments", tolerance, circle_segments(35, tolerance)
        );
        scene.add(drill);
        const BRep exact =
            drill->element_geometry_brep().transformed(Xform::translation(180, 0, 0));
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
    finish(scene, "16_drill_solids");
}
```

### Solid Features {#ex_17_solid_features}

Applies 3D boolean difference, intersection, union, and custom tetrahedral mesh cutters to timber stock blocks.

```cpp
#include "cutting_gallery.h"
using namespace cutting_gallery;

int main() {
    WoodSession scene("17_solid_features");
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
    finish(scene, "17_solid_features");
}
```

### Combined Cutting Gallery {#ex_18_cutting_gallery}

Combines the profile, drill, and solid cutting scenes side-by-side into a single gallery scene for `session_viewer`.

```cpp
#include "cutting_gallery.h"
using namespace cutting_gallery;

int main() {
    WoodSession gallery("18_cutting_gallery");
    int column = 0;
    for (const std::string file : {"15_profile_cuts", "16_drill_solids", "17_solid_features"}) {
        WoodSession scene = WoodSession::pb_load(pb_path(file));
        const Xform offset = Xform::translation(column++ * 1100, 0, 0);
        std::map<std::string, std::shared_ptr<TreeNode>> groups;
        for (const std::shared_ptr<Element>& element : *scene.objects.elements) {
            const std::shared_ptr<TreeNode> node = scene.get_node(element->guid());
            const std::shared_ptr<TreeNode> parent = node ? node->parent() : nullptr;
            const std::string label = file + " / " + (parent ? parent->name : "Drill tolerances");
            if (!groups.count(label))
                groups[label] = gallery.add_group(label);
            element->place(offset);
            if (element->name == "Result" || element->name.starts_with("Cutter")) {
                element->compute_geometry_mesh();
                Mesh mesh = element->geometry_mesh();
                mesh.set_objectcolor(
                    element->name == "Result" ? Color(0.24, 0.58, 0.7) : Color(0.9, 0.35, 0.2)
                );
                element->set_geometry(mesh);
            }
            gallery.add(element, groups.at(label));
        }
        std::unordered_set<std::string> copied;
        for (const std::pair<const std::string, std::map<std::string, Edge>>& source :
             scene.graph.edges)
            for (const std::pair<const std::string, Edge>& target : source.second) {
                const Edge& edge = target.second;
                if (!copied.insert(edge.guid()).second)
                    continue;
                const auto records = scene.interactions.find(edge.guid());
                if (records == scene.interactions.end())
                    continue;
                for (const std::shared_ptr<Interaction>& record : records->second)
                    gallery.Session::add_interaction(
                        gallery.get_element<Element>(source.first),
                        gallery.get_element<Element>(target.first),
                        record
                    );
            }
    }
    finish(gallery, "18_cutting_gallery");
}
```

### Plate Joint Library {#ex_19_plate_joint_library}

Instantiates six `top_side` plate joint presets side-by-side across different division counts and shift parameters.

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

int main() {

    WoodSession scene("19_plate_joint_library");
    const std::vector<std::shared_ptr<JointPlate>> joints = {
        JointPlate::top_side_three_tenons(),
        JointPlate::top_side_annen_tenons(),
        JointPlate::top_side_dense_tenons(8, 0.5),
        JointPlate::top_side_tenons(8, 0.5),
        JointPlate::top_side_tenons(16, 0.25),
        JointPlate::top_side_snap_fit(4)
    };

    for (size_t i = 0; i < joints.size(); ++i) {
        const double offset = i * 600.0;
        const std::shared_ptr<Plate> bottom =
            Plate::from_rectangle({offset, 0, 0}, {1, 0, 0}, {0, 1, 0}, 400, 300, 40);
        const std::shared_ptr<Plate> upright =
            Plate::from_rectangle({offset + 150, 0, 40}, {0, 1, 0}, {0, 0, 1}, 300, 250, 40);
        const std::shared_ptr<TreeNode> group =
            scene.add_group(joints[i]->parameters.library + " / " + std::to_string(i));
        scene.add(bottom, group);
        scene.add(upright, group);
        const std::shared_ptr<InteractionContactFace> contact =
            scene.compute_face_contact(bottom, upright);
        joints[i]->orient(contact, {bottom, upright});
        scene.add(joints[i], group);
        scene.add_interaction(joints[i], upright, joints[i]->interaction(0));
        scene.add_interaction(joints[i], bottom, joints[i]->interaction(1));
    }

    scene.pb_dump(pb_path("19_plate_joint_library"));
    scene.pb_dump(pb_path("live"));

    return 0;
}
```

## Butt Joints {#ex_butt_joints}

Members drawn as lines and mid-planes overlap where they meet. `JointBeam::butt(a, b)` detects how their ends meet and cuts each member along a shared contact plane so they meet face to face (@ref elements_joint_beam_butt). Trusses are generated by the [Truss](@ref templates_truss) template.

### Braced Butt Frame {#ex_20_butt_frame}

Builds a braced timber-frame wall from centerlines with sill, head, posts, and diagonal braces trimmed flat against each other.

![](examples/20_butt_frame.png)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A timber-frame wall from lines meeting at nodes: sill, head, three posts and two braces, every member butted flat on another.
int main() {

    WoodSession scene("butt_frame");

    // 140 x 140 beams on axes drawn node to node: a 3000 wide, 2400 high wall
    const double radius = 70.0;
    const std::shared_ptr<Beam> sill =
        std::make_shared<Beam>(Polyline({{0.0, 0.0, 0.0}, {3000.0, 0.0, 0.0}}), radius, "sill");
    const std::shared_ptr<Beam> head = std::make_shared<Beam>(
        Polyline({{0.0, 0.0, 2400.0}, {3000.0, 0.0, 2400.0}}), radius, "head"
    );
    const std::shared_ptr<Beam> left_post = std::make_shared<Beam>(
        Polyline({{0.0, 0.0, 0.0}, {0.0, 0.0, 2400.0}}), radius, "left_post"
    );
    const std::shared_ptr<Beam> middle_post = std::make_shared<Beam>(
        Polyline({{1500.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}}), radius, "middle_post"
    );
    const std::shared_ptr<Beam> right_post = std::make_shared<Beam>(
        Polyline({{3000.0, 0.0, 0.0}, {3000.0, 0.0, 2400.0}}), radius, "right_post"
    );
    const std::shared_ptr<Beam> left_brace = std::make_shared<Beam>(
        Polyline({{0.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}}), radius, "left_brace"
    );
    const std::shared_ptr<Beam> right_brace = std::make_shared<Beam>(
        Polyline({{3000.0, 0.0, 0.0}, {1500.0, 0.0, 2400.0}}), radius, "right_brace"
    );
    for (const std::shared_ptr<Beam>& beam :
         {sill, head, left_post, middle_post, right_post, left_brace, right_brace}) {
        scene.add(beam);

        std::shared_ptr<Polyline> axis = std::make_shared<Polyline>(beam->axis);
        axis->linecolor = Color(0.85f, 0.1f, 0.1f);
        axis->width = 4.0;
        scene.add_polyline(axis, scene.group_named("input"));
    }

    // the posts on the sill and under the head, which run on over the corner posts: butts read from the drawn members
    const std::vector<std::array<std::shared_ptr<Beam>, 2>> posts = {
        {sill, left_post},
        {sill, middle_post},
        {sill, right_post},
        {head, left_post},
        {head, middle_post},
        {head, right_post},
    };
    for (const std::array<std::shared_ptr<Beam>, 2>& pair : posts) {
        const std::shared_ptr<JointBeam> butt = JointBeam::butt(*pair[0], *pair[1]);
        butt->name = fmt::format("{}_{}_{}", butt->name, pair[0]->name, pair[1]->name);
        scene.add(butt);
        scene.add_interaction(butt, pair[0], butt->interaction(0));
        scene.add_interaction(butt, pair[1], butt->interaction(1));
    }

    const std::vector<std::array<std::shared_ptr<Beam>, 2>> braces = {
        {sill, left_brace},
        {left_post, left_brace},
        {head, left_brace},
        {middle_post, left_brace},
        {sill, right_brace},
        {right_post, right_brace},
        {head, right_brace},
        {middle_post, right_brace},
    };
    for (const std::array<std::shared_ptr<Beam>, 2>& pair : braces) {
        const std::shared_ptr<JointBeam> top_side = JointBeam::butt_top_side(*pair[0], *pair[1]);
        top_side->name = fmt::format("{}_{}_{}", top_side->name, pair[0]->name, pair[1]->name);
        scene.add(top_side);
        scene.add_interaction(top_side, pair[0], top_side->interaction(0));
        scene.add_interaction(top_side, pair[1], top_side->interaction(1));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}
```

### Plate Butt House {#ex_21_butt_house}

Constructs a small plate-framed house from floor, wall, and roof mid-planes joined by automatic butt cuts.

![](examples/21_butt_house.png)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A small timber house from plates drawn on their mid-planes.
int main() {

    WoodSession scene("butt_house");

    const std::array<Point, 4> footprint = {
        Point(0.0, 0.0, 0.0),
        Point(6000.0, 0.0, 0.0),
        Point(6000.0, 4000.0, 0.0),
        Point(0.0, 4000.0, 0.0)
    };
    const std::shared_ptr<Plate> floor = Plate::from_rectangle(
        {0.0, 0.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "floor"
    );
    const std::shared_ptr<Plate> roof = Plate::from_rectangle(
        {0.0, 0.0, 2700.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6000.0, 4000.0, 200.0, "roof"
    );
    std::array<std::shared_ptr<Plate>, 4> walls;
    for (size_t i = 0; i < 4; i++)
        walls[i] = Plate::from_line(
            Line::from_points(footprint[i], footprint[(i + 1) % 4]),
            {0.0, 0.0, 1.0},
            2800.0,
            120.0,
            fmt::format("wall_{}", i)
        );
    scene.add(floor);
    scene.add(roof);
    for (const std::shared_ptr<Plate>& wall : walls)
        scene.add(wall);

    for (const std::shared_ptr<Plate>& plate :
         {floor, roof, walls[0], walls[1], walls[2], walls[3]}) {
        std::vector<Point> points;
        for (size_t i = 0; i < plate->polylines[0].point_count(); i++)
            points.push_back(
                plate->polylines[0][i] + (plate->polylines[1][i] - plate->polylines[0][i]) * 0.5
            );
        std::shared_ptr<Polyline> outline = std::make_shared<Polyline>(points);
        outline->linecolor = Color(0.85f, 0.1f, 0.1f);
        outline->width = 4.0;
        scene.add_polyline(outline, scene.group_named("input"));
    }

    // the butts, running plate first, each on what the earlier ones left: the floor and the roof run on under and over every wall, then the
    const std::vector<std::array<std::shared_ptr<Plate>, 2>> pairs = {
        {floor, walls[0]},
        {floor, walls[1]},
        {floor, walls[2]},
        {floor, walls[3]},
        {roof, walls[0]},
        {roof, walls[1]},
        {roof, walls[2]},
        {roof, walls[3]},
        {walls[0], walls[1]},
        {walls[0], walls[3]},
        {walls[2], walls[1]},
        {walls[2], walls[3]},
    };
    for (const std::array<std::shared_ptr<Plate>, 2>& pair : pairs) {
        const std::shared_ptr<JointBeam> butt = JointBeam::butt(*pair[0], *pair[1]);
        butt->name = fmt::format("butt_{}_{}", pair[0]->name, pair[1]->name);
        scene.add(butt);
        scene.add_interaction(butt, pair[0], butt->interaction(0));
        scene.add_interaction(butt, pair[1], butt->interaction(1));
    }

    std::cout << scene << std::endl;
    scene.pb_dump(pb_path("live"));
    return 0;
}
```

### Pairwise Butt Configurations {#ex_22_butt_pairs}

Tests every pairwise beam, column, and plate butt configuration (`top_side`, `side_side` out-of-plane and in-plane, and `cross`) and writes `butt_pair_<case>.pb` for each pair.

#### Beam Top Side
A post landing square on a continuous beam.

![](examples/butt_pair_beam_top_side.png)

#### Beam Top Side Oblique
A strut meeting a continuous beam at 60 degrees.

![](examples/butt_pair_beam_top_side_oblique.png)

#### Beam Top Side End
Two beams meeting at their ends, where the first runs through and the second stops against its side face.

![](examples/butt_pair_beam_top_side_end.png)

#### Beam Top Side End Past
Two corner beams whose centerlines extend past the corner intersection before trimming.

![](examples/butt_pair_beam_top_side_end_past.png)

#### Beam Top Side End Short
Two corner beams whose centerlines stop short of the corner intersection and extend to meet cleanly.

![](examples/butt_pair_beam_top_side_end_short.png)

#### Beam Side Side Outplane
Two beams meeting at 120 degrees, cut on the bisector miter plane.

![](examples/butt_pair_beam_side_side_outplane.png)

#### Beam Side Side Outplane Square
Two beams meeting at 90 degrees, mitered on the 45-degree bisector by `butt_side_side`.

![](examples/butt_pair_beam_side_side_outplane_square.png)

#### Beam Side Side Inplane
Two collinear beams spliced end to end.

![](examples/butt_pair_beam_side_side_inplane.png)

#### Beam Cross
Two crossing beams joined with a half-lap cut.

![](examples/butt_pair_beam_cross.png)

#### Plate Top Side
An interior wall partitioning another wall along its mid-span.

![](examples/butt_pair_plate_top_side.png)

#### Plate Top Side End
Two walls meeting at an L-corner, where the first wall runs through.

![](examples/butt_pair_plate_top_side_end.png)

#### Plate Side Side Outplane
Two walls meeting at 120 degrees, trimmed on the bisector miter plane.

![](examples/butt_pair_plate_side_side_outplane.png)

#### Plate Side Side Inplane
Two coplanar walls joined edge to edge in line.

![](examples/butt_pair_plate_side_side_inplane.png)

#### Plate Top Side Floor
A vertical wall standing on a horizontal floor slab.

![](examples/butt_pair_plate_top_side_floor.png)

#### Plate Cross
Two crossing walls left uncut at their intersection.

![](examples/butt_pair_plate_cross.png)

#### Column Top Side Plate
A square column supporting a horizontal floor slab.

![](examples/butt_pair_column_top_side_plate.png)

#### Plate Top Side Beam
A wall panel bearing on top of a timber beam.

![](examples/butt_pair_plate_top_side_beam.png)

#### Beam Top Side Plate
A beam framing into a wall panel at 60 degrees in plan.

![](examples/butt_pair_beam_top_side_plate.png)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Drawing
// ═══════════════════════════════════════════════════════════════════════════

/// The user's line, as drawn: red over the computed solids, in the input group.
static void draw_line(WoodSession& scene, const Polyline& line) {

    std::shared_ptr<Polyline> drawn = std::make_shared<Polyline>(line);
    drawn->linecolor = Color(0.85f, 0.1f, 0.1f);
    drawn->width = 4.0;
    scene.add_polyline(drawn, scene.group_named("input"));
}

/// A member added with the line or the polygon it was drawn on.
static void add_drawn(WoodSession& scene, const std::shared_ptr<Element>& member) {

    scene.add(member);
    if (const std::shared_ptr<Beam> beam = std::dynamic_pointer_cast<Beam>(member))
        draw_line(scene, beam->axis);
    if (const std::shared_ptr<Column> column = std::dynamic_pointer_cast<Column>(member))
        draw_line(scene, Polyline({column->axis.start(), column->axis.end()}));
    if (const std::shared_ptr<Plate> plate = std::dynamic_pointer_cast<Plate>(member)) {
        std::vector<Point> points;
        for (size_t i = 0; i < plate->polylines[0].point_count(); i++)
            points.push_back(
                plate->polylines[0][i] + (plate->polylines[1][i] - plate->polylines[0][i]) * 0.5
            );
        draw_line(scene, Polyline(points));
    }
}

/// A pair, its butt or side_side butt passed to both, written to data/output/pb/butt_pair_<name>.pb.
static void pair(
    const std::string& name,
    const std::shared_ptr<Element>& a,
    const std::shared_ptr<Element>& b,
    bool side_side = false
) {

    WoodSession scene("butt_pair_" + name);
    add_drawn(scene, a);
    add_drawn(scene, b);
    const std::shared_ptr<JointBeam> butt =
        side_side ? JointBeam::butt_side_side(*a, *b) : JointBeam::butt(*a, *b);
    butt->name = fmt::format("{}_{}_{}", butt->name, a->name, b->name);
    scene.add(butt);
    scene.add_interaction(butt, a, butt->interaction(0));
    scene.add_interaction(butt, b, butt->interaction(1));
    scene.pb_dump(pb_path("butt_pair_" + name));
}

/// A beam of 150 square on a line.
static std::shared_ptr<Beam> beam(const Point& start, const Point& end, const std::string& name) {

    return std::make_shared<Beam>(Polyline({start, end}), 75.0, name);
}

/// A wall 1500 high and 120 thick standing on a plan line.
static std::shared_ptr<Plate> wall(const Point& start, const Point& end, const std::string& name) {

    return Plate::from_line(
        Line::from_points(start, end), Vector(0.0, 0.0, 1.0), 1500.0, 120.0, name
    );
}

/// The plan point at angle_degrees from x, length away.
static Point polar(double angle_degrees, double length) {

    return Point(
        length * std::cos(angle_degrees * Tolerance::TO_RADIANS),
        length * std::sin(angle_degrees * Tolerance::TO_RADIANS),
        0.0
    );
}

/// Butt pairs, beams, plates and both.
int main() {

    // beams: top_side square and at 60 degrees, top_side at an end drawn meeting, past and short, side_side_outplane at 120 and 90 degrees, side_side_inplane, a cross
    pair(
        "beam_top_side",
        beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}, "through"),
        beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0}, "post")
    );
    pair(
        "beam_top_side_oblique",
        beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}, "through"),
        beam({0.0, 0.0, 0.0}, {750.0, 0.0, 1300.0}, "strut")
    );
    pair(
        "beam_top_side_end",
        beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "running"),
        beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0}, "butting")
    );
    pair(
        "beam_top_side_end_past",
        beam({-150.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "running"),
        beam({0.0, 0.0, -150.0}, {0.0, 0.0, 1500.0}, "butting")
    );
    pair(
        "beam_top_side_end_short",
        beam({100.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "running"),
        beam({0.0, 0.0, 100.0}, {0.0, 0.0, 1500.0}, "butting")
    );
    pair(
        "beam_side_side_outplane",
        beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "left"),
        beam({0.0, 0.0, 0.0}, {-750.0, 0.0, 1300.0}, "right")
    );
    pair(
        "beam_side_side_outplane_square",
        beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "left"),
        beam({0.0, 0.0, 0.0}, {0.0, 0.0, 1500.0}, "right"),
        true
    );
    pair(
        "beam_side_side_inplane",
        beam({-1500.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, "left"),
        beam({0.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "right")
    );
    pair(
        "beam_cross",
        beam({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}, "first"),
        beam({0.0, -1000.0, 0.0}, {0.0, 1000.0, 0.0}, "second")
    );

    // plates: top_side, top_side at an end, side_side_outplane at 120 degrees, side_side_inplane, a wall top_side on a floor, a cross
    pair(
        "plate_top_side",
        wall({-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "through"),
        wall({0.0, 0.0, 0.0}, {0.0, 2000.0, 0.0}, "butting")
    );
    pair(
        "plate_top_side_end",
        wall({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0}, "running"),
        wall({0.0, 0.0, 0.0}, {0.0, 2000.0, 0.0}, "butting")
    );
    pair(
        "plate_side_side_outplane",
        wall({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0}, "left"),
        wall({0.0, 0.0, 0.0}, polar(120.0, 2000.0), "right")
    );
    pair(
        "plate_side_side_inplane",
        wall({-2000.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, "left"),
        wall({0.0, 0.0, 0.0}, {2000.0, 0.0, 0.0}, "right")
    );
    pair(
        "plate_top_side_floor",
        Plate::from_rectangle(
            {-1500.0, -1500.0, -100.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            3000.0,
            3000.0,
            200.0,
            "floor"
        ),
        wall({-1000.0, 0.0, 0.0}, {1000.0, 0.0, 0.0}, "wall")
    );
    pair(
        "plate_cross",
        wall({-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "first"),
        wall({0.0, -1500.0, 0.0}, {0.0, 1500.0, 0.0}, "second")
    );

    // beams and plates, all top_side: a column under a slab, a wall on a beam, a beam into a wall at 60 degrees in plan
    pair(
        "column_top_side_plate",
        Plate::from_rectangle(
            {-1500.0, -1500.0, -100.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            3000.0,
            3000.0,
            200.0,
            "slab"
        ),
        Column::square(
            Line::from_points({0.0, 0.0, -2000.0}, {0.0, 0.0, 0.0}),
            Plane({-100.0, -100.0, -2000.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}),
            200.0,
            "column"
        )
    );
    pair(
        "plate_top_side_beam",
        std::make_shared<Beam>(Polyline({{-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}}), 100.0, "beam"),
        wall({-1200.0, 0.0, 0.0}, {1200.0, 0.0, 0.0}, "wall")
    );
    pair(
        "beam_top_side_plate",
        wall({-1500.0, 0.0, 0.0}, {1500.0, 0.0, 0.0}, "wall"),
        beam(
            {0.0, 0.0, 800.0},
            Point(0.0, 0.0, 800.0) + Vector(polar(-60.0, 1500.0)[0], polar(-60.0, 1500.0)[1], 0.0),
            "beam"
        )
    );
    return 0;
}
```

### Multi-Member Butt Assemblies {#ex_23_butt_assemblies}

Combines pairwise butt joints into complete multi-member assemblies (`butt_<assembly>.pb`).

#### Brace Frame
A post on a sill with a diagonal knee brace trimmed between them.

![](examples/butt_brace_frame.png)

#### Curved Sill
Two vertical posts landing on a segmented arc sill of eight beams.

![](examples/butt_curved_sill.png)

#### Polygon Floor
Five perimeter walls (one with a sloped top edge) standing on a pentagonal floor plate.

![](examples/butt_polygon_floor.png)

#### Zigzag Walls
Folded zigzag walls in plan standing on a flat floor plate.

![](examples/butt_zigzag_walls.png)

#### Folded Roof
A zigzag folded-plate roof spanning over two gable end walls with overhanging eaves.

![](examples/butt_folded_roof.png)

```cpp
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Drawing
// ═══════════════════════════════════════════════════════════════════════════

/// The user's line, as drawn: red over the computed solids, in the input group.
static void draw_line(WoodSession& scene, const Polyline& line) {

    std::shared_ptr<Polyline> drawn = std::make_shared<Polyline>(line);
    drawn->linecolor = Color(0.85f, 0.1f, 0.1f);
    drawn->width = 4.0;
    scene.add_polyline(drawn, scene.group_named("input"));
}

/// A member added with the line or the polygon it was drawn on: a beam's axis, a plate's mid-plane polygon.
static void add_drawn(WoodSession& scene, const std::shared_ptr<Element>& member) {

    scene.add(member);
    if (const std::shared_ptr<Beam> beam = std::dynamic_pointer_cast<Beam>(member))
        draw_line(scene, beam->axis);
    if (const std::shared_ptr<Plate> plate = std::dynamic_pointer_cast<Plate>(member)) {
        std::vector<Point> points;
        for (size_t i = 0; i < plate->polylines[0].point_count(); i++)
            points.push_back(
                plate->polylines[0][i] + (plate->polylines[1][i] - plate->polylines[0][i]) * 0.5
            );
        draw_line(scene, Polyline(points));
    }
}

/// The butt of a pair, named by its configuration and its members, passed to both.
static void
join(WoodSession& scene, const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b) {

    const std::shared_ptr<JointBeam> butt = JointBeam::butt(*a, *b);
    butt->name = fmt::format("{}_{}_{}", butt->name, a->name, b->name);
    scene.add(butt);
    scene.add_interaction(butt, a, butt->interaction(0));
    scene.add_interaction(butt, b, butt->interaction(1));
}

// ═══════════════════════════════════════════════════════════════════════════
// Assemblies
// ═══════════════════════════════════════════════════════════════════════════

/// A post on a sill.
static void brace_frame() {

    WoodSession scene("butt_brace_frame");
    const std::shared_ptr<Beam> sill =
        std::make_shared<Beam>(Polyline({{-800.0, 0.0, 0.0}, {1400.0, 0.0, 0.0}}), 70.0, "sill");
    const std::shared_ptr<Beam> post =
        std::make_shared<Beam>(Polyline({{0.0, 0.0, 0.0}, {0.0, 0.0, 2400.0}}), 70.0, "post");
    const std::shared_ptr<Beam> brace =
        std::make_shared<Beam>(Polyline({{0.0, 0.0, 1500.0}, {1050.0, 0.0, 0.0}}), 60.0, "brace");
    for (const std::shared_ptr<Beam>& beam : {sill, post, brace})
        add_drawn(scene, beam);
    join(scene, sill, post);
    join(scene, post, brace);
    join(scene, sill, brace);

    scene.pb_dump(pb_path("butt_brace_frame"));
}

/// Two posts on a curved sill.
static void curved_sill() {

    WoodSession scene("butt_curved_sill");
    std::vector<Point> arc;
    for (int i = 0; i <= 8; i++) {
        const double angle = 0.5 * Tolerance::PI * i / 8.0;
        arc.emplace_back(2000.0 * std::cos(angle), 2000.0 * std::sin(angle), 0.0);
    }
    const std::shared_ptr<Beam> sill = std::make_shared<Beam>(Polyline(arc), 70.0, "curved_sill");
    add_drawn(scene, sill);
    for (const int i : {2, 6}) {
        const Point foot = arc[i] + (arc[i + 1] - arc[i]) * 0.5;
        const std::shared_ptr<Beam> post = std::make_shared<Beam>(
            Polyline({foot, foot + Vector(0.0, 0.0, 1800.0)}), 60.0, fmt::format("post_{}", i)
        );
        add_drawn(scene, post);
        join(scene, sill, post);
    }

    scene.pb_dump(pb_path("butt_curved_sill"));
}

/// A pentagon floor, walls on three of its edges.
static void polygon_floor() {

    WoodSession scene("butt_polygon_floor");
    std::vector<Point> pentagon;
    for (int i = 0; i <= 5; i++) {
        const double angle = 2.0 * Tolerance::PI * (i % 5) / 5.0 - 0.5 * Tolerance::PI;
        pentagon.emplace_back(2000.0 * std::cos(angle), 2000.0 * std::sin(angle), 0.0);
    }
    const Polyline middle(pentagon);
    const std::shared_ptr<Plate> floor = std::make_shared<Plate>(
        middle.translated({0.0, 0.0, -100.0}), middle.translated({0.0, 0.0, 100.0}), "floor"
    );
    const std::shared_ptr<Plate> first = Plate::from_line(
        Line::from_points(pentagon[0], pentagon[1]), {0.0, 0.0, 1.0}, 2400.0, 140.0, "wall_0"
    );
    const std::shared_ptr<Plate> second = Plate::from_line(
        Line::from_points(pentagon[1], pentagon[2]), {0.0, 0.0, 1.0}, 2400.0, 140.0, "wall_1"
    );

    // the sloped wall on the edge before the first: its outline rises from 1800 to 2400 along the edge, thickness across it, centred
    const Vector along = (pentagon[0] - pentagon[4]).normalized();
    const Vector across = along.cross(Vector(0.0, 0.0, 1.0)) * 70.0;
    const Polyline sloped(
        {pentagon[4],
         pentagon[0],
         pentagon[0] + Vector(0.0, 0.0, 2400.0),
         pentagon[4] + Vector(0.0, 0.0, 1800.0),
         pentagon[4]}
    );
    const std::shared_ptr<Plate> third = std::make_shared<Plate>(
        sloped.translated(across * -1.0), sloped.translated(across), "wall_sloped"
    );
    for (const std::shared_ptr<Plate>& plate : {floor, first, second, third})
        add_drawn(scene, plate);
    for (const std::shared_ptr<Plate>& wall : {first, second, third})
        join(scene, floor, wall);
    join(scene, first, second);
    join(scene, third, first);

    scene.pb_dump(pb_path("butt_polygon_floor"));
}

/// Walls on a zigzag in plan.
static void zigzag_walls() {

    WoodSession scene("butt_zigzag_walls");
    const std::vector<Point> zigzag = {
        {0.0, 0.0, 0.0},
        {1500.0, 900.0, 0.0},
        {3000.0, 0.0, 0.0},
        {4500.0, 900.0, 0.0},
        {6000.0, 0.0, 0.0}
    };
    const std::shared_ptr<Plate> floor = Plate::from_rectangle(
        {-300.0, -600.0, -100.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 6600.0, 2100.0, 200.0, "floor"
    );
    add_drawn(scene, floor);
    std::vector<std::shared_ptr<Plate>> walls;
    for (size_t i = 0; i + 1 < zigzag.size(); i++) {
        walls.push_back(Plate::from_line(
            Line::from_points(zigzag[i], zigzag[i + 1]),
            {0.0, 0.0, 1.0},
            2400.0,
            140.0,
            fmt::format("wall_{}", i)
        ));
        add_drawn(scene, walls.back());
    }
    for (const std::shared_ptr<Plate>& wall : walls)
        join(scene, floor, wall);
    for (size_t i = 0; i + 1 < walls.size(); i++)
        join(scene, walls[i], walls[i + 1]);

    scene.pb_dump(pb_path("butt_zigzag_walls"));
}

/// A folded roof on a zigzag in section.
static void folded_roof() {

    WoodSession scene("butt_folded_roof");
    const std::vector<Point> zigzag = {
        {0.0, 0.0, 2400.0},
        {1500.0, 0.0, 3200.0},
        {3000.0, 0.0, 2400.0},
        {4500.0, 0.0, 3200.0},
        {6000.0, 0.0, 2400.0}
    };
    std::vector<std::shared_ptr<Plate>> roof;
    for (size_t i = 0; i + 1 < zigzag.size(); i++) {
        const Vector along = (zigzag[i + 1] - zigzag[i]).normalized();
        const Vector across = along.cross(Vector(0.0, 1.0, 0.0));

        const double before = i == 0 ? 300.0 / along[0] : 0.0;
        const double after = i + 2 == zigzag.size() ? 300.0 / along[0] : 0.0;
        roof.push_back(Plate::from_rectangle(
            zigzag[i] - along * before - across * 80.0,
            along,
            {0.0, 1.0, 0.0},
            Point::distance(zigzag[i], zigzag[i + 1]) + before + after,
            4000.0,
            160.0,
            fmt::format("roof_{}", i)
        ));
        add_drawn(scene, roof.back());
    }

    std::vector<Point> outline = {{6000.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    outline.insert(outline.end(), zigzag.begin(), zigzag.end());
    outline.push_back(outline.front());
    std::array<std::shared_ptr<Plate>, 2> gables;
    for (size_t g = 0; g < 2; g++) {
        const Polyline drawn = Polyline(outline).translated({0.0, g == 0 ? 0.0 : 4000.0, 0.0});
        gables[g] = std::make_shared<Plate>(
            drawn.translated({0.0, -70.0, 0.0}),
            drawn.translated({0.0, 70.0, 0.0}),
            fmt::format("gable_{}", g)
        );
        add_drawn(scene, gables[g]);
    }
    for (const std::shared_ptr<Plate>& plate : roof)
        for (const std::shared_ptr<Plate>& gable : gables)
            join(scene, plate, gable);
    for (size_t i = 0; i + 1 < roof.size(); i++)
        join(scene, roof[i], roof[i + 1]);

    scene.pb_dump(pb_path("butt_folded_roof"));
}

/// Butt pairs combined into assemblies, beams and plates drawn on lines and polygons, each its own scene.
int main() {

    brace_frame();
    curved_sill();
    polygon_floor();
    zigzag_walls();
    folded_roof();
    return 0;
}
```
