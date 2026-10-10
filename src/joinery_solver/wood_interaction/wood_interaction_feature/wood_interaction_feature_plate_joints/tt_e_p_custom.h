/// tt_e_p_custom, id 49: the user's top-top outlines from settings.custom("tt_e_p"), kept pair by pair as 2024 kept them, oriented to the
/// contact. A top-top contact has no edge to merge an outline into, so where 2024 left them uncut every pair is milled: a closed pair is
/// a pocket between its two outlines, a two-point pair, too thin to mill, a marker that cuts nothing.
static void tt_e_p_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "tt_e_p_custom";
    custom_pairs(joint, settings.custom("tt_e_p")[0], settings.custom("tt_e_p")[1]);

    for (int face = 0; face < 2; face++) {
        joint.male_fabrication_types[face].assign(joint.male_outlines[face].size(), FabricationType::mill);
        joint.female_fabrication_types[face].assign(joint.female_outlines[face].size(), FabricationType::mill);
    }
}
