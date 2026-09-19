/// division_length drills on a circle of radius shift around the centroid, in the area plane.
static void tt_e_p_2(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "tt_e_p_2";
    joint.no_orient = true;

    int v0;
    int v1;
    if (!drill_ready(joint, elements, v0, v1, 3))
        return;

    const double radius = joint.shift;
    const int n_pts = std::max(1, std::min(100, (int)joint.division_length));
    const Point center = joint.contact.polygon.center();
    Point origin;
    Plane plane;
    joint.contact.polygon.get_fast_plane(origin, plane);
    Vector zp = plane.z_axis();
    zp.normalize_self();
    Vector xp = plane.x_axis();
    xp.normalize_self();
    Vector yp = zp.cross(xp);
    yp.normalize_self();

    std::vector<Point> points;
    if (radius < 1e-9 || n_pts <= 1) {
        points.push_back(center);
    } else {
        for (int i = 0; i < n_pts; ++i) {
            const double angle = 2.0 * Tolerance::PI * i / n_pts;
            const double cx = std::cos(angle) * radius;
            const double cy = std::sin(angle) * radius;
            points.push_back(center + xp * cx + yp * cy);
        }
    }

    Vector dir0;
    Vector dir1;
    drill_axes(joint, elements[v0]->thickness, elements[v1]->thickness, dir0, dir1);
    emit_drills(joint, points, dir0, dir1);
}
