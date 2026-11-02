/// The Annen corner fingers of Christopher Robeller's 2017 plugin, built in world space where the two walls of a box meet: their
/// corner from the box's bottom face to its top in n equal parts, cut at every part by a plane parallel to the box faces turned by
/// +angle and -angle in turn about the long edge of the axis wall, the one whose mitre face carries the design; the first part above
/// the bottom is a finger of the axis wall, every finger reaching the other wall's outer face. The mitre faces' insertion vectors
/// point from the bottom face to the top; n comes from the axis wall's feature_divisions on its mitre face when given, else from the
/// joint's divisions.
static void side_side_outplane_annen_fingers(
    InteractionFeaturePlate& joint,
    const std::vector<std::shared_ptr<Plate>>& elements,
    int design,
    double angle
) {

    joint.name = "side_side_outplane_annen_fingers";
    joint.no_orient = true;

    const std::array<int, 2> indices = {
        index_of_plate(elements, joint.element_a), index_of_plate(elements, joint.element_b)
    };
    const std::array<int, 2> faces = {joint.contact.face_a, joint.contact.face_b};
    for (int k = 0; k < 2; k++)
        if (indices[k] < 0 || faces[k] < 2 ||
            faces[k] >= (int)elements[indices[k]]->polylines.size() ||
            elements[indices[k]]->polylines[0].point_count() != 5)
            return;
    const std::array<const Plate*, 2> walls = {
        elements[indices[0]].get(), elements[indices[1]].get()
    };

    // the axis wall: the one whose mitre face carries the design, else the first; the box's up: a mitre face's insertion vector, else the joint line
    int axis = 0;
    Vector up(0.0, 0.0, 0.0);
    for (int k = 1; k >= 0; k--) {
        const int given = walls[k]->given_face(faces[k]);
        if (given >= 0 && given < (int)walls[k]->feature_types.size() &&
            std::abs(walls[k]->feature_types[given]) == design)
            axis = k;
        if (faces[k] < (int)walls[k]->insertion_vectors().size() &&
            walls[k]->insertion_vector(faces[k]).magnitude() > 1e-9)
            up = walls[k]->insertion_vector(faces[k]);
    }
    if (up.magnitude() < 1e-9)
        up = joint.joint_lines[0].to_vector();
    if (!up.normalize_self())
        return;

    // per wall: its inner face, the one facing the other wall, and its bottom and top edges, the outline edges beside the mitre edge, from the corner outward
    struct Corner {
        Plane inner;
        Plane outer;
        Line bottom;
        Line top;
    };

    std::array<Corner, 2> corners;
    for (int k = 0; k < 2; k++) {
        const Plate& wall = *walls[k];
        const Point other = walls[1 - k]->polylines[0].center();
        const bool first_inner = wall.planes[0].z_axis().dot(other - wall.planes[0].origin()) > 0.0;
        corners[k].inner = wall.planes[first_inner ? 0 : 1];
        corners[k].outer = wall.planes[first_inner ? 1 : 0];
        const Polyline& outline = wall.polylines[0];
        const int j = faces[k] - 2;
        const Line before = Line::from_points(outline.get_point(j), outline.get_point((j + 3) % 4));
        const Line after =
            Line::from_points(outline.get_point(j + 1), outline.get_point((j + 2) % 4));
        const bool before_lower = (before.center() - after.center()).dot(up) < 0.0;
        corners[k].bottom = before_lower ? before : after;
        corners[k].top = before_lower ? after : before;
    }

    // the box faces, through both walls' bottom edges and through both top edges
    const Plane bottom_plane(
        corners[axis].bottom.start(),
        corners[axis].bottom.to_vector(),
        corners[1 - axis].bottom.to_vector()
    );
    const Plane top_plane(
        corners[axis].top.start(), corners[axis].top.to_vector(), corners[1 - axis].top.to_vector()
    );

    // a corner line from the bottom face to the top where two planes meet
    const auto corner_line = [&](const Plane& first, const Plane& second, Line& line) {
        Line meet;
        Point start;
        Point end;
        if (!Intersection::plane_plane(first, second, meet) ||
            !Intersection::line_plane(meet, bottom_plane, start, false) ||
            !Intersection::line_plane(meet, top_plane, end, false))
            return false;
        line = Line::from_points(start, end);
        return true;
    };
    Line outer_corner;
    Line inner_corner;
    if (!corner_line(corners[0].outer, corners[1].outer, outer_corner) ||
        !corner_line(corners[0].inner, corners[1].inner, inner_corner))
        return;
    const Line centre = Line::from_points(
        Point::mid_point(outer_corner.start(), inner_corner.start()),
        Point::mid_point(outer_corner.end(), inner_corner.end())
    );

    // the cuts: the box faces at the ends, between them planes parallel to them through the centre line's stations, turned about the axis wall's long edge
    const int given = walls[axis]->given_face(faces[axis]);
    int parts = given >= 0 && given < (int)walls[axis]->feature_divisions.size()
                    ? walls[axis]->feature_divisions[given]
                    : 0;
    if (parts <= 0)
        parts = std::max(joint.divisions, 1);
    const Vector long_edge = corners[axis].bottom.to_vector().normalized();
    std::vector<Plane> cuts;
    for (int i = 0; i <= parts; i++) {
        if (i == 0 || i == parts) {
            cuts.push_back(i == 0 ? bottom_plane : top_plane);
            continue;
        }
        const Point station = centre.point_at((double)i / parts);
        const Plane level = Plane::from_point_normal(station, bottom_plane.z_axis());
        cuts.push_back(level.transformed(
            Xform::rotation_around_line(
                Line::from_points(station, station + long_edge), i % 2 == 0 ? angle : -angle, true
            )
        ));
    }

    // each wall's two faces: between the corner lines where the face meets the other wall's two faces, a zigzag from the bottom, the axis wall's fingers first
    for (int k = 0; k < 2; k++) {
        const Plate& wall = *walls[k];
        const Corner& other = corners[1 - k];
        for (int face = 0; face < 2; face++) {
            Line with_outer;
            Line with_inner;
            if (!corner_line(wall.planes[face], other.outer, with_outer) ||
                !corner_line(wall.planes[face], other.inner, with_inner))
                return;
            const Line& first = k == axis ? with_outer : with_inner;
            const Line& second = k == axis ? with_inner : with_outer;
            const auto at = [&](const Line& line, int i) {
                Point point;
                Intersection::line_plane(line, cuts[i], point, false);
                return point;
            };
            std::vector<Point> run = {at(first, 0)};
            for (int i = 1; i < parts; i++) {
                run.push_back(at(i % 2 == 1 ? first : second, i));
                run.push_back(at(i % 2 == 1 ? second : first, i));
            }
            run.push_back(at(parts % 2 == 1 ? first : second, parts));
            std::array<std::vector<Polyline>, 2>& outlines =
                k == 0 ? joint.male_outlines : joint.female_outlines;
            std::array<std::vector<int>, 2>& types =
                k == 0 ? joint.male_fabrication_types : joint.female_fabrication_types;
            outlines[face] = {Polyline(run), Polyline({run.front(), run.back()})};
            types[face] = {FabricationType::edge_insertion, FabricationType::edge_insertion};
        }
    }
}
