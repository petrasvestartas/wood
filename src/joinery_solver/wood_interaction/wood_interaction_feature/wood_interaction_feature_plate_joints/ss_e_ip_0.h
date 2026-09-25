/// ss_e_ip_0: hardcoded three-finger in-plane joint; male and female share the outline on each face.
static void ss_e_ip_0(InteractionFeaturePlate& joint) {

    joint.name = "ss_e_ip_0";

    const double a = 0.357142857142857;
    const double b = 0.214285714285714;
    const double c = 0.0714285714285715;

    const std::vector<Polyline> minus = {
        Polyline({
            Point( 0.0,-0.5, a),  Point(-0.5,-0.5, a),
            Point(-0.5,-0.5, b),  Point( 0.5,-0.5, b),
            Point( 0.5,-0.5, c),  Point(-0.5,-0.5, c),
            Point(-0.5,-0.5,-c),  Point( 0.5,-0.5,-c),
            Point( 0.5,-0.5,-b),  Point(-0.5,-0.5,-b),
            Point(-0.5,-0.5,-a),  Point( 0.0,-0.5,-a)}),
        Polyline({Point( 0.0,-0.5, 0.5), Point( 0.0,-0.5,-0.5)}),
    };
    const std::vector<Polyline> plus = {
        Polyline({
            Point( 0.0, 0.5, a),  Point(-0.5, 0.5, a),
            Point(-0.5, 0.5, b),  Point( 0.5, 0.5, b),
            Point( 0.5, 0.5, c),  Point(-0.5, 0.5, c),
            Point(-0.5, 0.5,-c),  Point( 0.5, 0.5,-c),
            Point( 0.5, 0.5,-b),  Point(-0.5, 0.5,-b),
            Point(-0.5, 0.5,-a),  Point( 0.0, 0.5,-a)}),
        Polyline({Point( 0.0, 0.5, 0.5), Point( 0.0, 0.5,-0.5)}),
    };

    joint.female_outlines[0] = minus;
    joint.female_outlines[1] = plus;

    joint.male_outlines[0] = minus;
    joint.male_outlines[1] = plus;

    joint.female_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.female_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
}
