/// top_top_polylabel_pin, id 41: one drill through the polylabel of the contact, the centre of its largest inscribed circle found at precision 1,
/// as 2024's get_polylabel; on a rectangle that is its centre.
static void top_top_polylabel_pin(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "top_top_polylabel_pin";
    joint.no_orient = true;

    Vector dir0;
    Vector dir1;
    if (!drill_directions(joint, elements, dir0, dir1))
        return;

    const std::tuple<Point, Plane, double> circle = Polyline::polylabel({joint.contact.polygon}, 1.0);
    emit_drills(joint, {std::get<0>(circle)}, dir0, dir1);
}
