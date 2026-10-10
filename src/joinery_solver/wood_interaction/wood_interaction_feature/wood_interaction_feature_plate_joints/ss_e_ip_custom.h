/// ss_e_ip_custom: the user's in-plane outlines from settings.custom("ss_e_ip"), kept pair by pair as 2024 did; an open profile is
/// merged into the edge as the library's own fingers are.
static void ss_e_ip_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "ss_e_ip_custom";
    custom_pairs(joint, settings.custom("ss_e_ip")[0], settings.custom("ss_e_ip")[1]);
    custom_profiles_as_edge_insertion(joint);
}
