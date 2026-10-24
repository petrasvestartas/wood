static void cross_half_lap_drill(InteractionFeaturePlate& joint) {

    joint.name = "cross_half_lap_drill";
    const std::vector<Polyline> drills = {
        Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)}),
    };
    cross_core(joint, 0.15, 0.15, drills, {FabricationType::drill});
}
