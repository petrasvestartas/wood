static void top_side_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "top_side_custom";
    custom_pairs(joint, settings.custom("top_side")[0], settings.custom("top_side")[1]);
    custom_profiles_as_edge_insertion(joint);

    // more than one mortise: each once and, as top_side_tenons lists them, the rectangle spanning the first and the last
    for (int face = 0; face < 2; face++) {
        std::vector<Polyline> once;
        for (size_t k = 0; k < joint.female_outlines[face].size(); k += 2)
            once.push_back(joint.female_outlines[face][k]);
        if (once.size() > 1 && once.front().point_count() == 5 && once.back().point_count() == 5) {
            once.push_back(Polyline(
                {once.front().get_point(0),
                 once.front().get_point(3),
                 once.back().get_point(3),
                 once.back().get_point(0),
                 once.front().get_point(0)}
            ));
            joint.female_outlines[face] = once;
        }
        joint.female_fabrication_types[face].assign(
            joint.female_outlines[face].size(), FabricationType::hole
        );
    }
}
