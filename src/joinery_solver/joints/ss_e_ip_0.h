/// ss_e_ip_0: hardcoded three-finger in-plane joint; male and female share the outline on each face.
static void ss_e_ip_0(WoodJoint& joint) {
    joint.name = "ss_e_ip_0";
    auto P = [](double x, double y, double z) { return Point(x, y, z); };
    const double a = 0.357142857142857;
    const double b = 0.214285714285714;
    const double c = 0.0714285714285715;

    const std::vector<Polyline> minus = {
        Polyline({
            P( 0.0,-0.5, a),  P(-0.5,-0.5, a),
            P(-0.5,-0.5, b),  P( 0.5,-0.5, b),
            P( 0.5,-0.5, c),  P(-0.5,-0.5, c),
            P(-0.5,-0.5,-c),  P( 0.5,-0.5,-c),
            P( 0.5,-0.5,-b),  P(-0.5,-0.5,-b),
            P(-0.5,-0.5,-a),  P( 0.0,-0.5,-a)}),
        Polyline({P( 0.0,-0.5, 0.5), P( 0.0,-0.5,-0.5)}),
    };
    const std::vector<Polyline> plus = {
        Polyline({
            P( 0.0, 0.5, a),  P(-0.5, 0.5, a),
            P(-0.5, 0.5, b),  P( 0.5, 0.5, b),
            P( 0.5, 0.5, c),  P(-0.5, 0.5, c),
            P(-0.5, 0.5,-c),  P( 0.5, 0.5,-c),
            P( 0.5, 0.5,-b),  P(-0.5, 0.5,-b),
            P(-0.5, 0.5,-a),  P( 0.0, 0.5,-a)}),
        Polyline({P( 0.0, 0.5, 0.5), P( 0.0, 0.5,-0.5)}),
    };
    joint.f_outlines[0] = minus;
    joint.f_outlines[1] = plus;
    joint.m_outlines[0] = minus;
    joint.m_outlines[1] = plus;
    joint.f_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.f_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.m_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.m_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
}
