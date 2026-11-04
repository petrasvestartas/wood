/// A skew T-bridle: the side member ends in a fork of two cheeks around a tongue left in the top member. In the unit box y runs across the top member
/// from its near face at -0.5 to its far face, z along it, x through the height. The tongue is the middle layer, `tongue` of the height; a cheek lies in a
/// pocket on either side of it, flush with both faces. The pocket has one wall square to the top member at the side member's obtuse corner; at its acute
/// corner a shoulder square to the top member, `joint.shift` of the width long, then a wall along the side member. The shoulder leaves a tooth in the
/// side member's edge, so that member only slides in square to the top one, from the near face. The slot stops square to the side member at the pocket's
/// corner, and the tongue is sawn away from that corner to the near end of the square wall, or to the slot's line where that lies further in, to clear the
/// solid behind the slot. The lean is read from the male plate, so the walls stay square at any angle.
static void top_side_bridle(
    InteractionFeaturePlate& joint,
    const std::vector<std::shared_ptr<Plate>>& elements,
    const double tongue
) {

    joint.name = "top_side_bridle";

    // the lean: how far the side member runs along the top one per width across it, and the box's length over its width
    double lean = 0.0;
    double aspect = 1.0;
    const int male = index_of_plate(elements, joint.element_a);
    if (male >= 0 && joint.joint_volumes[0] && joint.joint_volumes[1]) {
        const Polyline& first = *joint.joint_volumes[0];
        const Polyline& second = *joint.joint_volumes[1];
        const Vector across = first[3] - first[0];
        const Vector along = second[0] - first[0];
        const Plate& plate = *elements[male];
        const Vector middle =
            Point::mid_point(plate.polylines[0].center(), plate.polylines[1].center()) -
            Point::mid_point(first[0], second[1]);
        lean = (middle.dot(along) / along.magnitude_squared()) /
               (middle.dot(across) / across.magnitude_squared());
        aspect = along.magnitude() / across.magnitude();
    }

    // the plan drawn with the acute corner at z 0.5: the shoulder wall at z `wall`, the pocket's corner at y `corner`
    const double sign = lean > 0.0 ? -1.0 : 1.0;
    const double slope = std::abs(lean);
    const double half = 0.5 * tongue;
    const double overshoot = 0.1;
    const double near_side = -0.5 - overshoot;
    const double far_side = 0.5 + overshoot;
    const double reach = 0.5 + overshoot;
    const double past = 4.0;
    const double corner = joint.shift - 0.5;
    const double wall = 0.5 - slope * joint.shift;

    const auto layer = [sign](const std::vector<std::array<double, 2>>& plan, const double x) {
        std::vector<Point> points;
        for (const std::array<double, 2>& point : plan)
            points.push_back(Point(x, point[0], sign * point[1]));
        points.push_back(points.front());
        return Polyline(points);
    };

    // the slot's line, and where the saw cut through the tongue meets the square wall: the near face, or the slot's line when that is further in
    const double run = slope * aspect * aspect;
    const double sawn = std::max(-0.5, corner + run * (-0.5 - wall));

    // the female: a pocket in each face and the sawn piece of the tongue, past the top member's faces
    const std::vector<std::array<double, 2>> pocket = {
        {near_side, wall},
        {corner, wall},
        {far_side, 0.5 - slope * (far_side + 0.5)},
        {far_side, -0.5},
        {near_side, -0.5}
    };
    const std::vector<std::array<double, 2>> triangle = {
        {corner, wall}, {sawn, -0.5}, {near_side, -0.5}, {near_side, wall}
    };
    joint.female_outlines[0] = {
        layer(pocket, half), layer(pocket, -reach), layer(triangle, -reach)
    };
    joint.female_outlines[1] = {layer(pocket, reach), layer(pocket, -half), layer(triangle, reach)};

    // the male: through the whole height what lies past the far face, past the square wall and at the tooth, then the slot
    const std::vector<std::array<double, 2>> ends = {
        {-0.5, reach},
        {-0.5, wall},
        {0.5, wall},
        {0.5, -0.5},
        {near_side, -0.5},
        {near_side, -past},
        {past, -past},
        {past, reach}
    };
    const std::vector<std::array<double, 2>> slot = {
        {corner + run * (reach - wall), reach},
        {corner + run * (-past - wall), -past},
        {past, -past},
        {past, reach}
    };
    joint.male_outlines[0] = {layer(ends, -reach), layer(slot, -half)};
    joint.male_outlines[1] = {layer(ends, reach), layer(slot, half)};

    joint.female_fabrication_types = {
        std::vector<int>(3, FabricationType::mill), std::vector<int>(3, FabricationType::mill)
    };
    joint.male_fabrication_types = {
        std::vector<int>(2, FabricationType::mill), std::vector<int>(2, FabricationType::mill)
    };
}
