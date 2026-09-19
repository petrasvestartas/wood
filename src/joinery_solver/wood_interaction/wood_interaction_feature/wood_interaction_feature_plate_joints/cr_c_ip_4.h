/// cr_c_ip_4: five base rings plus one vertical drill.
static void cr_c_ip_4(FeaturePlate& joint) {

    joint.name = "cr_c_ip_4";

    const std::vector<Polyline> drills = {
        Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)}),
    };
    const std::vector<int> ct = {
        FabricationType::mill_project, FabricationType::mill_project,
        FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer,
        FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project,
        FabricationType::drill, FabricationType::drill,
    };

    cr_c_ip_core(joint, drills, 0.15, 0.6, 1.0, 1.0, 1, ct);
}
