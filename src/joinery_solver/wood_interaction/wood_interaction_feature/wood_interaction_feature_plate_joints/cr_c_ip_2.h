/// cr_c_ip_2: five base rings, no drills.
static void cr_c_ip_2(InteractionFeaturePlate& joint) {

    joint.name = "cr_c_ip_2";

    const std::vector<int> ct = {
        FabricationType::mill_project, FabricationType::mill_project,
        FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer,
        FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project,
    };

    cr_c_ip_core(joint, {}, 0.15, 0.6, 1.0, 1.0, 0, ct);
}
