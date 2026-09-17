/// ss_e_op_tutorial: one rectangular notch - the worked example of main_wood_03_new_joint.cpp.
static void ss_e_op_tutorial(WoodJoint& joint) {

    joint.name = "ss_e_op_tutorial";

    auto P = [](double x, double y, double z) { return Point(x, y, z); };

    joint.female_outlines[0] = {
        Polyline({
            P( 0.5,  0.5, -0.25), P( 0.5, -0.5, -0.25),
            P( 0.5, -0.5,  0.25), P( 0.5,  0.5,  0.25)
        }),
        Polyline({ P(0.5, 0.5, -0.5), P(0.5, 0.5, 0.5) })
    };
    joint.female_outlines[1] = {
        Polyline({
            P(-0.5,  0.5, -0.25), P(-0.5, -0.5, -0.25),
            P(-0.5, -0.5,  0.25), P(-0.5,  0.5,  0.25)
        }),
        Polyline({ P(-0.5, 0.5, -0.5), P(-0.5, 0.5, 0.5) })
    };

    joint.male_outlines[0] = {
        Polyline({
            P(-0.5, 0.5, -0.25), P( 0.5, 0.5, -0.25),
            P( 0.5, 0.5,  0.25), P(-0.5, 0.5,  0.25)
        }),
        Polyline({ P(-0.5, 0.5, 0.5), P(-0.5, 0.5, -0.5) })
    };
    joint.male_outlines[1] = {
        Polyline({
            P(-0.5, -0.5, -0.25), P( 0.5, -0.5, -0.25),
            P( 0.5, -0.5,  0.25), P(-0.5, -0.5,  0.25)
        }),
        Polyline({ P(-0.5, -0.5, 0.5), P(-0.5, -0.5, -0.5) })
    };

    joint.female_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.female_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.male_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.male_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
}
