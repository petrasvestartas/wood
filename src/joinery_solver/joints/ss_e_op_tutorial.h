/// ss_e_op_tutorial: one rectangular notch - the worked example of main_wood_03_new_joint.cpp.
static void ss_e_op_tutorial(WoodJoint& joint) {

    joint.name = "ss_e_op_tutorial";


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

    joint.female_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.female_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.male_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.male_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
}
