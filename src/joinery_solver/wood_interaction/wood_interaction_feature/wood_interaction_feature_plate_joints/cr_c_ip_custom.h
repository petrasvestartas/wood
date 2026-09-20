/// cr_c_ip_custom: user-supplied cross joint from settings.custom("cr_c_ip").
static void cr_c_ip_custom(FeaturePlate& joint, const Settings& settings) {
    joint.name = "cr_c_ip_custom";
    custom_outlines(joint, settings.custom("cr_c_ip")[0], settings.custom("cr_c_ip")[1]);
}
