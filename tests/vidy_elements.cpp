#include "wood_session.h"
#include "wood_element_geometry.h"
#include "src/templates/vidy/vidy.h"

using namespace session_cpp;
using namespace wood_session;

namespace {

/// Fails vidy_elements with a diagnostic message when ok is false.
void check(bool ok, const std::string& message) {

    if (!ok) {
        std::cerr << "vidy_elements FAILED: " << message << std::endl;
        throw std::runtime_error(message);
    }
}

/// Counts 3D solid overlaps (> 5 mm^3) across all AABB-intersecting plate pairs using Manifold CSG.
size_t report_plate_collisions(
    const std::vector<std::shared_ptr<FacePlanarElement>>& plates,
    const std::string& label
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
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            if (!boxes[i].intersects(boxes[j])) {
                continue;
            }
            const Mesh inter = solid_boolean(meshes[i], meshes[j], SolidOperation::intersect);
            const double vol = inter.number_of_faces() > 0 ? compute_volume(inter) : 0.0;
            if (vol > 5.0) {
                hits++;
                max_vol = std::max(max_vol, vol);
                if (hits <= 30) {
                    const AABB ibox = AABB::from_mesh(inter);
                    std::cout << fmt::format(
                        "  [{}] COLLISION #{}: {} <-> {} vol={:.1f} min=({:.0f},{:.0f},{:.0f}) max=({:.0f},{:.0f},{:.0f})\n",
                        label,
                        hits,
                        plates[i]->name,
                        plates[j]->name,
                        vol,
                        ibox.min_point()[0],
                        ibox.min_point()[1],
                        ibox.min_point()[2],
                        ibox.max_point()[0],
                        ibox.max_point()[1],
                        ibox.max_point()[2]
                    );
                }
            }
        }
    }
    std::cout << fmt::format(
        "[{}] total colliding pairs = {} (max vol = {:.1f} mm^3)\n", label, hits, max_vol
    );
    return hits;
}

/// Verifies every plate has exact thickness, valid BRep, positive volume, and non-fallback jointed vertices.
void check_plates(
    const std::vector<std::shared_ptr<FacePlanarElement>>& plates,
    double expected_thickness,
    bool require_joint_features,
    const std::string& label
) {

    for (const std::shared_ptr<FacePlanarElement>& plate : plates) {
        check(
            std::abs(plate->thickness - expected_thickness) < 0.25,
            fmt::format("{}: thickness {:.2f} != {:.2f}", label, plate->thickness, expected_thickness)
        );
        const Mesh& mesh = plate->model_geometry_mesh();
        const BRep& brep = plate->model_geometry_brep();
        check(mesh.number_of_vertices() >= 24, label + ": chamfered plate mesh vertex count >= 24");
        check(std::abs(mesh.volume()) > 1.0e6, label + ": plate mesh volume must be positive");
        check(brep.is_valid(), label + ": plate BRep must be valid");
        if (require_joint_features) {
            check(
                plate->features.bottom.size() == plate->features.top.size() &&
                    !plate->features.bottom.empty(),
                label + ": jointed plate paired bottom/top feature loops"
            );
            check(
                mesh.number_of_vertices() >= 2 * (plate->features.bottom[0].point_count() - 1),
                fmt::format(
                    "{} ({}): trimmed_mesh verts {} < expected {}",
                    label,
                    plate->name,
                    mesh.number_of_vertices(),
                    2 * (plate->features.bottom[0].point_count() - 1)
                )
            );
        }
    }
}

} // namespace

