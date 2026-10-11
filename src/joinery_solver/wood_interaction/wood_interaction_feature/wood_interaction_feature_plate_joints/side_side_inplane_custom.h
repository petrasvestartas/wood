/// side_side_inplane_custom: the user's in-plane outlines from settings.custom("side_side_inplane"), kept pair by pair as 2024 did; an open profile is
/// merged into the edge as the library's own fingers are.
static void side_side_inplane_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "side_side_inplane_custom";
    custom_pairs(joint, settings.custom("side_side_inplane")[0], settings.custom("side_side_inplane")[1]);
    custom_profiles_as_edge_insertion(joint);
}
