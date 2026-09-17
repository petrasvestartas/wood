/// ts_e_p_1: hardcoded two-mortise tenon joint; not routed by the dispatcher, kept complete.
[[maybe_unused]] static void ts_e_p_1(WoodJoint& joint) {

    const double z_top   = 0.166666666666667;
    const double z_top2  = 0.0555555555555556;
    const double z_bot   = -0.277777777777778;
    const double z_bot2  = -0.388888888888889;

    joint.female_outlines[0] = {
        Polyline({
            Point(-0.5,-0.5, z_bot ), Point( 0.5,-0.5, z_bot ),
            Point( 0.5,-0.5, z_bot2), Point(-0.5,-0.5, z_bot2),
            Point(-0.5,-0.5, z_bot )}),
        Polyline({
            Point(-0.5,-0.5, z_top ), Point( 0.5,-0.5, z_top ),
            Point( 0.5,-0.5, z_top2), Point(-0.5,-0.5, z_top2),
            Point(-0.5,-0.5, z_top )}),
        Polyline({
            Point(-0.5,-0.5, z_top ), Point(-0.5,-0.5, z_bot2),
            Point( 0.5,-0.5, z_bot2), Point( 0.5,-0.5, z_top ),
            Point(-0.5,-0.5, z_top )}),
    };
    joint.female_outlines[1] = {
        Polyline({
            Point(-0.5, 0.5, z_bot ), Point( 0.5, 0.5, z_bot ),
            Point( 0.5, 0.5, z_bot2), Point(-0.5, 0.5, z_bot2),
            Point(-0.5, 0.5, z_bot )}),
        Polyline({
            Point(-0.5, 0.5, z_top ), Point( 0.5, 0.5, z_top ),
            Point( 0.5, 0.5, z_top2), Point(-0.5, 0.5, z_top2),
            Point(-0.5, 0.5, z_top )}),
        Polyline({
            Point(-0.5, 0.5, z_top ), Point(-0.5, 0.5, z_bot2),
            Point( 0.5, 0.5, z_bot2), Point( 0.5, 0.5, z_top ),
            Point(-0.5, 0.5, z_top )}),
    };

    joint.male_outlines[0] = {
        Polyline({
            Point( 0.5,-0.5, z_top ), Point( 0.5, 0.5, z_top ),
            Point( 0.5, 0.5, z_top2), Point( 0.5,-0.5, z_top2),
            Point( 0.5,-0.5, z_bot ), Point( 0.5, 0.5, z_bot ),
            Point( 0.5, 0.5, z_bot2), Point( 0.5,-0.5, z_bot2)}),
        Polyline({Point( 0.5,-0.5, 0.5), Point( 0.5,-0.5,-0.5)}),
    };
    joint.male_outlines[1] = {
        Polyline({
            Point(-0.5,-0.5, z_top ), Point(-0.5, 0.5, z_top ),
            Point(-0.5, 0.5, z_top2), Point(-0.5,-0.5, z_top2),
            Point(-0.5,-0.5, z_bot ), Point(-0.5, 0.5, z_bot ),
            Point(-0.5, 0.5, z_bot2), Point(-0.5,-0.5, z_bot2)}),
        Polyline({Point(-0.5,-0.5, 0.5), Point(-0.5,-0.5,-0.5)}),
    };

    joint.female_cut_types[0] = { wood_cut::hole, wood_cut::hole, wood_cut::insert_between_multiple_edges };
    joint.female_cut_types[1] = { wood_cut::hole, wood_cut::hole, wood_cut::insert_between_multiple_edges };
    joint.male_cut_types[0] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
    joint.male_cut_types[1] = { wood_cut::edge_insertion, wood_cut::edge_insertion };
}
