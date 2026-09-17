/// cr_c_ip_3: five base rings plus two diagonal drills.
static void cr_c_ip_3(WoodJoint& joint) {
    joint.name = "cr_c_ip_3";
    const std::vector<Polyline> drills = {
        Polyline({Point(0.3, 0.041421, -0.928477), Point(0.041421, 0.3, 0.928477)}),
        Polyline({Point(-0.3, -0.041421, -0.928477), Point(-0.041421, -0.3, 0.928477)}),
    };
    const std::vector<int> ct = {
        wood_cut::mill_project, wood_cut::mill_project,
        wood_cut::slice_projectsheer, wood_cut::slice_projectsheer, wood_cut::slice_projectsheer, wood_cut::slice_projectsheer,
        wood_cut::mill_project, wood_cut::mill_project, wood_cut::mill_project, wood_cut::mill_project,
        wood_cut::drill, wood_cut::drill, wood_cut::drill, wood_cut::drill,
    };
    cr_c_ip_shared(joint, drills, 0.15, 0.6, 1.0, 1.0, 2, ct);
}
