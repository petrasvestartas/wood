/// cr_c_ip_2: five base rings, no drills.
static void cr_c_ip_2(WoodJoint& joint) {

    joint.name = "cr_c_ip_2";

    const std::vector<int> ct = {
        CutType::mill_project, CutType::mill_project,
        CutType::slice_projectsheer, CutType::slice_projectsheer, CutType::slice_projectsheer, CutType::slice_projectsheer,
        CutType::mill_project, CutType::mill_project, CutType::mill_project, CutType::mill_project,
    };

    cr_c_ip_shared(joint, {}, 0.15, 0.6, 1.0, 1.0, 0, ct);
}
