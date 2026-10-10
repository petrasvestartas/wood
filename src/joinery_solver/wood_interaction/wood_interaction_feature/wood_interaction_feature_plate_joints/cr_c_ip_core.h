/// The body cr_c_ip_2 to cr_c_ip_5 share, as 2024 wrote it four times: the half-lap's sixteen points on the shift, five
/// rings on them (the centre, two top sides, two bottom sides), the bottom sides extended along the plate by
/// `extension_first` on their first segment and `extension_second` on their third, both ends each, and 0.6 up and down
/// so the cut clears the plate, the variant's drill lines after them; face 1 is every ring offset along its normal (a
/// drill stays), the male the female with x and y swapped and z flipped, every outline written twice, and each top side's
/// pair replaced by the two walls between its ring and the offset ring. The cut types in 2024's order: the centre a
/// mill_project, the walls slice_projectsheer, the bottom sides mill_project, then each drill's type twice. 2024's
/// cr_c_ip_4 offset its drill line too, reading past its ring and length arrays; the line keeps where it was declared.
static void cr_c_ip_core(
    InteractionFeaturePlate& joint,
    double extension_first,
    double extension_second,
    const std::vector<Polyline>& drills,
    const std::vector<int>& drill_types
) {

    // the half-lap on the shift: the centre square of half side a, its offset by c, the top and the bottom squares
    double s = std::max(std::min(joint.shift, 1.0), 0.0);
    s = 0.05 + (s - 0.0) * (0.4 - 0.05) / (1.0 - 0.0);
    const double a = 0.5 - s;
    const double b = 0.5;
    const double c = 2.0 * (b - a);
    const double z = 0.5;
    const Point p[16] = {
        Point(a, -a, 0), Point(-a, -a, 0), Point(-a, a, 0), Point(a, a, 0), // center
        Point(a + c, -a - c, 0), Point(-a - c, -a - c, 0), Point(-a - c, a + c, 0), Point(a + c, a + c, 0), // center offset
        Point(b, -b, z), Point(-b, -b, z), Point(-b, b, z), Point(b, b, z), // top
        Point(b, -b, -z), Point(-b, -b, -z), Point(-b, b, -z), Point(b, b, -z), // bottom
    };
    const Vector v0 = ((p[0] - p[1]) * (1.0 / (a * 2.0))) * (0.5 - a);

    // the five rings, then the drill lines
    std::vector<Polyline> rings = {
        Polyline({p[0] + v0, p[1] - v0, p[2] - v0, p[3] + v0, p[0] + v0}), // center
        Polyline({p[1] - v0, p[0] + v0, p[8] + v0, p[9] - v0, p[1] - v0}), // top side 0
        Polyline({p[3] + v0, p[2] - v0, p[10] - v0, p[11] + v0, p[3] + v0}), // top side 1
        Polyline({p[2], p[1], p[13], p[14], p[2]}), // bottom side 0
        Polyline({p[0], p[3], p[15], p[12], p[0]}), // bottom side 1
    };

    // the bottom sides extended to both sides, then vertically, to compensate for irregularities
    for (size_t i = 3; i < 5; i++) {
        rings[i].extend_segment_equally(0, extension_first);
        rings[i].extend_segment_equally(2, extension_second);
    }
    for (size_t i = 3; i < 5; i++) {
        rings[i].extend_segment_equally(1, 0.6);
        rings[i].extend_segment_equally(3, 0.6);
    }
    rings.insert(rings.end(), drills.begin(), drills.end());

    // face 1 offset along each ring's normal, the male rotated into the other plate
    const double lengths[5] = {0.5, 0.4, 0.4, 0.4, 0.4};
    const Xform rotation = Xform::from_axes(Vector(0, 1, 0), Vector(1, 0, 0), Vector(0, 0, -1));
    const size_t n = rings.size();
    std::vector<Polyline> f0 = rings;
    std::vector<Polyline> f1 = rings;
    std::vector<Polyline> m0(n);
    std::vector<Polyline> m1(n);
    for (size_t i = 0; i < n; i++) {
        if (i < 5) {
            Vector normal = (f1[i][1] - f1[i][0]).cross(f1[i][1] - f1[i][2]);
            normal.normalize_self();
            for (size_t j = 0; j < f1[i].point_count(); j++)
                f1[i].set_point(j, f1[i][j] + normal * lengths[i]);
        }
        m0[i] = f0[i].transformed(rotation);
        m1[i] = f1[i].transformed(rotation);
    }

    // every outline twice
    joint.female_outlines[0].clear();
    joint.female_outlines[1].clear();
    joint.male_outlines[0].clear();
    joint.male_outlines[1].clear();
    for (size_t i = 0; i < n; i++) {
        joint.female_outlines[0].insert(joint.female_outlines[0].end(), {f0[i], f0[i]});
        joint.female_outlines[1].insert(joint.female_outlines[1].end(), {f1[i], f1[i]});
        joint.male_outlines[0].insert(joint.male_outlines[0].end(), {m0[i], m0[i]});
        joint.male_outlines[1].insert(joint.male_outlines[1].end(), {m1[i], m1[i]});
    }

    // the top sides, at the doubled ids 2 and 4, become the two walls between their ring and its offset
    for (size_t id = 2; id <= 4; id += 2) {
        std::vector<Polyline>& fo0 = joint.female_outlines[0];
        std::vector<Polyline>& fo1 = joint.female_outlines[1];
        const Polyline female_wall_0({fo0[id][0], fo0[id][1], fo1[id][1], fo1[id][0], fo0[id][0]});
        const Polyline female_wall_1({fo0[id][3], fo0[id][2], fo1[id][2], fo1[id][3], fo0[id][3]});
        fo0[id] = female_wall_0;
        fo1[id] = female_wall_1;
        fo0[id + 1] = female_wall_0;
        fo1[id + 1] = female_wall_1;

        std::vector<Polyline>& mo0 = joint.male_outlines[0];
        std::vector<Polyline>& mo1 = joint.male_outlines[1];
        const Polyline male_wall_0({mo0[id][0], mo0[id][1], mo1[id][1], mo1[id][0], mo0[id][0]});
        const Polyline male_wall_1({mo0[id][3], mo0[id][2], mo1[id][2], mo1[id][3], mo0[id][3]});
        mo0[id] = male_wall_0;
        mo1[id] = male_wall_1;
        mo0[id + 1] = male_wall_0;
        mo1[id + 1] = male_wall_1;
    }

    // the cut types
    std::vector<int> types = {
        FabricationType::mill_project, FabricationType::mill_project,
        FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer, FabricationType::slice_projectsheer,
        FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project, FabricationType::mill_project,
    };
    for (const int type : drill_types)
        types.insert(types.end(), {type, type});
    for (int face = 0; face < 2; face++) {
        joint.female_fabrication_types[face] = types;
        joint.male_fabrication_types[face] = types;
    }
}
