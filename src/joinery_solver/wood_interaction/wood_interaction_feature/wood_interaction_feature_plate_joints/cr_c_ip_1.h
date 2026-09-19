/// cr_c_ip_1: parametric cross joint - nine base rings, offset along their normals for face 1, rotated for the male.
static void cr_c_ip_1(FeaturePlate& joint) {

    joint.name = "cr_c_ip_1";

    double s_param = std::max(std::min(joint.shift, 1.0), 0.0);
    s_param = 0.05 + (s_param - 0.0) * (0.4 - 0.05) / (1.0 - 0.0);

    const double a = 0.5 - s_param;
    const double b = 0.5;
    const double c = 2.0 * (b - a);
    const double z = 0.5;

    const Point p[16] = {
        Point(a, -a, 0),          Point(-a, -a, 0),          Point(-a, a, 0),          Point(a, a, 0),
        Point(a+c, -a-c, 0),      Point(-a-c, -a-c, 0),      Point(-a-c, a+c, 0),      Point(a+c, a+c, 0),
        Point(b, -b, z),          Point(-b, -b, z),          Point(-b, b, z),          Point(b, b, z),
        Point(b, -b, -z),         Point(-b, -b, -z),         Point(-b, b, -z),         Point(b, b, -z),
    };

    const double inv_2a = (a > 1e-12) ? 1.0 / (a * 2.0) : 0.0;
    const Vector d01 = p[0] - p[1];
    const Vector v0 = d01 * inv_2a * (0.5 - a);

    std::vector<Polyline> base(9);
    base[0] = Polyline({p[0]+v0, p[1]-v0, p[2]-v0, p[3]+v0, p[0]+v0});
    base[1] = Polyline({p[1]-v0, p[0]+v0, p[8]+v0, p[9]-v0, p[1]-v0});
    base[2] = Polyline({p[3]+v0, p[2]-v0, p[10]-v0, p[11]+v0, p[3]+v0});
    base[3] = Polyline({p[2], p[1], p[13], p[14], p[2]});
    base[4] = Polyline({p[0], p[3], p[15], p[12], p[0]});
    base[5] = Polyline({p[0], p[12], p[4], p[8], p[0]});
    base[6] = Polyline({p[13], p[1], p[9], p[5], p[13]});
    base[7] = Polyline({p[2], p[14], p[6], p[10], p[2]});
    base[8] = Polyline({p[15], p[3], p[11], p[7], p[15]});

    const double lengths[9] = { 0.5, 0.4, 0.4, 0.4, 0.4, 0.1, 0.1, 0.1, 0.1 };
    const Xform xf_rot = Xform::from_axes(Vector(0,1,0), Vector(1,0,0), Vector(0,0,-1));

    const std::vector<Polyline> f0 = base;
    std::vector<Polyline> f1(9);
    std::vector<Polyline> m0(9);
    std::vector<Polyline> m1(9);

    for (int i = 0; i < 9; i++) {
        f1[i] = base[i];
        const Point pa = f1[i].get_point(0);
        const Point pb = f1[i].get_point(1);
        const Point pc = f1[i].get_point(2);
        const Vector ab = pb - pa;
        const Vector cb = pb - pc;
        Vector cross = ab.cross(cb);
        cross.normalize_self();
        for (size_t j = 0; j < f1[i].point_count(); j++)
            f1[i].set_point(j, f1[i].get_point(j) + cross * lengths[i]);
        m0[i] = base[i].transformed(xf_rot);
        m1[i] = f1[i].transformed(xf_rot);
    }

    joint.female_outlines[0].clear();
    joint.female_outlines[1].clear();
    joint.male_outlines[0].clear();
    joint.male_outlines[1].clear();
    for (int i = 0; i < 9; i++) {
        joint.female_outlines[0].push_back(f0[i]);
        joint.female_outlines[0].push_back(f0[i]);
        joint.female_outlines[1].push_back(f1[i]);
        joint.female_outlines[1].push_back(f1[i]);
        joint.male_outlines[0].push_back(m0[i]);
        joint.male_outlines[0].push_back(m0[i]);
        joint.male_outlines[1].push_back(m1[i]);
        joint.male_outlines[1].push_back(m1[i]);
    }

    std::vector<int> ct(18);
    for (int i = 0; i < 6; i++)
        ct[i] = FabricationType::mill_project;
    for (int i = 6; i < 18; i++)
        ct[i] = FabricationType::slice;

    for (int face = 0; face < 2; face++) {
        joint.female_fabrication_types[face] = ct;
        joint.male_fabrication_types[face] = ct;
    }
}
