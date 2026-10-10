/// The frame 2024's inscribe_rectangle_in_convex_polygon drew the contact in, no direction given: x along the ring's longest edge from
/// its start, z the sum of the corner cross products, y = x cross z, the left-handed frame 2024 kept, each unit, and the plane turned
/// 0.0001 radians about z afterwards, "without which the algorithm does not work".
struct InscribeFrame {
    Point origin;
    Vector x;
    Vector y;
    double cosine = 1.0; // Of the 0.0001 turn.
    double sine = 0.0;
};

static InscribeFrame inscribe_frame(const Polyline& ring) {

    const size_t count = ring.is_closed() ? ring.point_count() - 1 : ring.point_count();
    InscribeFrame frame;
    Vector normal(0.0, 0.0, 0.0);
    double longest = 0.0;
    for (size_t i = 0; i < count; i++) {
        const size_t previous = (i + count - 1) % count;
        const size_t next = (i + 1) % count;
        normal += (ring[i] - ring[previous]).cross(ring[next] - ring[i]);
        const double length = (ring[next] - ring[i]).magnitude_squared();
        if (length > longest) {
            longest = length;
            frame.x = ring[next] - ring[i];
            frame.origin = ring[i];
        }
    }

    frame.y = frame.x.cross(normal);
    frame.x.normalize_self();
    frame.y.normalize_self();
    frame.cosine = std::cos(0.0001);
    frame.sine = std::sin(0.0001);

    return frame;
}

/// A point in the turned plane.
static std::array<double, 2> inscribe_to_2d(const InscribeFrame& frame, const Point& point) {

    const Vector d = point - frame.origin;
    const double u = d.dot(frame.x);
    const double v = d.dot(frame.y);

    return {u * frame.cosine - v * frame.sine, u * frame.sine + v * frame.cosine};
}

/// A point of the turned plane back in space.
static Point inscribe_to_3d(const InscribeFrame& frame, double u, double v) {

    const double u0 = u * frame.cosine + v * frame.sine;
    const double v0 = -u * frame.sine + v * frame.cosine;

    return frame.origin + frame.x * u0 + frame.y * v0;
}

/// A point turned a quarter counter-clockwise k times, so one sweep serves each side of a rectangle in turn.
static std::array<double, 2> quarter_turns(const std::array<double, 2>& point, int k) {

    std::array<double, 2> turned = point;
    for (int i = 0; i < k; i++)
        turned = {-turned[1], turned[0]};

    return turned;
}

/// The largest axis-parallel rectangle inside box (min x, min y, max x, max y) with none of the points strictly inside it, the
/// largest empty iso-rectangle CGAL found for 2024, the first of equal areas kept. Every such rectangle has a point on one of its
/// sides, or is the box: so, with each side in turn made the left by a quarter turn, every point is the support of a left side and
/// the band above and below it narrows at each point further right until one closes it, each band a candidate up to the point that
/// narrows it and up to the box's right side.
static std::array<double, 4> largest_empty_rectangle(const std::vector<std::array<double, 2>>& points, const std::array<double, 4>& box) {

    std::array<double, 4> best = box;
    double best_area = 0.0;
    for (const std::array<double, 2>& point : points)
        if (point[0] > box[0] && point[0] < box[2] && point[1] > box[1] && point[1] < box[3])
            best_area = -1.0;

    for (int turn = 0; turn < 4; turn++) {

        // the box and the points turned, the points by x then y
        const std::array<double, 2> corner_a = quarter_turns({box[0], box[1]}, turn);
        const std::array<double, 2> corner_b = quarter_turns({box[2], box[3]}, turn);
        const double x_max = std::max(corner_a[0], corner_b[0]);
        const double y_min = std::min(corner_a[1], corner_b[1]);
        const double y_max = std::max(corner_a[1], corner_b[1]);
        std::vector<std::array<double, 2>> sorted;
        sorted.reserve(points.size());
        for (const std::array<double, 2>& point : points)
            sorted.push_back(quarter_turns(point, turn));
        std::sort(sorted.begin(), sorted.end());

        for (size_t i = 0; i < sorted.size(); i++) {
            const double x_i = sorted[i][0];
            const double y_i = sorted[i][1];
            double top = y_max;
            double bottom = y_min;
            double right = x_max;
            for (size_t j = i + 1; j < sorted.size(); j++) {
                const double x_j = sorted[j][0];
                const double y_j = sorted[j][1];
                if (x_j <= x_i || y_j <= bottom || y_j >= top)
                    continue;
                const double area = (x_j - x_i) * (top - bottom);
                if (area > best_area) {
                    best_area = area;
                    const std::array<double, 2> a = quarter_turns({x_i, bottom}, (4 - turn) % 4);
                    const std::array<double, 2> b = quarter_turns({x_j, top}, (4 - turn) % 4);
                    best = {std::min(a[0], b[0]), std::min(a[1], b[1]), std::max(a[0], b[0]), std::max(a[1], b[1])};
                }
                if (y_j > y_i)
                    top = y_j;
                else if (y_j < y_i)
                    bottom = y_j;
                else {
                    right = x_j;
                    break;
                }
            }

            // the band up to the box's right side, unless a point on the support's own height closed it
            if (right < x_max)
                continue;
            const double area = (x_max - x_i) * (top - bottom);
            if (area > best_area) {
                best_area = area;
                const std::array<double, 2> a = quarter_turns({x_i, bottom}, (4 - turn) % 4);
                const std::array<double, 2> b = quarter_turns({x_max, top}, (4 - turn) % 4);
                best = {std::min(a[0], b[0]), std::min(a[1], b[1]), std::max(a[0], b[0]), std::max(a[1], b[1])};
            }
        }
    }

    return best;
}

