/// ss_e_ip_custom: each user pair (face0, face1) from CUSTOM_JOINTS_SS_E_IP_MALE / FEMALE is one tooth,
/// tiled `divisions` times along z like ss_e_ip_2 and concatenated into one outline per face; unit_scale.
static void ss_e_ip_custom(WoodJoint& joint) {
    joint.name = "ss_e_ip_custom";

    const auto& cm = wood_session::globals::CUSTOM_JOINTS_SS_E_IP_MALE;
    const auto& cf = wood_session::globals::CUSTOM_JOINTS_SS_E_IP_FEMALE;
    if (cm.size() < 2 || cf.size() < 2)
        return;

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

    auto tile_face = [&](const std::vector<Polyline>& src, bool pick_face0, std::vector<Point>& out) {
        const size_t n_pairs = src.size() / 2;
        out.reserve(divisions * 8);
        for (int i = 0; i < divisions; ++i) {
            const double dz = mv_end + mv_step * i;
            const Polyline& base = pick_face0 ? src[2 * (i % n_pairs)] : src[2 * (i % n_pairs) + 1];
            for (size_t k = 0; k < base.point_count(); k++) {
                const Point p = base.get_point(k);
                out.emplace_back(p[0], p[1], p[2] + dz);
            }
        }
    };

    std::vector<Point> m0;
    std::vector<Point> m1;
    std::vector<Point> f0;
    std::vector<Point> f1;
    tile_face(cm, true,  m0);
    tile_face(cm, false, m1);
    tile_face(cf, true,  f0);
    tile_face(cf, false, f1);
    if (m0.empty() || m1.empty() || f0.empty() || f1.empty())
        return;

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
