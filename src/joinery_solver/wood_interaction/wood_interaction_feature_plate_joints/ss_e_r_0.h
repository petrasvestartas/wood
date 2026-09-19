/// ss_e_r_0: world-space relief - each volume split in half along the thickness, the halves offset four ways along the joint line; no orient.
static void ss_e_r_0(FeaturePlate& joint) {

    joint.name = "ss_e_r_0";

    if (!joint.joint_volumes[0] || !joint.joint_volumes[1])
        return;

    const Polyline& vol0 = *joint.joint_volumes[0];
    const Polyline& vol1 = *joint.joint_volumes[1];
    if (vol0.point_count() < 4 || vol1.point_count() < 4)
        return;

    const Point r0 = Point::lerp(vol0.get_point(0), vol1.get_point(0), 0.5);
    const Point r1 = Point::lerp(vol0.get_point(1), vol1.get_point(1), 0.5);
    const Point r2 = Point::lerp(vol0.get_point(2), vol1.get_point(2), 0.5);
    const Point r3 = Point::lerp(vol0.get_point(3), vol1.get_point(3), 0.5);

    const Point p_mid_01 = Point::mid_point(r0, r1);
    const Point p_mid_32 = Point::mid_point(r3, r2);
    const Vector zs = (p_mid_01 - r1) * 0.75;
    const Vector x = (vol1.get_point(0) - vol0.get_point(0)) * 0.5;
    const double x_len = std::sqrt(x.magnitude_squared());
    if (x_len < 1e-12)
        return;

    Polyline rh0({
        p_mid_01,
        r0 - zs,
        r3 - zs,
        p_mid_32,
        p_mid_01,
    });
    Polyline rh1({
        p_mid_01,
        r1 + zs,
        r2 + zs,
        p_mid_32,
        p_mid_01,
    });
    const double y_ext = joint.scale[1];
    rh0.extend_edge_equally(1, y_ext);
    rh0.extend_edge_equally(3, y_ext);
    rh1.extend_edge_equally(1, y_ext);
    rh1.extend_edge_equally(3, y_ext);

    const double x_target = joint.scale[0];
    const double factor_far  = (5.0 + x_len + x_target) / x_len;
    const double factor_near = 0.25 / x_len;
    const std::array<Vector, 4> offsets = {
        x * factor_near, x * factor_far,
        x * -factor_near, x * -factor_far,
    };

    for (int fi = 0; fi < 2; fi++) {
        joint.male_outlines[fi].clear();
        joint.male_outlines[fi].reserve(4);
        joint.female_outlines[fi].clear();
        joint.female_outlines[fi].reserve(4);
    }

    for (const int oi : {0, 0, 2, 2})
        joint.male_outlines[0].push_back(rh0.translated(offsets[oi]));
    for (const int oi : {1, 1, 3, 3})
        joint.male_outlines[1].push_back(rh0.translated(offsets[oi]));

    for (const int oi : {0, 0, 2, 2})
        joint.female_outlines[0].push_back(rh1.translated(offsets[oi]));
    for (const int oi : {1, 1, 3, 3})
        joint.female_outlines[1].push_back(rh1.translated(offsets[oi]));

    joint.male_fabrication_types[0] = std::vector<int>(4, FabricationType::slice);
    joint.male_fabrication_types[1] = std::vector<int>(4, FabricationType::slice);
    joint.female_fabrication_types[0] = std::vector<int>(4, FabricationType::slice);
    joint.female_fabrication_types[1] = std::vector<int>(4, FabricationType::slice);

    joint.no_orient = true;
}
