/// ss_e_ip_3: hardcoded mill-and-drill in-plane joint, six outlines per face (two mill_project, four drill).
static void ss_e_ip_3(InteractionFeaturePlate& joint) {

    joint.name = "ss_e_ip_3";

    joint.female_outlines[0] = {
        Polyline({Point(-1.25,-0.5,-0.5), Point(1,-0.5,-0.5), Point(1,-0.2,-0.5), Point(-1,0.2,-0.5), Point(-1,0.5,-0.5), Point(-1.25,0.5,-0.5), Point(-1.25,-0.5,-0.5)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
    };
    joint.female_outlines[1] = {
        Polyline({Point(-1.25,-0.5,0.5), Point(1,-0.5,0.5), Point(1,-0.2,0.5), Point(-1,0.2,0.5), Point(-1,0.5,0.5), Point(-1.25,0.5,0.5), Point(-1.25,-0.5,0.5)}),
        Polyline({Point(0,0.5,0.5), Point(0,0.5,-0.5)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
    };

    joint.male_outlines[0] = {
        Polyline({Point(1.25,0.5,-0.5), Point(-1,0.5,-0.5), Point(-1,0.2,-0.5), Point(1,-0.2,-0.5), Point(1,-0.5,-0.5), Point(1.25,-0.5,-0.5), Point(1.25,0.5,-0.5)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
    };
    joint.male_outlines[1] = {
        Polyline({Point(1.25,0.5,0.5), Point(-1,0.5,0.5), Point(-1,0.2,0.5), Point(1,-0.2,0.5), Point(1,-0.5,0.5), Point(1.25,-0.5,0.5), Point(1.25,0.5,0.5)}),
        Polyline({Point(0,0.5,0.5), Point(0,0.5,-0.5)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
    };

    for (int face = 0; face < 2; face++) {
        joint.female_fabrication_types[face] = { FabricationType::mill_project, FabricationType::mill_project, FabricationType::drill, FabricationType::drill, FabricationType::drill, FabricationType::drill };
        joint.male_fabrication_types[face] = { FabricationType::mill_project, FabricationType::mill_project, FabricationType::drill, FabricationType::drill, FabricationType::drill, FabricationType::drill };
    }
}
