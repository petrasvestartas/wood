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

/// The contact ring offset inward by shift as 2024's clipper offset_in_3d moved it: a miter offset in the ring's own plane, the
/// result turned to start nearest the ring's first point; a ring the offset swallows is left as it is, as 2024 left the polyline
/// when Clipper returned nothing.
static Polyline offset_contact(const InteractionFeaturePlate& joint, double shift) {

    Polyline ring = joint.contact.polygon;
    Point origin;
    Plane plane;
    ring.get_fast_plane(origin, plane);
    Intersection::offset_in_3d(ring, plane, -shift);

    return ring;
}
