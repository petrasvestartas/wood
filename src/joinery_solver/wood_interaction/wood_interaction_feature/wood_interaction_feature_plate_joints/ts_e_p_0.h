/// ts_e_p_0: hardcoded three-finger tenon-mortise - three mortise holes plus a bounding rectangle per female face, a zigzag per male face.
static void ts_e_p_0(FeaturePlate& joint) {

    joint.name = "ts_e_p_0";

    const double a = 0.357142857142857;
    const double b = 0.214285714285714;
    const double c = 0.0714285714285715;

    joint.female_outlines[0].clear();
    joint.female_outlines[0].push_back(Polyline({
        Point(-0.5,-0.5, a), Point( 0.5,-0.5, a), Point( 0.5,-0.5, b), Point(-0.5,-0.5, b), Point(-0.5,-0.5, a)}));
    joint.female_outlines[0].push_back(Polyline({
        Point(-0.5,-0.5, c), Point( 0.5,-0.5, c), Point( 0.5,-0.5,-c), Point(-0.5,-0.5,-c), Point(-0.5,-0.5, c)}));
    joint.female_outlines[0].push_back(Polyline({
        Point(-0.5,-0.5,-b), Point( 0.5,-0.5,-b), Point( 0.5,-0.5,-a), Point(-0.5,-0.5,-a), Point(-0.5,-0.5,-b)}));
    joint.female_outlines[0].push_back(Polyline({
        Point(-0.5,-0.5, a), Point(-0.5,-0.5,-a), Point( 0.5,-0.5,-a), Point( 0.5,-0.5, a), Point(-0.5,-0.5, a)}));

    joint.female_outlines[1].clear();
    joint.female_outlines[1].push_back(Polyline({
        Point(-0.5, 0.5, a), Point( 0.5, 0.5, a), Point( 0.5, 0.5, b), Point(-0.5, 0.5, b), Point(-0.5, 0.5, a)}));
    joint.female_outlines[1].push_back(Polyline({
        Point(-0.5, 0.5, c), Point( 0.5, 0.5, c), Point( 0.5, 0.5,-c), Point(-0.5, 0.5,-c), Point(-0.5, 0.5, c)}));
    joint.female_outlines[1].push_back(Polyline({
        Point(-0.5, 0.5,-b), Point( 0.5, 0.5,-b), Point( 0.5, 0.5,-a), Point(-0.5, 0.5,-a), Point(-0.5, 0.5,-b)}));
    joint.female_outlines[1].push_back(Polyline({
        Point(-0.5, 0.5, a), Point(-0.5, 0.5,-a), Point( 0.5, 0.5,-a), Point( 0.5, 0.5, a), Point(-0.5, 0.5, a)}));

    const Polyline m0_outline({
        Point( 0.5,-0.5,-a), Point( 0.5, 0.5,-a),
        Point( 0.5, 0.5,-b), Point( 0.5,-0.5,-b),
        Point( 0.5,-0.5,-c), Point( 0.5, 0.5,-c),
        Point( 0.5, 0.5, c), Point( 0.5,-0.5, c),
        Point( 0.5,-0.5, b), Point( 0.5, 0.5, b),
        Point( 0.5, 0.5, a), Point( 0.5,-0.5, a)
    });
    const Polyline m0_endpoints({ Point( 0.5,-0.5,-a), Point( 0.5,-0.5, a) });
    joint.male_outlines[0] = { m0_outline, m0_endpoints };

    const Polyline m1_outline({
        Point(-0.5,-0.5,-a), Point(-0.5, 0.5,-a),
        Point(-0.5, 0.5,-b), Point(-0.5,-0.5,-b),
        Point(-0.5,-0.5,-c), Point(-0.5, 0.5,-c),
        Point(-0.5, 0.5, c), Point(-0.5,-0.5, c),
        Point(-0.5,-0.5, b), Point(-0.5, 0.5, b),
        Point(-0.5, 0.5, a), Point(-0.5,-0.5, a)
    });
    const Polyline m1_endpoints({ Point(-0.5,-0.5,-a), Point(-0.5,-0.5, a) });
    joint.male_outlines[1] = { m1_outline, m1_endpoints };

    joint.female_fabrication_types[0] = { FabricationType::hole, FabricationType::hole, FabricationType::hole, FabricationType::insert_between_multiple_edges };
    joint.female_fabrication_types[1] = { FabricationType::hole, FabricationType::hole, FabricationType::hole, FabricationType::insert_between_multiple_edges };
    joint.male_fabrication_types[0] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
    joint.male_fabrication_types[1] = { FabricationType::edge_insertion, FabricationType::edge_insertion };
}
