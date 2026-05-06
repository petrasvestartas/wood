#include "reflex_fold.h"
#include <filesystem>

int main() {
    auto data = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "data" / "output";

    // ── ReflexFold surface ────────────────────────────────────────────────────
    ReflexFold rf;  // uses default cross_section + profile, thickness=10, chamfer=20/20, angle=45

    // ── Wood joints: out-of-plane at fold edges, in-plane at arch edges ──────
    // Threshold 150°: fold profile edges (~90-150° dihedral) → ss_e_op
    //                 arch cross-section edges (~160-175° dihedral) → ss_e_ip
    globals::globals_yaml("type_plates_name_side_to_side_edge_outofplane_folding");
    globals::DATA_SET_INPUT_NAME  = "wood_fold_1_mixed";
    globals::DATA_SET_OUTPUT_FILE = "wood_fold_1.pb";
    // ss_e_op (fold edges, ≤150°): 150mm div, id=17 (parametric miter endings)
    globals::JOINTS_PARAMETERS_AND_TYPES[1*3+0] = 150.0;
    globals::JOINTS_PARAMETERS_AND_TYPES[1*3+1] = 0.5;
    globals::JOINTS_PARAMETERS_AND_TYPES[1*3+2] = 17.0;
    globals::JOINT_VOLUME_EXTENSION = {0.0, 0.0, -60.0, 0.0, 0.0};
    globals::LIMIT_MIN_JOINT_LENGTH = std::abs(globals::JOINT_VOLUME_EXTENSION[2]) * 2;

    auto joints = get_connection_zones(rf.elements, face_to_face);

    Session session(globals::DATA_SET_INPUT_NAME);
    auto g_surf = session.add_group("Surface");
    session.add_mesh(std::make_shared<Mesh>(rf.mesh), g_surf);
    fill_session(session, rf.elements, joints, true);

    session.pb_dump((data / globals::DATA_SET_OUTPUT_FILE).string());

    return 0;
}
