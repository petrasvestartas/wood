/// cr_c_ip_4: the five rings of the half-lap, the bottom sides extended 0.15 to each side, and one vertical drill through its centre.
static void cr_c_ip_4(InteractionFeaturePlate& joint) {

    joint.name = "cr_c_ip_4";
    const std::vector<Polyline> drills = {
        Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)}),
    };
    cr_c_ip_core(joint, 0.15, 0.15, drills, {FabricationType::drill});
}
