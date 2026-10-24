static void side_side_outplane_miter_tenon(InteractionFeaturePlate& joint) {

    joint.name = "side_side_outplane_miter_tenon";

    joint.female_outlines[0] = {
        Polyline(
            {Point(0.5, 0.5, 0.3),
             Point(0.5, -1.499975, 0.3),
             Point(0.5, -1.499975, -0.3),
             Point(0.5, 0.5, -0.3)}
        ),
        Polyline({Point(0.5, 0.5, 0.3), Point(0.5, 0.5, -0.3)}),
        Polyline(
            {Point(0.5, -0.5, 0.15),
             Point(0.5, 0.5, 0.15),
             Point(0.5, 0.5, -0.15),
             Point(0.5, -0.5, -0.15),
             Point(0.5, -0.5, 0.15)}
        ),
        Polyline(
            {Point(0.5, -0.5, 0.15),
             Point(0.5, 0.5, 0.15),
             Point(0.5, 0.5, -0.15),
             Point(0.5, -0.5, -0.15),
             Point(0.5, -0.5, 0.15)}
        ),
    };
    joint.female_outlines[1] = {
        Polyline(
            {Point(-0.5, -0.499975, 0.3),
             Point(-0.5, -1.49995, 0.3),
             Point(-0.5, -1.49995, -0.3),
             Point(-0.5, -0.5, -0.3)}
        ),
        Polyline({Point(-0.5, -0.499975, 0.3), Point(-0.5, -0.5, -0.3)}),
        Polyline(
            {Point(-0.5, -0.499975, 0.15),
             Point(-0.5, 0.500025, 0.15),
             Point(-0.5, 0.500025, -0.15),
             Point(-0.5, -0.499975, -0.15),
             Point(-0.5, -0.499975, 0.15)}
        ),
        Polyline(
            {Point(-0.5, -0.499975, 0.15),
             Point(-0.5, 0.500025, 0.15),
             Point(-0.5, 0.500025, -0.15),
             Point(-0.5, -0.499975, -0.15),
             Point(-0.5, -0.499975, 0.15)}
        ),
    };

    joint.male_outlines[0] = {
        Polyline(
            {Point(-0.5, -0.5, 0.3),
             Point(0.5, -0.5, 0.3),
             Point(0.5, -0.5, 0.15),
             Point(-0.5, -0.5, 0.15),
             Point(-0.5, -0.5, -0.15),
             Point(0.5, -0.5, -0.15),
             Point(0.5, -0.5, -0.3),
             Point(-0.5, -0.5, -0.3)}
        ),
        Polyline({Point(-0.5, -0.5, 0.3), Point(-0.5, -0.5, -0.3)}),
    };
    joint.male_outlines[1] = {
        Polyline(
            {Point(0.5, 0.5, 0.3),
             Point(0.510075, 0.5, 0.3),
             Point(0.5, 0.5, 0.15),
             Point(-0.5, 0.5, 0.15),
             Point(-0.5, 0.5, -0.15),
             Point(0.5, 0.5, -0.15),
             Point(0.510075, 0.5, -0.3),
             Point(0.5, 0.5, -0.3)}
        ),
        Polyline({Point(0.5, 0.5, 0.3), Point(0.5, 0.5, -0.3)}),
    };

    for (int face = 0; face < 2; face++) {
        joint.female_fabrication_types[face] = {
            FabricationType::insert_between_multiple_edges,
            FabricationType::insert_between_multiple_edges,
            FabricationType::hole,
            FabricationType::hole
        };
        joint.male_fabrication_types[face] = {
            FabricationType::insert_between_multiple_edges,
            FabricationType::insert_between_multiple_edges
        };
    }
}
