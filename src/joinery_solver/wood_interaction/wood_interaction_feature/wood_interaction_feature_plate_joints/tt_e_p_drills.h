/// The two drill directions of a top-top pair, as 2024 read them off the first joint volume: dir0 is the volume's second edge, its
/// point 1 to its point 2, unit, which runs from the contact into the first plate (element_a, 2024's v0), scaled by that plate's
/// thickness; dir1 is the reverse, into the second plate, scaled by its thickness. False, and no drills, when a plate is missing or
/// the volume has no second edge, where 2024 would have read past the end.
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

/// One drill per point, laid out as 2024 laid it: a two-point line from the point one plate thickness along the drill direction,
/// written twice on each face of each side, every copy of type drill, in world space (no orient). 2024 handed the male the line
/// along dir1 by the female's thickness and the female the line along dir0 by the male's: each plate's record ran through the other
/// plate, where the 2025 reference still shows it (output type 3, the lower plate of a stacked pair bored from its top upward). The
/// port gives each side the line through its own plate, dir0 to the male and dir1 to the female, so the hole a side hosts is bored
/// where the dowel goes; the points, the lengths and the structure are 2024's.
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
            joint.male_outlines[face].push_back(line0);
            joint.male_outlines[face].push_back(line0);
            joint.female_outlines[face].push_back(line1);
            joint.female_outlines[face].push_back(line1);
            joint.male_fabrication_types[face].push_back(FabricationType::drill);
            joint.male_fabrication_types[face].push_back(FabricationType::drill);
            joint.female_fabrication_types[face].push_back(FabricationType::drill);
            joint.female_fabrication_types[face].push_back(FabricationType::drill);
        }
    }
}

/// The contact ring offset inward by shift as 2024's clipper offset_in_3d moved it: Clipper2's miter inflate by -shift at its default
/// two decimals, in the frame of the ring's own plane at its first point, the first path it returns when that covers more than 0.0001
/// square units, turned to start nearest the ring's first point and closed; the contact itself when Clipper returns nothing or next
/// to nothing, a ring the offset swallows. The kernel's Intersection::offset_in_3d is a plain miter that turns a swallowed ring into
/// a self-crossing bow-tie instead, so 2024's Clipper call is made here, in 2024's frame, so the ring rounds to the same hundredths:
/// the kernel's plane picks CGAL's base vectors, but for a normal with a component of exactly zero, where CGAL takes that axis.
static Polyline offset_contact(const InteractionFeaturePlate& joint, double shift) {

    const Polyline& contact = joint.contact.polygon;
    Point origin;
    Plane plane;
    contact.get_fast_plane(origin, plane);
    const Point first = contact[0];
    const Vector x_axis = plane.base1();
    const Vector y_axis = plane.base2();

    // the ring inset in its frame, as 2024 asked Clipper
    const Clipper2Lib::PathD path = clipper_path(contact, first, x_axis, y_axis, false);
    const Clipper2Lib::PathsD inset = Clipper2Lib::InflatePaths(
        {path},
        -shift,
        Clipper2Lib::JoinType::Miter,
        Clipper2Lib::EndType::Polygon
    );
    if (inset.empty() || std::abs(Clipper2Lib::Area(inset[0])) <= 0.0001)
        return contact;

    // back in world space, from the vertex nearest the contact's first point, closed
    const Clipper2Lib::PathD& ring = inset[0];
    size_t start = 0;
    for (size_t i = 1; i < ring.size(); i++)
        if (Clipper2Lib::DistanceSqr(ring[i], path[0]) < Clipper2Lib::DistanceSqr(ring[start], path[0]))
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
