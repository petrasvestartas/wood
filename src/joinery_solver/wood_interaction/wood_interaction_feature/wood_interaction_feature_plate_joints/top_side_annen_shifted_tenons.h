/// The face of a wall standing across a plate's side near the corner: a face plane of another plate within reach of the corner, neither
/// along the plate's normal nor along the side, with its outline within four thicknesses of the corner, the step of a shifted plate at a
/// junction being up to about a hundred mm; none when there is no such wall.
static std::optional<Plane> annen_shifted_wall_across(
    const FacePlanarElement& plate,
    int plate_index,
    int side,
    const FacePlanarElement& wall,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const Point& corner,
    double reach
) {

    const Vector normal = plate.planes[0].z_axis();
    const Vector across = plate.planes[side].z_axis();
    std::optional<Plane> end;
    double nearest = reach;
    for (size_t k = 0; k < elements.size(); k++) {
        const FacePlanarElement& other = *elements[k];
        if ((int)k == plate_index || &other == &wall || other.planes.size() < 2)
            continue;
        for (int face = 0; face < 2; face++) {
            const Plane& candidate = other.planes[face];
            if (std::abs(candidate.z_axis().dot(normal)) > 0.5 ||
                std::abs(candidate.z_axis().dot(across)) > 0.9)
                continue;
            const double distance = Point::distance(corner, candidate.project(corner));
            size_t edge = 0;
            Point closest;
            if (distance < nearest &&
                other.polylines[face].closest_distance_and_point(corner, edge, closest) <
                    4.0 * other.thickness) {
                nearest = distance;
                end = candidate;
            }
        }
    }
    return end;
}

/// The plane bounding a plate's side at one end in the 2020 definition's outer loop, where every wall side stood a plate thickness further
/// out than in the plate's own outline: the far face of the wall joined on the side beside this one; at the junction of a merged long
/// side, where the side beside continues the same plane, the face through the corner of a wall standing across the junction, the
/// neighbour cell's cross wall, else the wall's own end when it ends there, else the plane square across the side through the corner;
/// the side's plane moved out by a thickness when its face is marked for these tenons and no wall is joined there; on a plain side with
/// no wall of its own, the face of the wall standing across it; else that plane itself. beside_plane gets the plane the definition's
/// outline met at that end when it is not the plate's own side beside, the face across a cut step, else nothing.
static Plane annen_shifted_bound(
    const FacePlanarElement& plate,
    int plate_index,
    int side,
    int beside,
    const FacePlanarElement& wall,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::vector<InteractionFeaturePlate>& joints,
    const Point& corner,
    std::optional<Plane>& beside_plane
) {

    const Plane& there = plate.planes[beside];
    beside_plane = std::nullopt;

    // the junction of a merged long side: the face of another wall through the corner across the side, else the wall's own end plane when it ends within a thickness of the corner, else square across the side
    if (std::abs(plate.planes[side].z_axis().dot(there.z_axis())) > 0.999) {
        const Vector normal = plate.planes[0].z_axis();
        std::optional<Plane> end =
            annen_shifted_wall_across(plate, plate_index, side, wall, elements, corner, 1.0);
        if (end)
            return *end;
        double nearest = plate.thickness;
        for (size_t k = 2; k < wall.planes.size(); k++) {
            if (std::abs(wall.planes[k].z_axis().dot(normal)) > 0.9)
                continue;
            const double distance = Point::distance(corner, wall.planes[k].project(corner));
            if (distance < nearest) {
                nearest = distance;
                end = wall.planes[k];
            }
        }
        if (end)
            return *end;
        return Plane::from_point_normal(
            corner, plate.polylines[0].get_point(side - 1) - plate.polylines[0].get_point(side - 2)
        );
    }

    // the wall joined on the side beside: its far face
    const std::string& guid = elements[plate_index]->guid();
    for (const InteractionFeaturePlate& other : joints) {
        int index = -1;
        int face = -1;
        if (other.element_a == guid && other.contact.face_a == beside) {
            index = index_of_plate(elements, other.element_b);
            face = other.contact.face_b;
        } else if (other.element_b == guid && other.contact.face_b == beside) {
            index = index_of_plate(elements, other.element_a);
            face = other.contact.face_a;
        }
        if (index >= 0 && face >= 0 && face <= 1 && elements[index]->planes.size() >= 2)
            return elements[index]->planes[1 - face];
    }

    // no wall joined there: a thickness out when the face is marked for these tenons, as the definition moved every wall side
    const int given = plate.given_face(beside);
    if (given >= 0 && given < (int)plate.feature_types.size() && plate.feature_types[given] == 23)
        return there.translate_by_normal(plate.thickness);

    // a plain side with no wall of its own, the cut step of a shifted plate: the face of the wall standing across it within four thicknesses, the cut's far end standing off that plane, as the definition's outer loop met that plane there
    beside_plane = annen_shifted_wall_across(
        plate, plate_index, side, wall, elements, corner, 4.0 * plate.thickness
    );
    return beside_plane ? *beside_plane : there;
}

