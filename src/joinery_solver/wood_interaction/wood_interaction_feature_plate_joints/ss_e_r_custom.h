/// ss_e_r_custom: user-supplied relief joint from CUSTOM_JOINTS_SS_E_R_MALE / FEMALE.
static void ss_e_r_custom(FeaturePlate& joint) {
    joint.name = "ss_e_r_custom";
    custom_outlines(joint, wood_session::config::CUSTOM_JOINTS_SS_E_R_MALE, wood_session::config::CUSTOM_JOINTS_SS_E_R_FEMALE);
}
