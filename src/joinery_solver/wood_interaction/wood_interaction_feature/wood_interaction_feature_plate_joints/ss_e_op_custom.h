/// ss_e_op_custom: the user's out-of-plane outlines from settings.custom("ss_e_op"), kept pair by pair as 2024 kept them; an open
/// profile is merged into the edge as the library's own fingers are.
static void ss_e_op_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "ss_e_op_custom";
    custom_pairs(joint, settings.custom("ss_e_op")[0], settings.custom("ss_e_op")[1]);
    custom_profiles_as_edge_insertion(joint);
}
