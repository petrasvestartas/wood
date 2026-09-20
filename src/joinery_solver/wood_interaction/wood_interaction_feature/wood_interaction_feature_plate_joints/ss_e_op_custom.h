/// ss_e_op_custom: user-supplied out-of-plane joint from settings.custom("ss_e_op").
static void ss_e_op_custom(FeaturePlate& joint, const Settings& settings) {
    joint.name = "ss_e_op_custom";
    custom_outlines(joint, settings.custom("ss_e_op")[0], settings.custom("ss_e_op")[1]);
}
