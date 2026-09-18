/// ss_e_ip_4: hardcoded mill-and-drill in-plane joint, eight outlines per face (four mill_project, four drill).
static void ss_e_ip_4(WoodJoint& joint) {

    joint.name = "ss_e_ip_4";

    joint.female_outlines[0] = {
        Polyline({Point(-1.25,-0.5,0), Point(1,-0.5,0), Point(1,-0.2,0), Point(-1,0.2,0), Point(-1,0.5,0), Point(-1.25,0.5,0), Point(-1.25,-0.5,0)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(-1.25,0.5,0), Point(1,0.5,0), Point(1,0.2,0), Point(-1,-0.2,0), Point(-1,-0.5,0), Point(-1.25,-0.5,0), Point(-1.25,0.5,0)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
    };
    joint.female_outlines[1] = {
        Polyline({Point(-1.25,-0.5,-0.5), Point(1,-0.5,-0.5), Point(1,-0.2,-0.5), Point(-1,0.2,-0.5), Point(-1,0.5,-0.5), Point(-1.25,0.5,-0.5), Point(-1.25,-0.5,-0.5)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(-1.25,0.5,0.5), Point(1,0.5,0.5), Point(1,0.2,0.5), Point(-1,-0.2,0.5), Point(-1,-0.5,0.5), Point(-1.25,-0.5,0.5), Point(-1.25,0.5,0.5)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
    };

    joint.male_outlines[0] = {
        Polyline({Point(1.25,0.5,0), Point(-1,0.5,0), Point(-1,0.2,0), Point(1,-0.2,0), Point(1,-0.5,0), Point(1.25,-0.5,0), Point(1.25,0.5,0)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(1.25,-0.5,0), Point(-1,-0.5,0), Point(-1,-0.2,0), Point(1,0.2,0), Point(1,0.5,0), Point(1.25,0.5,0), Point(1.25,-0.5,0)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
    };
    joint.male_outlines[1] = {
        Polyline({Point(1.25,0.5,-0.5), Point(-1,0.5,-0.5), Point(-1,0.2,-0.5), Point(1,-0.2,-0.5), Point(1,-0.5,-0.5), Point(1.25,-0.5,-0.5), Point(1.25,0.5,-0.5)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(1.25,-0.5,0.5), Point(-1,-0.5,0.5), Point(-1,-0.2,0.5), Point(1,0.2,0.5), Point(1,0.5,0.5), Point(1.25,0.5,0.5), Point(1.25,-0.5,0.5)}),
        Polyline({Point(0,-0.5,0.5), Point(0,-0.5,-0.5)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(-0.333333,-0.6,0), Point(-0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
        Polyline({Point(0.333333,-0.6,0), Point(0.333333,0.6,0)}),
    };

    for (int face = 0; face < 2; face++) {
        joint.female_cut_types[face] = { CutType::mill_project, CutType::mill_project, CutType::mill_project, CutType::mill_project, CutType::drill, CutType::drill, CutType::drill, CutType::drill };
        joint.male_cut_types[face] = { CutType::mill_project, CutType::mill_project, CutType::mill_project, CutType::mill_project, CutType::drill, CutType::drill, CutType::drill, CutType::drill };
    }
}
