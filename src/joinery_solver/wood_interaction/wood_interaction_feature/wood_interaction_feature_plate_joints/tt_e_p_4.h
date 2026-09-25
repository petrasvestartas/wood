/// Boundary drills, open rings by a fixed 0.01.
static void tt_e_p_4(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "tt_e_p_4";
    joint.no_orient = true;

    boundary_drill(joint, elements, joint.division_length, 0.01);
}
