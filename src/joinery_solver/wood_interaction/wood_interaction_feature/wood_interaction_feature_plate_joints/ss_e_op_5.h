/// ss_e_op_5: ss_e_op_4 on this joint and on its one or two linked joints, with the merge sequences that stitch them.
static void ss_e_op_5(FeaturePlate& jo, std::vector<FeaturePlate>& all_joints, bool disable_joint_divisions) {

    jo.name = "ss_e_op_5";

    if (jo.linked_joints.empty() || jo.linked_joints.size() > 2) {
        jo.linked_joints_seq.clear();
        ss_e_op_4(jo, 0.00, true, true, -0.75, 0.5, -0.5, 0.5, -0.5, 0.5);
        return;
    }

    std::vector<int> linked;
    for (const std::string& guid : jo.linked_joints) {
        const int index = index_of(all_joints, guid);
        if (index < 0 || &all_joints[index] == &jo)
            return;
        linked.push_back(index);
    }

    jo.linked_joints_seq.clear();
    ss_e_op_4(jo, 0.00, false, true, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5);

    const int a = linked[0];
    const int b = linked.size() > 1 ? linked[1] : -1;
    all_joints[a].divisions = jo.divisions;
    ss_e_op_4(all_joints[a], 0.5, true, false, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5);

    std::vector<std::array<int, 4>> linked_joints_seq_0;
    linked_joints_seq_0.push_back({2, 4, 2, 8});
    jo.linked_joints_seq.push_back(linked_joints_seq_0);

    if (jo.linked_joints.size() == 2) {
        all_joints[b].divisions = disable_joint_divisions ? 0 : jo.divisions;
        ss_e_op_4(all_joints[b], 0.00, true, false, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5);

        std::vector<std::array<int, 4>> linked_joints_seq_1;
        for (size_t i = 0; i < jo.female_outlines[0].size(); i += 2) {
            if (i == 0)
                linked_joints_seq_1.push_back({
                    1,
                    (int)jo.female_outlines[0][0].point_count() - 2,
                    1,
                    (int)all_joints[b].male_outlines[0][0].point_count() - 2,
                });
            else
                linked_joints_seq_1.push_back({0, 0, 0, 0});
        }

        jo.linked_joints_seq.push_back(linked_joints_seq_1);
    }
}
