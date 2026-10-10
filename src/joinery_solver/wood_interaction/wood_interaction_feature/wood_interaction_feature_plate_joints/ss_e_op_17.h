/// ss_e_op_17: ss_e_op_0 with N = div/2 fingers over the whole joint line, its ends squared as ss_e_op_1 squares them: every run starts and ends on the floor's bottom level and the wall's inner face, where the markers relocate the plate corners, so the mitre becomes a butt at the plate ends and both faces of a plate carry the same vertices.
static void ss_e_op_17(InteractionFeaturePlate& joint) {

    joint.name = "ss_e_op_17";

    int div = std::max(2, std::min(20, joint.divisions));
    div += div % 2;

    const int N = div / 2;
    const double d = 1.0 / (4 * N + 2);

    // the floor's two faces, x = 0.5 and x = -0.5: from the start of the joint line, the fingers swinging between the wall's faces y = 0.5 and y = -0.5
    for (int face = 0; face < 2; face++) {
        const double x = face == 0 ? 0.5 : -0.5;
        std::vector<Point> pts;
        pts.reserve(2 + 4 * N);
        pts.push_back(Point(x, 0.5, -0.5));
        for (int k = 0; k < 2 * N; k++) {
            const double z = -(2 * N - 1 - 2 * k) * d;
            if (k % 2 == 0) {
                pts.push_back(Point(x, 0.5, z));
                pts.push_back(Point(x, -0.5, z));
            } else {
                pts.push_back(Point(x, -0.5, z));
                pts.push_back(Point(x, 0.5, z));
            }
        }
        pts.push_back(Point(x, 0.5, 0.5));
        joint.female_outlines[face] = { Polyline(pts), Polyline({ pts.front(), pts.back() }) };
    }

    // the wall's two faces, y = 0.5 and y = -0.5: from the end of the joint line, the fingers swinging between the floor's faces x = -0.5 and x = 0.5
    for (int face = 0; face < 2; face++) {
        const double y = face == 0 ? 0.5 : -0.5;
        std::vector<Point> pts;
        pts.reserve(2 + 4 * N);
        pts.push_back(Point(-0.5, y, 0.5));
        for (int k = 0; k < 2 * N; k++) {
            const double z = (2 * N - 1 - 2 * k) * d;
            if (k % 2 == 0) {
                pts.push_back(Point(-0.5, y, z));
                pts.push_back(Point(0.5, y, z));
            } else {
                pts.push_back(Point(0.5, y, z));
                pts.push_back(Point(-0.5, y, z));
            }
        }
        pts.push_back(Point(-0.5, y, -0.5));
        joint.male_outlines[face] = { Polyline(pts), Polyline({ pts.front(), pts.back() }) };
    }

    joint.female_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.female_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
}
