/// corner_notch: one rectangular notch - the worked example of main_wood_03_new_joint.cpp.
static void corner_notch(InteractionFeaturePlate& joint) {

    joint.name = "corner_notch";


    joint.female_outlines[0] = {
        Polyline({
            Point( 0.5,  0.5, -0.25), Point( 0.5, -0.5, -0.25),
            Point( 0.5, -0.5,  0.25), Point( 0.5,  0.5,  0.25)
        }),
        Polyline({ Point(0.5, 0.5, -0.5), Point(0.5, 0.5, 0.5) })
    };
    joint.female_outlines[1] = {
        Polyline({
            Point(-0.5,  0.5, -0.25), Point(-0.5, -0.5, -0.25),
            Point(-0.5, -0.5,  0.25), Point(-0.5,  0.5,  0.25)
        }),
        Polyline({ Point(-0.5, 0.5, -0.5), Point(-0.5, 0.5, 0.5) })
    };

    joint.male_outlines[0] = {
        Polyline({
            Point(-0.5, 0.5, -0.25), Point( 0.5, 0.5, -0.25),
            Point( 0.5, 0.5,  0.25), Point(-0.5, 0.5,  0.25)
        }),
        Polyline({ Point(-0.5, 0.5, 0.5), Point(-0.5, 0.5, -0.5) })
    };
    joint.male_outlines[1] = {
        Polyline({
            Point(-0.5, -0.5, -0.25), Point( 0.5, -0.5, -0.25),
            Point( 0.5, -0.5,  0.25), Point(-0.5, -0.5,  0.25)
        }),
        Polyline({ Point(-0.5, -0.5, 0.5), Point(-0.5, -0.5, -0.5) })
    };

    joint.female_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.female_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
}
