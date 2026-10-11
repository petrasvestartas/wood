/// corner_custom: the user's out-of-plane outlines from settings.custom("corner"), kept pair by pair as 2024 kept them; an open
/// profile is merged into the edge as the library's own fingers are.
static void corner_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "corner_custom";
    custom_pairs(joint, settings.custom("corner")[0], settings.custom("corner")[1]);
    custom_profiles_as_edge_insertion(joint);
}
