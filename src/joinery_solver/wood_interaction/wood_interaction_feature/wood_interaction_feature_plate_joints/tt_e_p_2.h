/// tt_e_p_2, id 42: division_length drills, the family's count, on the polylabel circle of the contact with its radius scaled by shift,
/// from 45 degrees on in a frame whose x axis runs along the edge closest to the centre, as 2024 called
/// get_polylabel_circle_division_points with no direction, orient_to_closest_edge and precision 1. The kernel steps 360 / count
/// exactly where 2024 stepped the integer quotient and rounded each angle to float. Nothing for a count below one.
static void tt_e_p_2(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "tt_e_p_2";
    joint.no_orient = true;

    Vector dir0;
    Vector dir1;
    if (!drill_directions(joint, elements, dir0, dir1))
        return;

    const int count = (int)joint.division_length;
    if (count < 1)
        return;

    const std::vector<Point> points = Polyline::polylabel_circle_division_points(
        Vector(0.0, 0.0, 0.0),
        {joint.contact.polygon},
        count,
        joint.shift,
        1.0,
        true
    );
    emit_drills(joint, points, dir0, dir1);
}
