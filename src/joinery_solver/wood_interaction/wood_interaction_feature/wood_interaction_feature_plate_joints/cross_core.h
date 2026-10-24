static Polyline bottom_side(
    const Point& a,
    const Point& b,
    const Point& c,
    const Point& d,
    double extension_first,
    double extension_second
) {

    Polyline ring({a, b, c, d, a});
    ring.extend_segment_equally(0, extension_first);
    ring.extend_segment_equally(2, extension_second);
    ring.extend_segment_equally(1, 0.6);
    ring.extend_segment_equally(3, 0.6);

    return ring;
}

/// The body cross_half_lap to cross_brussels_half_lap share, as 2024 wrote it four times.
static void cross_core(
    InteractionFeaturePlate& joint,
    double extension_first,
    double extension_second,
    const std::vector<Polyline>& drills,
    const std::vector<int>& drill_types
) {

    // the half-lap on the shift: the centre square of half side a, its offset by c, the top and the bottom squares
    double s = std::max(std::min(joint.shift, 1.0), 0.0);
    s = 0.05 + (s - 0.0) * (0.4 - 0.05) / (1.0 - 0.0);
    const double a = 0.5 - s;
    const double b = 0.5;
    const double c = 2.0 * (b - a);
    const double z = 0.5;
    const Point p[16] = {
        Point(a, -a, 0),
        Point(-a, -a, 0),
        Point(-a, a, 0),
        Point(a, a, 0),
        Point(a + c, -a - c, 0),
        Point(-a - c, -a - c, 0),
        Point(-a - c, a + c, 0),
        Point(a + c, a + c, 0),
        Point(b, -b, z),
        Point(-b, -b, z),
        Point(-b, b, z),
        Point(b, b, z),
        Point(b, -b, -z),
        Point(-b, -b, -z),
        Point(-b, b, -z),
        Point(b, b, -z),
    };
    const Vector v0 = ((p[0] - p[1]) * (1.0 / (a * 2.0))) * (0.5 - a);

    std::vector<Polyline> rings = {
        Polyline({p[0] + v0, p[1] - v0, p[2] - v0, p[3] + v0, p[0] + v0}),
        Polyline({p[1] - v0, p[0] + v0, p[8] + v0, p[9] - v0, p[1] - v0}),
        Polyline({p[3] + v0, p[2] - v0, p[10] - v0, p[11] + v0, p[3] + v0}),
        bottom_side(p[2], p[1], p[13], p[14], extension_first, extension_second),
        bottom_side(p[0], p[3], p[15], p[12], extension_first, extension_second),
    };
    rings.insert(rings.end(), drills.begin(), drills.end());

    const double lengths[5] = {0.5, 0.4, 0.4, 0.4, 0.4};
    const Xform rotation = Xform::from_axes(Vector(0, 1, 0), Vector(1, 0, 0), Vector(0, 0, -1));
    const size_t n = rings.size();
    std::vector<Polyline> f0 = rings;
    std::vector<Polyline> f1 = rings;
    std::vector<Polyline> m0(n);
    std::vector<Polyline> m1(n);
    for (size_t i = 0; i < n; i++) {
        if (i < 5) {
            Vector normal = (f1[i][1] - f1[i][0]).cross(f1[i][1] - f1[i][2]);
            normal.normalize_self();
            for (size_t j = 0; j < f1[i].point_count(); j++)
                f1[i].set_point(j, f1[i][j] + normal * lengths[i]);
        }
        m0[i] = f0[i].transformed(rotation);
        m1[i] = f1[i].transformed(rotation);
    }

    joint.female_outlines[0].clear();
    joint.female_outlines[1].clear();
    joint.male_outlines[0].clear();
    joint.male_outlines[1].clear();
    for (size_t i = 0; i < n; i++) {
        joint.female_outlines[0].insert(joint.female_outlines[0].end(), {f0[i], f0[i]});
        joint.female_outlines[1].insert(joint.female_outlines[1].end(), {f1[i], f1[i]});
        joint.male_outlines[0].insert(joint.male_outlines[0].end(), {m0[i], m0[i]});
        joint.male_outlines[1].insert(joint.male_outlines[1].end(), {m1[i], m1[i]});
    }

    for (size_t id = 2; id <= 4; id += 2) {
        std::vector<Polyline>& fo0 = joint.female_outlines[0];
        std::vector<Polyline>& fo1 = joint.female_outlines[1];
        const Polyline female_wall_0({fo0[id][0], fo0[id][1], fo1[id][1], fo1[id][0], fo0[id][0]});
        const Polyline female_wall_1({fo0[id][3], fo0[id][2], fo1[id][2], fo1[id][3], fo0[id][3]});
        fo0[id] = female_wall_0;
        fo1[id] = female_wall_1;
        fo0[id + 1] = female_wall_0;
        fo1[id + 1] = female_wall_1;

        std::vector<Polyline>& mo0 = joint.male_outlines[0];
        std::vector<Polyline>& mo1 = joint.male_outlines[1];
        const Polyline male_wall_0({mo0[id][0], mo0[id][1], mo1[id][1], mo1[id][0], mo0[id][0]});
        const Polyline male_wall_1({mo0[id][3], mo0[id][2], mo1[id][2], mo1[id][3], mo0[id][3]});
        mo0[id] = male_wall_0;
        mo1[id] = male_wall_1;
        mo0[id + 1] = male_wall_0;
        mo1[id + 1] = male_wall_1;
    }

    std::vector<int> types = {
        FabricationType::mill_project,
        FabricationType::mill_project,
        FabricationType::slice_projectsheer,
        FabricationType::slice_projectsheer,
        FabricationType::slice_projectsheer,
        FabricationType::slice_projectsheer,
        FabricationType::mill_project,
        FabricationType::mill_project,
        FabricationType::mill_project,
        FabricationType::mill_project,
    };
    for (const int type : drill_types)
        types.insert(types.end(), {type, type});
    for (int face = 0; face < 2; face++) {
        joint.female_fabrication_types[face] = types;
        joint.male_fabrication_types[face] = types;
    }
}
