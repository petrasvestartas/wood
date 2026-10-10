/// cr_c_ip_3: the five rings of the half-lap, the bottom sides extended 0.15 to each side, and two diagonal drills through the lap.
static void cr_c_ip_3(InteractionFeaturePlate& joint) {

    joint.name = "cr_c_ip_3";
    const std::vector<Polyline> drills = {
        Polyline({Point(0.3, 0.041421, -0.928477), Point(0.041421, 0.3, 0.928477)}),
        Polyline({Point(-0.3, -0.041421, -0.928477), Point(-0.041421, -0.3, 0.928477)}),
    };
    cr_c_ip_core(joint, 0.15, 0.15, drills, {FabricationType::drill, FabricationType::drill});
}
