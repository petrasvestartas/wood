static void top_top_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "top_top_custom";
    custom_pairs(joint, settings.custom("top_top")[0], settings.custom("top_top")[1]);

    for (int face = 0; face < 2; face++) {
        joint.male_fabrication_types[face].assign(
            joint.male_outlines[face].size(), FabricationType::mill
        );
        joint.female_fabrication_types[face].assign(
            joint.female_outlines[face].size(), FabricationType::mill
        );
    }
}
