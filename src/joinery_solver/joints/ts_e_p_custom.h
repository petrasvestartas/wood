/// ts_e_p_custom: user-supplied top-side joint from CUSTOM_JOINTS_TS_E_P_MALE / FEMALE.
static void ts_e_p_custom(WoodJoint& joint) {
    joint.name = "ts_e_p_custom";
    custom_outlines(joint, wood_session::globals::CUSTOM_JOINTS_TS_E_P_MALE, wood_session::globals::CUSTOM_JOINTS_TS_E_P_FEMALE);
}
