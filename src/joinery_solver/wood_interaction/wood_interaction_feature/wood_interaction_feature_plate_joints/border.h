/// The border of a plate: a slab cut off its side face along the whole side, through the whole thickness, 15 mm deep (scale z adds
/// to it), the side extended by scale x at both ends and the thickness by scale y beyond each face, a millimetre each by default. Written as two pairs
/// of slice rectangles parallel to the face, a quarter millimetre and the depth off it, one pair into the plate and one out of it, so the
/// slab is cut whichever way the face looks.
static void border(InteractionFeaturePlate& joint) {

    joint.name = "border";
    joint.no_orient = true;

    if (!joint.joint_volumes[0] || !joint.joint_volumes[1])
        return;

    // the two volume rectangles lie on the plate's faces along the side, straddling it: between them the side's mid-line and its frame
    const Polyline& lower = *joint.joint_volumes[0];
    const Polyline& upper = *joint.joint_volumes[1];
    if (lower.point_count() < 5 || upper.point_count() < 5)
        return;

    const Point start = Point::lerp(
        Point::lerp(lower.get_point(0), lower.get_point(1), 0.5),
        Point::lerp(upper.get_point(0), upper.get_point(1), 0.5),
        0.5
    );
    const Point end = Point::lerp(
        Point::lerp(lower.get_point(2), lower.get_point(3), 0.5),
        Point::lerp(upper.get_point(2), upper.get_point(3), 0.5),
        0.5
    );
    const Vector half_thickness = (upper.get_point(0) - lower.get_point(0)) * 0.5;
    const Vector side = lower.get_point(1) - lower.get_point(0);
    const double half = std::sqrt(half_thickness.magnitude_squared());
    const double side_length = std::sqrt(side.magnitude_squared());
    const double length = std::sqrt((end - start).magnitude_squared());
    if (half < 1e-12 || side_length < 1e-12 || length < 1e-12)
        return;

    const Vector along = (end - start) * (joint.scale[0] / length);
    const Vector across = half_thickness * ((half + joint.scale[1]) / half);
    const Vector normal = side * (1.0 / side_length);

    // the slice rectangle over the whole side, moved off the face by a distance along its normal
    const auto rectangle = [&](double offset) {
        const Vector off = normal * offset;
        return Polyline(
            {start - along - across + off,
             start - along + across + off,
             end + along + across + off,
             end + along - across + off,
             start - along - across + off}
        );
    };
    const double depth = 15.0 + joint.scale[2];
    const Polyline near_in = rectangle(0.25);
    const Polyline far_in = rectangle(depth);
    const Polyline near_out = rectangle(-0.25);
    const Polyline far_out = rectangle(-depth);

    joint.male_outlines[0] = {near_in, near_in, near_out, near_out};
    joint.male_outlines[1] = {far_in, far_in, far_out, far_out};

    joint.male_fabrication_types[0] = {
        FabricationType::slice,
        FabricationType::slice,
        FabricationType::slice,
        FabricationType::slice
    };
    joint.male_fabrication_types[1] = {
        FabricationType::slice,
        FabricationType::slice,
        FabricationType::slice,
        FabricationType::slice
    };
}
