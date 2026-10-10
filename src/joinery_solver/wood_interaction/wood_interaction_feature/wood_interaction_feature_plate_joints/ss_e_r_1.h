/// A yz profile placed in the plane x = constant.
static Polyline yz_profile_at_x(double x, const double data[][2], size_t n) {

    std::vector<Point> pts;
    pts.reserve(n);
    for (size_t i = 0; i < n; ++i)
        pts.emplace_back(x, data[i][0], data[i][1]);

    return Polyline(pts);
}

/// ss_e_r_1: the miter tenon-mortise tile of side_removal_ss_e_r_1, a profile and its box marker in the yz-plane, the female
/// at x = 0.5 and 0, the male at x = 0 and 0.5; type 1 the 15-point arc that side_removal_ss_e_r_1 tiles, any other type the
/// 39-point arc; conic cuts, no unit scale of its own, as 2024: the caller that tiles it turns the unit scale on. The male and
/// female profiles coincide, so alone on a pair it is no joint: 2024 gave it no id, and the library does not expose it.
static void ss_e_r_1(InteractionFeaturePlate& joint, int type = 1) {

    joint.name = "ss_e_r_1";

    static const double yz_1[][2] = {
        {-0.625, -0.2        },
        {-0.625,  0.2        },
        { 0.125798405, 0.062633245 },
        { 0.141579173, 0.057404662 },
        { 0.155319467, 0.04804651  },
        { 0.16596445,  0.035277208 },
        { 0.172696911, 0.020077054 },
        { 0.175,       0.003612957 },
        { 0.175,      -0.003612957 },
        { 0.172696911,-0.020077054 },
        { 0.16596445, -0.035277208 },
        { 0.155319467,-0.04804651  },
        { 0.141579173,-0.057404662 },
        { 0.125798405,-0.062633245 },
        {-0.625,      -0.2         },
    };
    static const double yz_marker_1[][2] = {
        {-0.625, -0.2 },
        {-0.625,  0.2 },
        { 0.175,  0.2 },
        { 0.175, -0.2 },
        {-0.625, -0.2 },
    };

    static const double yz[][2] = {
        {-0.825,  0.0         },
        {-0.825, -0.151041813 },
        {-0.825, -0.302083626 },
        {-0.825, -0.39066965  },
        {-0.764910275, -0.37364172  },
        {-0.619590501, -0.332461718 },
        {-0.474270727, -0.291281717 },
        {-0.328950953, -0.250101715 },
        {-0.183631179, -0.208921714 },
        {-0.038311405, -0.167741712 },
        { 0.078145959, -0.134740598 },
        { 0.097349939, -0.129031066 },
        { 0.106158874, -0.124374202 },
        { 0.11914747,  -0.117507751 },
        { 0.138265279, -0.101937742 },
        { 0.153962352, -0.082924124 },
        { 0.165630347, -0.061203771 },
        { 0.172817069, -0.037618462 },
        { 0.175,       -0.01554903  },
        { 0.175,       -3.4e-08     },
        { 0.175,        0.01554903  },
        { 0.172817069,  0.037618462 },
        { 0.165630347,  0.061203771 },
        { 0.153962352,  0.082924124 },
        { 0.138265279,  0.101937742 },
        { 0.11914747,   0.117507751 },
        { 0.106158839,  0.12437399  },
        { 0.09734984,   0.129030732 },
        { 0.078145959,  0.134740598 },
        {-0.038311405,  0.167741712 },
        {-0.183631179,  0.208921714 },
        {-0.328950953,  0.250101715 },
        {-0.474270727,  0.291281717 },
        {-0.619590501,  0.332461718 },
        {-0.764910275,  0.37364172  },
        {-0.825,        0.39066965  },
        {-0.825,        0.302083626 },
        {-0.825,        0.151041813 },
        {-0.825,        0.0         },
    };
    static const double yz_marker[][2] = {
        {-0.825,  0.39066965 },
        { 0.175,  0.39066965 },
        { 0.175, -0.39066965 },
        {-0.825, -0.39066965 },
        {-0.825,  0.39066965 },
    };

    const bool arc_15 = type == 1;
    const double (*profile)[2] = arc_15 ? yz_1 : yz;
    const double (*marker)[2] = arc_15 ? yz_marker_1 : yz_marker;
    const size_t count = arc_15 ? 15 : 39;

    joint.female_outlines[0] = { yz_profile_at_x(0.5, profile, count), yz_profile_at_x(0.5, marker, 5) };
    joint.female_outlines[1] = { yz_profile_at_x(0.0, profile, count), yz_profile_at_x(0.0, marker, 5) };

    joint.male_outlines[0] = { yz_profile_at_x(0.0, profile, count), yz_profile_at_x(0.0, marker, 5) };
    joint.male_outlines[1] = { yz_profile_at_x(0.5, profile, count), yz_profile_at_x(0.5, marker, 5) };

    joint.female_fabrication_types[0] = { FabricationType::conic, FabricationType::conic };
    joint.female_fabrication_types[1] = { FabricationType::conic, FabricationType::conic };
    joint.male_fabrication_types[0] = { FabricationType::conic_reverse, FabricationType::conic_reverse };
    joint.male_fabrication_types[1] = { FabricationType::conic_reverse, FabricationType::conic_reverse };
}
