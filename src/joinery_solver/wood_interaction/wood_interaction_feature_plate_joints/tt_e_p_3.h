/// Drill grid along the offset area boundary; open rings by the runtime DISTANCE_SQUARED.
static void tt_e_p_3(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "tt_e_p_3";
    joint.no_orient = true;

    boundary_drill(joint, elements, joint.division_length, wood_session::config::DISTANCE_SQUARED);
}
