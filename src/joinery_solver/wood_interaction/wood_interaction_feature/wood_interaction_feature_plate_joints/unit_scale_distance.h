/// The axial size a unit-scale in-plane design is pinned to, as 2024 read it: the thickness of the male plate when the
/// plates are given, else the thickness edge of the first volume rectangle (p1 to p2, what apply_unit_scale reads), else
/// what the joint already carries.
static double unit_scale_distance(const InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    const int v0 = index_of_plate(elements, joint.element_a);
    if (v0 >= 0)
        return elements[v0]->thickness;

    const std::optional<Polyline>& volume = joint.joint_volumes[0];
    if (volume && volume->point_count() >= 3)
        return Point::distance(volume->get_point(1), volume->get_point(2));

    return joint.unit_scale_distance;
}
