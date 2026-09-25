/// A tooth tiled `divisions` times along z, copy i shifted by mv_end + mv_step * i.
static std::vector<Point> tile_tooth_along_z(const std::vector<Point>& base, int divisions, double mv_end, double mv_step) {

    std::vector<Point> out;
    out.reserve(base.size() * divisions);
    for (int i = 0; i < divisions; ++i) {
        const double dz = mv_end + mv_step * i;
        for (const Point& p : base)
            out.emplace_back(p[0], p[1], p[2] + dz);
    }

    return out;
}

/// ss_e_ip_2: butterfly (X-fix) joint - `divisions` copies of a four-point tooth tiled along z; unit_scale.
static void ss_e_ip_2(InteractionFeaturePlate& joint) {

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

    const std::vector<Point> m0 = tile_tooth_along_z(base_m0, divisions, mv_end, mv_step);
    const std::vector<Point> m1 = tile_tooth_along_z(base_m1, divisions, mv_end, mv_step);
    const std::vector<Point> f0 = tile_tooth_along_z(base_f0, divisions, mv_end, mv_step);
    const std::vector<Point> f1 = tile_tooth_along_z(base_f1, divisions, mv_end, mv_step);

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
