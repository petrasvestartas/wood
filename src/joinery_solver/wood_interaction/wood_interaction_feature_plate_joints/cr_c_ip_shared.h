/// cr_c_ip_2..5 core: the same 16-point layout, extension, offset, rotation, duplication and side-face rebuild;
/// the variants differ in drills, extension factors, the offset threshold and cut types.
static void cr_c_ip_shared(
    FeaturePlate& joint,
    const std::vector<Polyline>& extra_drills,
    double ext_side,
    double ext_vert,
    double ext_side_factor_a,
    double ext_side_factor_b,
    size_t offset_min_pts,
    const std::vector<int>& cut_types
) {

    double s_param = std::max(std::min(joint.shift, 1.0), 0.0);
    s_param = 0.05 + (s_param - 0.0) * (0.4 - 0.05) / (1.0 - 0.0);

    const double a = 0.5 - s_param;
    const double b = 0.5;
    const double c = 2.0 * (b - a);
    const double z = 0.5;

    const Point p[16] = {
        Point(a, -a, 0),      Point(-a, -a, 0),      Point(-a, a, 0),      Point(a, a, 0),
        Point(a+c, -a-c, 0),  Point(-a-c, -a-c, 0),  Point(-a-c, a+c, 0),  Point(a+c, a+c, 0),
        Point(b, -b, z),      Point(-b, -b, z),      Point(-b, b, z),      Point(b, b, z),
        Point(b, -b, -z),     Point(-b, -b, -z),     Point(-b, b, -z),     Point(b, b, -z),
    };

    const double inv_2a = (a > 1e-12) ? 1.0 / (a * 2.0) : 0.0;
    const Vector d01 = p[0] - p[1];
    const Vector v0 = d01 * inv_2a * (0.5 - a);

    std::vector<Polyline> base;
    base.push_back(Polyline({p[0]+v0, p[1]-v0, p[2]-v0, p[3]+v0, p[0]+v0}));
    base.push_back(Polyline({p[1]-v0, p[0]+v0, p[8]+v0, p[9]-v0, p[1]-v0}));
    base.push_back(Polyline({p[3]+v0, p[2]-v0, p[10]-v0, p[11]+v0, p[3]+v0}));
    base.push_back(Polyline({p[2], p[1], p[13], p[14], p[2]}));
    base.push_back(Polyline({p[0], p[3], p[15], p[12], p[0]}));

    base[3].extend_segment_equally(0, ext_side * ext_side_factor_a, ext_side * ext_side_factor_b);
    base[3].extend_segment_equally(2, ext_side * ext_side_factor_a, ext_side * ext_side_factor_b);
    base[4].extend_segment_equally(0, ext_side * ext_side_factor_a, ext_side * ext_side_factor_b);
    base[4].extend_segment_equally(2, ext_side * ext_side_factor_a, ext_side * ext_side_factor_b);
    base[3].extend_segment_equally(1, ext_vert, ext_vert);
    base[3].extend_segment_equally(3, ext_vert, ext_vert);
    base[4].extend_segment_equally(1, ext_vert, ext_vert);
    base[4].extend_segment_equally(3, ext_vert, ext_vert);

    for (const Polyline& dr : extra_drills)
        base.push_back(dr);

    const int n = (int)base.size();

    const double lengths[5] = { 0.5, 0.4, 0.4, 0.4, 0.4 };
    const Xform xf_rot = Xform::from_axes(Vector(0,1,0), Vector(1,0,0), Vector(0,0,-1));

    const std::vector<Polyline> f0 = base;
    std::vector<Polyline> f1(n);
    std::vector<Polyline> m0(n);
    std::vector<Polyline> m1(n);

    for (int i = 0; i < n; i++) {
        f1[i] = base[i];
        if (base[i].point_count() > offset_min_pts && i < 5) {
            const Point pa = f1[i].get_point(0);
            const Point pb = f1[i].get_point(1);
            const Point pc = f1[i].get_point(2);
            const Vector ab = pb - pa;
            const Vector cb = pb - pc;
            Vector cross = ab.cross(cb);
            cross.normalize_self();
            for (size_t j = 0; j < f1[i].point_count(); j++)
                f1[i].set_point(j, f1[i].get_point(j) + cross * lengths[i]);
        }
        m0[i] = base[i].transformed(xf_rot);
        m1[i] = f1[i].transformed(xf_rot);
    }

    joint.female_outlines[0].clear();
    joint.female_outlines[1].clear();
    joint.male_outlines[0].clear();
    joint.male_outlines[1].clear();
    for (int i = 0; i < n; i++) {
        joint.female_outlines[0].push_back(f0[i]);
        joint.female_outlines[0].push_back(f0[i]);
        joint.female_outlines[1].push_back(f1[i]);
        joint.female_outlines[1].push_back(f1[i]);
        joint.male_outlines[0].push_back(m0[i]);
        joint.male_outlines[0].push_back(m0[i]);
        joint.male_outlines[1].push_back(m1[i]);
        joint.male_outlines[1].push_back(m1[i]);
    }

    for (int i = 0; i < 2; i++) {

        const int id = (i + 1) * 2;
        std::vector<Polyline>& fo0 = joint.female_outlines[0];
        std::vector<Polyline>& fo1 = joint.female_outlines[1];
        std::vector<Polyline>& mo0 = joint.male_outlines[0];
        std::vector<Polyline>& mo1 = joint.male_outlines[1];

        const Polyline side00({fo0[id].get_point(0), fo0[id].get_point(1), fo1[id].get_point(1), fo1[id].get_point(0), fo0[id].get_point(0)});
        const Polyline side01({fo0[id].get_point(3), fo0[id].get_point(2), fo1[id].get_point(2), fo1[id].get_point(3), fo0[id].get_point(3)});
        fo0[id] = side00;
        fo1[id] = side01;
        fo0[id + 1] = side00;
        fo1[id + 1] = side01;

        const Polyline mside00({mo0[id].get_point(0), mo0[id].get_point(1), mo1[id].get_point(1), mo1[id].get_point(0), mo0[id].get_point(0)});
        const Polyline mside01({mo0[id].get_point(3), mo0[id].get_point(2), mo1[id].get_point(2), mo1[id].get_point(3), mo0[id].get_point(3)});
        mo0[id] = mside00;
        mo1[id] = mside01;
        mo0[id + 1] = mside00;
        mo1[id + 1] = mside01;
    }

    for (int face = 0; face < 2; face++) {
        joint.female_fabrication_types[face] = cut_types;
        joint.male_fabrication_types[face] = cut_types;
    }
}
