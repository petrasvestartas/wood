/// ts_e_p_0: the fixed three-tenon design, 2024's literals: per female face three mortise rectangles and the rectangle that bounds
/// them, every one a hole (the merge drops the last, the bound, as 2024 did), per male face the zigzag of the three tenons and its
/// two-point edge marker, both edge insertions. By name only: no id of the 2024 table reaches it, case 23 fell through into ts_e_p_4.
static void ts_e_p_0(InteractionFeaturePlate& joint) {

    joint.name = "ts_e_p_0";

    joint.female_outlines[0] = {
        Polyline({Point(-0.5, -0.5, 0.357142857142857), Point(0.5, -0.5, 0.357142857142857), Point(0.5, -0.5, 0.214285714285714), Point(-0.5, -0.5, 0.214285714285714), Point(-0.5, -0.5, 0.357142857142857)}),
        Polyline({Point(-0.5, -0.5, 0.0714285714285715), Point(0.5, -0.5, 0.0714285714285715), Point(0.5, -0.5, -0.0714285714285713), Point(-0.5, -0.5, -0.0714285714285713), Point(-0.5, -0.5, 0.0714285714285715)}),
        Polyline({Point(-0.5, -0.5, -0.214285714285714), Point(0.5, -0.5, -0.214285714285714), Point(0.5, -0.5, -0.357142857142857), Point(-0.5, -0.5, -0.357142857142857), Point(-0.5, -0.5, -0.214285714285714)}),
        Polyline({Point(-0.5, -0.5, 0.357142857142857), Point(-0.5, -0.5, -0.357142857142857), Point(0.5, -0.5, -0.357142857142857), Point(0.5, -0.5, 0.357142857142857), Point(-0.5, -0.5, 0.357142857142857)}),
    };
    joint.female_outlines[1] = {
        Polyline({Point(-0.5, 0.5, 0.357142857142857), Point(0.5, 0.5, 0.357142857142857), Point(0.5, 0.5, 0.214285714285714), Point(-0.5, 0.5, 0.214285714285714), Point(-0.5, 0.5, 0.357142857142857)}),
        Polyline({Point(-0.5, 0.5, 0.0714285714285713), Point(0.5, 0.5, 0.0714285714285713), Point(0.5, 0.5, -0.0714285714285715), Point(-0.5, 0.5, -0.0714285714285715), Point(-0.5, 0.5, 0.0714285714285713)}),
        Polyline({Point(-0.5, 0.5, -0.214285714285714), Point(0.5, 0.5, -0.214285714285714), Point(0.5, 0.5, -0.357142857142857), Point(-0.5, 0.5, -0.357142857142857), Point(-0.5, 0.5, -0.214285714285714)}),
        Polyline({Point(-0.5, 0.5, 0.357142857142857), Point(-0.5, 0.5, -0.357142857142857), Point(0.5, 0.5, -0.357142857142857), Point(0.5, 0.5, 0.357142857142857), Point(-0.5, 0.5, 0.357142857142857)}),
    };

    joint.male_outlines[0] = {
        Polyline({
            Point(0.5, -0.5, -0.357142857142857), Point(0.5, 0.5, -0.357142857142857), Point(0.5, 0.5, -0.214285714285714), Point(0.5, -0.5, -0.214285714285714),
            Point(0.5, -0.5, -0.0714285714285715), Point(0.5, 0.5, -0.0714285714285713), Point(0.5, 0.5, 0.0714285714285715), Point(0.5, -0.5, 0.0714285714285714),
            Point(0.5, -0.5, 0.214285714285714), Point(0.5, 0.5, 0.214285714285714), Point(0.5, 0.5, 0.357142857142857), Point(0.5, -0.5, 0.357142857142857),
        }),
        Polyline({Point(0.5, -0.5, -0.357142857142857), Point(0.5, -0.5, 0.357142857142857)}),
    };
    joint.male_outlines[1] = {
        Polyline({
            Point(-0.5, -0.5, -0.357142857142857), Point(-0.5, 0.5, -0.357142857142857), Point(-0.5, 0.5, -0.214285714285714), Point(-0.5, -0.5, -0.214285714285714),
            Point(-0.5, -0.5, -0.0714285714285715), Point(-0.5, 0.5, -0.0714285714285713), Point(-0.5, 0.5, 0.0714285714285715), Point(-0.5, -0.5, 0.0714285714285714),
            Point(-0.5, -0.5, 0.214285714285714), Point(-0.5, 0.5, 0.214285714285714), Point(-0.5, 0.5, 0.357142857142857), Point(-0.5, -0.5, 0.357142857142857),
        }),
        Polyline({Point(-0.5, -0.5, -0.357142857142857), Point(-0.5, -0.5, 0.357142857142857)}),
    };

    const std::vector<int> female_types(4, FabricationType::hole);
    joint.female_fabrication_types = {female_types, female_types};
    joint.male_fabrication_types[0] = {FabricationType::edge_insertion, FabricationType::edge_insertion};
    joint.male_fabrication_types[1] = {FabricationType::edge_insertion, FabricationType::edge_insertion};
}
