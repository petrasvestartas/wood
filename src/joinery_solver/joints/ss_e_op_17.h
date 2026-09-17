/// ss_e_op_17: ss_e_op_0 with N = div/2 fingers and flat miter caps (full cross on m0 and f1, near-duplicate on m1 and f0).
static void ss_e_op_17(WoodJoint& joint) {
    joint.name = "ss_e_op_17";
    auto P = [](double x, double y, double z) { return Point(x, y, z); };

    int div = std::max(2, std::min(20, joint.divisions));
    div += div % 2;

    const int N = div / 2;
    const double d = 1.0 / (4 * N + 2);

    {
        std::vector<Point> pts;
        pts.reserve(4 + 4 * N);
        pts.push_back(P( 0.5,  0.5, -0.5));
        pts.push_back(P( 0.5,  0.5, -0.5));
        for (int k = 0; k < 2 * N; k++) {
            const double z = -(2 * N - 1 - 2 * k) * d;
            if (k % 2 == 0) {
                pts.push_back(P( 0.5,  0.5, z));
                pts.push_back(P( 0.5, -0.5, z));
            } else {
                pts.push_back(P( 0.5, -0.5, z));
                pts.push_back(P( 0.5,  0.5, z));
            }
        }
        pts.push_back(P( 0.5,  0.5,  0.5));
        pts.push_back(P( 0.5,  0.5,  0.5));
        const Polyline f0_endpoints({ P( 0.5, 0.5, -0.5), P( 0.5, 0.5, 0.5) });
        joint.f_outlines[0] = { Polyline(pts), f0_endpoints };
    }

    {
        std::vector<Point> pts;
        pts.reserve(4 + 4 * N);
        pts.push_back(P(-0.5, -0.5, -0.5));
        pts.push_back(P(-0.5,  0.5, -0.5));
        for (int k = 0; k < 2 * N; k++) {
            const double z = -(2 * N - 1 - 2 * k) * d;
            if (k % 2 == 0) {
                pts.push_back(P(-0.5,  0.5, z));
                pts.push_back(P(-0.5, -0.5, z));
            } else {
                pts.push_back(P(-0.5, -0.5, z));
                pts.push_back(P(-0.5,  0.5, z));
            }
        }
        pts.push_back(P(-0.5,  0.5,  0.5));
        pts.push_back(P(-0.5, -0.5,  0.5));
        const Polyline f1_endpoints({ P(-0.5, -0.5, -0.5), P(-0.5, -0.5, 0.5) });
        joint.f_outlines[1] = { Polyline(pts), f1_endpoints };
    }

    {
        std::vector<Point> pts;
        pts.reserve(4 + 4 * N);
        pts.push_back(P( 0.5,  0.5,  0.5));
        pts.push_back(P(-0.5,  0.5,  0.5));
        for (int k = 0; k < 2 * N; k++) {
            const double z = (2 * N - 1 - 2 * k) * d;
            if (k % 2 == 0) {
                pts.push_back(P(-0.5,  0.5, z));
                pts.push_back(P( 0.5,  0.5, z));
            } else {
                pts.push_back(P( 0.5,  0.5, z));
                pts.push_back(P(-0.5,  0.5, z));
            }
        }
        pts.push_back(P(-0.5,  0.5, -0.5));
        pts.push_back(P( 0.5,  0.5, -0.5));
        const Polyline m0_endpoints({ P( 0.5, 0.5, 0.5), P( 0.5, 0.5, -0.5) });
        joint.m_outlines[0] = { Polyline(pts), m0_endpoints };
    }

    {
        std::vector<Point> pts;
        pts.reserve(4 + 4 * N);
        pts.push_back(P(-0.5, -0.5,  0.5));
        pts.push_back(P(-0.5, -0.5,  0.5));
        for (int k = 0; k < 2 * N; k++) {
            const double z = (2 * N - 1 - 2 * k) * d;
            if (k % 2 == 0) {
                pts.push_back(P(-0.5, -0.5, z));
                pts.push_back(P( 0.5, -0.5, z));
            } else {
                pts.push_back(P( 0.5, -0.5, z));
                pts.push_back(P(-0.5, -0.5, z));
            }
        }
        pts.push_back(P(-0.5, -0.5, -0.5));
        pts.push_back(P(-0.5, -0.5, -0.5));
        const Polyline m1_endpoints({ P(-0.5, -0.5, 0.5), P(-0.5, -0.5, -0.5) });
        joint.m_outlines[1] = { Polyline(pts), m1_endpoints };
    }

    joint.f_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.f_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.m_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.m_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
}
