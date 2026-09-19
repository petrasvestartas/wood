/// ss_e_op_custom: user-supplied out-of-plane joint from CUSTOM_JOINTS_SS_E_OP_MALE / FEMALE.
static void ss_e_op_custom(WoodJoint& joint) {
    joint.name = "ss_e_op_custom";
    custom_outlines(joint, wood_session::config::CUSTOM_JOINTS_SS_E_OP_MALE, wood_session::config::CUSTOM_JOINTS_SS_E_OP_FEMALE);
}
