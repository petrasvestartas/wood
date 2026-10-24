static void top_top_circle_pins(
    InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements
) {

    joint.name = "top_top_circle_pins";
    joint.no_orient = true;

    Vector dir0;
    Vector dir1;
    if (!drill_directions(joint, elements, dir0, dir1))
        return;

    const int count = (int)joint.division_length;
    if (count < 1)
        return;

    const std::vector<Point> points = Polyline::polylabel_circle_division_points(
        Vector(0.0, 0.0, 0.0), {joint.contact.polygon}, count, joint.shift, 1.0, true
    );
    emit_drills(joint, points, dir0, dir1);
}
