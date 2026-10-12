/// cross_brussels_half_lap: the five rings of the half-lap, the bottom sides extended 0.27 on their first segment and shortened 0.075
/// on their third, the asymmetry 2024 gave the Brussels sports tower, a vertical pin of a 50 mm bit through the centre
/// and a horizontal one of a 10 mm bit below the lap.
static void cross_brussels_half_lap(InteractionFeaturePlate& joint) {

    joint.name = "cross_brussels_half_lap";
    const std::vector<Polyline> drills = {
        Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)}),
        Polyline({Point(-0.5, 0.0, -0.55), Point(0.5, 0.0, -0.55)}),
    };
    cross_core(joint, 0.15 * 1.8, -0.15 * 0.5, drills, {FabricationType::drill_50, FabricationType::drill_10});
}
