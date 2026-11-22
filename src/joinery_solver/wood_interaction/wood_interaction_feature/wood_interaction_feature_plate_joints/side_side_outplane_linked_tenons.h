static void side_side_outplane_linked_tenons(
    InteractionFeaturePlate& jo,
    std::vector<InteractionFeaturePlate>& all_joints,
    bool disable_joint_divisions
) {

    jo.name = "side_side_outplane_linked_tenons";

    if (jo.linked_joints.empty() || jo.linked_joints.size() > 2) {
        jo.linked_joints_seq.clear();
        plate_from_library("side_side_outplane_tenons", jo, {}, {{"t", 0.00}, {"chamfer", 1.0}, {"modify", 1.0}, {"x0", -0.75}, {"x1", 0.5}, {"y0", -0.5}, {"y1", 0.5}, {"z0e", -0.5}, {"z1e", 0.5}});
        return;
    }

    std::vector<int> linked;
    for (const std::string& guid : jo.linked_joints) {
        const int index = index_of_joint(all_joints, guid);
        if (index < 0 || &all_joints[index] == &jo)
            return;
        linked.push_back(index);
    }

    jo.linked_joints_seq.clear();
    plate_from_library("side_side_outplane_tenons", jo, {}, {{"t", 0.00}, {"chamfer", 0.0}, {"modify", 1.0}, {"x0", -0.5}, {"x1", 0.5}, {"y0", -0.5}, {"y1", 0.5}, {"z0e", -0.5}, {"z1e", 0.5}});

    const int a = linked[0];
    const int b = linked.size() > 1 ? linked[1] : -1;
    all_joints[a].divisions = jo.divisions;
    plate_from_library("side_side_outplane_tenons", all_joints[a], {}, {{"t", 0.5}, {"chamfer", 1.0}, {"modify", 0.0}, {"x0", -0.5}, {"x1", 0.5}, {"y0", -0.5}, {"y1", 0.5}, {"z0e", -0.5}, {"z1e", 0.5}});

    std::vector<std::array<int, 4>> linked_joints_seq_0;
    linked_joints_seq_0.push_back({2, 4, 2, 8});
    jo.linked_joints_seq.push_back(linked_joints_seq_0);

    if (jo.linked_joints.size() == 2) {
        all_joints[b].divisions = disable_joint_divisions ? 0 : jo.divisions;
        plate_from_library("side_side_outplane_tenons", all_joints[b], {}, {{"t", 0.00}, {"chamfer", 1.0}, {"modify", 0.0}, {"x0", -0.5}, {"x1", 0.5}, {"y0", -0.5}, {"y1", 0.5}, {"z0e", -0.5}, {"z1e", 0.5}});

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
