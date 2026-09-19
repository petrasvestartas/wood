/// cr_c_ip_5: five base rings, one vertical and one horizontal drill, asymmetric side extension (1.8 / -0.5).
static void cr_c_ip_5(FeaturePlate& joint) {

    joint.name = "cr_c_ip_5";

    const std::vector<Polyline> drills = {
        Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)}),
        Polyline({Point(-0.5, 0.0, -0.55), Point(0.5, 0.0, -0.55)}),
    };
    const std::vector<int> ct = {
        FabricationType::mill_project, FabricationType::mill_project,
        FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer,
        FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project,
        FabricationType::drill, FabricationType::drill, FabricationType::drill, FabricationType::drill,
    };

    cr_c_ip_core(joint, drills, 0.15, 0.6, 1.8, -0.5, 2, ct);
}
