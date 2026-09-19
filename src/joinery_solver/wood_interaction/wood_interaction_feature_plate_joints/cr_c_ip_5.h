/// cr_c_ip_5: five base rings, one vertical and one horizontal drill, asymmetric side extension (1.8 / -0.5).
static void cr_c_ip_5(WoodJoint& joint) {

    joint.name = "cr_c_ip_5";

    const std::vector<Polyline> drills = {
        Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)}),
        Polyline({Point(-0.5, 0.0, -0.55), Point(0.5, 0.0, -0.55)}),
    };
    const std::vector<int> ct = {
        CutType::mill_project, CutType::mill_project,
        CutType::slice_projectsheer, CutType::slice_projectsheer, CutType::slice_projectsheer, CutType::slice_projectsheer,
        CutType::mill_project, CutType::mill_project, CutType::mill_project, CutType::mill_project,
        CutType::drill, CutType::drill, CutType::drill, CutType::drill,
    };

    cr_c_ip_shared(joint, drills, 0.15, 0.6, 1.8, -0.5, 2, ct);
}
