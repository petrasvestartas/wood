/// corner_linked_tenons_wall: corner_linked_tenons with the divisions of its second link disabled, the Vidy wall version that merges tenons with one side only.
static void corner_linked_tenons_wall(InteractionFeaturePlate& jo, std::vector<InteractionFeaturePlate>& all_joints) {

    corner_linked_tenons(jo, all_joints, true);
    jo.name = "corner_linked_tenons_wall";
}