/// The rectangle 2024 inscribed in the contact: in the inscribe frame, the ring's edges sampled every fiftieth of its box's diagonal, at
/// most 100 points an edge, the largest empty rectangle of the box among them, inset on every side by (1 - scale) of its shorter
/// extent, its corners from the lower left round by the upper left, back in space.
static Polyline inscribed_rectangle(const Polyline& ring, double scale) {

    const InscribeFrame frame = inscribe_frame(ring);
    const size_t count = ring.is_closed() ? ring.point_count() - 1 : ring.point_count();

    // the box of the corners
    std::array<double, 4> box = {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()};
    for (size_t i = 0; i < count; i++) {
        const std::array<double, 2> p = inscribe_to_2d(frame, ring[i]);
        box = {std::min(box[0], p[0]), std::min(box[1], p[1]), std::max(box[2], p[0]), std::max(box[3], p[1])};
    }
    const double step = std::sqrt((box[2] - box[0]) * (box[2] - box[0]) + (box[3] - box[1]) * (box[3] - box[1])) / 50.0;

    // the edges sampled from their starts
    std::vector<std::array<double, 2>> samples;
    for (size_t i = 0; i + 1 < ring.point_count(); i++) {
        const int divisions = (int)std::min(100.0, Point::distance(ring[i], ring[i + 1]) / step);
        for (const Point& point : Point::interpolate(ring[i], ring[i + 1], divisions, 2))
            samples.push_back(inscribe_to_2d(frame, point));
    }

    // the largest empty rectangle, inset by the scale
    const std::array<double, 4> empty = largest_empty_rectangle(samples, box);
    const double inset = std::min((1.0 - scale) * std::abs(empty[2] - empty[0]), (1.0 - scale) * std::abs(empty[3] - empty[1]));

    return Polyline({
        inscribe_to_3d(frame, empty[0] + inset, empty[1] + inset),
        inscribe_to_3d(frame, empty[0] + inset, empty[3] - inset),
        inscribe_to_3d(frame, empty[2] - inset, empty[3] - inset),
        inscribe_to_3d(frame, empty[2] - inset, empty[1] + inset),
        inscribe_to_3d(frame, empty[0] + inset, empty[1] + inset),
    });
}

/// The rectangle's edges divided every division_length from their starts, at most 25 times each: 2024's points for a positive length.
static std::vector<Point> rectangle_edge_points(const Polyline& rectangle, double division_length) {

    std::vector<Point> points;
    for (size_t i = 0; i + 1 < rectangle.point_count(); i++) {
        const int divisions = (int)std::min(25.0, Point::distance(rectangle[i], rectangle[i + 1]) / division_length);
        const std::vector<Point> edge = Point::interpolate(rectangle[i], rectangle[i + 1], divisions, 2);
        points.insert(points.end(), edge.begin(), edge.end());
    }

    return points;
}

/// The rectangle filled with a grid of division_length, the step count the ceiling of each pair of facing edges over it, the smaller of
/// the two, ends included: 2024's points for a negative length.
static std::vector<Point> rectangle_grid_points(const Polyline& rectangle, double division_length) {

    const double edge_00 = Point::distance(rectangle[0], rectangle[1]);
    const double edge_01 = Point::distance(rectangle[3], rectangle[2]);
    const double edge_10 = Point::distance(rectangle[1], rectangle[2]);
    const double edge_11 = Point::distance(rectangle[0], rectangle[3]);
    const int divisions_u = (int)std::min(std::ceil(edge_00 / division_length), std::ceil(edge_01 / division_length));
    const int divisions_v = (int)std::min(std::ceil(edge_10 / division_length), std::ceil(edge_11 / division_length));

    const std::vector<Point> edge_0 = Point::interpolate(rectangle[0], rectangle[1], divisions_u, 1);
    const std::vector<Point> edge_2 = Point::interpolate(rectangle[3], rectangle[2], divisions_u, 1);
    std::vector<Point> points;
    points.reserve(edge_0.size() * (divisions_v + 2));
    for (size_t i = 0; i < edge_0.size(); i++) {
        const std::vector<Point> across = Point::interpolate(edge_0[i], edge_2[i], divisions_v, 1);
        points.insert(points.end(), across.begin(), across.end());
    }

    return points;
}

/// tt_e_p_5, id 45: drills on the rectangle 2024 inscribed in the contact, scaled by shift, precision 1: a division_length above the
/// distance tolerance divides its edges, one below minus the tolerance fills it with a grid of that step, one in between drills nothing.
/// The rectangle itself 2024 kept commented out, so no mill. Nothing where a plate is missing.
static void tt_e_p_5(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements, const Settings& settings) {

    joint.name = "tt_e_p_5";
    joint.no_orient = true;

    Vector dir0;
    Vector dir1;
    if (!drill_directions(joint, elements, dir0, dir1))
        return;

    const Polyline rectangle = inscribed_rectangle(joint.contact.polygon, joint.shift);
    std::vector<Point> points;
    if (joint.division_length > settings.distance)
        points = rectangle_edge_points(rectangle, joint.division_length);
    else if (joint.division_length < -settings.distance)
        points = rectangle_grid_points(rectangle, std::abs(joint.division_length));

    emit_drills(joint, points, dir0, dir1);
}
