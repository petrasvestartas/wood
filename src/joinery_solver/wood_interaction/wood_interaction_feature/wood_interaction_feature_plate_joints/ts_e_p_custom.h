/// ts_e_p_custom: the user's top-side outlines from settings.custom("ts_e_p"), kept pair by pair as 2024 kept them, and cut as the
/// library's own top-side designs cut them (2024 left them uncut): the upright's open profile and its end line an edge insertion, the
/// base's closed outlines its mortise holes.
static void ts_e_p_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "ts_e_p_custom";
    custom_pairs(joint, settings.custom("ts_e_p")[0], settings.custom("ts_e_p")[1]);
    custom_profiles_as_edge_insertion(joint);

    for (int face = 0; face < 2; face++)
        joint.female_fabrication_types[face].assign(joint.female_outlines[face].size(), FabricationType::hole);
}
