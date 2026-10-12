/// side_side_outplane_custom: the user's out-of-plane outlines from settings.custom("side_side_outplane"), kept pair by pair as 2024 kept them; an open
/// profile is merged into the edge as the library's own fingers are.
static void side_side_outplane_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "side_side_outplane_custom";
    custom_pairs(joint, settings.custom("side_side_outplane")[0], settings.custom("side_side_outplane")[1]);
    custom_profiles_as_edge_insertion(joint);
}
