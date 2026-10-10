// The scenes of docs/joint_library.md, two session files per design for the viewer, never overlapping: <id>_unit.pb, the design's male and
// female outlines in its unit box, polylines since a plate joint is outlines merged into the plates', with <id>_unit.txt, its factory call with
// the parameters the user can change, the page's code snippet; and <id>.pb, the oracle's pair joined
// by it, the plates with the merged outlines and any solid the joint owns as BReps.
//   ./build/joint_tiles <out_dir> [family/library/parameters ... | beam/<design>/<angle> ...]     every oracle variant when no id is given
#define main joint_library_main
#include "../../tests/joint_library.cpp"
#undef main
#include "src/templates/folding/chevron.h"

namespace {
using namespace wood_session;
#include "wood_interaction_feature_plate_joints.h"
}

static const double BOX = 200.0; // mm, the unit box
static const double APART = 250.0; // mm, the second plate moved off the first
static const Color MALE = Color(0.807f, 0.063f, 0.258f);   // the viewer's x axis pink #E8478B, linear: the viewer encodes line colours to sRGB
static const Color FEMALE = Color(0.015f, 0.305f, 0.823f); // the viewer's z axis blue #2196EA, linear
static const Color EDGE = Color(0.6f, 0.6f, 0.6f);

// ═══════════════════════════════════════════════════════════════════════════
// The unit box
// ═══════════════════════════════════════════════════════════════════════════

/// The design's outlines in its unit box, the library function run on a joint with the built one's parameters; false for a design that is
/// built on the plates in their own space (top to top, side removal, custom) or leaves the box empty, which has no unit box to draw.
static bool unit_outlines(const Built& built, InteractionFeaturePlate& unit) {

    const InteractionFeaturePlate& built_connection = built.joint->connections.at(0);
    const JointPlateParameters& p = built.joint->parameters;
    // the built connection's lines, scale and parameters, which some designs read (ss_e_ip_2 its tooth pitch), its outlines cleared
    unit = built_connection;
    for (int face = 0; face < 2; face++) {
        unit.male_outlines[face].clear();
        unit.female_outlines[face].clear();
    }
    // a butterfly, key or snap-fit design tiles its teeth along the joint line far past the box: one tooth shows the design, the plates
    // show the divisions
    if (built.library == "ss_e_ip_2" || built.library == "ss_e_ip_5" || built.library == "ss_e_r_2" || built.library == "ss_e_r_3" || built.library == "ts_e_p_5")
        unit.divisions = 1;
    const std::vector<std::shared_ptr<Plate>> plates = {built.fixture.a, built.fixture.b};
    std::vector<InteractionFeaturePlate> no_joints;

    const std::map<std::string, std::function<void()>> designs = {
        {"ss_e_ip_0", [&] { ss_e_ip_0(unit); }}, {"ss_e_ip_1", [&] { ss_e_ip_1(unit); }}, {"ss_e_ip_2", [&] { ss_e_ip_2(unit, plates); }},
        {"ss_e_ip_3", [&] { ss_e_ip_3(unit); }}, {"ss_e_ip_4", [&] { ss_e_ip_4(unit); }}, {"ss_e_ip_5", [&] { ss_e_ip_5(unit, plates); }},
        {"ss_e_op_0", [&] { ss_e_op_0(unit); }}, {"ss_e_op_1", [&] { ss_e_op_1(unit); }}, {"ss_e_op_2", [&] { ss_e_op_2(unit); }},
        {"ss_e_op_3", [&] { ss_e_op_3(unit); }},
        {"ss_e_op_4", [&] { ss_e_op_4(unit, p.taper, p.chamfer, p.modify_outline, p.x[0], p.x[1], p.y[0], p.y[1], p.z[0], p.z[1]); }},
        {"ss_e_op_5", [&] { ss_e_op_5(unit, no_joints, p.disable_divisions); }}, {"ss_e_op_6", [&] { ss_e_op_6(unit, no_joints); }},
        {"ss_e_op_17", [&] { ss_e_op_17(unit); }}, {"ss_e_op_tutorial", [&] { ss_e_op_tutorial(unit); }},
        {"ts_e_p_0", [&] { ts_e_p_0(unit); }}, {"ts_e_p_1", [&] { ts_e_p_1(unit); }}, {"ts_e_p_2", [&] { ts_e_p_2(unit); }},
        {"ts_e_p_3", [&] { ts_e_p_3(unit); }}, {"ts_e_p_4", [&] { ts_e_p_4(unit); }}, {"ts_e_p_5", [&] { ts_e_p_5(unit); }},
        {"cr_c_ip_0", [&] { cr_c_ip_0(unit); }}, {"cr_c_ip_1", [&] { cr_c_ip_1(unit); }}, {"cr_c_ip_2", [&] { cr_c_ip_2(unit); }},
        {"cr_c_ip_3", [&] { cr_c_ip_3(unit); }}, {"cr_c_ip_4", [&] { cr_c_ip_4(unit); }}, {"cr_c_ip_5", [&] { cr_c_ip_5(unit); }},
        {"ss_e_r_2", [&] { ss_e_r_2(unit, plates); }}, {"ss_e_r_3", [&] { ss_e_r_3(unit, plates); }},
    };

    const auto design = designs.find(built.library);
    if (design == designs.end())
        return false;
    design->second();

    // a design that cuts only on the plates (ss_e_ip_2's pockets) leaves the box empty
    for (const auto* outlines : {&unit.male_outlines, &unit.female_outlines})
        for (const std::vector<Polyline>& face : *outlines)
            for (const Polyline& outline : face)
                if (outline.point_count() >= 3)
                    return true;
    return false;
}

