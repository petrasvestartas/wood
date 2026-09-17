/// ss_e_op_3: miter tenon-mortise - four female outlines (two insert_between, two hole), two male.
static void ss_e_op_3(WoodJoint& joint) {
    joint.name = "ss_e_op_3";
    joint.female_outlines[0] = {
        Polyline({Point(0.5,0.5,0.3), Point(0.5,-1.499975,0.3), Point(0.5,-1.499975,-0.3), Point(0.5,0.5,-0.3)}),
        Polyline({Point(0.5,0.5,0.3), Point(0.5,0.5,-0.3)}),
        Polyline({Point(0.5,-0.5,0.25), Point(0.5,0.5,0.25), Point(0.5,0.5,-0.25), Point(0.5,-0.5,-0.25), Point(0.5,-0.5,0.25)}),
        Polyline({Point(0.5,-0.5,0.25), Point(0.5,0.5,0.25), Point(0.5,0.5,-0.25), Point(0.5,-0.5,-0.25), Point(0.5,-0.5,0.25)}),
    };
    joint.female_outlines[1] = {
        Polyline({Point(-0.5,-0.499975,0.3), Point(-0.5,-1.49995,0.3), Point(-0.5,-1.49995,-0.3), Point(-0.5,-0.5,-0.3)}),
        Polyline({Point(-0.5,-0.499975,0.3), Point(-0.5,-0.5,-0.3)}),
        Polyline({Point(-0.5,-0.499975,0.25), Point(-0.5,0.500025,0.25), Point(-0.5,0.500025,-0.25), Point(-0.5,-0.499975,-0.25), Point(-0.5,-0.499975,0.25)}),
        Polyline({Point(-0.5,-0.499975,0.25), Point(-0.5,0.500025,0.25), Point(-0.5,0.500025,-0.25), Point(-0.5,-0.499975,-0.25), Point(-0.5,-0.499975,0.25)}),
    };
    joint.male_outlines[0] = {
        Polyline({Point(-0.5,-0.5,0.3), Point(0.5,-0.5,0.3), Point(0.5,-0.5,0.25), Point(-1,-0.5,0.25), Point(-1,-0.5,-0.25), Point(0.5,-0.5,-0.25), Point(0.5,-0.5,-0.3), Point(-0.5,-0.5,-0.3)}),
        Polyline({Point(-0.5,-0.5,0.3), Point(-0.5,-0.5,-0.3)}),
    };
    joint.male_outlines[1] = {
        Polyline({Point(0.5,0.5,0.3), Point(0.510075,0.5,0.3), Point(0.5,0.5,0.25), Point(-1,0.5,0.25), Point(-1,0.5,-0.25), Point(0.5,0.5,-0.25), Point(0.510075,0.5,-0.3), Point(0.5,0.5,-0.3)}),
        Polyline({Point(0.5,0.5,0.3), Point(0.5,0.5,-0.3)}),
    };
    for (int face = 0; face < 2; face++) {
        joint.female_cut_types[face] = { wood_cut::insert_between_multiple_edges, wood_cut::insert_between_multiple_edges, wood_cut::hole, wood_cut::hole };
        joint.male_cut_types[face] = { wood_cut::insert_between_multiple_edges, wood_cut::insert_between_multiple_edges };
    }
}
