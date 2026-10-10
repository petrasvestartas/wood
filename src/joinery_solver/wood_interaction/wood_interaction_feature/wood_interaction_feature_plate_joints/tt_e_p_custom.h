/// tt_e_p_custom, id 49: the user's top-top outlines from settings.custom("tt_e_p"), kept pair by pair as 2024 did, oriented to the contact.
static void tt_e_p_custom(InteractionFeaturePlate& joint, const Settings& settings) {

    joint.name = "tt_e_p_custom";
    custom_pairs(joint, settings.custom("tt_e_p")[0], settings.custom("tt_e_p")[1]);
}
