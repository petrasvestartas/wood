static void side_side_outplane_capped_fingers(InteractionFeaturePlate& joint) {

    joint.name = "side_side_outplane_capped_fingers";

    int div = std::max(2, std::min(20, joint.divisions));
    div += div % 2;

    const int N = div / 2;
    const double d = 1.0 / (4 * N + 2);

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
        joint.female_outlines[face] = {Polyline(pts), Polyline({pts.front(), pts.back()})};
    }

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
        joint.male_outlines[face] = {Polyline(pts), Polyline({pts.front(), pts.back()})};
    }

    joint.female_fabrication_types[0] = {
        FabricationType::edge_insertion, FabricationType::edge_insertion
    };
    joint.female_fabrication_types[1] = {
        FabricationType::edge_insertion, FabricationType::edge_insertion
    };
    joint.male_fabrication_types[0] = {
        FabricationType::edge_insertion, FabricationType::edge_insertion
    };
    joint.male_fabrication_types[1] = {
        FabricationType::edge_insertion, FabricationType::edge_insertion
    };
}
