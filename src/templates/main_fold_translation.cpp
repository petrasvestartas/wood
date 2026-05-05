#include "session.h"
#include "wood_session.h"
#include "translation_shell.cpp"
#include <filesystem>
using namespace session_cpp;
using namespace wood_session;

int main() {
    auto data = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data" / "output";

    TranslationShell ts;

    // ── Wood joints: in-plane (ss_e_ip) — translation shell ──────────────────
    // Translation shell dihedrals ~170-175° → all > 150° threshold → all ss_e_ip naturally
    globals::globals_yaml("type_plates_name_side_to_side_edge_inplane_2_butterflies");
    globals::DATA_SET_INPUT_NAME  = "wood_fold_2_inplane";
    globals::DATA_SET_OUTPUT_FILE = "wood_fold_2.pb";
    // Plate edges ~160-200mm, 60mm division → 2-3 teeth per edge
    globals::JOINTS_PARAMETERS_AND_TYPES[0*3+0] = 60.0;  // ss_e_ip division_length
    globals::JOINTS_PARAMETERS_AND_TYPES[0*3+2] = 1.0;   // ss_e_ip_1: parametric zigzag
    globals::JOINT_VOLUME_EXTENSION = {-7.0, 0.0, 0.0, 0.0, 0.0};

    auto joints = get_connection_zones(ts.elements, face_to_face);

    Session session(globals::DATA_SET_INPUT_NAME);
    auto g_surf = session.add_group("Surface");
    session.add_mesh(std::make_shared<Mesh>(ts.surface), g_surf);
    fill_session(session, ts.elements, joints, true);
    session.pb_dump((data / globals::DATA_SET_OUTPUT_FILE).string());

    return 0;
}
