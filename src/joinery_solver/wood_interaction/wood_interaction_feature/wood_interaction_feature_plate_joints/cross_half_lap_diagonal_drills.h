static void cross_half_lap_diagonal_drills(InteractionFeaturePlate& joint) {

    joint.name = "cross_half_lap_diagonal_drills";
    const std::vector<Polyline> drills = {
        Polyline({Point(0.3, 0.041421, -0.928477), Point(0.041421, 0.3, 0.928477)}),
        Polyline({Point(-0.3, -0.041421, -0.928477), Point(-0.041421, -0.3, 0.928477)}),
    };
    cross_core(joint, 0.15, 0.15, drills, {FabricationType::drill, FabricationType::drill});
}
