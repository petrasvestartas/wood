/// User-supplied joint geometry: pairs (face0, face1) from the male and female lists, every pair
/// concatenated into one outline per face plus a two-point endpoint marker, all edge_insertion.
static void custom_outlines(WoodJoint& joint, const std::vector<Polyline>& cm, const std::vector<Polyline>& cf) {
    if (cm.size() < 2 || cf.size() < 2)
        return;
    std::vector<Point> m0;
    std::vector<Point> m1;
    std::vector<Point> f0;
    std::vector<Point> f1;
    for (size_t i = 0; i + 1 < cm.size(); i += 2) {
        for (size_t k = 0; k < cm[i].point_count(); k++)
            m0.push_back(cm[i].get_point(k));
        for (size_t k = 0; k < cm[i + 1].point_count(); k++)
            m1.push_back(cm[i + 1].get_point(k));
    }
    for (size_t i = 0; i + 1 < cf.size(); i += 2) {
        for (size_t k = 0; k < cf[i].point_count(); k++)
            f0.push_back(cf[i].get_point(k));
        for (size_t k = 0; k < cf[i + 1].point_count(); k++)
            f1.push_back(cf[i + 1].get_point(k));
    }
    if (m0.empty() || m1.empty() || f0.empty() || f1.empty())
        return;
    joint.m_outlines[0] = { Polyline(m0), Polyline({ m0.front(), m0.back() }) };
    joint.m_outlines[1] = { Polyline(m1), Polyline({ m1.front(), m1.back() }) };
    joint.f_outlines[0] = { Polyline(f0), Polyline({ f0.front(), f0.back() }) };
    joint.f_outlines[1] = { Polyline(f1), Polyline({ f1.front(), f1.back() }) };
    joint.m_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.m_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.f_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.f_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
}
