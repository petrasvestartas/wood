/// cross_half_lap: the five rings of the half-lap, the bottom sides extended 0.15 to each side, no drill.
static void cross_half_lap(InteractionFeaturePlate& joint) {

    joint.name = "cross_half_lap";
    cross_core(joint, 0.15, 0.15, {}, {});
}
