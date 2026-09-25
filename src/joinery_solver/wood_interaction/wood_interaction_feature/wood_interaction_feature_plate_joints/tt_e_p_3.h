/// Drill grid along the offset area boundary; open rings by `distance_squared`.
static void tt_e_p_3(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements, double distance_squared) {

    joint.name = "tt_e_p_3";
    joint.no_orient = true;

    boundary_drill(joint, elements, joint.division_length, distance_squared);
}
