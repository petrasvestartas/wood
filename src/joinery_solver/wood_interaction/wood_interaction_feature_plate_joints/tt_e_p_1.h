/// Single drill at the visual centre, approximated by the centroid.
static void tt_e_p_1(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "tt_e_p_1";
    joint.no_orient = true;

    centroid_drill(joint, elements);
}
