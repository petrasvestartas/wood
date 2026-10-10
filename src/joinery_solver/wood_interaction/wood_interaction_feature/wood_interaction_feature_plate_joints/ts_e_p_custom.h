/// ts_e_p_custom: the user's top-side outlines from settings.custom("ts_e_p"), kept pair by pair as 2024 kept them. 2024 left a custom
/// pair uncut, so the base, whose outlines lie on its face, never took its mortises; here each side is cut as the library's own top-side
/// designs cut it: the upright's outlines are the profile of its tenons, merged into its edge, the base's outlines are its mortise holes.
static void ts_e_p_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "ts_e_p_custom";
    custom_pairs(joint, settings.custom("ts_e_p")[0], settings.custom("ts_e_p")[1]);

    for (int face = 0; face < 2; face++) {
        joint.male_fabrication_types[face].assign(joint.male_outlines[face].size(), FabricationType::edge_insertion);
        joint.female_fabrication_types[face].assign(joint.female_outlines[face].size(), FabricationType::hole);
    }
}
