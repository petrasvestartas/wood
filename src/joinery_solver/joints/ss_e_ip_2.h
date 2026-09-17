/// ss_e_ip_2: butterfly (X-fix) joint - `divisions` copies of a four-point tooth tiled along z; unit_scale.
static void ss_e_ip_2(WoodJoint& joint) {
    joint.name = "ss_e_ip_2";
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
    const double mv_end  = (total_length_scaled * 0.5) - (move_length_scaled * 0.5);
    const double mv_step = -move_length_scaled;

    const std::vector<Point> base_m0 = {
        Point( 0.0, -0.5,  0.1166666667),
        Point(-0.5, -0.5,  0.4),
        Point(-0.5, -0.5, -0.4),
        Point( 0.0, -0.5, -0.1166666667),
    };
    const std::vector<Point> base_m1 = {
        Point( 0.0,  0.5,  0.1166666667),
        Point(-0.5,  0.5,  0.4),
        Point(-0.5,  0.5, -0.4),
        Point( 0.0,  0.5, -0.1166666667),
    };
    const std::vector<Point> base_f0 = {
        Point( 0.0, -0.5,  0.1166666667),
        Point( 0.5, -0.5,  0.4),
        Point( 0.5, -0.5, -0.4),
        Point( 0.0, -0.5, -0.1166666667),
    };
    const std::vector<Point> base_f1 = {
        Point( 0.0,  0.5,  0.1166666667),
        Point( 0.5,  0.5,  0.4),
        Point( 0.5,  0.5, -0.4),
        Point( 0.0,  0.5, -0.1166666667),
    };

    auto build = [&](const std::vector<Point>& base) -> std::vector<Point> {
        std::vector<Point> out;
        out.reserve(base.size() * divisions);
        for (int i = 0; i < divisions; ++i) {
            const double dz = mv_end + mv_step * i;
            for (const Point& p : base)
                out.emplace_back(p[0], p[1], p[2] + dz);
        }
        return out;
    };
    const std::vector<Point> m0 = build(base_m0);
    const std::vector<Point> m1 = build(base_m1);
    const std::vector<Point> f0 = build(base_f0);
    const std::vector<Point> f1 = build(base_f1);

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
