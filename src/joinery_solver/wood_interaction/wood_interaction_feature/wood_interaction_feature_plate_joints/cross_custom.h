static void cross_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "cross_custom";
    custom_pairs(joint, settings.custom("cross")[0], settings.custom("cross")[1]);

    for (int face = 0; face < 2; face++) {
        joint.male_fabrication_types[face].assign(
            joint.male_outlines[face].size(), FabricationType::insert_between_multiple_edges
        );
        joint.female_fabrication_types[face].assign(
            joint.female_outlines[face].size(), FabricationType::insert_between_multiple_edges
        );
    }
}
