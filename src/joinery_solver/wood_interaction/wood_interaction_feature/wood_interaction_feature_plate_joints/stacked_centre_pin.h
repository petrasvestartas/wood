/// stacked_centre_pin, id 40 and the family default: one drill through the centre of the contact, the average of its corners, as 2024's center.
static void stacked_centre_pin(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "stacked_centre_pin";
    joint.no_orient = true;

    Vector dir0;
    Vector dir1;
    if (!drill_directions(joint, elements, dir0, dir1))
        return;

    emit_drills(joint, {joint.contact.polygon.center()}, dir0, dir1);
}