int main() {

    // 1. Stage 1 & 2: Full 11-bay Théâtre de Vidy base mesh, bay axes, miter planes, and 4 offset shells.
    {
        const wood_vidy::VidyGuide guide(11);
        check(guide.base_mesh().number_of_vertices() == 127, "11-bay base mesh vertex count == 127");
        check(guide.facets().size() == 154, "11-bay facet count == 154");
        check(guide.bay_axes().size() == 12, "11-bay transverse axis count == 12");
        check(std::abs(guide.cavity_gap() - 210.0) < 1e-6, "210 mm clear insulation cavity");
        check(
            guide.miter_planes_mesh().number_of_faces() == 236,
            "236 miter and separation connection planes (176 regular + 50 bay + 10 gable)"
        );

        size_t roof_tris = 0;
        size_t wall_quads = 0;
        for (const wood_vidy::VidyFacet& facet : guide.facets()) {
            if (facet.role == wood_vidy::VidyFacetRole::Roof) {
                check(facet.vertices.size() == 3, "roof facet must be planar triangle");
                check(facet.normal[2] > 0.25, "roof normal must point upward/outward");
                roof_tris++;
            } else {
                check(facet.vertices.size() == 4, "wall facet must be planar quad");
                check(std::abs(facet.normal[2]) < 1e-6, "wall normal must be strictly horizontal");
                wall_quads++;
            }
        }
        check(roof_tris == 110 && wall_quads == 44, "110 roof triangles and 44 wall quads");
        check(guide.outer_mesh().number_of_faces() == 154, "outer offset mesh face count == 154");
    }

    // 2. Stage 3: Full 308 chamfered 45 mm CLT panels without joinery.
    {
        const wood_vidy::VidyGuide guide(11);
        const wood_vidy::VidyPavilion panels(guide, false);
        check(panels.all_plates().size() == 308, "308 double-layer CLT plates");
        check_plates(panels.all_plates(), 45.0, false, "stage3_panels");
        check(
            report_plate_collisions(panels.all_plates(), "stage3_308") == 0,
            "Stage 3 308 panels must have zero 3D solid collisions"
        );
    }

    // 3. Stage 4a: First-row verification section (1 bay = 28 jointed plates) + Protobuf round-trip.
    {
        const wood_vidy::VidyGuide section_guide(1);
        wood_vidy::VidyPavilion section(section_guide, true, 1.0, 500.0);
        std::cout << fmt::format(
            "[stage4_28] seams={} double_pins={} single_pins={} total_pins={} mortise_slots={}\n",
            section.total_jointed_seams(),
            section.total_double_pins(),
            section.total_single_pins(),
            section.total_pins(),
            section.total_mortise_slots()
        );
        check(section.all_plates().size() == 28, "1-bay jointed plate count == 28");
        check(section.total_jointed_seams() == 16, "1-bay jointed seam count == 16");
        check(section.total_double_pins() == 90, "1-bay Double_Pin count == 90 (58 roof + 32 wall/eave)");
        check(section.total_pins() >= 280 && section.total_pins() <= 305, "1-bay total pin count matches Rhino (~284-297)");
        check(section.total_mortise_slots() >= 360, "1-bay through-mortise slot count >= 360");
        check_plates(section.all_plates(), 45.0, true, "stage4_first_row");
        check(
            report_plate_collisions(section.all_plates(), "stage4_28") == 0,
            "Stage 4 28 first-row jointed plates must have zero 3D solid collisions"
        );

        const WoodSession reloaded = WoodSession::pb_loads(section.pb_dumps());
        check(
            reloaded.world_elements<FacePlanarElement>().size() == 28,
            "pb_loads preserved all 28 jointed Vidy plates"
        );
    }

    // 4. Stage 4b: Full 11-bay pavilion with joinery applied to ALL 308 panels + 6-step module assembly.
    {
        const wood_vidy::VidyGuide full_guide(11);
        wood_vidy::VidyPavilion full_jointed(full_guide, true, 1.0, 500.0);
        std::cout << fmt::format(
            "[stage4_308] seams={} double_pins={} single_pins={} total_pins={} mortise_slots={}\n",
            full_jointed.total_jointed_seams(),
            full_jointed.total_double_pins(),
            full_jointed.total_single_pins(),
            full_jointed.total_pins(),
            full_jointed.total_mortise_slots()
        );
        check(full_jointed.all_plates().size() == 308, "11-bay jointed plate count == 308");
        check(full_jointed.total_jointed_seams() == 176, "11-bay jointed seam count == 176");
        check(full_jointed.total_double_pins() == 990, "11-bay Double_Pin count == 990");
        check(full_jointed.total_pins() >= 3050 && full_jointed.total_pins() <= 3300, "11-bay total pin count matches Rhino (~3120)");
        check(
            full_jointed.total_mortise_slots() >= 3900,
            "11-bay through-mortise slot count >= 3900 across all 308 plates"
        );
        check_plates(full_jointed.all_plates(), 45.0, true, "stage4_full_308_jointed");
        check(
            report_plate_collisions(full_jointed.all_plates(), "stage4_308") == 0,
            "Stage 4 full 308 jointed plates must have zero 3D solid collisions"
        );

        full_jointed.add_module_assembly_groups(5, 1650.0);
        const std::array<std::string, 6> group_names = {
            "module_1_haunch_wings",
            "module_2_haunches_joined",
            "module_3_inner_crowns_eaves",
            "module_4_outer_covers",
            "module_5_vertical_walls",
            "module_6_roof_onto_walls"
        };
        for (const std::string& gname : group_names) {
            const std::vector<std::shared_ptr<FacePlanarElement>> gplates =
                full_jointed.get_branch(gname).world_elements<FacePlanarElement>();
            check(!gplates.empty(), gname + " must contain assembly plates");
            check(
                report_plate_collisions(gplates, gname) == 0,
                gname + " assembly step must have zero 3D solid collisions"
            );
        }
    }

    std::cout << "vidy_elements: all checks passed\n";
    return 0;
}
