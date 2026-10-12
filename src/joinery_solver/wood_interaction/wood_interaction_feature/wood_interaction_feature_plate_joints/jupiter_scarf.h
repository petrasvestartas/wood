/// jupiter_scarf: the trait de Jupiter, a scarf through the thickness with x across the seam, z along the joint line and the
/// faces at y = -0.5 and 0.5, x in the unit of the thickness and z of the joint line: from one end of the joint line an about, a slope
/// six thicknesses long to the middle redan, a slope and the other about, each side notched around the key slot at the redan; the third
/// pair is the key, which compute_key draws and no plate cuts.
static void jupiter_scarf(InteractionFeaturePlate& joint) {

    joint.name = "jupiter_scarf";

    // the line from the first about to the redan, around the slot, away from the male (x > 0) or from the female, and on to the last about
    const std::array<double, 2> notch = {-0.6, 0.6};
    std::array<Polyline, 2> lines;
    for (int side = 0; side < 2; side++)
        lines[side] = Polyline({
            Point(3.0, -0.5, 0.5),
            Point(3.0, -0.5, 0.4),
            Point(0.0, -0.5, 0.08),
            Point(0.0, -0.5, 0.05),
            Point(notch[side], -0.5, 0.05),
            Point(notch[side], -0.5, -0.05),
            Point(0.0, -0.5, -0.05),
            Point(0.0, -0.5, -0.08),
            Point(-3.0, -0.5, -0.4),
            Point(-3.0, -0.5, -0.5),
        });
    const Polyline key({Point(-0.6, -0.5, 0.05), Point(0.6, -0.5, 0.05), Point(0.6, -0.5, -0.05), Point(-0.6, -0.5, -0.05), Point(-0.6, -0.5, 0.05)});

    // each face: the line, its endpoints and the key, the top face the bottom one moved through the thickness
    const Xform through = Xform::translation(0.0, 1.0, 0.0);
    for (int face = 0; face < 2; face++) {
        const Xform move = face == 0 ? Xform() : through;
        const Polyline male = lines[0].transformed(move);
        const Polyline female = lines[1].transformed(move);
        joint.male_outlines[face] = {male, Polyline({male.get_point(0), male.get_point(male.point_count() - 1)}), key.transformed(move)};
        joint.female_outlines[face] = {female, Polyline({female.get_point(0), female.get_point(female.point_count() - 1)}), key.transformed(move)};
        joint.male_fabrication_types[face] = {FabricationType::edge_insertion, FabricationType::edge_insertion, FabricationType::nothing};
        joint.female_fabrication_types[face] = {FabricationType::edge_insertion, FabricationType::edge_insertion, FabricationType::nothing};
    }
}
