/// cr_c_ip_4: five base rings plus one vertical drill.
static void cr_c_ip_4(WoodJoint& joint) {

    joint.name = "cr_c_ip_4";

    const std::vector<Polyline> drills = {
        Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)}),
    };
    const std::vector<int> ct = {
        CutType::mill_project, CutType::mill_project,
        CutType::slice_projectsheer, CutType::slice_projectsheer, CutType::slice_projectsheer, CutType::slice_projectsheer,
        CutType::mill_project, CutType::mill_project, CutType::mill_project, CutType::mill_project,
        CutType::drill, CutType::drill,
    };

    cr_c_ip_shared(joint, drills, 0.15, 0.6, 1.0, 1.0, 1, ct);
}