/// A plate's own line on a wall as the 2020 definition drew it: from the middle of its inner corner on the first face, where its edge meets
/// the plane across when the side is cut short of it, and its outer corner on the second face at each end of the edge, the outer corner
/// where the second face, the wall's far face and the bound of the side beside meet, in the order the plate was given; false on a joint
/// that is no plate side on a wall face or on a degenerate edge.
static bool annen_shifted_line(
    const InteractionFeaturePlate& joint,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::vector<InteractionFeaturePlate>& joints,
    Line& line
) {

    const int male = index_of_plate(elements, joint.element_a);
    const int female = index_of_plate(elements, joint.element_b);
    const int side = joint.contact.face_a;
    const int wall_face = joint.contact.face_b;
    if (male < 0 || female < 0 || side < 2 || wall_face < 0 || wall_face > 1)
        return false;

    const FacePlanarElement& plate = *elements[male];
    const FacePlanarElement& wall = *elements[female];
    if (side >= (int)plate.planes.size() || wall.planes.size() < 2 ||
        plate.polylines[0].point_count() <= side - 1 ||
        plate.polylines[1].point_count() <= side - 1)
        return false;

    // the edge's ends and the sides beside them, in the order the plate was given
    const int count = (int)plate.planes.size() - 2;
    std::array<int, 2> ends = {side - 2, side - 1};
    std::array<int, 2> beside = {
        side == 2 ? count + 1 : side - 1, side == count + 1 ? 2 : side + 1
    };
    if (plate.reversed) {
        std::swap(ends[0], ends[1]);
        std::swap(beside[0], beside[1]);
    }

    // a side cut short of the plane the definition's outline met, a shifted plate's step cut for the wall passing it, starts its line where its edge would have met that plane
    std::array<Point, 2> middles;
    for (int end = 0; end < 2; end++) {
        Point inner = plate.polylines[0].get_point(ends[end]);
        std::optional<Plane> beside_plane;
        const Plane bound = annen_shifted_bound(
            plate, male, side, beside[end], wall, elements, joints, inner, beside_plane
        );
        if (beside_plane) {
            const Line edge = Line::from_points(
                plate.polylines[0].get_point(ends[1 - end]), plate.polylines[0].get_point(ends[end])
            );
            Point met;
            if (Intersection::line_plane(edge, *beside_plane, met, false))
                inner = met;
        }
        Point outer;
        if (!Intersection::plane_plane_plane(
                plate.planes[1], wall.planes[1 - wall_face], bound, outer
            ))
            return false;
        middles[end] = Point::mid_point(inner, outer);
    }

    line = Line::from_points(middles[0], middles[1]);
    return line.length() > 1e-9;
}

