/// cr_c_ip_custom: the user's cross outlines from settings.custom("cr_c_ip"), kept pair by pair as 2024 did.
static void cr_c_ip_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "cr_c_ip_custom";
    custom_pairs(joint, settings.custom("cr_c_ip")[0], settings.custom("cr_c_ip")[1]);
}
