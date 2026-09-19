/// cr_c_ip_0: cross-joint stub, two identical closed rectangles per face.
static void cr_c_ip_0(FeaturePlate& joint) {

    joint.name = "cr_c_ip_0";
    const double s = 1.0;

    joint.female_outlines[0] = {
        Polyline({Point(-0.5,0.5,s), Point(-0.5,-0.5,s), Point(-0.5,-0.5,0), Point(-0.5,0.5,0), Point(-0.5,0.5,s)}),
        Polyline({Point(-0.5,0.5,s), Point(-0.5,-0.5,s), Point(-0.5,-0.5,0), Point(-0.5,0.5,0), Point(-0.5,0.5,s)}),
    };
    joint.female_outlines[1] = {
        Polyline({Point(0.5,0.5,s), Point(0.5,-0.5,s), Point(0.5,-0.5,0), Point(0.5,0.5,0), Point(0.5,0.5,s)}),
        Polyline({Point(0.5,0.5,s), Point(0.5,-0.5,s), Point(0.5,-0.5,0), Point(0.5,0.5,0), Point(0.5,0.5,s)}),
    };

    joint.male_outlines[0] = {
        Polyline({Point(0.5,0.5,-s), Point(-0.5,0.5,-s), Point(-0.5,0.5,0), Point(0.5,0.5,0), Point(0.5,0.5,-s)}),
        Polyline({Point(0.5,0.5,-s), Point(-0.5,0.5,-s), Point(-0.5,0.5,0), Point(0.5,0.5,0), Point(0.5,0.5,-s)}),
    };
    joint.male_outlines[1] = {
        Polyline({Point(0.5,-0.5,-s), Point(-0.5,-0.5,-s), Point(-0.5,-0.5,0), Point(0.5,-0.5,0), Point(0.5,-0.5,-s)}),
        Polyline({Point(0.5,-0.5,-s), Point(-0.5,-0.5,-s), Point(-0.5,-0.5,0), Point(0.5,-0.5,0), Point(0.5,-0.5,-s)}),
    };

    for (int face = 0; face < 2; face++) {
        joint.female_fabrication_types[face] = { FabricationType::insert_between_multiple_edges, FabricationType::insert_between_multiple_edges };
        joint.male_fabrication_types[face] = { FabricationType::insert_between_multiple_edges, FabricationType::insert_between_multiple_edges };
    }
}
