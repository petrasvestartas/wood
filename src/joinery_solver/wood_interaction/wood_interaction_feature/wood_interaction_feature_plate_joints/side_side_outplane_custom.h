static void side_side_outplane_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "side_side_outplane_custom";
    custom_pairs(
        joint, settings.custom("side_side_outplane")[0], settings.custom("side_side_outplane")[1]
    );
    custom_profiles_as_edge_insertion(joint);
}
