/// Both plates found, a first joint volume with 3+ points, an area with min_area+ points.
static bool drill_ready(
    const FeaturePlate& joint,
    const std::vector<std::shared_ptr<Plate>>& elements,
    int& v0,
    int& v1,
    size_t min_area
) {

    v0 = index_of(elements, joint.element_a);
    v1 = index_of(elements, joint.element_b);
    if (v0 < 0 || v0 >= (int)elements.size() || v1 < 0 || v1 >= (int)elements.size())
        return false;
    if (!joint.joint_volumes[0])
        return false;
    if (joint.joint_volumes[0]->point_count() < 3)
        return false;

    return joint.contact.polygon.point_count() >= min_area;
}

/// dir0: the first volume's [1]->[2] edge, unit, times plate v0's thickness; dir1: the reverse times v1's.
static void drill_axes(const FeaturePlate& joint, double t0, double t1, Vector& dir0, Vector& dir1) {
    const Polyline& jv0 = *joint.joint_volumes[0];
    dir0 = jv0.get_point(1) - jv0.get_point(2);
    dir0.normalize_self();
    dir1 = -dir0;
    dir0 = dir0 * t0;
    dir1 = dir1 * t1;
}

/// One two-point drill line per point on every face, twice per face as the merge expects.
static void emit_drills(FeaturePlate& joint, const std::vector<Point>& points, const Vector& dir0, const Vector& dir1) {

    for (int f = 0; f < 2; f++) {
        joint.male_outlines[f].clear();
        joint.female_outlines[f].clear();
        joint.male_fabrication_types[f].clear();
        joint.female_fabrication_types[f].clear();
        joint.male_outlines[f].reserve(points.size() * 2);
        joint.female_outlines[f].reserve(points.size() * 2);
        joint.male_fabrication_types[f].reserve(points.size() * 2);
        joint.female_fabrication_types[f].reserve(points.size() * 2);
    }

    for (const Point& pt : points) {
        const Polyline line0({pt, pt + dir0});
        const Polyline line1({pt, pt + dir1});
        for (int f = 0; f < 2; f++) {
            joint.female_outlines[f].push_back(line0);
            joint.female_outlines[f].push_back(line0);
            joint.male_outlines[f].push_back(line1);
            joint.male_outlines[f].push_back(line1);
            joint.male_fabrication_types[f].push_back(FabricationType::drill);
            joint.male_fabrication_types[f].push_back(FabricationType::drill);
            joint.female_fabrication_types[f].push_back(FabricationType::drill);
            joint.female_fabrication_types[f].push_back(FabricationType::drill);
        }
    }
}

/// The area offset inward by shift, divided every division_distance; the last vertex too when the ring is open.
static std::vector<Point> offset_boundary_points(
    const Polyline& area,
    double shift,
    double division_distance,
    double open_tolerance
) {

    Polyline poly = area;
    Point origin;
    Plane plane;
    poly.get_fast_plane(origin, plane);
    const double offset_distance = -shift;
    Intersection::offset_in_3d(poly, plane, offset_distance);

    std::vector<Point> points;
    for (size_t i = 0; i + 1 < poly.point_count(); i++) {
        const double seg_len = Point::distance(poly[i], poly[i + 1]);
        const int divisions = (int)std::min(100.0, seg_len / division_distance);
        const std::vector<Point> dp = Polyline::interpolate_points(poly[i], poly[i + 1], divisions, 2);
        points.insert(points.end(), dp.begin(), dp.end());
    }

    if (poly.point_count() > 0) {
        const Vector gap = poly[0] - poly[poly.point_count() - 1];
        if (gap.magnitude_squared() > open_tolerance)
            points.push_back(poly[poly.point_count() - 1]);
    }

    return points;
}

/// One drill through the area centroid.
static void centroid_drill(FeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    int v0;
    int v1;
    if (!drill_ready(joint, elements, v0, v1, 3))
        return;

    Vector dir0;
    Vector dir1;
    drill_axes(joint, elements[v0]->thickness, elements[v1]->thickness, dir0, dir1);
    emit_drills(joint, {joint.contact.polygon.center()}, dir0, dir1);
}

/// Drills along the offset area boundary.
static void boundary_drill(
    FeaturePlate& joint,
    const std::vector<std::shared_ptr<Plate>>& elements,
    double division_distance,
    double open_tolerance
) {

    int v0;
    int v1;
    if (!drill_ready(joint, elements, v0, v1, 4))
        return;
    if (division_distance <= 0.0)
        return;

    const std::vector<Point> points = offset_boundary_points(joint.contact.polygon, joint.shift, division_distance, open_tolerance);
    Vector dir0;
    Vector dir1;
    drill_axes(joint, elements[v0]->thickness, elements[v1]->thickness, dir0, dir1);
    emit_drills(joint, points, dir0, dir1);
}

