static void cross_half_lap(InteractionFeaturePlate& joint, CrossDrills drills = CrossDrills::none) {

    joint.name = "cross_half_lap";

    std::vector<Polyline> drill_polylines;
    std::vector<int> drill_types;
    if (drills == CrossDrills::two_diagonal) {
        drill_polylines = {
            Polyline({Point(0.3, 0.041421, -0.928477), Point(0.041421, 0.3, 0.928477)}),
            Polyline({Point(-0.3, -0.041421, -0.928477), Point(-0.041421, -0.3, 0.928477)}),
        };
        drill_types = {FabricationType::drill, FabricationType::drill};
    } else if (drills == CrossDrills::one) {
        drill_polylines = {Polyline({Point(0.0, 0.0, -1.0), Point(0.0, 0.0, 1.0)})};
        drill_types = {FabricationType::drill};
    }

    cross_core(joint, 0.15, 0.15, drill_polylines, drill_types);
}
