/// ss_e_r_custom: the user's rotated outlines from settings.custom("ss_e_r"), kept pair by pair as 2024 did.
static void ss_e_r_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "ss_e_r_custom";
    custom_pairs(joint, settings.custom("ss_e_r")[0], settings.custom("ss_e_r")[1]);
}
