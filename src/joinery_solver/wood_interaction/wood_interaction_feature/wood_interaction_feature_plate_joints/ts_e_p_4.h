/// ts_e_p_4: the fixed wedge design 2024 kept as literals, 240 points: per female face four mill pockets, per male face two
/// wedge flanks, two walls and two caps as mill_project and two slices, every outline written twice as 2024 doubled them (the
/// solid cutter counts a repeated pair once); face 0 of each side pairs with face 1 outline for outline, each pair the two ends
/// of one solid. The ids 23 and 24: 2024's case 23 ran ts_e_p_0 and fell through into ts_e_p_4, which overwrote all of it.
static void ts_e_p_4(InteractionFeaturePlate& joint) {

    joint.name = "ts_e_p_4";

    // the female pockets: on face 0 their floors at z -0.01 and the four side tiles, each listed twice
    const std::array<Polyline, 4> female_0 = {
        Polyline({Point(0.64333, -0.78666, -0.01), Point(0.375, -0.25, -0.01), Point(-0.375, -0.25, -0.01), Point(-0.64333, -0.78666, -0.01), Point(0.64333, -0.78666, -0.01)}),
        Polyline({Point(0.64333, -0.78666, 0.01), Point(0.375, -0.25, 0.01), Point(-0.375, -0.25, 0.01), Point(-0.64333, -0.78666, 0.01), Point(0.64333, -0.78666, 0.01)}),
        Polyline({Point(-0.375, 0.075, 0.15), Point(0.375, 0.075, 0.15), Point(0.375, 0.075, -0.15), Point(-0.375, 0.075, -0.15), Point(-0.375, 0.075, 0.15)}),
        Polyline({Point(-0.375, 0.075, 0.15), Point(0.375, 0.075, 0.15), Point(0.375, 0.075, -0.15), Point(-0.375, 0.075, -0.15), Point(-0.375, 0.075, 0.15)}),
    };

    // the female pockets on face 1: the mouths at z +-0.5 and the far side tiles
    const std::array<Polyline, 4> female_1 = {
        Polyline({Point(0.64333, -0.78666, 0.5), Point(0.375, -0.25, 0.5), Point(-0.375, -0.25, 0.5), Point(-0.64333, -0.78666, 0.5), Point(0.64333, -0.78666, 0.5)}),
        Polyline({Point(0.64333, -0.78666, -0.5), Point(0.375, -0.25, -0.5), Point(-0.375, -0.25, -0.5), Point(-0.64333, -0.78666, -0.5), Point(0.64333, -0.78666, -0.5)}),
        Polyline({Point(-0.375, -0.35, 0.15), Point(0.375, -0.35, 0.15), Point(0.375, -0.35, -0.15), Point(-0.375, -0.35, -0.15), Point(-0.375, -0.35, 0.15)}),
        Polyline({Point(-0.375, 0.5, 0.15), Point(0.375, 0.5, 0.15), Point(0.375, 0.5, -0.15), Point(-0.375, 0.5, -0.15), Point(-0.375, 0.5, 0.15)}),
    };

    // the male on face 0: the two wedge flanks, two walls, two caps and two slice planes, each listed twice
    const std::array<Polyline, 8> male_0 = {
        Polyline({Point(-0.59861, -0.69721, 0.5), Point(-0.375, -0.25, 0.5), Point(-0.375, -0.25, -0.5), Point(-0.59861, -0.69721, -0.5), Point(-0.59861, -0.69721, 0.5)}),
        Polyline({Point(0.59861, -0.69721, 0.5), Point(0.375, -0.25, 0.5), Point(0.375, -0.25, -0.5), Point(0.59861, -0.69721, -0.5), Point(0.59861, -0.69721, 0.5)}),
        Polyline({Point(-0.375, 0.7, 0.5), Point(-0.375, 0.7, -0.5), Point(-0.375, -0.25, -0.5), Point(-0.375, -0.25, 0.5), Point(-0.375, 0.7, 0.5)}),
        Polyline({Point(0.375, 0.7, 0.5), Point(0.375, 0.7, -0.5), Point(0.375, -0.25, -0.5), Point(0.375, -0.25, 0.5), Point(0.375, 0.7, 0.5)}),
        Polyline({Point(-0.5, 0.7, 0.15), Point(0.5, 0.7, 0.15), Point(0.5, -0.25, 0.15), Point(-0.5, -0.25, 0.15), Point(-0.5, 0.7, 0.15)}),
        Polyline({Point(-0.5, 0.7, -0.15), Point(0.5, 0.7, -0.15), Point(0.5, -0.25, -0.15), Point(-0.5, -0.25, -0.15), Point(-0.5, 0.7, -0.15)}),
        Polyline({Point(0.05, 0.7, -0.6), Point(0.05, 0.8, -0.6), Point(0.05, 0.8, 0.6), Point(0.05, 0.7, 0.6), Point(0.05, 0.7, -0.6)}),
        Polyline({Point(-0.05, 0.7, -0.6), Point(-0.05, 0.8, -0.6), Point(-0.05, 0.8, 0.6), Point(-0.05, 0.7, 0.6), Point(-0.05, 0.7, -0.6)}),
    };

    // the male on face 1
    const std::array<Polyline, 8> male_1 = {
        Polyline({Point(-0.95638, -0.51833, 0.5), Point(-0.73277, -0.07111, 0.5), Point(-0.73277, -0.07111, -0.5), Point(-0.95638, -0.51833, -0.5), Point(-0.95638, -0.51833, 0.5)}),
        Polyline({Point(0.95638, -0.51833, 0.5), Point(0.73277, -0.07111, 0.5), Point(0.73277, -0.07111, -0.5), Point(0.95638, -0.51833, -0.5), Point(0.95638, -0.51833, 0.5)}),
        Polyline({Point(-0.675, 0.7, 0.5), Point(-0.675, 0.7, -0.5), Point(-0.675, -0.25, -0.5), Point(-0.675, -0.25, 0.5), Point(-0.675, 0.7, 0.5)}),
        Polyline({Point(0.675, 0.7, 0.5), Point(0.675, 0.7, -0.5), Point(0.675, -0.25, -0.5), Point(0.675, -0.25, 0.5), Point(0.675, 0.7, 0.5)}),
        Polyline({Point(-0.5, 0.7, 0.55), Point(0.5, 0.7, 0.55), Point(0.5, -0.25, 0.55), Point(-0.5, -0.25, 0.55), Point(-0.5, 0.7, 0.55)}),
        Polyline({Point(-0.5, 0.7, -0.55), Point(0.5, 0.7, -0.55), Point(0.5, -0.25, -0.55), Point(-0.5, -0.25, -0.55), Point(-0.5, 0.7, -0.55)}),
        Polyline({Point(0.7, 0.7, -0.6), Point(0.7, 0.8, -0.6), Point(0.7, 0.8, 0.6), Point(0.7, 0.7, 0.6), Point(0.7, 0.7, -0.6)}),
        Polyline({Point(-0.7, 0.7, -0.6), Point(-0.7, 0.8, -0.6), Point(-0.7, 0.8, 0.6), Point(-0.7, 0.7, 0.6), Point(-0.7, 0.7, -0.6)}),
    };

    joint.female_outlines[0] = {female_0[0], female_0[0], female_0[1], female_0[1], female_0[2], female_0[2], female_0[3], female_0[3]};
    joint.female_outlines[1] = {female_1[0], female_1[0], female_1[1], female_1[1], female_1[2], female_1[2], female_1[3], female_1[3]};
    joint.male_outlines[0] = {male_0[0], male_0[0], male_0[1], male_0[1], male_0[2], male_0[2], male_0[3], male_0[3], male_0[4], male_0[4], male_0[5], male_0[5], male_0[6], male_0[6], male_0[7], male_0[7]};
    joint.male_outlines[1] = {male_1[0], male_1[0], male_1[1], male_1[1], male_1[2], male_1[2], male_1[3], male_1[3], male_1[4], male_1[4], male_1[5], male_1[5], male_1[6], male_1[6], male_1[7], male_1[7]};

    // the 2024 types: every female outline a mill, the male's first twelve mill_project and the last four slice
    const std::vector<int> female_types(8, FabricationType::mill);
    std::vector<int> male_types(12, FabricationType::mill_project);
    male_types.insert(male_types.end(), 4, FabricationType::slice);
    joint.female_fabrication_types = {female_types, female_types};
    joint.male_fabrication_types = {male_types, male_types};
}
