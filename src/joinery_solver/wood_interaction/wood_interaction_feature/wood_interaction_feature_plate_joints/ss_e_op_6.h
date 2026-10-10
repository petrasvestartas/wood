/// ss_e_op_6: ss_e_op_5 with the divisions of its second link disabled, the Vidy wall version that merges tenons with one side only.
static void ss_e_op_6(InteractionFeaturePlate& jo, std::vector<InteractionFeaturePlate>& all_joints) {

    ss_e_op_5(jo, all_joints, true);
    jo.name = "ss_e_op_6";
}
