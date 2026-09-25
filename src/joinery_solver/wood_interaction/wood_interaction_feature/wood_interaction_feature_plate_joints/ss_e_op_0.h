/// ss_e_op_0: hardcoded three-finger out-of-plane joint.
static void ss_e_op_0(InteractionFeaturePlate& joint) {

    joint.name = "ss_e_op_0";

    const double a = 0.357142857142857;
    const double b = 0.214285714285714;
    const double c = 0.0714285714285715;

    const Polyline f0_outline({
        Point( 0.5,  0.5, -a), Point( 0.5, -0.5, -a),
        Point( 0.5, -0.5, -b), Point( 0.5,  0.5, -b),
        Point( 0.5,  0.5, -c), Point( 0.5, -0.5, -c),
        Point( 0.5, -0.5,  c), Point( 0.5,  0.5,  c),
        Point( 0.5,  0.5,  b), Point( 0.5, -0.5,  b),
        Point( 0.5, -0.5,  a), Point( 0.5,  0.5,  a)
    });
    const Polyline f0_endpoints({ Point( 0.5, 0.5, -0.5), Point( 0.5, 0.5, 0.5) });
    joint.female_outlines[0] = { f0_outline, f0_endpoints };

    const Polyline f1_outline({
        Point(-0.5,  0.5, -a), Point(-0.5, -0.5, -a),
        Point(-0.5, -0.5, -b), Point(-0.5,  0.5, -b),
        Point(-0.5,  0.5, -c), Point(-0.5, -0.5, -c),
        Point(-0.5, -0.5,  c), Point(-0.5,  0.5,  c),
        Point(-0.5,  0.5,  b), Point(-0.5, -0.5,  b),
        Point(-0.5, -0.5,  a), Point(-0.5,  0.5,  a)
    });
    const Polyline f1_endpoints({ Point(-0.5, 0.5, -0.5), Point(-0.5, 0.5, 0.5) });
    joint.female_outlines[1] = { f1_outline, f1_endpoints };

    const Polyline m0_outline({
        Point(-0.5,  0.5,  a), Point( 0.5,  0.5,  a),
        Point( 0.5,  0.5,  b), Point(-0.5,  0.5,  b),
        Point(-0.5,  0.5,  c), Point( 0.5,  0.5,  c),
        Point( 0.5,  0.5, -c), Point(-0.5,  0.5, -c),
        Point(-0.5,  0.5, -b), Point( 0.5,  0.5, -b),
        Point( 0.5,  0.5, -a), Point(-0.5,  0.5, -a)
    });
    const Polyline m0_endpoints({ Point(-0.5, 0.5, 0.5), Point(-0.5, 0.5, -0.5) });
    joint.male_outlines[0] = { m0_outline, m0_endpoints };

    const Polyline m1_outline({
        Point(-0.5, -0.5,  a), Point( 0.5, -0.5,  a),
        Point( 0.5, -0.5,  b), Point(-0.5, -0.5,  b),
        Point(-0.5, -0.5,  c), Point( 0.5, -0.5,  c),
        Point( 0.5, -0.5, -c), Point(-0.5, -0.5, -c),
        Point(-0.5, -0.5, -b), Point( 0.5, -0.5, -b),
        Point( 0.5, -0.5, -a), Point(-0.5, -0.5, -a)
    });
    const Polyline m1_endpoints({ Point(-0.5, -0.5, 0.5), Point(-0.5, -0.5, -0.5) });
    joint.male_outlines[1] = { m1_outline, m1_endpoints };

    joint.female_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.female_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
}
