/// ss_e_op_0: hardcoded three-finger out-of-plane joint.
static void ss_e_op_0(WoodJoint& joint) {
    joint.name = "ss_e_op_0";
    auto P = [](double x, double y, double z) { return Point(x, y, z); };
    const double a = 0.357142857142857;
    const double b = 0.214285714285714;
    const double c = 0.0714285714285715;
    const Polyline f0_outline({
        P( 0.5,  0.5, -a), P( 0.5, -0.5, -a),
        P( 0.5, -0.5, -b), P( 0.5,  0.5, -b),
        P( 0.5,  0.5, -c), P( 0.5, -0.5, -c),
        P( 0.5, -0.5,  c), P( 0.5,  0.5,  c),
        P( 0.5,  0.5,  b), P( 0.5, -0.5,  b),
        P( 0.5, -0.5,  a), P( 0.5,  0.5,  a)
    });
    const Polyline f0_endpoints({ P( 0.5, 0.5, -0.5), P( 0.5, 0.5, 0.5) });
    joint.f_outlines[0] = { f0_outline, f0_endpoints };

    const Polyline f1_outline({
        P(-0.5,  0.5, -a), P(-0.5, -0.5, -a),
        P(-0.5, -0.5, -b), P(-0.5,  0.5, -b),
        P(-0.5,  0.5, -c), P(-0.5, -0.5, -c),
        P(-0.5, -0.5,  c), P(-0.5,  0.5,  c),
        P(-0.5,  0.5,  b), P(-0.5, -0.5,  b),
        P(-0.5, -0.5,  a), P(-0.5,  0.5,  a)
    });
    const Polyline f1_endpoints({ P(-0.5, 0.5, -0.5), P(-0.5, 0.5, 0.5) });
    joint.f_outlines[1] = { f1_outline, f1_endpoints };

    const Polyline m0_outline({
        P(-0.5,  0.5,  a), P( 0.5,  0.5,  a),
        P( 0.5,  0.5,  b), P(-0.5,  0.5,  b),
        P(-0.5,  0.5,  c), P( 0.5,  0.5,  c),
        P( 0.5,  0.5, -c), P(-0.5,  0.5, -c),
        P(-0.5,  0.5, -b), P( 0.5,  0.5, -b),
        P( 0.5,  0.5, -a), P(-0.5,  0.5, -a)
    });
    const Polyline m0_endpoints({ P(-0.5, 0.5, 0.5), P(-0.5, 0.5, -0.5) });
    joint.m_outlines[0] = { m0_outline, m0_endpoints };

    const Polyline m1_outline({
        P(-0.5, -0.5,  a), P( 0.5, -0.5,  a),
        P( 0.5, -0.5,  b), P(-0.5, -0.5,  b),
        P(-0.5, -0.5,  c), P( 0.5, -0.5,  c),
        P( 0.5, -0.5, -c), P(-0.5, -0.5, -c),
        P(-0.5, -0.5, -b), P( 0.5, -0.5, -b),
        P( 0.5, -0.5, -a), P(-0.5, -0.5, -a)
    });
    const Polyline m1_endpoints({ P(-0.5, -0.5, 0.5), P(-0.5, -0.5, -0.5) });
    joint.m_outlines[1] = { m1_outline, m1_endpoints };
    joint.f_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.f_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.m_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.m_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
}
