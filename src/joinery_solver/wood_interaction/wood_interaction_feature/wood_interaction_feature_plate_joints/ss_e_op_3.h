/// ss_e_op_3: miter tenon-mortise - four female outlines (two insert_between, two hole), two male; the tenon and its mortise 0.3 of the
/// joint line wide, a shoulder of 0.15 on each side. 2024 ran the tenon half a thickness
/// past the floor; here it ends flush with the floor's bottom. The floor's lip around the mortise still runs a thickness past the wall.
static void ss_e_op_3(InteractionFeaturePlate& joint) {

    joint.name = "ss_e_op_3";

    joint.female_outlines[0] = {
        Polyline({Point(0.5,0.5,0.3), Point(0.5,-1.499975,0.3), Point(0.5,-1.499975,-0.3), Point(0.5,0.5,-0.3)}),
        Polyline({Point(0.5,0.5,0.3), Point(0.5,0.5,-0.3)}),
        Polyline({Point(0.5,-0.5,0.15), Point(0.5,0.5,0.15), Point(0.5,0.5,-0.15), Point(0.5,-0.5,-0.15), Point(0.5,-0.5,0.15)}),
        Polyline({Point(0.5,-0.5,0.15), Point(0.5,0.5,0.15), Point(0.5,0.5,-0.15), Point(0.5,-0.5,-0.15), Point(0.5,-0.5,0.15)}),
    };
    joint.female_outlines[1] = {
        Polyline({Point(-0.5,-0.499975,0.3), Point(-0.5,-1.49995,0.3), Point(-0.5,-1.49995,-0.3), Point(-0.5,-0.5,-0.3)}),
        Polyline({Point(-0.5,-0.499975,0.3), Point(-0.5,-0.5,-0.3)}),
        Polyline({Point(-0.5,-0.499975,0.15), Point(-0.5,0.500025,0.15), Point(-0.5,0.500025,-0.15), Point(-0.5,-0.499975,-0.15), Point(-0.5,-0.499975,0.15)}),
        Polyline({Point(-0.5,-0.499975,0.15), Point(-0.5,0.500025,0.15), Point(-0.5,0.500025,-0.15), Point(-0.5,-0.499975,-0.15), Point(-0.5,-0.499975,0.15)}),
    };

    joint.male_outlines[0] = {
        Polyline({Point(-0.5,-0.5,0.3), Point(0.5,-0.5,0.3), Point(0.5,-0.5,0.15), Point(-0.5,-0.5,0.15), Point(-0.5,-0.5,-0.15), Point(0.5,-0.5,-0.15), Point(0.5,-0.5,-0.3), Point(-0.5,-0.5,-0.3)}),
        Polyline({Point(-0.5,-0.5,0.3), Point(-0.5,-0.5,-0.3)}),
    };
    joint.male_outlines[1] = {
        Polyline({Point(0.5,0.5,0.3), Point(0.510075,0.5,0.3), Point(0.5,0.5,0.15), Point(-0.5,0.5,0.15), Point(-0.5,0.5,-0.15), Point(0.5,0.5,-0.15), Point(0.510075,0.5,-0.3), Point(0.5,0.5,-0.3)}),
        Polyline({Point(0.5,0.5,0.3), Point(0.5,0.5,-0.3)}),
    };

    for (int face = 0; face < 2; face++) {
        joint.female_fabrication_types[face] = { FabricationType::insert_between_multiple_edges, FabricationType::insert_between_multiple_edges, FabricationType::hole, FabricationType::hole };
        joint.male_fabrication_types[face] = { FabricationType::insert_between_multiple_edges, FabricationType::insert_between_multiple_edges };
    }
}
