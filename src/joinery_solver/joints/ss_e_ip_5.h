/// ss_e_ip_5: reversed-tooth in-plane joint - `divisions` copies of an eight-point tooth along z, each reversed; unit_scale.
static void ss_e_ip_5(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {
    joint.name = "ss_e_ip_5";

    const int v0 = index_of(elements, joint.element_a);
    if (v0 < 0 || v0 >= (int)elements.size())
        return;
    joint.unit_scale_distance = elements[v0]->thickness;

    double edge_length = 1000.0;
    {
        const double d = Point::distance(joint.joint_lines[0].start(), joint.joint_lines[0].end());
        if (d > 1e-9)
            edge_length = d;
    }
    const int divisions = std::max(1, std::min(100, joint.divisions));
    const double joint_volume_edge_length =
        (joint.unit_scale_distance > 0.0) ? joint.unit_scale_distance : 40.0;

    edge_length *= joint.scale[2];
    const double move_length_scaled = edge_length / (divisions * joint_volume_edge_length);
    const double total_length_scaled = edge_length / joint_volume_edge_length;
    const double dz_start = (total_length_scaled * 0.5) - (move_length_scaled * 0.5);
    const double dz_step  = -move_length_scaled;

    static const double m0_pts[8][3] = {
        { 0,        -0.5, -0.166667},
        {-0.116667, -0.5, -0.166667},
        {-0.619628, -0.5, -0.375   },
        {-1.0,      -0.5, -0.375   },
        {-1.0,      -0.5,  0.375   },
        {-0.619628, -0.5,  0.375   },
        {-0.116667, -0.5,  0.166667},
        { 0,        -0.5,  0.166667},
    };
    static const double f0_pts[8][3] = {
        { 0,        -0.5, -0.166667},
        { 0.116667, -0.5, -0.166667},
        { 0.619628, -0.5, -0.375   },
        { 1.0,      -0.5, -0.375   },
        { 1.0,      -0.5,  0.375   },
        { 0.619628, -0.5,  0.375   },
        { 0.116667, -0.5,  0.166667},
        { 0,        -0.5,  0.166667},
    };

    std::vector<Point> m0;
    std::vector<Point> m1;
    std::vector<Point> f0;
    std::vector<Point> f1;
    m0.reserve(8 * divisions);
    m1.reserve(8 * divisions);
    f0.reserve(8 * divisions);
    f1.reserve(8 * divisions);

    for (int i = 0; i < divisions; ++i) {
        const double dz = dz_start + dz_step * i;
        for (int k = 7; k >= 0; --k) {
            m0.emplace_back(m0_pts[k][0], m0_pts[k][1], m0_pts[k][2] + dz);
            m1.emplace_back(m0_pts[k][0], 0.5,          m0_pts[k][2] + dz);
            f0.emplace_back(f0_pts[k][0], m0_pts[k][1], f0_pts[k][2] + dz);
            f1.emplace_back(f0_pts[k][0], 0.5,          f0_pts[k][2] + dz);
        }
    }

    joint.male_outlines[0] = { Polyline(m0), Polyline({ m0.front(), m0.back() }) };
    joint.male_outlines[1] = { Polyline(m1), Polyline({ m1.front(), m1.back() }) };
    joint.female_outlines[0] = { Polyline(f0), Polyline({ f0.front(), f0.back() }) };
    joint.female_outlines[1] = { Polyline(f1), Polyline({ f1.front(), f1.back() }) };
    joint.male_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.male_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.female_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.female_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.unit_scale = true;
}
