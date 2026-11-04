/// The Annen through-tenons of Christopher Robeller's 2017 plugin, built in world space on the plate's own edge: the middle 80% of
/// the edge, moved up to 200 towards the midpoint it shares with the plate across the wall, in 4m + 1 equal segments with a tenon on
/// every fourth from the third; each tenon is the edge pushed along the insertion vector through the wall to its outer face, so the
/// plate across the wall, which runs the shared edge the other way, lands its tenons in the segments between. m comes from the
/// plate's feature_divisions on this face when given, Christopher's round(0.8 mesh edge / tab length), else from the edge itself.
static void top_side_annen_tenons(
    InteractionFeaturePlate& joint,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::vector<InteractionFeaturePlate>& joints
) {

    joint.name = "top_side_annen_tenons";
    joint.no_orient = true;

    const int male = index_of_plate(elements, joint.element_a);
    const int female = index_of_plate(elements, joint.element_b);
    const int side = joint.contact.face_a;
    const int wall_face = joint.contact.face_b;
    if (male < 0 || female < 0 || side < 2 || wall_face < 0 || wall_face > 1)
        return;

    const FacePlanarElement& plate = *elements[male];
    const FacePlanarElement& wall = *elements[female];
    if (side >= (int)plate.polylines.size() || wall.planes.size() < 2)
        return;

    const Plane& inner = wall.planes[wall_face];
    const Plane& outer = wall.planes[1 - wall_face];

    // the tenon direction: the plate's insertion vector on this face, else the edge's perpendicular in the plate's plane, from the wall's inner face to its outer
    const Vector normal = plate.planes[0].z_axis();
    Vector along = side < (int)plate.insertion_vectors().size() ? plate.insertion_vector(side)
                                                                : Vector(0.0, 0.0, 0.0);
    if (along.magnitude() < 1e-9)
        along = normal.cross(
            plate.polylines[0].get_point(side - 1) - plate.polylines[0].get_point(side - 2)
        );
    if (!along.normalize_self())
        return;
    if (along.dot(outer.origin() - inner.origin()) < 0.0)
        along = along * -1.0;

    // the segments: Christopher's round(0.8 edge / tab length), the mesh edge when the plate gives it, raised to 4m + 1
    const int given = plate.given_face(side);
    int divisions = given >= 0 && given < (int)plate.feature_divisions.size()
                        ? plate.feature_divisions[given]
                        : 0;
    if (divisions <= 0 && joint.division_length > 0.0) {
        const double edge = Point::distance(
            plate.polylines[0].get_point(side - 2), plate.polylines[0].get_point(side - 1)
        );
        divisions = (int)std::nearbyint(0.8 * edge / joint.division_length);
    }
    int segments = std::max(divisions, 0);
    while ((segments - 1) % 4 != 0)
        segments++;
    const int tenons = (segments - 1) / 4;
    if (tenons < 1)
        return;

    // the shared midpoint: the middle of this edge averaged with the nearest edge middle of a plate on the wall's other face, in this
    // plate's plane and within 1000, as Christopher averaged the mid-surface edge midpoints of the two plates that meet through a wall
    const Point middle = plate.polylines[side].center();
    std::optional<Point> shared;
    double nearest = 1000.0;
    for (const InteractionFeaturePlate& other : joints) {
        if (other.element_b != joint.element_b || other.element_a == joint.element_a ||
            other.contact.face_b != 1 - wall_face)
            continue;
        const int index = index_of_plate(elements, other.element_a);
        if (index < 0 || other.contact.face_a < 2 ||
            other.contact.face_a >= (int)elements[index]->polylines.size())
            continue;
        const Point candidate = elements[index]->polylines[other.contact.face_a].center();
        const double distance = Point::distance(middle, candidate);
        if (distance < nearest && std::abs((candidate - middle).dot(normal)) < 100.0) {
            nearest = distance;
            shared = Point::mid_point(middle, candidate);
        }
    }

    // on each face of the plate: the zone and its tenons, counted from the edge's start in the plate's given outline
    std::array<std::vector<Point>, 2> runs;
    for (int face = 0; face < 2; face++) {
        const Polyline& outline = plate.polylines[face];
        Point p0 = outline.get_point(side - 2);
        Point p1 = outline.get_point(side - 1);
        if (plate.reversed)
            std::swap(p0, p1);

        // the middle 80% of the edge, moved towards the shared midpoint by up to 200
        const double w = Point::distance(p0, p1);
        if (w < 1e-9)
            return;
        const Vector n_e = (p0 - p1) * (1.0 / w);
        const Point p0a = p0 - n_e * (0.1 * w);
        const Point p1a = p1 + n_e * (0.1 * w);
        double shift = 0.0;
        bool towards_p1 = false;
        if (shared) {
            const Vector axis = p1a - p0a;
            const Point mp = p0a + axis * ((*shared - p0a).dot(axis) / axis.magnitude_squared());
            shift = Point::distance(mp, Point::mid_point(p0a, p1a));
            if (shift > 200.0)
                shift = 0.0;
            towards_p1 = Point::distance(mp, p1a) < Point::distance(mp, p0a);
        }
        const Vector move = n_e * (towards_p1 ? -shift : shift);

        // the zone ends on the wall's inner face along the edge's in-plane perpendicular, the tenon tips on its outer face along the insertion vector
        Vector perpendicular = normal.cross(n_e);
        if (perpendicular.dot(along) < 0.0)
            perpendicular = perpendicular * -1.0;
        std::array<Point, 2> root = {p0a + move, p1a + move};
        std::array<Point, 2> tip;
        for (int end = 0; end < 2; end++) {
            Point on_inner;
            if (Intersection::line_plane(
                    Line::from_points(root[end], root[end] + perpendicular), inner, on_inner, false
                ))
                root[end] = on_inner;
            if (!Intersection::line_plane(
                    Line::from_points(root[end], root[end] + along), outer, tip[end], false
                ))
                return;
        }

        // the run from the first tenon's base to the last one's: the zone ends lie on the edge and are no vertices of the plate
        std::vector<Point>& run = runs[face];
        for (int tenon = 0; tenon < tenons; tenon++) {
            const double t0 = (4.0 * tenon + 3.0) / segments;
            const double t1 = (4.0 * tenon + 4.0) / segments;
            run.push_back(root[0] + (root[1] - root[0]) * t0);
            run.push_back(tip[0] + (tip[1] - tip[0]) * t0);
            run.push_back(tip[0] + (tip[1] - tip[0]) * t1);
            run.push_back(root[0] + (root[1] - root[0]) * t1);
        }
        joint.male_outlines[face] = {Polyline(run), Polyline({run.front(), run.back()})};
        joint.male_fabrication_types[face] = {
            FabricationType::edge_insertion, FabricationType::edge_insertion
        };
    }

    // the wall's slots: per tenon the quad through its four roots on the inner face and the one through its tips on the outer, then the hull of them all
    const std::vector<Point>& bottom = runs[0];
    const std::vector<Point>& top = runs[1];
    for (int tenon = 0; tenon < tenons; tenon++) {
        const size_t j = 4 * tenon;
        joint.female_outlines[0].push_back(
            Polyline({top[j], top[j + 3], bottom[j + 3], bottom[j], top[j]})
        );
        joint.female_outlines[1].push_back(
            Polyline({top[j + 1], top[j + 2], bottom[j + 2], bottom[j + 1], top[j + 1]})
        );
    }
    for (int face = 0; face < 2; face++) {
        const Polyline& first = joint.female_outlines[face].front();
        const Polyline& last = joint.female_outlines[face].back();
        joint.female_outlines[face].push_back(Polyline(
            {first.get_point(0),
             first.get_point(3),
             last.get_point(3),
             last.get_point(0),
             first.get_point(0)}
        ));
    }
    const std::vector<int> holes(joint.female_outlines[0].size(), FabricationType::hole);
    joint.female_fabrication_types = {holes, holes};
}
