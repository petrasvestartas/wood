static void custom_pairs(
    InteractionFeaturePlate& joint, const std::vector<Polyline>& cm, const std::vector<Polyline>& cf
) {

    for (size_t i = 0; i + 1 < cm.size(); i += 2) {
        joint.male_outlines[0].push_back(cm[i]);
        joint.male_outlines[0].push_back(cm[i]);
        joint.male_outlines[1].push_back(cm[i + 1]);
        joint.male_outlines[1].push_back(cm[i + 1]);
        joint.male_fabrication_types[0].insert(
            joint.male_fabrication_types[0].end(),
            {FabricationType::nothing, FabricationType::nothing}
        );
        joint.male_fabrication_types[1].insert(
            joint.male_fabrication_types[1].end(),
            {FabricationType::nothing, FabricationType::nothing}
        );
    }

    for (size_t i = 0; i + 1 < cf.size(); i += 2) {
        joint.female_outlines[0].push_back(cf[i]);
        joint.female_outlines[0].push_back(cf[i]);
        joint.female_outlines[1].push_back(cf[i + 1]);
        joint.female_outlines[1].push_back(cf[i + 1]);
        joint.female_fabrication_types[0].insert(
            joint.female_fabrication_types[0].end(),
            {FabricationType::nothing, FabricationType::nothing}
        );
        joint.female_fabrication_types[1].insert(
            joint.female_fabrication_types[1].end(),
            {FabricationType::nothing, FabricationType::nothing}
        );
    }
}

/// The open profiles of a side-to-side custom joint merged into the plate's edge as the library's fingers are.
static void custom_profiles_as_edge_insertion(InteractionFeaturePlate& joint) {

    for (int side = 0; side < 2; side++) {
        std::array<std::vector<Polyline>, 2>& outlines =
            side == 0 ? joint.male_outlines : joint.female_outlines;
        std::array<std::vector<int>, 2>& types =
            side == 0 ? joint.male_fabrication_types : joint.female_fabrication_types;
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
