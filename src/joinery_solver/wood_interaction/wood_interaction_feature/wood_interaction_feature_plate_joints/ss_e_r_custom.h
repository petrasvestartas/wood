/// ss_e_r_custom: user-supplied relief joint from settings.custom("ss_e_r").
static void ss_e_r_custom(InteractionFeaturePlate& joint, const Settings& settings) {
    joint.name = "ss_e_r_custom";
    custom_outlines(joint, settings.custom("ss_e_r")[0], settings.custom("ss_e_r")[1]);
}
