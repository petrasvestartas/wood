static void cross_brussels_half_lap(InteractionFeaturePlate& joint) {

    joint.name = "cross_brussels_half_lap";
    const std::vector<Polyline> drills = {
        Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)}),
        Polyline({Point(-0.5, 0.0, -0.55), Point(0.5, 0.0, -0.55)}),
    };
    cross_core(
        joint,
        0.15 * 1.8,
        -0.15 * 0.5,
        drills,
        {FabricationType::drill_50, FabricationType::drill_10}
    );
}
