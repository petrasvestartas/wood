/// side_side_rotated_custom: the user's rotated outlines from settings.custom("side_side_rotated"), pair by pair: an open profile and its end line merged into
/// the plate's edge as the library's fingers are, a closed pair milled out of the plate between its two outlines, so a pocket may stop
/// inside the thickness (2024 kept every pair uncut and clipped a closed one through the plate).
static void side_side_rotated_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "side_side_rotated_custom";
    custom_pairs(joint, settings.custom("side_side_rotated")[0], settings.custom("side_side_rotated")[1]);
    custom_profiles_as_edge_insertion(joint);

    for (int side = 0; side < 2; side++) {
        std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? joint.male_outlines : joint.female_outlines;
        std::array<std::vector<int>, 2>& types = side == 0 ? joint.male_fabrication_types : joint.female_fabrication_types;
        for (int face = 0; face < 2; face++)
            for (size_t k = 0; k < outlines[face].size(); k++)
                if (outlines[face][k].is_closed())
                    types[face][k] = FabricationType::mill;
    }
}
