static bool drill_directions(
    const InteractionFeaturePlate& joint,
    const std::vector<std::shared_ptr<Plate>>& elements,
    Vector& dir0,
    Vector& dir1
) {

    const int v0 = index_of_plate(elements, joint.element_a);
    const int v1 = index_of_plate(elements, joint.element_b);
    if (v0 < 0 || v1 < 0)
        return false;
    if (!joint.joint_volumes[0] || joint.joint_volumes[0]->point_count() < 3)
        return false;
    if (joint.contact.polygon.point_count() < 3)
        return false;

    const Polyline& volume = *joint.joint_volumes[0];
    Vector axis = volume.get_point(1) - volume.get_point(2);
    axis.normalize_self();
    dir0 = axis * elements[v0]->thickness;
    dir1 = -axis * elements[v1]->thickness;

    return true;
}

/// One drill per point, laid out as 2024 laid it.
static void emit_drills(
    InteractionFeaturePlate& joint,
    const std::vector<Point>& points,
    const Vector& dir0,
    const Vector& dir1
) {

    for (int face = 0; face < 2; face++) {
        joint.male_outlines[face].clear();
        joint.female_outlines[face].clear();
        joint.male_fabrication_types[face].clear();
        joint.female_fabrication_types[face].clear();
        joint.male_outlines[face].reserve(points.size() * 2);
        joint.female_outlines[face].reserve(points.size() * 2);
        joint.male_fabrication_types[face].reserve(points.size() * 2);
        joint.female_fabrication_types[face].reserve(points.size() * 2);
    }

    for (const Point& point : points) {
        const Polyline line0({point, point + dir0});
        const Polyline line1({point, point + dir1});
        for (int face = 0; face < 2; face++) {
            joint.male_outlines[face].push_back(line1);
            joint.male_outlines[face].push_back(line1);
            joint.female_outlines[face].push_back(line0);
            joint.female_outlines[face].push_back(line0);
            joint.male_fabrication_types[face].push_back(FabricationType::drill);
            joint.male_fabrication_types[face].push_back(FabricationType::drill);
            joint.female_fabrication_types[face].push_back(FabricationType::drill);
            joint.female_fabrication_types[face].push_back(FabricationType::drill);
        }
    }
}

/// The contact ring offset inward by shift as 2024's clipper offset_in_3d moved it.
static Polyline offset_contact(const InteractionFeaturePlate& joint, double shift) {

    const Polyline& contact = joint.contact.polygon;
    Point origin;
    Plane plane;
    contact.get_fast_plane(origin, plane);
    const Point first = contact[0];
    const Vector x_axis = plane.base1();
    const Vector y_axis = plane.base2();

    const Clipper2Lib::PathD path = clipper_path(contact, first, x_axis, y_axis, false);
    const Clipper2Lib::PathsD inset = Clipper2Lib::InflatePaths(
        {path}, -shift, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon
    );
    if (inset.empty() || std::abs(Clipper2Lib::Area(inset[0])) <= 0.0001)
        return contact;

    const Clipper2Lib::PathD& ring = inset[0];
    size_t start = 0;
    for (size_t i = 1; i < ring.size(); i++)
        if (Clipper2Lib::DistanceSqr(ring[i], path[0]) <
            Clipper2Lib::DistanceSqr(ring[start], path[0]))
            start = i;
    std::vector<Point> points;
    points.reserve(ring.size() + 1);
    for (size_t i = 0; i < ring.size(); i++) {
        const Clipper2Lib::PointD& p = ring[(start + i) % ring.size()];
        points.push_back(first + x_axis * p.x + y_axis * p.y);
    }
    points.push_back(points[0]);

    return Polyline(points);
}