/// The through-tenons of the 2020 Annen shifted-pattern definition (Annen200917.gh), built in world space on the wall line the plate shares
/// with the plate across the wall: its own line averaged with the other plate's, run the same way, and shortened to its middle 90%, in
/// 4d + 1 equal steps when longer than the division length and 2d + 1 when shorter, a tenon half a step wide around every odd step counted
/// from the edge's start, so the plate across the wall, which runs the line the other way, lands its tenons in the even steps; each tenon
/// is cut by two planes through the insertion vector and the plate's feature axis (the definition's cut planes through the surface normals
/// averaged at the insertion corner) from the wall's inner face to its outer, and gets a slot through the wall. d comes from divisions when
/// given, else from the plate's feature_divisions on this face, else 2; an edge shorter than minimum_edge gets no tenons, as the definition skipped it.
static void top_side_annen_shifted_tenons(
    InteractionFeaturePlate& joint,
    const std::vector<std::shared_ptr<FacePlanarElement>>& elements,
    const std::vector<InteractionFeaturePlate>& joints,
    int divisions,
    double minimum_edge
) {

    joint.name = "top_side_annen_shifted_tenons";
    joint.no_orient = true;

    // nothing until the tenons are built: a joint that makes none leaves both plates uncut
    for (int face = 0; face < 2; face++) {
        joint.male_fabrication_types[face] = {FabricationType::nothing};
        joint.female_fabrication_types[face] = {FabricationType::nothing};
    }

    Line own;
    if (!annen_shifted_line(joint, elements, joints, own))
        return;

    const int male = index_of_plate(elements, joint.element_a);
    const int side = joint.contact.face_a;
    const int wall_face = joint.contact.face_b;
    const FacePlanarElement& plate = *elements[male];
    const FacePlanarElement& wall = *elements[index_of_plate(elements, joint.element_b)];
    const Plane& outer = wall.planes[1 - wall_face];
    const Vector normal = plate.planes[0].z_axis();

    // the edge on each face in the order the plate was given; no tenons on an edge shorter than the minimum
    std::array<std::array<Point, 2>, 2> edges;
    for (int face = 0; face < 2; face++) {
        edges[face] = {
            plate.polylines[face].get_point(side - 2), plate.polylines[face].get_point(side - 1)
        };
        if (plate.reversed)
            std::swap(edges[face][0], edges[face][1]);
    }
    if (Point::distance(edges[0][0], edges[0][1]) < minimum_edge)
        return;

    // the cut planes' normal: across the insertion vector and the plate's feature axis on this face, the plate's normal when none is given
    const int given = plate.given_face(side);
    Vector along = side < (int)plate.insertion_vectors().size() ? plate.insertion_vector(side)
                                                                : Vector(0.0, 0.0, 0.0);
    if (along.magnitude() < 1e-9)
        along = normal.cross(edges[0][1] - edges[0][0]);
    Vector axis = given >= 0 && given < (int)plate.feature_axes.size() ? plate.feature_axes[given]
                                                                       : Vector(0.0, 0.0, 0.0);
    if (axis.magnitude() < 1e-9)
        axis = normal;
    Vector across = along.cross(axis);
    if (!across.normalize_self())
        return;

    // the line: this plate's own averaged with that of the plate across the wall, the nearest edge middle on the wall's other face within 1000 in this plate's plane, run the same way
    Line line = own;
    const Point middle = Point::mid_point(own.start(), own.end());
    double nearest = 1000.0;
    for (const InteractionFeaturePlate& other : joints) {
        if (other.element_b != joint.element_b || other.element_a == joint.element_a ||
            other.contact.face_b != 1 - wall_face)
            continue;
        Line theirs;
        if (!annen_shifted_line(other, elements, joints, theirs))
            continue;
        const Point centre = Point::mid_point(theirs.start(), theirs.end());
        const double distance = Point::distance(middle, centre);
        if (distance >= nearest || std::abs((centre - middle).dot(normal)) > 100.0)
            continue;
        nearest = distance;
        if (Point::distance(theirs.start(), own.start()) >
            Point::distance(theirs.end(), own.start()))
            theirs = Line::from_points(theirs.end(), theirs.start());
        line = Line::from_points(
            Point::mid_point(own.start(), theirs.start()), Point::mid_point(own.end(), theirs.end())
        );
    }

    // the steps on the middle 90% of the line: 4d + 1 when it is longer than the division length, 2d + 1 when shorter
    int d = divisions;
    if (d <= 0 && given >= 0 && given < (int)plate.feature_divisions.size())
        d = plate.feature_divisions[given];
    if (d <= 0)
        d = 2;
    const int steps =
        joint.division_length > 0.0 && line.length() > joint.division_length ? 4 * d : 2 * d;
    const Point centre = Point::mid_point(line.start(), line.end());
    const Point start = centre + (line.start() - centre) * 0.9;
    const Point end = centre + (line.end() - centre) * 0.9;
    const double step = 1.0 / (steps + 1);

    // on each face: a tenon between the cut planes a quarter step either side of every odd step, from the edge, on the wall's inner face, to the edge moved onto its outer face
    std::array<std::vector<Point>, 2> runs;
    for (int face = 0; face < 2; face++) {
        const Point& a = edges[face][0];
        const Vector u = edges[face][1] - a;
        Vector sideways = u.cross(plate.planes[face].z_axis());
        if (!sideways.normalize_self())
            return;
        const double reach = sideways.dot(outer.z_axis());
        if (std::abs(reach) < 1e-9)
            return;
        const Point b = a + sideways * ((outer.origin() - a).dot(outer.z_axis()) / reach);

        for (int k = 1; k < steps; k += 2) {
            std::array<Point, 2> root;
            std::array<Point, 2> tip;
            for (int station = 0; station < 2; station++) {
                const double t = (k + (station == 0 ? -0.25 : 0.25)) * step;
                const Plane cut = Plane::from_point_normal(start + (end - start) * t, across);
                if (!Intersection::line_plane(
                        Line::from_points(a, a + u), cut, root[station], false
                    ) ||
                    !Intersection::line_plane(
                        Line::from_points(b, b + u), cut, tip[station], false
                    ))
                    return;
            }
            runs[face].insert(runs[face].end(), {root[0], tip[0], tip[1], root[1]});
        }
        if (runs[face].empty())
            return;
        joint.male_outlines[face] = {
            Polyline(runs[face]), Polyline({runs[face].front(), runs[face].back()})
        };
        joint.male_fabrication_types[face] = {
            FabricationType::edge_insertion, FabricationType::edge_insertion
        };
    }

    // the wall's slots: per tenon the quad through its four roots on the inner face and the one through its tips on the outer, then the hull of them all
    const std::vector<Point>& bottom = runs[0];
    const std::vector<Point>& top = runs[1];
    std::vector<Polyline>& inner_slots = joint.female_outlines[wall_face];
    std::vector<Polyline>& outer_slots = joint.female_outlines[1 - wall_face];
    for (size_t j = 0; j + 3 < bottom.size(); j += 4) {
        inner_slots.push_back(Polyline({top[j], top[j + 3], bottom[j + 3], bottom[j], top[j]}));
        outer_slots.push_back(
            Polyline({top[j + 1], top[j + 2], bottom[j + 2], bottom[j + 1], top[j + 1]})
        );
    }
    for (int face = 0; face < 2; face++) {
        const Polyline& first = joint.female_outlines[face].front();
        const Polyline& last = joint.female_outlines[face].back();
        joint.female_outlines[face].push_back(Polyline(
            {first.get_point(0),
             last.get_point(1),
             last.get_point(2),
             first.get_point(3),
             first.get_point(0)}
        ));
    }
    const std::vector<int> holes(joint.female_outlines[0].size(), FabricationType::hole);
    joint.female_fabrication_types = {holes, holes};
}
