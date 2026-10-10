/// ts_e_p_custom: the user's top-side outlines from settings.custom("ts_e_p"), kept pair by pair as 2024 did.
static void ts_e_p_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "ts_e_p_custom";
    custom_pairs(joint, settings.custom("ts_e_p")[0], settings.custom("ts_e_p")[1]);
}
