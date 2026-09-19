/// b_custom: user-supplied beam joint from CUSTOM_JOINTS_B_MALE / FEMALE.
static void b_custom(WoodJoint& joint) {
    joint.name = "b_custom";
    custom_outlines(joint, wood_session::config::CUSTOM_JOINTS_B_MALE, wood_session::config::CUSTOM_JOINTS_B_FEMALE);
}
