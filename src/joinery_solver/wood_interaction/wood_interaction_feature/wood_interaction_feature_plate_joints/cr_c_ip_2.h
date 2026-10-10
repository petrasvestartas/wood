/// cr_c_ip_2: the five rings of the half-lap, the bottom sides extended 0.15 to each side, no drill.
static void cr_c_ip_2(InteractionFeaturePlate& joint) {

    joint.name = "cr_c_ip_2";
    cr_c_ip_core(joint, 0.15, 0.15, {}, {});
}
