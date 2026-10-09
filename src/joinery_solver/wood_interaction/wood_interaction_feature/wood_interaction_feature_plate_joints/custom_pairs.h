/// The user's outlines as 2024 kept them: every pair (face0, face1) of the male and of the female list is its own polyline
/// on each face, written twice so the copy stands where a library design keeps its endpoint marker, with the fabrication
/// type nothing; no divisions and no unit scale, the unit box is mapped onto the full contact.
static void custom_pairs(InteractionFeaturePlate& joint, const std::vector<Polyline>& cm, const std::vector<Polyline>& cf) {

    for (size_t i = 0; i + 1 < cm.size(); i += 2) {
        joint.male_outlines[0].push_back(cm[i]);
        joint.male_outlines[0].push_back(cm[i]);
        joint.male_outlines[1].push_back(cm[i + 1]);
        joint.male_outlines[1].push_back(cm[i + 1]);
        joint.male_fabrication_types[0].insert(joint.male_fabrication_types[0].end(), {FabricationType::nothing, FabricationType::nothing});
        joint.male_fabrication_types[1].insert(joint.male_fabrication_types[1].end(), {FabricationType::nothing, FabricationType::nothing});
    }

    for (size_t i = 0; i + 1 < cf.size(); i += 2) {
        joint.female_outlines[0].push_back(cf[i]);
        joint.female_outlines[0].push_back(cf[i]);
        joint.female_outlines[1].push_back(cf[i + 1]);
        joint.female_outlines[1].push_back(cf[i + 1]);
        joint.female_fabrication_types[0].insert(joint.female_fabrication_types[0].end(), {FabricationType::nothing, FabricationType::nothing});
        joint.female_fabrication_types[1].insert(joint.female_fabrication_types[1].end(), {FabricationType::nothing, FabricationType::nothing});
    }
}
