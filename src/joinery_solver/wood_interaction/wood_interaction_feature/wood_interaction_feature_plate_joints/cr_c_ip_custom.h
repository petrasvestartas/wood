/// cr_c_ip_custom: user-supplied cross joint from CUSTOM_JOINTS_CR_C_IP_MALE / FEMALE.
static void cr_c_ip_custom(FeaturePlate& joint) {
    joint.name = "cr_c_ip_custom";
    custom_outlines(joint, wood_session::config::CUSTOM_JOINTS_CR_C_IP_MALE, wood_session::config::CUSTOM_JOINTS_CR_C_IP_FEMALE);
}
