/// The lattice 2024 laid in a ring of four corners: the first edge and the edge facing it divided (int)(length / division_length)
/// times, the smaller count of the two, the edges across them likewise, ends included, from the ring's first corner.
static std::vector<Point> rectangle_lattice(const Polyline& ring, double division_length) {

    const double edge_00 = Point::distance(ring[0], ring[1]);
    const double edge_01 = Point::distance(ring[3], ring[2]);
    const double edge_10 = Point::distance(ring[1], ring[2]);
    const double edge_11 = Point::distance(ring[0], ring[3]);
    const int divisions_u = std::min((int)(edge_00 / division_length), (int)(edge_01 / division_length));
    const int divisions_v = std::min((int)(edge_10 / division_length), (int)(edge_11 / division_length));

    const std::vector<Point> edge_0 = Point::interpolate(ring[0], ring[1], divisions_u, 1);
    const std::vector<Point> edge_2 = Point::interpolate(ring[3], ring[2], divisions_u, 1);
    std::vector<Point> points;
    points.reserve(edge_0.size() * (divisions_v + 2));
    for (size_t i = 0; i < edge_0.size(); i++) {
        const std::vector<Point> across = Point::interpolate(edge_0[i], edge_2[i], divisions_v, 1);
        points.insert(points.end(), across.begin(), across.end());
    }

    return points;
}

/// The lattice 2024 laid in any other ring: division_length steps about the centre of the ring's minimum-area bounding rectangle,
/// (int)(2 * floor(half extent) / division_length) steps each way along each side and at most 20, the points inside the ring kept.
static std::vector<Point> bounding_lattice(const Polyline& ring, double division_length) {

    const std::optional<Polyline> rectangle = Polyline::bounding_rectangle(ring);
    if (!rectangle)
        return {};

    // the rectangle's frame, its centre and the steps each way
    const Polyline& box = *rectangle;
    Vector u = box[1] - box[0];
    Vector v = box[3] - box[0];
    const double width = u.magnitude();
    const double height = v.magnitude();
    u.normalize_self();
    v.normalize_self();
    const Point centre = box[0] + u * (width * 0.5) + v * (height * 0.5);
    const int steps_u = (int)std::min(20.0, 2.0 * std::floor(width * 0.5) / division_length);
    const int steps_v = (int)std::min(20.0, 2.0 * std::floor(height * 0.5) / division_length);
    const Point start = centre - u * (division_length * steps_u) - v * (division_length * steps_v);

    // the ring in that frame, for the inside test
    std::vector<Point> ring_2d;
    const size_t count = ring.is_closed() ? ring.point_count() - 1 : ring.point_count();
    for (size_t i = 0; i < count; i++) {
        const Vector d = ring[i] - box[0];
        ring_2d.emplace_back(d.dot(u), d.dot(v), 0.0);
    }
    const Polyline ring_flat(ring_2d);

    std::vector<Point> points;
    for (int i = 0; i < steps_u * 2 + 1; i++) {
        for (int j = 0; j < steps_v * 2 + 1; j++) {
            const Point point = start + u * (division_length * i) + v * (division_length * j);
            const Vector d = point - box[0];
            if (ring_flat.point_in_polygon_2d(Point(d.dot(u), d.dot(v), 0.0)))
                points.push_back(point);
        }
    }

    return points;
}

/// tt_e_p_4, id 44: a lattice of drills in the contact offset inward by shift, every division_length, as 2024's
/// grid_of_points_in_a_polygon laid it: a ring of four corners takes the rectangle lattice from its first corner, any other ring the
/// lattice about the centre of its bounding rectangle; a zero shift leaves the contact unoffset, and so does one that swallows the
/// ring, as 2024 did when Clipper returned nothing. Nothing for a division length of zero or less.
static void tt_e_p_4(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "tt_e_p_4";
    joint.no_orient = true;

    Vector dir0;
    Vector dir1;
    if (!drill_directions(joint, elements, dir0, dir1))
        return;
    if (!(joint.division_length > 0.0))
        return;

    const Polyline ring = joint.shift != 0.0 ? offset_contact(joint, joint.shift) : joint.contact.polygon;
    const std::vector<Point> points = ring.point_count() == 5
        ? rectangle_lattice(ring, joint.division_length)
        : bounding_lattice(ring, joint.division_length);
    emit_drills(joint, points, dir0, dir1);
}
