#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/cavity_border.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

namespace {

/// Throws with a diagnostic message when ok is false.
void check(bool ok, const std::string& message) {

    if (ok)
        return;

    std::cerr << "vidy_elements FAILED: " << message << std::endl;
    throw std::runtime_error(message);
}

/// Counts 3D solid overlaps (> 5 mm^3) across AABB-intersecting plate pairs via Manifold CSG.
size_t report_plate_collisions(
    const std::vector<std::shared_ptr<FacePlanarElement>>& plates, const std::string& label
) {

    const size_t n = plates.size();
    std::vector<Mesh> meshes(n);
    std::vector<AABB> boxes(n);

    for (size_t i = 0; i < n; i++) {
        meshes[i] = plates[i]->model_geometry_mesh();
        boxes[i] = plates[i]->aabb(-0.05);
    }

    size_t hits = 0;
    double max_vol = 0.0;

    for (size_t i = 0; i < n; i++)
        for (size_t j = i + 1; j < n; j++) {
            if (!boxes[i].intersects(boxes[j]))
                continue;

            const Mesh inter = solid_boolean(meshes[i], meshes[j], SolidOperation::intersect);
            const double vol = inter.number_of_faces() > 0 ? compute_volume(inter) : 0.0;

            if (vol <= 5.0)
                continue;

            hits++;
            max_vol = std::max(max_vol, vol);
        }

    std::cout << fmt::format(
        "[{}] total colliding pairs = {} (max vol = {:.1f} mm^3)\n", label, hits, max_vol
    );

    return hits;
}

/// Checks thickness, valid BRep, positive volume and jointed feature loops of shell plates.
void check_plates(
    const std::vector<std::shared_ptr<FacePlanarElement>>& plates,
    double expected_thickness,
    bool require_joint_features,
    const std::string& label
) {

    for (const std::shared_ptr<FacePlanarElement>& plate : plates) {
        check(
            std::abs(plate->thickness - expected_thickness) < 0.25,
            fmt::format(
                "{}: thickness {:.2f} != {:.2f}", label, plate->thickness, expected_thickness
            )
        );
        const Mesh& mesh = plate->model_geometry_mesh();
        const BRep& brep = plate->model_geometry_brep();
        check(mesh.number_of_vertices() >= 24, label + ": chamfered plate mesh vertex count >= 24");
        check(std::abs(mesh.volume()) > 1.0e6, label + ": plate mesh volume must be positive");
        check(brep.is_valid(), label + ": plate BRep must be valid");

        if (!require_joint_features)
            continue;

        check(
            plate->features.bottom.size() == plate->features.top.size() &&
                !plate->features.bottom.empty(),
            label + ": jointed plate paired bottom/top feature loops"
        );
        check(
            mesh.number_of_vertices() >= 2 * (plate->features.bottom[0].point_count() - 1),
            fmt::format("{} ({}): trimmed_mesh vertex count check", label, plate->name)
        );
    }
}

/// Checks thickness, eight-vertex prism topology, positive volume and valid BRep of border boards.
void check_border_plates(
    const std::vector<std::shared_ptr<FacePlanarElement>>& borders,
    double expected_thickness,
    const std::string& label
) {

    for (const std::shared_ptr<FacePlanarElement>& board : borders) {
        check(
            std::abs(board->thickness - expected_thickness) < 0.25,
            fmt::format(
                "{}: border thickness {:.2f} != {:.2f}", label, board->thickness, expected_thickness
            )
        );
        const Mesh& mesh = board->model_geometry_mesh();
        const BRep& brep = board->model_geometry_brep();
        check(mesh.number_of_vertices() == 8, label + ": border board mesh vertex count == 8");
        check(std::abs(mesh.volume()) > 1.0e6, label + ": border board mesh volume > 0");
        check(brep.is_valid(), label + ": border board BRep must be valid");
    }
}

/// Tests compute_cavity_border_board on a generic tilted double-layer facet outside Vidy.
void test_reusable_cavity_border() {

    const Vector facet_n = Vector{0.25, 0.0, 0.9682458}.normalized();
    CavityBorderSpec spec;
    spec.facet_plane = Plane::from_point_normal({0.0, 0.0, 0.0}, facet_n);
    spec.boundary_plane = Plane::from_point_normal({0.0, 0.0, 0.0}, {0.0, -1.0, 0.0});
    spec.start_plane = Plane::from_point_normal({0.0, 0.0, 0.0}, {-0.96, -0.28, 0.0});
    spec.end_plane = Plane::from_point_normal({1800.0, 0.0, -465.0}, {0.96, -0.28, 0.0});
    spec.inner_edge = {
        Point{0.0, 0.0, 0.0} + facet_n * 45.0, Point{1800.0, 0.0, -465.0} + facet_n * 45.0
    };
    spec.outer_edge = {
        Point{0.0, 0.0, 0.0} + facet_n * 255.0, Point{1800.0, 0.0, -465.0} + facet_n * 255.0
    };
    spec.inner_level = 45.5;
    spec.outer_level = 254.5;
    spec.thickness = 40.0;
    spec.outer_gap = 1.0;
    spec.start_setback = 60.0;
    spec.end_setback = 60.0;

    const std::shared_ptr<FacePlanarElement> board =
        compute_cavity_border_board(spec, "generic_cavity_border");
    check(board != nullptr, "reusable cavity border board built on tilted facet");
    check_border_plates({board}, 40.0, "generic_cavity_border");
}

/// Verifies Stage 1 base mesh and Stage 2 four-surface miter shells on the 11-bay guide.
void test_guide() {

    const wood_vidy::VidyGuide guide(11);
    check(guide.base_mesh().number_of_vertices() == 127, "11-bay base mesh vertex count == 127");
    check(guide.facets().size() == 154, "11-bay facet count == 154");
    check(guide.bay_axes().size() == 12, "11-bay transverse axis count == 12");
    check(std::abs(guide.cavity_gap() - 210.0) < 1e-6, "210 mm clear insulation cavity");
    check(guide.miter_planes_mesh().number_of_faces() == 236, "236 miter and separation planes");

    size_t roof_tris = 0;
    size_t wall_quads = 0;

    for (const wood_vidy::VidyFacet& facet : guide.facets())
        if (facet.role == wood_vidy::VidyFacetRole::Roof) {
            check(facet.vertices.size() == 3 && facet.normal[2] > 0.25, "roof triangle normal");
            roof_tris++;
        } else {
            check(
                facet.vertices.size() == 4 && std::abs(facet.normal[2]) < 1e-6, "wall quad normal"
            );
            wall_quads++;
        }

    check(roof_tris == 110 && wall_quads == 44, "110 roof triangles and 44 wall quads");
    check(guide.outer_mesh().number_of_faces() == 154, "outer offset mesh face count == 154");
}

/// Verifies Stage 3 unjointed 308 shell plates + 154 cavity border boards have zero collisions.
void test_stage3_panels() {

    const wood_vidy::VidyGuide guide(11);
    const wood_vidy::VidyPavilion panels(guide, false);
    check(panels.shell_plates().size() == 308, "308 double-layer CLT shell plates");
    check(panels.border_plates().size() == 154, "154 cavity-closing border boards");
    check(panels.all_plates().size() == 462, "462 total Stage 3 plates");
    check_plates(panels.shell_plates(), 45.0, false, "stage3_panels");
    check_border_plates(panels.border_plates(), 45.0, "stage3_borders");
    check(
        report_plate_collisions(panels.all_plates(), "stage3_462") == 0, "Stage 3 zero collisions"
    );
}

/// Verifies Stage 4 single-bay section (28 jointed shells + 14 border boards) and Protobuf round-trip.
void test_stage4_section() {

    const wood_vidy::VidyGuide section_guide(1);
    wood_vidy::VidyPavilion section(section_guide, true, 1.0, 500.0);
    std::cout << fmt::format(
        "[stage4_42] shell={} border={} seams={} double_pins={} single_pins={} total_pins={} mortise_slots={}\n",
        section.shell_plates().size(),
        section.border_plates().size(),
        section.total_jointed_seams(),
        section.total_double_pins(),
        section.total_single_pins(),
        section.total_pins(),
        section.total_mortise_slots()
    );
    check(
        section.shell_plates().size() == 28 && section.border_plates().size() == 14, "1-bay 28+14"
    );
    check(section.all_plates().size() == 42 && section.total_jointed_seams() == 16, "1-bay 42/16");
    check(section.total_double_pins() == 90, "1-bay Double_Pin count == 90");
    check(section.total_pins() >= 280 && section.total_pins() <= 305, "1-bay total pin count");
    check(section.total_mortise_slots() >= 360, "1-bay mortise slot count >= 360");
    check_plates(section.shell_plates(), 45.0, true, "stage4_first_row");
    check_border_plates(section.border_plates(), 45.0, "stage4_first_row_borders");
    check(
        report_plate_collisions(section.all_plates(), "stage4_42") == 0, "Stage 4a zero collisions"
    );

    const WoodSession reloaded = WoodSession::pb_loads(section.pb_dumps());
    check(
        reloaded.world_elements<FacePlanarElement>().size() == 42, "pb_loads preserved 42 plates"
    );
}

/// Verifies Stage 4 full 11-bay pavilion (462 plates) and all six module assembly steps.
void test_stage4_pavilion() {

    const wood_vidy::VidyGuide full_guide(11);
    wood_vidy::VidyPavilion full_jointed(full_guide, true, 1.0, 500.0);
    std::cout << fmt::format(
        "[stage4_462] shell={} border={} seams={} double_pins={} single_pins={} total_pins={} mortise_slots={}\n",
        full_jointed.shell_plates().size(),
        full_jointed.border_plates().size(),
        full_jointed.total_jointed_seams(),
        full_jointed.total_double_pins(),
        full_jointed.total_single_pins(),
        full_jointed.total_pins(),
        full_jointed.total_mortise_slots()
    );
    check(
        full_jointed.shell_plates().size() == 308 && full_jointed.border_plates().size() == 154,
        "308+154"
    );
    check(
        full_jointed.all_plates().size() == 462 && full_jointed.total_jointed_seams() == 176,
        "462/176"
    );
    check(full_jointed.total_double_pins() == 990, "11-bay Double_Pin count == 990");
    check(
        full_jointed.total_pins() >= 3050 && full_jointed.total_pins() <= 3300, "11-bay total pins"
    );
    check(full_jointed.total_mortise_slots() >= 3900, "11-bay mortise slots >= 3900");
    check_plates(full_jointed.shell_plates(), 45.0, true, "stage4_full_308_jointed");
    check_border_plates(full_jointed.border_plates(), 45.0, "stage4_full_154_borders");
    check(
        report_plate_collisions(full_jointed.all_plates(), "stage4_462") == 0,
        "Stage 4b zero collisions"
    );

    full_jointed.add_module_assembly_groups(5, 1650.0);
    const std::array<std::string, 6> groups = {
        "module_1_haunch_wings",
        "module_2_haunches_joined",
        "module_3_inner_crowns_eaves",
        "module_4_outer_covers",
        "module_5_vertical_walls",
        "module_6_roof_onto_walls"
    };

    for (const std::string& gname : groups) {
        const std::vector<std::shared_ptr<FacePlanarElement>> gplates =
            full_jointed.get_branch(gname).world_elements<FacePlanarElement>();
        check(!gplates.empty() && report_plate_collisions(gplates, gname) == 0, gname + " clean");
    }
}

}

/// Runs the reusable cavity border, Vidy guide, Stage 3, Stage 4 section and full pavilion checks.
int main() {

    test_reusable_cavity_border();
    test_guide();
    test_stage3_panels();
    test_stage4_section();
    test_stage4_pavilion();
    std::cout << "vidy_elements: all checks passed\n";

    return 0;
}
