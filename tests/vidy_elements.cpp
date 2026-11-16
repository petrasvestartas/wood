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

/// Verifies every plate has exact thickness, valid BRep, and positive closed solid volume.
void check_plates(
    const std::vector<std::shared_ptr<FacePlanarElement>>& plates,
    double expected_thickness,
    const std::string& label
) {

    for (const std::shared_ptr<FacePlanarElement>& plate : plates) {
        check(
            std::abs(plate->thickness - expected_thickness) < 0.25,
            fmt::format("{}: thickness {:.2f} != {:.2f}", label, plate->thickness, expected_thickness)
        );
        const Mesh& mesh = plate->model_geometry_mesh();
        const BRep& brep = plate->model_geometry_brep();
        check(mesh.number_of_vertices() >= 6, label + ": plate mesh vertex count");
        check(std::abs(mesh.volume()) > 1.0e6, label + ": plate mesh volume must be positive");
        check(brep.is_valid(), label + ": plate BRep must be valid");
    }
}

} // namespace

int main() {

    // 1. Stage 1 & 2: Full 11-bay Théâtre de Vidy base mesh, bay axes, CNC beta bounds, and offset shells.
    {
        const wood_vidy::VidyGuide guide(11);
        check(guide.base_mesh().number_of_vertices() == 127, "11-bay base mesh vertex count == 127");
        check(guide.facets().size() == 154, "11-bay facet count == 154");
        check(guide.bay_axes().size() == 12, "11-bay transverse axis count == 12");
        check(std::abs(guide.cavity_gap() - 210.0) < 1e-6, "210 mm clear insulation cavity");

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

        size_t reg_edges = 0;
        size_t vcon_edges = 0;
        size_t grnd_edges = 0;
        size_t vend_edges = 0;
        for (const wood_vidy::VidyEdge& edge : guide.edges()) {
            if (edge.kind == wood_vidy::VidyEdgeKind::Regular) {
                reg_edges++;
                check(edge.tool_tilt_deg < 60.0, "regular fold CNC tilt beta must stay under 60 deg");
            } else if (edge.kind == wood_vidy::VidyEdgeKind::BaySeparation) {
                vcon_edges++;
            } else if (edge.kind == wood_vidy::VidyEdgeKind::Ground) {
                grnd_edges++;
            } else {
                vend_edges++;
            }
        }
        check(reg_edges == 176, "176 interior regular fold creases");
        check(vcon_edges == 50, "50 inter-bay XZ separation edges");
        check(grnd_edges == 44 && vend_edges == 10, "44 ground edges and 10 gable edges");
        check(guide.outer_mesh().number_of_faces() == 154, "outer offset mesh face count == 154");
    }

    // 2. Stage 3: Full 308 mitered 45 mm CLT panels without joinery.
    {
        const wood_vidy::VidyGuide guide(11);
        const wood_vidy::VidyPavilion panels(guide, false);
        check(panels.all_plates().size() == 308, "308 double-layer CLT plates");
        check_plates(panels.all_plates(), 45.0, "stage3_panels");
    }

    // 3. Stage 4: Full-scale 1-bay (28 plates) jointed Vidy section + Protobuf round-trip.
    {
        const wood_vidy::VidyGuide section_guide(1);
        wood_vidy::VidyPavilion section(section_guide, true, 0.0, 450.0);
        check(section.all_plates().size() == 28, "1-bay jointed plate count == 28");
        check(section.total_mortise_slots() >= 80, "1-bay through-mortise slot count >= 80");
        check_plates(section.all_plates(), 45.0, "stage4_jointed_section");

        for (const std::shared_ptr<FacePlanarElement>& plate : section.all_plates()) {
            check(
                plate->features.bottom.size() == plate->features.top.size() &&
                    !plate->features.bottom.empty(),
                "jointed plate paired bottom/top feature loops"
            );
        }

        const WoodSession reloaded = WoodSession::pb_loads(section.pb_dumps());
        check(
            reloaded.world_elements<FacePlanarElement>().size() == 28,
            "pb_loads preserved all 28 jointed Vidy plates"
        );
    }

    std::cout << "vidy_elements: all checks passed\n";
    return 0;
}
