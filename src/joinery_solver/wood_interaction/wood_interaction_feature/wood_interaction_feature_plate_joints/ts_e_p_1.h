/// ts_e_p_1: the fixed two-tenon design of the Annen project, 2024's literals: per female face two mortise rectangles and the
/// rectangle that bounds them, every one a hole (2024 listed a fourth hole type with no outline; it did nothing), per male face
/// the two tenons as one run and the full-height edge marker, both edge insertions. By name only, no id of the 2024 table reaches it.
static void ts_e_p_1(InteractionFeaturePlate& joint) {

    joint.name = "ts_e_p_1";

    joint.female_outlines[0] = {
        Polyline({Point(-0.5, -0.5, -0.277777777777778), Point(0.5, -0.5, -0.277777777777778), Point(0.5, -0.5, -0.388888888888889), Point(-0.5, -0.5, -0.388888888888889), Point(-0.5, -0.5, -0.277777777777778)}),
        Polyline({Point(-0.5, -0.5, 0.166666666666667), Point(0.5, -0.5, 0.166666666666667), Point(0.5, -0.5, 0.0555555555555556), Point(-0.5, -0.5, 0.0555555555555556), Point(-0.5, -0.5, 0.166666666666667)}),
        Polyline({Point(-0.5, -0.5, 0.166666666666667), Point(-0.5, -0.5, -0.388888888888889), Point(0.5, -0.5, -0.388888888888889), Point(0.5, -0.5, 0.166666666666667), Point(-0.5, -0.5, 0.166666666666667)}),
    };
    joint.female_outlines[1] = {
        Polyline({Point(-0.5, 0.5, -0.277777777777778), Point(0.5, 0.5, -0.277777777777778), Point(0.5, 0.5, -0.388888888888889), Point(-0.5, 0.5, -0.388888888888889), Point(-0.5, 0.5, -0.277777777777778)}),
        Polyline({Point(-0.5, 0.5, 0.166666666666667), Point(0.5, 0.5, 0.166666666666667), Point(0.5, 0.5, 0.0555555555555556), Point(-0.5, 0.5, 0.0555555555555556), Point(-0.5, 0.5, 0.166666666666667)}),
        Polyline({Point(-0.5, 0.5, 0.166666666666667), Point(-0.5, 0.5, -0.388888888888889), Point(0.5, 0.5, -0.388888888888889), Point(0.5, 0.5, 0.166666666666667), Point(-0.5, 0.5, 0.166666666666667)}),
    };

    joint.male_outlines[0] = {
        Polyline({
            Point(0.5, -0.5, 0.166666666666667), Point(0.5, 0.5, 0.166666666666667), Point(0.5, 0.5, 0.0555555555555556), Point(0.5, -0.5, 0.0555555555555556),
            Point(0.5, -0.5, -0.277777777777778), Point(0.5, 0.5, -0.277777777777778), Point(0.5, 0.5, -0.388888888888889), Point(0.5, -0.5, -0.388888888888889),
        }),
        Polyline({Point(0.5, -0.5, 0.5), Point(0.5, -0.5, -0.5)}),
    };
    joint.male_outlines[1] = {
        Polyline({
            Point(-0.5, -0.5, 0.166666666666667), Point(-0.5, 0.5, 0.166666666666667), Point(-0.5, 0.5, 0.0555555555555558), Point(-0.5, -0.5, 0.0555555555555557),
            Point(-0.5, -0.5, -0.277777777777778), Point(-0.5, 0.5, -0.277777777777778), Point(-0.5, 0.5, -0.388888888888889), Point(-0.5, -0.5, -0.388888888888889),
        }),
        Polyline({Point(-0.5, -0.5, 0.5), Point(-0.5, -0.5, -0.5)}),
    };

    const std::vector<int> female_types(3, FabricationType::hole);
    joint.female_fabrication_types = {female_types, female_types};
    joint.male_fabrication_types[0] = {FabricationType::edge_insertion, FabricationType::edge_insertion};
    joint.male_fabrication_types[1] = {FabricationType::edge_insertion, FabricationType::edge_insertion};
}
