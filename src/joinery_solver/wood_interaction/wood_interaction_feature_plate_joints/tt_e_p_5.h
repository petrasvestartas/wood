/// Boundary drills with the division length taken absolute.
static void tt_e_p_5(FeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "tt_e_p_5";
    joint.no_orient = true;

    boundary_drill(joint, elements, std::abs(joint.division_length), 0.01);
}
