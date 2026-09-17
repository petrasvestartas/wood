/// ts_e_p_3: parametric tenon-mortise skipping every other point pair; female holes from every four male points plus a bounding rectangle.
static void ts_e_p_3(WoodJoint& joint) {
    joint.name = "ts_e_p_3";

    int div = std::max(8, std::min(100, joint.divisions));
    div -= div % 4;
    if (div == 0)
        return;
    const int size = div / 4 + 1;

    std::vector<Point> arr3 = Point::interpolate(Point( 0.5,-0.5,-0.5), Point( 0.5,-0.5, 0.5), div, 0);
    std::vector<Point> arr0 = Point::interpolate(Point(-0.5,-0.5,-0.5), Point(-0.5,-0.5, 0.5), div, 0);
    std::vector<Point> arr1 = Point::interpolate(Point(-0.5, 0.5,-0.5), Point(-0.5, 0.5, 0.5), div, 0);
    std::vector<Point> arr2 = Point::interpolate(Point( 0.5, 0.5,-0.5), Point( 0.5, 0.5, 0.5), div, 0);
    std::vector<Point>* arrays[4] = {&arr0, &arr1, &arr2, &arr3};

    const double vz = (joint.shift == 0) ? 0.0 : (joint.shift * 1.0 - 0.5) / (div + 1);
    const Vector v(0, 0, vz);
    for (int i = 0; i < 4; i++) {
        auto& a = *arrays[i];
        for (int j = 0; j < (int)a.size(); j++) {
            int flip = (j % 2 == 0) ? 1 : -1;
            if (i >= 2)
                flip *= -1;
            a[j] = a[j] + v * flip;
        }
    }

    for (int i = 0; i < 4; i += 2) {
        std::vector<Point> pts;
        pts.reserve(arr0.size());
        const auto& aA = *arrays[i];
        const auto& aB = *arrays[i + 1];
        for (int j = 0; j < (int)aA.size(); j++) {
            if (j % 4 > 1)
                continue;
            bool flip = (j % 2 == 0);
            if (i >= 2)
                flip = !flip;
            pts.push_back(flip ? aA[j] : aB[j]);
            pts.push_back(flip ? aB[j] : aA[j]);
        }
        pts.push_back(i < 2 ? aA.back() : aB.back());
        const Polyline outline(pts);
        const Polyline endpoints({pts.front(), pts.back()});
        const int idx = (i < 2) ? 1 : 0;
        joint.male_outlines[idx] = {outline, endpoints};
    }

    const Polyline& m0pts = joint.male_outlines[0][0];
    const Polyline& m1pts = joint.male_outlines[1][0];
    const int m0n = (int)m0pts.point_count();
    const int nrects = m0n - m0n % 4;
    for (int i = 0; i < nrects; i += 4) {
        const Point p00 = m0pts.get_point(i);
        const Point p03 = m0pts.get_point(i + 3);
        const Point p10 = m1pts.get_point(i);
        const Point p13 = m1pts.get_point(i + 3);
        joint.female_outlines[0].push_back(Polyline({p00, p03, p13, p10, p00}));
        const Point p01 = m0pts.get_point(i + 1);
        const Point p02 = m0pts.get_point(i + 2);
        const Point p11 = m1pts.get_point(i + 1);
        const Point p12 = m1pts.get_point(i + 2);
        joint.female_outlines[1].push_back(Polyline({p01, p02, p12, p11, p01}));
    }
    for (int f = 0; f < 2; f++) {
        if (size < 2 || joint.female_outlines[f].empty())
            continue;
        const Polyline& first = joint.female_outlines[f].front();
        const Polyline& last = joint.female_outlines[f].back();
        joint.female_outlines[f].push_back(Polyline({
            first.get_point(0), first.get_point(3),
            last.get_point(3), last.get_point(0), first.get_point(0)}));
    }
    for (int f = 0; f < 2; f++) {
        std::vector<int> cuts;
        cuts.reserve(joint.female_outlines[f].size());
        for (size_t k = 0; k + 1 < joint.female_outlines[f].size(); k++)
            cuts.push_back(wood_cut::hole);
        if (!joint.female_outlines[f].empty())
            cuts.push_back(wood_cut::insert_between_multiple_edges);
        joint.female_cut_types[f] = std::move(cuts);
    }
    joint.male_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.male_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
}
