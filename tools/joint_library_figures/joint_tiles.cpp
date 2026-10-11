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
static const std::map<std::string, std::string> SHIFT_MEANING = { // What the shift does in each family, for the snippet and the tile's legend.
    {"ip", "the dovetail lean: straight at 0.5, one way at 0, the other at 1"},
    {"op", "the fingers' lean through the thickness, only above 0.5"},
    {"ts", "the tenon width against the gap"},
    {"r", "the size of each key along the joint line"},
    {"cr", "how narrow the half-lap's centre square is: 0 widest, 1 narrowest"},
};

// ═══════════════════════════════════════════════════════════════════════════
// The unit box
// ═══════════════════════════════════════════════════════════════════════════

/// The design's outlines in its unit box, the library function run on a joint with the built one's parameters, a custom design's the
/// user's own pairs; false for a design that is built on the plates in their own space (top to top, side removal, side_side_rotated_mitre) or leaves the
/// box empty, which has no unit box to draw.
static bool unit_outlines(const Built& built, InteractionFeaturePlate& unit) {

    const InteractionFeaturePlate& built_connection = built.joint->connections.at(0);
    const JointPlateParameters& p = built.joint->parameters;
    // the built connection's lines, scale and parameters, which some designs read (side_side_inplane_butterfly_keys its tooth pitch), its outlines cleared
    unit = built_connection;
    for (int face = 0; face < 2; face++) {
        unit.male_outlines[face].clear();
        unit.female_outlines[face].clear();
    }
    // a butterfly, key or snap-fit design tiles its teeth along the joint line far past the box: one tooth shows the design, the plates
    // show the divisions
    if (built.library == "side_side_inplane_butterfly_keys" || built.library == "side_side_inplane_tooth_keys" || built.library == "side_side_rotated_hooks" || built.library == "side_side_rotated_keys" || built.library == "top_side_snap_fit")
        unit.divisions = 1;
    const std::vector<std::shared_ptr<Plate>> plates = {built.fixture.a, built.fixture.b};
    std::vector<InteractionFeaturePlate> no_joints;

    const std::map<std::string, std::function<void()>> designs = {
        {"side_side_inplane_three_fingers", [&] { side_side_inplane_three_fingers(unit); }}, {"side_side_inplane_dovetails", [&] { side_side_inplane_dovetails(unit); }}, {"side_side_inplane_butterfly_keys", [&] { side_side_inplane_butterfly_keys(unit, plates); }},
        {"side_side_inplane_splayed_scarf", [&] { side_side_inplane_splayed_scarf(unit); }}, {"side_side_inplane_cross_splayed_scarf", [&] { side_side_inplane_cross_splayed_scarf(unit); }}, {"side_side_inplane_tooth_keys", [&] { side_side_inplane_tooth_keys(unit, plates); }},
        {"side_side_outplane_three_fingers", [&] { side_side_outplane_three_fingers(unit); }}, {"side_side_outplane_fingers", [&] { side_side_outplane_fingers(unit); }}, {"side_side_outplane_graded_fingers", [&] { side_side_outplane_graded_fingers(unit); }},
        {"side_side_outplane_mitre_tenon", [&] { side_side_outplane_mitre_tenon(unit); }},
        {"side_side_outplane_tenons", [&] { side_side_outplane_tenons(unit, p.taper, p.chamfer, p.modify_outline, p.x[0], p.x[1], p.y[0], p.y[1], p.z[0], p.z[1]); }},
        {"side_side_outplane_linked_tenons", [&] { side_side_outplane_linked_tenons(unit, no_joints, p.disable_divisions); }}, {"side_side_outplane_linked_tenons_wall", [&] { side_side_outplane_linked_tenons_wall(unit, no_joints); }},
        {"side_side_outplane_capped_fingers", [&] { side_side_outplane_capped_fingers(unit); }}, {"side_side_outplane_notch", [&] { side_side_outplane_notch(unit); }},
        {"top_side_three_tenons", [&] { top_side_three_tenons(unit); }}, {"top_side_annen_tenons", [&] { top_side_annen_tenons(unit); }}, {"top_side_dense_tenons", [&] { top_side_dense_tenons(unit); }},
        {"top_side_tenons", [&] { top_side_tenons(unit); }}, {"top_side_milled_wedge", [&] { top_side_milled_wedge(unit); }}, {"top_side_snap_fit", [&] { top_side_snap_fit(unit); }},
        {"cross_slot", [&] { cross_slot(unit); }}, {"cross_sliced_half_lap", [&] { cross_sliced_half_lap(unit); }}, {"cross_half_lap", [&] { cross_half_lap(unit); }},
        {"cross_half_lap_diagonal_drills", [&] { cross_half_lap_diagonal_drills(unit); }}, {"cross_half_lap_drill", [&] { cross_half_lap_drill(unit); }}, {"cross_brussels_half_lap", [&] { cross_brussels_half_lap(unit); }},
        {"side_side_rotated_hooks", [&] { side_side_rotated_hooks(unit, plates); }}, {"side_side_rotated_keys", [&] { side_side_rotated_keys(unit, plates); }},
        {"jupiter_scarf", [&] { jupiter_scarf(unit); }},
    };

    // a custom design: the user's pairs, face 0 then face 1
    if (built.library.ends_with("_custom")) {
        for (int side = 0; side < 2; side++)
            for (size_t i = 0; i + 1 < p.outlines[side].size(); i += 2)
                for (int face = 0; face < 2; face++)
                    (side == 0 ? unit.male_outlines : unit.female_outlines)[face].push_back(p.outlines[side][i + face]);
        return true;
    }

    const auto design = designs.find(built.library);
    if (design == designs.end())
        return false;
    design->second();

    // a design that cuts only on the plates (side_side_inplane_butterfly_keys's pockets) leaves the box empty
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

    const std::map<std::string, std::vector<std::string>> editable = {
        {"side_side_inplane_dovetails", {"divisions", "shift"}}, {"side_side_inplane_butterfly_keys", {"divisions"}}, {"side_side_inplane_tooth_keys", {"divisions"}},
        {"side_side_outplane_fingers", {"divisions", "shift"}}, {"side_side_outplane_graded_fingers", {"divisions", "shift"}}, {"side_side_outplane_tenons", {"divisions", "taper", "chamfer"}},
        {"side_side_outplane_linked_tenons", {"divisions", "disable_divisions"}}, {"side_side_outplane_linked_tenons_wall", {"divisions"}}, {"side_side_outplane_capped_fingers", {"divisions"}},
        {"top_side_dense_tenons", {"divisions", "shift"}}, {"top_side_tenons", {"divisions", "shift"}},
        {"side_side_rotated_hooks", {"divisions", "shift"}}, {"side_side_rotated_keys", {"divisions", "shift"}},
        {"cross_sliced_half_lap", {"shift"}}, {"cross_half_lap", {"shift"}}, {"cross_half_lap_diagonal_drills", {"shift"}}, {"cross_half_lap_drill", {"shift"}}, {"cross_brussels_half_lap", {"shift"}},
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
            meaning = SHIFT_MEANING.at(built.family);
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

/// The unit tile's annotations: an axis tripod at the box's corner, a dimension along the seam with a tick per division, and the labels for
/// shoot_native.sh, one "x y z|text" per line for a text at a point, "|text" for a line of the legend under the drawing.
static std::string add_unit_annotations(WoodSession& scene, const Built& built, const InteractionFeaturePlate& unit) {

    const std::shared_ptr<TreeNode> group = scene.group_named("annotations");
    const double h = 0.5 * BOX;
    std::string labels;
    const auto label = [&labels](const Point& point, const std::string& text) {
        labels += fmt::format("{} {} {}|{}\n", point[0], point[1], point[2], text);
    };
    const auto line = [&scene, &group](const Point& start, const Point& end) {
        auto drawn = std::make_shared<Polyline>(std::vector<Point>{start, end});
        drawn->linecolor = Color(0.05f, 0.05f, 0.05f);
        drawn->width = 3.0;
        scene.add_polyline(drawn, group);
    };

    // the axis tripod: an arrow along each box edge leaving the corner (-h, -h, -h), a third of the box long, its letter at the tip
    const Point corner(-h, -h, -h);
    const std::array<Vector, 3> axes = {Vector(1.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0), Vector(0.0, 0.0, 1.0)};
    const std::array<std::string, 3> letters = {"x", "y", "z"};
    for (size_t a = 0; a < 3; a++) {
        const Point tip = corner + axes[a] * (0.35 * BOX);
        const Vector back = axes[a] * (-0.06 * BOX);
        const Vector side = axes[(a + 1) % 3] * (0.03 * BOX);
        line(corner, tip);
        line(tip, tip + back + side);
        line(tip, tip + back - side);
        label(tip, letters[a]);
    }
    labels += "|x across the seam, y through the plates, z along the seam\n";
    labels += "|pink: the male plate's cut, blue: the female plate's cut\n";

    // each parameter the design takes: the divisions as a dimension along z beside the box, a tick at every division; the rest in the legend
    const std::map<std::string, std::vector<std::string>> editable = {
        {"side_side_inplane_dovetails", {"divisions", "shift"}}, {"side_side_outplane_fingers", {"divisions", "shift"}}, {"side_side_outplane_graded_fingers", {"divisions", "shift"}},
        {"side_side_outplane_tenons", {"divisions", "taper", "chamfer"}}, {"side_side_outplane_linked_tenons", {"divisions"}}, {"side_side_outplane_linked_tenons_wall", {"divisions"}}, {"side_side_outplane_capped_fingers", {"divisions"}},
        {"top_side_dense_tenons", {"divisions", "shift"}}, {"top_side_tenons", {"divisions", "shift"}},
        {"cross_sliced_half_lap", {"shift"}}, {"cross_half_lap", {"shift"}}, {"cross_half_lap_diagonal_drills", {"shift"}}, {"cross_half_lap_drill", {"shift"}}, {"cross_brussels_half_lap", {"shift"}},
    };
    const auto names = editable.find(built.library);
    if (names == editable.end())
        return labels;

    const JointPlateParameters& p = built.joint->parameters;
    for (const std::string& name : names->second) {
        if (name == "divisions") {
            const int divisions = std::max(1, built.joint->connections.at(0).divisions);
            const double x = 1.2 * h;
            line(Point(x, -h, -h), Point(x, -h, h));
            for (int k = 0; k <= divisions; k++) {
                const double z = -h + BOX * k / divisions;
                line(Point(x - 0.04 * BOX, -h, z), Point(x + 0.04 * BOX, -h, z));
            }
            label(Point(x + 0.06 * BOX, -h, 0.0), fmt::format("{} divisions", divisions));
        } else if (name == "shift") {
            labels += fmt::format("|shift {:g}: {}\n", unit.shift, SHIFT_MEANING.at(built.family));
        } else if (name == "taper") {
            labels += fmt::format("|taper {:g}: the tenons' side lean\n", p.taper);
        } else {
            labels += fmt::format("|chamfer {}: bevelled tenon ends\n", p.chamfer ? "true" : "false");
        }
    }
    return labels;
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
        const std::string labels = add_unit_annotations(unit_scene, built, unit);
        unit_scene.pb_dump(dir + "/" + name + "_unit.pb");
        std::ofstream(dir + "/" + name + "_unit.txt") << unit_snippet(built, unit) << std::endl;
        std::ofstream(dir + "/" + name + "_unit.labels") << labels;
    }

    // a design built in world space: its outlines and drills as built on the contact, a scene of their own
    else {
        WoodSession unit_scene(name + "_unit");
        const std::shared_ptr<TreeNode> group = unit_scene.group_named("joint");
        const InteractionFeaturePlate& connection = built.joint->connections.at(0);
        for (int side = 0; side < 2; side++)
            for (int face = 0; face < 2; face++)
                for (const Polyline& outline : side == 0 ? connection.male_outlines[face] : connection.female_outlines[face]) {
                    auto drawn = std::make_shared<Polyline>(outline);
                    drawn->linecolor = side == 0 ? MALE : FEMALE;
                    drawn->width = 6.0;
                    unit_scene.add_polyline(drawn, group);
                }
        auto contact = std::make_shared<Polyline>(built.fixture.face ? built.fixture.face->polygon : Polyline());
        contact->linecolor = EDGE;
        contact->width = 2.0;
        if (contact->point_count() > 0)
            unit_scene.add_polyline(contact, group);
        unit_scene.pb_dump(dir + "/" + name + "_unit.pb");
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

/// A design that belongs to beams, on two beams of half-width 75: a cross design (cross_*) on two beams crossing in plan at the angle, the
/// wedge top_side_milled_wedge on a beam ending on the side of another; the second beam lifted 300 off along z after the joint, so both cuts read.
///   id: beam/<design>/<angle>[/<second beam's half-width>], written to <dir>/beam_<design>_<angle>[_<half-width>].pb
static void write_beam_scene(const std::string& id, const std::string& dir) {

    const std::vector<std::string> parts = split(id, '/');
    const std::string& design = parts.at(1);
    const double angle = std::stod(parts.at(2));
    const bool cross = design.starts_with("cross_");

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
