/// cross_custom: the user's cross outlines from settings.custom("cross"), kept pair by pair as 2024 did, each pair inserted
/// between the plate's edges as the library's own half laps are; 2024 left them uncut.
static void cross_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "cross_custom";
    custom_pairs(joint, settings.custom("cross")[0], settings.custom("cross")[1]);

    for (int face = 0; face < 2; face++) {
        joint.male_fabrication_types[face].assign(joint.male_outlines[face].size(), FabricationType::insert_between_multiple_edges);
        joint.female_fabrication_types[face].assign(joint.female_outlines[face].size(), FabricationType::insert_between_multiple_edges);
    }
}
