/// top_top_ring_pins, id 43: drills along the contact offset inward by shift, every division_length of its ring, as 2024's
/// offset_and_divide_to_points laid them: each edge divided (int)min(100, length / division_length) times from its start point, and the
/// ring's last point too when the ring is open by more than distance_squared; the contact itself when the offset swallows the ring,
/// as 2024 drilled the contact when Clipper returned nothing. With a drill radius, a point nearer than two diameters to the last one kept,
/// or the last to the first, is dropped, so one diameter of wood stays between every two holes.
static void top_top_ring_pins(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings, double radius) {

    joint.name = "top_top_ring_pins";
    joint.no_orient = true;

    Vector dir0;
    Vector dir1;
    if (!drill_directions(joint, elements, dir0, dir1))
        return;

    // the ring offset inward, each edge divided from its start
    const Polyline ring = offset_contact(joint, joint.shift);
    std::vector<Point> points;
    for (size_t i = 0; i + 1 < ring.point_count(); i++) {
        const int divisions = (int)std::min(100.0, Point::distance(ring[i], ring[i + 1]) / joint.division_length);
        const std::vector<Point> edge = Point::interpolate(ring[i], ring[i + 1], divisions, 2);
        points.insert(points.end(), edge.begin(), edge.end());
    }

    // the last point of an open ring, which no edge starts
    const size_t last = ring.point_count() - 1;
    if ((ring[0] - ring[last]).magnitude_squared() > settings.distance_squared)
        points.push_back(ring[last]);

    // the points two diameters apart
    std::vector<Point> spaced;
    for (const Point& point : points)
        if (spaced.empty() || Point::distance(spaced.back(), point) >= 4.0 * radius)
            spaced.push_back(point);
    if (spaced.size() > 2 && Point::distance(spaced.back(), spaced.front()) < 4.0 * radius)
        spaced.pop_back();

    emit_drills(joint, spaced, dir0, dir1);
}
