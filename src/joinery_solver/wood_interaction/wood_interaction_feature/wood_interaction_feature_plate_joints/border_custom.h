/// border_custom: user-supplied beam joint from settings.custom("b").
static void border_custom(InteractionFeaturePlate& joint, const Settings& settings) {
    joint.name = "border_custom";
    custom_outlines(joint, settings.custom("b")[0], settings.custom("b")[1]);
}