/// The design's factory call as the page's code snippet: each argument the user can change with its value in the picture and what it does;
/// a fixed design takes none.
static std::string unit_snippet(const Built& built, const InteractionFeaturePlate& unit) {

    const std::map<std::string, std::string> shift_meaning = {
        {"ip", "the dovetail lean: straight at 0.5, one way at 0, the other at 1"},
        {"op", "the fingers' lean through the thickness, only above 0.5"},
        {"ts", "the tenon width against the gap"},
        {"r", "the size of each key along the joint line"},
        {"cr", "how narrow the half-lap's centre square is: 0 widest, 1 narrowest"},
    };
    const std::map<std::string, std::vector<std::string>> editable = {
        {"ss_e_ip_1", {"divisions", "shift"}}, {"ss_e_ip_2", {"divisions"}}, {"ss_e_ip_5", {"divisions"}},
        {"ss_e_op_1", {"divisions", "shift"}}, {"ss_e_op_2", {"divisions", "shift"}}, {"ss_e_op_4", {"divisions", "taper", "chamfer"}},
        {"ss_e_op_5", {"divisions", "disable_divisions"}}, {"ss_e_op_6", {"divisions"}}, {"ss_e_op_17", {"divisions"}},
        {"ts_e_p_2", {"divisions", "shift"}}, {"ts_e_p_3", {"divisions", "shift"}},
        {"ss_e_r_2", {"divisions", "shift"}}, {"ss_e_r_3", {"divisions", "shift"}},
        {"cr_c_ip_1", {"shift"}}, {"cr_c_ip_2", {"shift"}}, {"cr_c_ip_3", {"shift"}}, {"cr_c_ip_4", {"shift"}}, {"cr_c_ip_5", {"shift"}},
    };

    const auto names = editable.find(built.library);
    if (names == editable.end())
        return fmt::format("const std::shared_ptr<JointPlate> joint = JointPlate::{}(); // a fixed design, no parameters", built.library);

    const JointPlateParameters& p = built.joint->parameters;
    std::vector<std::pair<std::string, std::string>> arguments;
    for (const std::string& name : names->second) {
        std::string value;
        std::string meaning;
        if (name == "divisions") {
            value = fmt::format("{}", built.joint->connections.at(0).divisions);
            meaning = "how many teeth along the joint line; 0 takes them from its length";
        } else if (name == "shift") {
            value = fmt::format("{:g}", unit.shift);
            meaning = shift_meaning.at(built.family);
        } else if (name == "taper") {
            value = fmt::format("{:g}", p.taper);
            meaning = "the tenons' side lean";
        } else if (name == "chamfer") {
            value = p.chamfer ? "true" : "false";
            meaning = "bevelled tenon ends inside the floor";
        } else {
            value = p.disable_divisions ? "true" : "false";
            meaning = "the second linked joint (Vidy) without divisions";
        }
        arguments.push_back({value, fmt::format("{}: {}", name, meaning)});
    }

    // one argument per line, its comment aligned after the widest value
    size_t widest = 0;
    for (const std::pair<std::string, std::string>& argument : arguments)
        widest = std::max(widest, argument.first.size() + 1);
    std::string snippet = fmt::format("const std::shared_ptr<JointPlate> joint = JointPlate::{}(", built.library);
    for (size_t i = 0; i < arguments.size(); i++) {
        const std::string value = arguments[i].first + (i + 1 < arguments.size() ? "," : "");
        snippet += fmt::format("\n    {:<{}} // {}", value, widest, arguments[i].second);
    }
    return snippet + "\n);";
}

