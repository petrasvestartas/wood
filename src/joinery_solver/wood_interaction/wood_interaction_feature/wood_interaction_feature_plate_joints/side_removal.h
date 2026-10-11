/// The outlines 2024's side_removal and side_removal_ss_e_r_1 both begin with: each side face widened at its convex corners by
/// scale[0] and up and down by scale[1], the male's outlines the female's side face pushed scale[2] into the male and, with the shift
/// on under a merge, scale[2] + 2 + shift as well, the female's the male's side face pushed scale[2] + 2 into the female: two
/// mill_project pairs per side, four on the shifted male; no orient. 2024 swapped v0 and v1 and then built m from v0, while the
/// first plate kept its male flag: so the male keeps element_a and reads the neighbour's face, v0 here is element_b. Nothing when a
/// plate or a face is missing, as the 2024 try block left it.
static void side_removal_outlines(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements, bool merge_with_joint) {

    joint.no_orient = true;

    const int v0 = index_of_plate(elements, joint.element_b);
    const int v1 = index_of_plate(elements, joint.element_a);
    const int f0_0 = joint.contact.face_b;
    const int f1_0 = joint.contact.face_a;

    if (v0 < 0 || v1 < 0)
        return;
    if (f0_0 < 0 || f0_0 >= (int)elements[v0]->polylines.size() || f1_0 < 0 || f1_0 >= (int)elements[v1]->polylines.size())
        return;

    // offset vectors
    const Vector n0 = elements[v0]->planes[f0_0].z_axis().normalized();
    const Vector n1 = elements[v1]->planes[f1_0].z_axis().normalized();
    const double s2 = joint.scale[2];
    const Vector f0_0_normal = n0 * s2;
    const Vector f1_0_normal = n1 * (s2 + 2.0);
    const Vector f0_1_normal = n0 * (s2 + 2.0 + joint.shift);

    // the side rectangles, extended only at convex corners and only when both are rectangles
    Polyline pline0 = elements[v0]->polylines[f0_0];
    Polyline pline1 = elements[v1]->polylines[f1_0];

    std::vector<bool> convex_corner0;
    std::vector<bool> convex_corner1;
    elements[v0]->polylines[0].get_convex_corners(convex_corner0);
    elements[v1]->polylines[0].get_convex_corners(convex_corner1);

    if (pline0.point_count() == 5 && pline1.point_count() == 5 && !convex_corner0.empty() && !convex_corner1.empty()) {
        const int n0_corners = (int)convex_corner0.size();
        const int n1_corners = (int)convex_corner1.size();
        const double scale0_0 = convex_corner0[wrap_index(f0_0 - 2, n0_corners)] ? joint.scale[0] : 0.0;
        const double scale0_1 = convex_corner0[wrap_index(f0_0 - 1, n0_corners)] ? joint.scale[0] : 0.0;
        const double scale1_0 = convex_corner1[wrap_index(f1_0 - 2, n1_corners)] ? joint.scale[0] : 0.0;
        const double scale1_1 = convex_corner1[wrap_index(f1_0 - 1, n1_corners)] ? joint.scale[0] : 0.0;

        pline0.extend_segment(0, scale0_0, scale0_1);
        pline0.extend_segment(2, scale0_1, scale0_0);
        pline1.extend_segment(0, scale1_0, scale1_1);
        pline1.extend_segment(2, scale1_1, scale1_0);

        const double vertical = joint.scale[1];
        pline0.extend_segment(1, vertical, vertical);
        pline0.extend_segment(3, vertical, vertical);
        pline1.extend_segment(1, vertical, vertical);
        pline1.extend_segment(3, vertical, vertical);
    }

    // the outlines moved by the vectors
    const Polyline pline0_moved0 = pline0.translated(f0_0_normal);
    const Polyline pline0_moved1 = pline0.translated(f0_1_normal);
    const Polyline pline1_moved = pline1.translated(f1_0_normal);

    const bool shifted = joint.shift > 0.0 && merge_with_joint;
    if (shifted) {
        joint.male_outlines[0] = { pline0_moved0, pline0_moved0, pline0, pline0 };
        joint.male_outlines[1] = { pline0_moved1, pline0_moved1, pline0_moved0, pline0_moved0 };
    } else {
        joint.male_outlines[0] = { pline0, pline0 };
        joint.male_outlines[1] = { pline0_moved0, pline0_moved0 };
    }
    joint.female_outlines[0] = { pline1, pline1 };
    joint.female_outlines[1] = { pline1_moved, pline1_moved };

    const std::vector<int> male_types(shifted ? 4 : 2, FabricationType::mill_project);
    joint.male_fabrication_types[0] = male_types;
    joint.male_fabrication_types[1] = male_types;
    joint.female_fabrication_types[0] = { FabricationType::mill_project, FabricationType::mill_project };
    joint.female_fabrication_types[1] = { FabricationType::mill_project, FabricationType::mill_project };
}

/// side_removal: the side faces of both plates as mill outlines, the male's doubled when the shift is on under a merge.
static void side_removal(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements, bool merge_with_joint = false) {

    joint.name = "side_removal";
    side_removal_outlines(joint, elements, merge_with_joint);
}
