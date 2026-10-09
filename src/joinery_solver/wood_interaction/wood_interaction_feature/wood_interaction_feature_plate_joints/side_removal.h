/// The outlines 2024's side_removal and side_removal_ss_e_r_1 both begin with: the male and female swapped, each side face
/// widened at its convex corners by scale[0] and up and down by scale[1], the female's pushed scale[2] + 2 along its normal, the
/// male's scale[2] and, with the shift on under a merge, scale[2] + 2 + shift as well: two mill_project pairs per side, four on the
/// shifted male; no orient. Nothing when a plate or a face is missing, as the 2024 try block left it.
static void side_removal_outlines(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements, bool merge_with_joint) {

    joint.no_orient = true;

    std::swap(joint.element_a, joint.element_b);
    std::swap(joint.contact.face_a, joint.contact.face_b);
    std::swap(joint.cross_faces[0], joint.cross_faces[1]);
    std::swap(joint.joint_lines[0], joint.joint_lines[1]);

    const int v0 = index_of_plate(elements, joint.element_a);
    const int v1 = index_of_plate(elements, joint.element_b);
    const int f0_0 = joint.contact.face_a;
    const int f1_0 = joint.contact.face_b;

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

/// The two 20 x 20 rectangles the ss_e_r_1 tile is oriented on, 10 either way along the averaged joint line from its midpoint:
/// their x axis across the plates' mean half thickness, the end farther from the male side's first edge first, their z the male
/// side normal scaled by scale[2] over the 10, as 2024 placed them.
static std::array<Polyline, 2> side_removal_tile_rectangles(const InteractionFeaturePlate& joint, const Plate& male, const Plate& female, const Polyline& side, const Vector& n0) {

    const double half_dist = 10.0;
    const Vector z_axis = n0.normalized() * (joint.scale[2] / half_dist);

    Line average_line = joint.joint_lines[0];
    joint.joint_lines[0].overlap_average(joint.joint_lines[1], average_line);

    const double half_thickness = (male.thickness + female.thickness) / 4.0;
    const Vector x_axis = z_axis.cross(average_line.to_vector()).normalized();
    const Point middle = Point::mid_point(average_line.start(), average_line.end());
    Point p0 = middle + x_axis * half_thickness;
    Point p1 = middle - x_axis * half_thickness;

    const Point edge_middle = Point::mid_point(side.get_point(0), side.get_point(1));
    if (Point::distance(edge_middle, p0) < Point::distance(edge_middle, p1))
        std::swap(p0, p1);

    const Vector y_axis = average_line.to_vector().normalized();
    std::array<Polyline, 2> rectangles;
    for (int k = 0; k < 2; k++) {
        const Vector along = y_axis * (half_dist * (k == 0 ? -1.0 : 1.0));
        const Vector up = z_axis * half_dist;
        rectangles[k] = Polyline({
            p0 + along - up,
            p0 + along + up,
            p1 + along + up,
            p1 + along - up,
            p0 + along - up,
        });
    }

    return rectangles;
}

/// side_removal_ss_e_r_1: side_removal and, under merge_with_joint, the ss_e_r_1 tile oriented on two 20 x 20 rectangles at
/// the joint line's middle with the unit scale on, its male outlines offset in the male side face by the 2024 conic allowance
/// (0.8440 less the 15 degree draft over the distance between the two moved outlines, added back), the third male outline cut
/// by the tile's first on each face, the tile's male outlines appended as conic, its female outlines appended twice, once as
/// mill and once as conic_reverse, and the male and female lists swapped along with the second fabrication type of each.
static void side_removal_ss_e_r_1(InteractionFeaturePlate& joint, const std::vector<std::shared_ptr<Plate>>& elements, bool merge_with_joint = false) {

    joint.name = "side_removal_ss_e_r_1";
    side_removal_outlines(joint, elements, merge_with_joint);

    if (!merge_with_joint)
        return;

    const int v0 = index_of_plate(elements, joint.element_a);
    const int v1 = index_of_plate(elements, joint.element_b);
    if (v0 < 0 || v1 < 0 || joint.male_outlines[0].size() < 3)
        return;

    // the tile oriented between the two rectangles
    const Vector n0 = elements[v0]->planes[joint.contact.face_a].z_axis();
    const std::array<Polyline, 2> rectangles = side_removal_tile_rectangles(joint, *elements[v0], *elements[v1], joint.male_outlines[0][2], n0);
    InteractionFeaturePlate tile;
    ss_e_r_1(tile);
    tile.unit_scale = true;
    tile.joint_volumes = {rectangles[0], rectangles[1], rectangles[0], rectangles[1]};
    joint_orient_to_connection_area(tile);

    // the conic allowance in the male side face
    const Plane plane_0_0 = Plane::from_point_normal(joint.male_outlines[0][0].get_point(0), n0);
    const Plane plane_0_1 = Plane::from_point_normal(joint.male_outlines[0][2].get_point(0), n0);
    const double dist_two_outlines = std::abs(plane_0_1.signed_distance(joint.male_outlines[0][0].get_point(0)));
    const double conic_offset = -triangle_edge_by_angle(dist_two_outlines, 15.0);
    const double conic_offset_opposite = -(0.8440 + conic_offset);
    const double offset_value = -(1.0 * conic_offset_opposite) - conic_offset;

    for (int face = 0; face < 2; face++)
        for (Polyline& outline : tile.male_outlines[face])
            Intersection::offset_in_3d(outline, plane_0_0, offset_value);

    // the male's third outline less the tile's first, the tile's outlines appended
    Polyline cut_bottom;
    if (Intersection::polyline_boolean_2d_in_plane(joint.male_outlines[0][2], tile.male_outlines[0][0], plane_0_0, cut_bottom, 2))
        joint.male_outlines[0][2] = cut_bottom;
    Polyline cut_top;
    if (Intersection::polyline_boolean_2d_in_plane(joint.male_outlines[1][2], tile.male_outlines[1][0], plane_0_1, cut_top, 2))
        joint.male_outlines[1][2] = cut_top;

    std::vector<int> male_types = joint.male_fabrication_types[0];
    std::vector<int> female_types = joint.female_fabrication_types[0];
    for (int face = 0; face < 2; face++) {
        joint.male_outlines[face].insert(joint.male_outlines[face].end(), tile.male_outlines[face].begin(), tile.male_outlines[face].end());
        joint.female_outlines[face].insert(joint.female_outlines[face].end(), tile.female_outlines[face].begin(), tile.female_outlines[face].end());
        joint.female_outlines[face].insert(joint.female_outlines[face].end(), tile.female_outlines[face].begin(), tile.female_outlines[face].end());
    }
    male_types.insert(male_types.end(), tile.male_outlines[0].size(), FabricationType::conic);
    female_types.insert(female_types.end(), tile.female_outlines[0].size(), FabricationType::mill);
    female_types.insert(female_types.end(), tile.female_outlines[0].size(), FabricationType::conic_reverse);

    // the sides swapped, the types staying but for their second entry, as 2024 swapped them
    std::swap(joint.male_outlines, joint.female_outlines);
    std::swap(male_types[1], female_types[1]);
    joint.male_fabrication_types = {male_types, male_types};
    joint.female_fabrication_types = {female_types, female_types};
}
