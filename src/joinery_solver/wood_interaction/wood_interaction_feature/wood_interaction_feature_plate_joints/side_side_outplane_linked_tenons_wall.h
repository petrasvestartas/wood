/// side_side_outplane_linked_tenons_wall: side_side_outplane_linked_tenons with the divisions of its second link disabled, the Vidy wall version that merges tenons with one side only.
static void side_side_outplane_linked_tenons_wall(InteractionFeaturePlate& jo, std::vector<InteractionFeaturePlate>& all_joints) {

    side_side_outplane_linked_tenons(jo, all_joints, true);
    jo.name = "side_side_outplane_linked_tenons_wall";
}
