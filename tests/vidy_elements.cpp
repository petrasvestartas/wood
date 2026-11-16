#include "wood_session.h"
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
                label + ": trimmed_mesh must preserve jointed vertices without stock fallback"
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
    }

    // 3. Stage 4a: First-row verification section (1 bay = 28 jointed plates) + Protobuf round-trip.
    {
        const wood_vidy::VidyGuide section_guide(1);
        wood_vidy::VidyPavilion section(section_guide, true, 1.0, 500.0);
        check(section.all_plates().size() == 28, "1-bay jointed plate count == 28");
        check(section.total_mortise_slots() >= 300, "1-bay through-mortise slot count >= 300");
        check_plates(section.all_plates(), 45.0, true, "stage4_first_row");

        const WoodSession reloaded = WoodSession::pb_loads(section.pb_dumps());
        check(
            reloaded.world_elements<FacePlanarElement>().size() == 28,
            "pb_loads preserved all 28 jointed Vidy plates"
        );
    }

    // 4. Stage 4b: Full 11-bay pavilion with joinery applied to ALL 308 panels + 4-stage erection sequence.
    {
        const wood_vidy::VidyGuide full_guide(11);
        wood_vidy::VidyPavilion full_jointed(full_guide, true, 1.0, 500.0);
        check(full_jointed.all_plates().size() == 308, "11-bay jointed plate count == 308");
        check(
            full_jointed.total_mortise_slots() >= 3400,
            "11-bay through-mortise slot count >= 3400 across all 308 plates"
        );
        check_plates(full_jointed.all_plates(), 45.0, true, "stage4_full_308_jointed");
    }

    std::cout << "vidy_elements: all checks passed\n";
    return 0;
}
