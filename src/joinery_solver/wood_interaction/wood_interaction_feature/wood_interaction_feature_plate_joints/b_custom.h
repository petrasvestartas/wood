/// b_custom: user-supplied beam joint from settings.custom("b").
static void b_custom(InteractionFeaturePlate& joint, const Settings& settings) {
    joint.name = "b_custom";
    custom_outlines(joint, settings.custom("b")[0], settings.custom("b")[1]);
}