/// The unit box and the outlines in it, scaled to BOX.
static void add_unit_box(WoodSession& scene, const InteractionFeaturePlate& unit) {

    const std::shared_ptr<TreeNode> group = scene.group_named("unit_box");
    const Xform place = Xform::scale_uniform(Point(0.0, 0.0, 0.0), BOX);

    // the box's twelve edges
    const double h = 0.5;
    for (int axis = 0; axis < 3; axis++)
        for (double u : {-h, h})
            for (double v : {-h, h}) {
                std::array<double, 3> start = {u, v, -h};
                std::array<double, 3> end = {u, v, h};
                std::rotate(start.begin(), start.begin() + 2 - axis, start.end());
                std::rotate(end.begin(), end.begin() + 2 - axis, end.end());
                auto edge = std::make_shared<Polyline>(std::vector<Point>{Point(start[0], start[1], start[2]), Point(end[0], end[1], end[2])});
                *edge = edge->transformed(place);
                edge->linecolor = EDGE;
                edge->width = 2.0;
                scene.add_polyline(edge, group);
            }

    // every male and female outline of both faces, the 2-point seam markers left out; where the two cuts are the same line on both faces
    // (the in-plane designs) the male is drawn on the bottom face and the female on the top, so both show
    const bool shared = unit.male_outlines == unit.female_outlines;
    for (int side = 0; side < 2; side++)
        for (int face = 0; face < 2; face++) {
            if (shared && face != side)
                continue;
            for (const Polyline& outline : side == 0 ? unit.male_outlines[face] : unit.female_outlines[face]) {
                if (outline.point_count() < 3)
                    continue;
                auto drawn = std::make_shared<Polyline>(outline.transformed(place));
                drawn->linecolor = side == 0 ? MALE : FEMALE;
                drawn->width = 6.0;
                drawn->name = fmt::format("{}_face_{}", side == 0 ? "male" : "female", face);
                scene.add_polyline(drawn, group);
            }
        }
}

// ═══════════════════════════════════════════════════════════════════════════
// Scenes
// ═══════════════════════════════════════════════════════════════════════════

/// One design's scenes: its unit box alone when it has one, and its pair as the oracle joins it.
static void write_scene(const std::string& id, const std::string& dir) {

    Built built = build_variant(id, Xform::identity());
    std::string name = id;
    std::replace(name.begin(), name.end(), '/', '_');

    // the unit box, a scene of its own
    InteractionFeaturePlate unit;
    if (unit_outlines(built, unit)) {
        WoodSession unit_scene(name + "_unit");
        add_unit_box(unit_scene, unit);
        unit_scene.pb_dump(dir + "/" + name + "_unit.pb");
        std::ofstream(dir + "/" + name + "_unit.txt") << unit_snippet(built, unit) << std::endl;
    }

    // the pair drawn apart, the second plate moved along the contact from the first so the merged outlines read, a key half way and shown
    const Fixture& fixture = built.fixture;
    Vector apart = fixture.cross
        ? fixture.a->planes[0].z_axis().cross(fixture.b->planes[0].z_axis()).normalized()
        : compute_newell(fixture.face->polygon.get_points()).normalized();
    if ((fixture.b->element_geometry_mesh().centroid() - fixture.a->element_geometry_mesh().centroid()).dot(apart) < 0.0)
        apart = apart * -1.0;
    fixture.b->place(Xform::translation(apart[0] * APART, apart[1] * APART, apart[2] * APART));
    built.joint->place(Xform::translation(apart[0] * 0.5 * APART, apart[1] * 0.5 * APART, apart[2] * 0.5 * APART));
    built.joint->is_visible = built.joint->key_mesh().number_of_faces() > 0;
    built.scene->pb_dump(dir + "/" + name + ".pb");
}

/// A design that belongs to beams, on two beams of half-width 75: a cross design (cr_c_ip_*) on two beams crossing in plan at the angle, the
/// wedge ts_e_p_4 on a beam ending on the side of another; the second beam lifted 300 off along z after the joint, so both cuts read.
///   id: beam/<design>/<angle>[/<second beam's half-width>], written to <dir>/beam_<design>_<angle>[_<half-width>].pb
static void write_beam_scene(const std::string& id, const std::string& dir) {

    const std::vector<std::string> parts = split(id, '/');
    const std::string& design = parts.at(1);
    const double angle = std::stod(parts.at(2));
    const bool cross = design.starts_with("cr_c_ip_");

    WoodSession scene("beam_" + design);
    scene.settings.joint_parameters[cross ? 11 : 8] = JointPlate::library_id(design);
    const double turn = angle * std::numbers::pi / 180.0;
    // turned clockwise in plan, so the lifted beam runs across the isometric view and its cut faces the camera
    const Vector along(std::cos(turn), -std::sin(turn), 0.0);
    const Point origin(0.0, 0.0, 0.0);
    const std::shared_ptr<Beam> a = std::make_shared<Beam>(Polyline({Point(-600.0, 0.0, 0.0), Point(600.0, 0.0, 0.0)}), 75.0, "beam");
    const double b_half_width = parts.size() > 3 ? std::stod(parts.at(3)) : 75.0;
    const std::shared_ptr<Beam> b = std::make_shared<Beam>(Polyline({cross ? origin - along * 600.0 : origin, origin + along * 600.0}), b_half_width, "beam");
    a->name = "beam_a";
    b->name = "beam_b";
    scene.add(a);
    scene.add(b);
    scene.compute_axis_contacts(20.0);
    scene.compute_beam_features(500.0, 0.91, 1);

    // the design's drills as 16 mm dowels, the joint passed to each beam again with that radius
    for (const std::shared_ptr<Element>& element : *scene.objects.elements)
        if (const std::shared_ptr<JointBeam> joint = std::dynamic_pointer_cast<JointBeam>(element)) {
            joint->line_radius = 8.0;
            scene.add_interaction(joint, a, joint->interaction(0));
            scene.add_interaction(joint, b, joint->interaction(1));
        }
    b->place(Xform::translation(0.0, 0.0, 300.0));

    scene.pb_dump(fmt::format("{}/beam_{}_{:g}{}.pb", dir, design, angle, parts.size() > 3 ? "_" + parts.at(3) : ""));
}

