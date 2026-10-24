static void side_side_outplane_notch(InteractionFeaturePlate& joint) {

    joint.name = "side_side_outplane_notch";

    joint.female_outlines[0] = {
        Polyline(
            {Point(0.5, 0.5, -0.25),
             Point(0.5, -0.5, -0.25),
             Point(0.5, -0.5, 0.25),
             Point(0.5, 0.5, 0.25)}
        ),
        Polyline({Point(0.5, 0.5, -0.5), Point(0.5, 0.5, 0.5)})
    };
    joint.female_outlines[1] = {
        Polyline(
            {Point(-0.5, 0.5, -0.25),
             Point(-0.5, -0.5, -0.25),
             Point(-0.5, -0.5, 0.25),
             Point(-0.5, 0.5, 0.25)}
        ),
        Polyline({Point(-0.5, 0.5, -0.5), Point(-0.5, 0.5, 0.5)})
    };

    joint.male_outlines[0] = {
        Polyline(
            {Point(-0.5, 0.5, -0.25),
             Point(0.5, 0.5, -0.25),
             Point(0.5, 0.5, 0.25),
             Point(-0.5, 0.5, 0.25)}
        ),
        Polyline({Point(-0.5, 0.5, 0.5), Point(-0.5, 0.5, -0.5)})
    };
    joint.male_outlines[1] = {
        Polyline(
            {Point(-0.5, -0.5, -0.25),
             Point(0.5, -0.5, -0.25),
             Point(0.5, -0.5, 0.25),
             Point(-0.5, -0.5, 0.25)}
        ),
        Polyline({Point(-0.5, -0.5, 0.5), Point(-0.5, -0.5, -0.5)})
    };

    joint.female_fabrication_types[0] = {
        FabricationType::edge_insertion, FabricationType::edge_insertion
    };
    joint.female_fabrication_types[1] = {
        FabricationType::edge_insertion, FabricationType::edge_insertion
    };
    joint.male_fabrication_types[0] = {
        FabricationType::edge_insertion, FabricationType::edge_insertion
    };
    joint.male_fabrication_types[1] = {
        FabricationType::edge_insertion, FabricationType::edge_insertion
    };
}
