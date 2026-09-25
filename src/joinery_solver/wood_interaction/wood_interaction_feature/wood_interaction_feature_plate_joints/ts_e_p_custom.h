/// ts_e_p_custom: user-supplied top-side joint from settings.custom("ts_e_p").
static void ts_e_p_custom(InteractionFeaturePlate& joint, const Settings& settings) {
    joint.name = "ts_e_p_custom";
    custom_outlines(joint, settings.custom("ts_e_p")[0], settings.custom("ts_e_p")[1]);
}
