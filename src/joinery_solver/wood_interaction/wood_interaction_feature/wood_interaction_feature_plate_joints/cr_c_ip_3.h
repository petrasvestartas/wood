/// cr_c_ip_3: five base rings plus two diagonal drills.
static void cr_c_ip_3(FeaturePlate& joint) {

    joint.name = "cr_c_ip_3";

    const std::vector<Polyline> drills = {
        Polyline({Point(0.3, 0.041421, -0.928477), Point(0.041421, 0.3, 0.928477)}),
        Polyline({Point(-0.3, -0.041421, -0.928477), Point(-0.041421, -0.3, 0.928477)}),
    };
    const std::vector<int> ct = {
        FabricationType::mill_project, FabricationType::mill_project,
        FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer,
        FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project,
        FabricationType::drill, FabricationType::drill, FabricationType::drill, FabricationType::drill,
    };

    cr_c_ip_core(joint, drills, 0.15, 0.6, 1.0, 1.0, 2, ct);
}
