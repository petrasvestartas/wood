/// ss_e_op_custom: the user's out-of-plane outlines from settings.custom("ss_e_op"), kept pair by pair as 2024 kept them. A closed
/// rectangle is clipped into the plate as 2024 clipped a custom pair; an open profile, which 2024 left uncut, is merged into the edge as the
/// library's own fingers are, an edge insertion, listed once with its end line.
static void ss_e_op_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "ss_e_op_custom";
    custom_pairs(joint, settings.custom("ss_e_op")[0], settings.custom("ss_e_op")[1]);

    // an open profile and its end line once each, as the library's fingers list them, where custom_pairs wrote every outline twice
    for (int side = 0; side < 2; side++) {
        std::array<std::vector<Polyline>, 2>& outlines = side == 0 ? joint.male_outlines : joint.female_outlines;
        std::array<std::vector<int>, 2>& types = side == 0 ? joint.male_fabrication_types : joint.female_fabrication_types;
        for (int face = 0; face < 2; face++) {
            if (outlines[face].empty() || outlines[face][0].is_closed())
                continue;
            std::vector<Polyline> once;
            for (size_t k = 0; k < outlines[face].size(); k += 2)
                once.push_back(outlines[face][k]);
            outlines[face] = once;
            types[face].assign(once.size(), FabricationType::edge_insertion);
        }
    }
}
