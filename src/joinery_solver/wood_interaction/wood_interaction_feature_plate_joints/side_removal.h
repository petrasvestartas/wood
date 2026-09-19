/// Four side-face rectangles, widened at convex corners and pushed along the face normals; no orient.
static void side_removal_ss_e_r_1_port(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements) {

    joint.name = "side_removal";
    joint.no_orient = true;

    std::swap(joint.element_a, joint.element_b);
    std::swap(joint.contact.face_a, joint.contact.face_b);
    std::swap(joint.cross_faces[0], joint.cross_faces[1]);
    std::swap(joint.joint_lines[0], joint.joint_lines[1]);

    const int v0 = index_of(elements, joint.element_a);
    const int v1 = index_of(elements, joint.element_b);
    const int f0_0 = joint.contact.face_a;
    const int f1_0 = joint.contact.face_b;

    if (v0 < 0 || v0 >= (int)elements.size() || v1 < 0 || v1 >= (int)elements.size()) {
        ss_e_r_0(joint);
        return;
    }
    if (f0_0 < 0 || f0_0 >= (int)elements[v0]->planes.size() ||
        f1_0 < 0 || f1_0 >= (int)elements[v1]->planes.size() ||
        f0_0 >= (int)elements[v0]->polylines.size() ||
        f1_0 >= (int)elements[v1]->polylines.size()) {
        ss_e_r_0(joint);
        return;
    }

    Vector n0 = elements[v0]->planes[f0_0].z_axis();
    n0.normalize_self();
    Vector n1 = elements[v1]->planes[f1_0].z_axis();
    n1.normalize_self();

    const double s2 = joint.scale[2];
    const Vector f0_0_normal = n0 * s2;
    const Vector f1_0_normal = n1 * (s2 + 2.0);
    const Vector f0_1_normal = n0 * (s2 + 2.0 + joint.shift);

    Polyline pline0 = elements[v0]->polylines[f0_0];
    Polyline pline1 = elements[v1]->polylines[f1_0];

    if (pline0.point_count() == 5 && pline1.point_count() == 5) {
        std::vector<bool> cc0;
        std::vector<bool> cc1;
        elements[v0]->polylines[0].get_convex_corners(cc0);
        elements[v1]->polylines[0].get_convex_corners(cc1);
        const double sc0 = joint.scale[0];

        if (!cc0.empty()) {
            const int a_idx = f0_0 - 2;
            const int b_idx = (a_idx + 1) % (int)cc0.size();
            const double sc0_0 = (a_idx >= 0 && a_idx < (int)cc0.size() && cc0[a_idx]) ? sc0 : 0.0;
            const double sc0_1 = (b_idx >= 0 && b_idx < (int)cc0.size() && cc0[b_idx]) ? sc0 : 0.0;
            pline0.extend_segment(0, sc0_0, sc0_1);
            pline0.extend_segment(2, sc0_1, sc0_0);
        }

        if (!cc1.empty()) {
            const int a_idx = f1_0 - 2;
            const int b_idx = (a_idx + 1) % (int)cc1.size();
            const double sc1_0 = (a_idx >= 0 && a_idx < (int)cc1.size() && cc1[a_idx]) ? sc0 : 0.0;
            const double sc1_1 = (b_idx >= 0 && b_idx < (int)cc1.size() && cc1[b_idx]) ? sc0 : 0.0;
            pline1.extend_segment(0, sc1_0, sc1_1);
            pline1.extend_segment(2, sc1_1, sc1_0);
        }

        const double sv = joint.scale[1];
        pline0.extend_segment(1, sv, sv);
        pline0.extend_segment(3, sv, sv);
        pline1.extend_segment(1, sv, sv);
        pline1.extend_segment(3, sv, sv);
    }

    const Polyline pline0_moved0 = pline0.translated(f0_0_normal);
    const Polyline pline0_moved1 = pline0.translated(f0_1_normal);
    const Polyline pline1_moved  = pline1.translated(f1_0_normal);

    if (!(joint.shift > 0.0)) {
        joint.male_outlines[0] = { pline0,        pline0 };
        joint.male_outlines[1] = { pline0_moved0, pline0_moved0 };
        joint.female_outlines[0] = { pline1,        pline1 };
        joint.female_outlines[1] = { pline1_moved,  pline1_moved };
        joint.male_cut_types[0] = { CutType::mill_project, CutType::mill_project };
        joint.male_cut_types[1] = { CutType::mill_project, CutType::mill_project };
        joint.female_cut_types[0] = { CutType::mill_project, CutType::mill_project };
        joint.female_cut_types[1] = { CutType::mill_project, CutType::mill_project };
        return;
    }

    joint.male_outlines[0] = { pline0_moved0, pline0_moved0, pline0, pline0 };
    joint.male_outlines[1] = { pline0_moved1, pline0_moved1, pline0_moved0, pline0_moved0 };
    joint.female_outlines[0] = { pline1,        pline1 };
    joint.female_outlines[1] = { pline1_moved,  pline1_moved };
    joint.male_cut_types[0] = { CutType::mill_project, CutType::mill_project,
                             CutType::mill_project, CutType::mill_project };
    joint.male_cut_types[1] = { CutType::mill_project, CutType::mill_project,
                             CutType::mill_project, CutType::mill_project };
    joint.female_cut_types[0] = { CutType::mill_project, CutType::mill_project };
    joint.female_cut_types[1] = { CutType::mill_project, CutType::mill_project };
}

/// side_removal_ss_e_r_1_port with the merge branch forced off unless merge_with_joint.
static void side_removal(WoodJoint& joint, const std::vector<std::shared_ptr<Plate>>& elements, bool merge_with_joint = false) {

    const double saved_shift = joint.shift;
    if (!merge_with_joint)
        joint.shift = 0.0;

    side_removal_ss_e_r_1_port(joint, elements);

    if (!merge_with_joint)
        joint.shift = saved_shift;
}

