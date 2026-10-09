/// ss_e_ip_5: reversed-tooth in-plane joint - `divisions` copies of an eight-point tooth along z, each reversed; unit_scale
/// pinned to the male plate's thickness, the tooth pitch as ss_e_ip_2.
static void ss_e_ip_5(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "ss_e_ip_5";
    joint.unit_scale_distance = unit_scale_distance(joint, elements);

    const double edge_length = joint.joint_lines[0].length() * joint.scale[2];
    const int divisions = joint.divisions;
    const double joint_volume_edge_length = joint.unit_scale_distance;
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

    joint.male_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.female_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.female_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };

    joint.unit_scale = true;
}
