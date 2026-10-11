/// in_plane_custom: the user's in-plane outlines from settings.custom("in_plane"), kept pair by pair as 2024 did; an open profile is
/// merged into the edge as the library's own fingers are.
static void in_plane_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "in_plane_custom";
    custom_pairs(joint, settings.custom("in_plane")[0], settings.custom("in_plane")[1]);
    custom_profiles_as_edge_insertion(joint);
}