/// A design on a pair of plates taken from a template, where it is used: the first two plates of the Chevron on the first Annen surface
/// that meet side to side at a fold, copied into a scene of their own, joined by the design with the given divisions and shift, the second lifted 400 off along
/// the contact.
///   id: tpl/chevron/<design>/<divisions>/<shift>, written to <dir>/tpl_chevron_<design>_<divisions>_<shift>.pb
static void write_template_scene(const std::string& id, const std::string& dir) {

    const std::vector<std::string> parts = split(id, '/');
    const std::string& design = parts.at(2);
    const std::vector<NurbsSurface> surfaces = wood_chevron::annen_surfaces((config::session_data_dir() / "annen_surfaces.json").string());
    const Chevron chevron(surfaces.at(0));
    const std::vector<std::shared_ptr<Plate>> plates = chevron.get_elements_numbered<Plate>("plate");

    // the first two plates that meet side to side at a fold, copied
    WoodSession scene("tpl_chevron_" + design);
    std::shared_ptr<Plate> first;
    std::shared_ptr<Plate> second;
    std::shared_ptr<InteractionContactFace> contact;
    for (size_t i = 0; i < plates.size() && !contact; i++)
        for (size_t j = i + 1; j < plates.size() && !contact; j++) {
            WoodSession probe("probe");
            const std::shared_ptr<Plate> a = std::make_shared<Plate>(*plates[i]);
            const std::shared_ptr<Plate> b = std::make_shared<Plate>(*plates[j]);
            probe.add(a);
            probe.add(b);
            const std::shared_ptr<InteractionContactFace> touch = probe.compute_face_contact(a, b);
            const bool folded = std::abs(a->planes[0].z_axis().dot(b->planes[0].z_axis())) < 0.95;
            if (touch && touch->type == ContactType::side_side && folded) {
                first = a;
                second = b;
                scene.add(first);
                scene.add(second);
                contact = scene.compute_face_contact(first, second);
            }
        }
    if (!contact)
        throw std::runtime_error("no two chevron plates touch");

    // the design on them, as the oracle builds a variant
    const std::shared_ptr<JointPlate> joint = make_variant({"op", design, parts.at(3), parts.at(4)});
    joint->orient(contact, {first, second}, scene.settings);
    scene.add(joint);
    for (size_t i = 0; i < joint->interaction_count(); i++)
        scene.add_interaction(joint, scene.get_element<Plate>(joint->interaction_target(i)), joint->interaction(i));
    Vector apart = compute_newell(contact->polygon.get_points()).normalized();
    if ((second->element_geometry_mesh().centroid() - first->element_geometry_mesh().centroid()).dot(apart) < 0.0)
        apart = apart * -1.0;
    second->place(Xform::translation(apart[0] * 400.0, apart[1] * 400.0, apart[2] * 400.0));

    scene.pb_dump(fmt::format("{}/tpl_chevron_{}_{}_{}.pb", dir, design, parts.at(3), parts.at(4)));
}

int main(int argc, char** argv) {

    if (argc < 2)
        throw std::invalid_argument("joint_tiles <out_dir> [ids...]");

    std::vector<std::string> ids(argv + 2, argv + argc);
    if (ids.empty())
        ids = VARIANTS;

    for (const std::string& id : ids) {
        try {
            if (id.starts_with("tpl/"))
                write_template_scene(id, argv[1]);
            else if (id.starts_with("beam/"))
                write_beam_scene(id, argv[1]);
            else
                write_scene(id, argv[1]);
        } catch (const std::exception& e) {
            std::cout << id << ": " << e.what() << std::endl;
        }
    }

    return 0;
}
