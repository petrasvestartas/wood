/// Single centroid drill.
static void tt_e_p_0(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "tt_e_p_0";
    joint.no_orient = true;

    centroid_drill(joint, elements);
}
