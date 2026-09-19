/// A profile of xyz points shifted along z.
static Polyline profile_shifted_along_z(const double pts[][3], int n, double z_offset) {

    std::vector<Point> v;
    v.reserve(n);
    for (int k = 0; k < n; k++)
        v.emplace_back(pts[k][0], pts[k][1], pts[k][2] + z_offset);

    return Polyline(v);
}

/// ss_e_r_2/3 core: `divisions` copies of each profile along z, pushed twice per face as mill_project; unit_scale on, and
/// every joint volume rebuilt as a 120*shift square. unit_scale_distance must already hold the element thickness.
static void ss_e_r_impl(
    WoodJoint& joint,
    const double m0[][3], int m0n,
    const double m1[][3], int m1n,
    const double f0[][3], int f0n,
    const double f1[][3], int f1n
) {

    const int divisions = std::max(1, joint.divisions);
    const double edge_length = joint.length * joint.scale[2];
    const double jv_len = (joint.unit_scale_distance > 0) ? joint.unit_scale_distance : 40.0;
    const double step = edge_length / (divisions * jv_len);
    const double total = edge_length / jv_len;
    const double z0 = total * 0.5 - step * 0.5;

    joint.male_outlines[0].reserve(2 * divisions);
    joint.male_outlines[1].reserve(2 * divisions);
    joint.female_outlines[0].reserve(2 * divisions);
    joint.female_outlines[1].reserve(2 * divisions);

    for (int i = 0; i < divisions; i++) {

        const double z_off = z0 - step * i;
        const Polyline pm0 = profile_shifted_along_z(m0, m0n, z_off);
        const Polyline pm1 = profile_shifted_along_z(m1, m1n, z_off);
        const Polyline pf0 = profile_shifted_along_z(f0, f0n, z_off);
        const Polyline pf1 = profile_shifted_along_z(f1, f1n, z_off);

        joint.male_outlines[0].push_back(pm0);
        joint.male_outlines[0].push_back(pm0);
        joint.male_outlines[1].push_back(pm1);
        joint.male_outlines[1].push_back(pm1);

        joint.female_outlines[0].push_back(pf0);
        joint.female_outlines[0].push_back(pf0);
        joint.female_outlines[1].push_back(pf1);
        joint.female_outlines[1].push_back(pf1);
    }

    const int n = 2 * divisions;
    joint.male_cut_types[0] = std::vector<int>(n, CutType::mill_project);
    joint.male_cut_types[1] = std::vector<int>(n, CutType::mill_project);
    joint.female_cut_types[0] = std::vector<int>(n, CutType::mill_project);
    joint.female_cut_types[1] = std::vector<int>(n, CutType::mill_project);
    joint.unit_scale = true;

    const double size = 120.0 * joint.shift;
    joint.unit_scale_distance = size;
    for (int vi = 0; vi < 4; vi++) {

        std::optional<Polyline>& opt = joint.joint_volumes[vi];
        if (!opt || opt->point_count() != 5)
            continue;

        Polyline& vol = *opt;
        const Point p0 = vol.get_point(0);
        const Point p1 = vol.get_point(1);
        const Point p2 = vol.get_point(2);
        const Point c = Point::mid_point(p0, p1);

        Vector xd = p1 - p0;
        const double xl = std::sqrt(xd.magnitude_squared());
        if (xl < 1e-12)
            continue;
        xd = xd * (size * 0.5 / xl);

        Vector yd = p2 - p1;
        const double yl = std::sqrt(yd.magnitude_squared());
        if (yl < 1e-12)
            continue;
        yd = yd * (size * 0.5 / yl);

        vol = Polyline({
            c + xd + 2 * yd,
            c - xd + 2 * yd,
            c - xd,
            c + xd,
            c + xd + 2 * yd,
        });
    }
}
