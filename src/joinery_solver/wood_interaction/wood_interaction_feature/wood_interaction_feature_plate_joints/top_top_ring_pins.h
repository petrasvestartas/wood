static void top_top_ring_pins(
    InteractionFeaturePlate& joint,
    const std::vector<std::shared_ptr<Plate>>& elements,
    const Settings& settings,
    double radius
) {

    joint.name = "top_top_ring_pins";
    joint.no_orient = true;

    Vector dir0;
    Vector dir1;
    if (!drill_directions(joint, elements, dir0, dir1))
        return;

    const Polyline ring = offset_contact(joint, joint.shift);
    if (ring.point_count() < 2)
        return;

    std::vector<Point> points;
    for (size_t i = 0; i + 1 < ring.point_count(); i++) {
        const int divisions =
            (int)std::min(100.0, Point::distance(ring[i], ring[i + 1]) / joint.division_length);
        const std::vector<Point> edge = Point::interpolate(ring[i], ring[i + 1], divisions, 2);
        points.insert(points.end(), edge.begin(), edge.end());
    }

    const size_t last = ring.point_count() - 1;
    if ((ring[0] - ring[last]).magnitude_squared() > settings.distance_squared)
        points.push_back(ring[last]);

    std::vector<Point> spaced;
    for (const Point& point : points)
        if (spaced.empty() || Point::distance(spaced.back(), point) >= 4.0 * radius)
            spaced.push_back(point);
    if (spaced.size() > 2 && Point::distance(spaced.back(), spaced.front()) < 4.0 * radius)
        spaced.pop_back();

    emit_drills(joint, spaced, dir0, dir1);
}
