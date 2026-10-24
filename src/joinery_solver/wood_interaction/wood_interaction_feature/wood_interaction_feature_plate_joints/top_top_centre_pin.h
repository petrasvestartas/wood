static void top_top_centre_pin(
    InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements
) {

    joint.name = "top_top_centre_pin";
    joint.no_orient = true;

    Vector dir0;
    Vector dir1;
    if (!drill_directions(joint, elements, dir0, dir1))
        return;

    emit_drills(joint, {joint.contact.polygon.center()}, dir0, dir1);
}
