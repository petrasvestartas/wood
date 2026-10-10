/// cr_c_ip_5: the five rings of the half-lap, the bottom sides extended 0.27 on their first segment and shortened 0.075
/// on their third, the asymmetry 2024 gave the Brussels sports tower, a vertical pin of a 50 mm bit through the centre
/// and a horizontal one of a 10 mm bit below the lap.
static void cr_c_ip_5(InteractionFeaturePlate& joint) {

    joint.name = "cr_c_ip_5";
    const std::vector<Polyline> drills = {
        Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)}),
        Polyline({Point(-0.5, 0.0, -0.55), Point(0.5, 0.0, -0.55)}),
    };
    cr_c_ip_core(joint, 0.15 * 1.8, -0.15 * 0.5, drills, {FabricationType::drill_50, FabricationType::drill_10});
}
