/// b_0: beam slice - the tween rectangle of the two volumes, widened by scale[0] / scale[1], as four slice planes.
static void b_0(FeaturePlate& joint) {

    joint.name = "b_0";
    joint.no_orient = true;

    if (!joint.joint_volumes[0] || !joint.joint_volumes[1])
        return;

    const Polyline& vol0 = *joint.joint_volumes[0];
    const Polyline& vol1 = *joint.joint_volumes[1];
    if (vol0.point_count() < 5 || vol1.point_count() < 5)
        return;

    std::vector<Point> r(5);
    for (int i = 0; i < 5; ++i)
        r[i] = Point::lerp(vol0.get_point(i), vol1.get_point(i), 0.5);

    for (const int i : {1, 3}) {
        const Vector e = r[i + 1] - r[i];
        const double len = std::sqrt(e.magnitude_squared());
        if (len > 1e-12) {
            const double inv = 1.0 / len;
            r[i] = r[i] - e * inv * joint.scale[0];
            r[i + 1] = r[i + 1] + e * inv * joint.scale[0];
            if (i == 3)
                r[0] = r[4];
        }
    }

    const Vector v = (r[1] - r[0]) * 0.5;
    r[0] = r[0] + v;
    r[1] = r[1] - v;
    r[2] = r[2] - v;
    r[3] = r[3] + v;
    r[4] = r[4] + v;
    const double vlen = std::sqrt(v.magnitude_squared());
    if (vlen > 1e-12) {
        const double sc = (joint.scale[1] + 5.0) / vlen;
        r[0] = r[0] + v * sc;
        r[3] = r[3] + v * sc;
        r[4] = r[4] + v * sc;
    }

    double nx = 0;
    double ny = 0;
    double nz = 0;
    for (int i = 0; i < 4; ++i) {
        const int j = (i + 1) % 4;
        nx += (r[i][1] - r[j][1]) * (r[i][2] + r[j][2]);
        ny += (r[i][2] - r[j][2]) * (r[i][0] + r[j][0]);
        nz += (r[i][0] - r[j][0]) * (r[i][1] + r[j][1]);
    }

    Vector n(nx, ny, nz);
    const double nlen = std::sqrt(n.magnitude_squared());
    if (nlen < 1e-12)
        return;

    n = n / nlen;
    const Vector off_near = n * 0.25;
    const Vector off_far = n * (joint.scale[2] + 15.0);

    std::rotate(r.begin(), r.begin() + 2, r.end() - 1);
    r[4] = r[0];
    const Polyline rect(r);
    const Polyline rect0 = rect.translated(off_near);
    const Polyline rect1 = rect.translated(off_far);
    const Polyline rect2 = rect.translated(-off_near);
    const Polyline rect3 = rect.translated(-off_far);

    joint.male_outlines[0] = { rect0, rect0, rect2, rect2 };
    joint.male_outlines[1] = { rect1, rect1, rect3, rect3 };

    joint.male_fabrication_types[0] = { FabricationType::slice, FabricationType::slice, FabricationType::slice, FabricationType::slice };
    joint.male_fabrication_types[1] = { FabricationType::slice, FabricationType::slice, FabricationType::slice, FabricationType::slice };
}
