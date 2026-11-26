#include "src/templates/shells/translation_shell.h"

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
TranslationShell::TranslationShell(const TranslationShellGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // curves: the cross section and the profile it is swept along
    const std::shared_ptr<TreeNode> curves = add_group("curves");
    add_polyline(guide.cross_section, curves);
    add_polyline(guide.profile, curves);

    // sections: the cross section moved to every profile point
    const std::vector<Polyline> sections = compute_sections();
    const std::shared_ptr<TreeNode> sections_group = add_group("sections");

    for (const Polyline& section : sections)
        add_polyline(section, sections_group);

    // mesh: a quad between every two neighbouring sections
    const Mesh mesh = compute_mesh(sections);
    add_mesh(mesh, add_group("mesh"));

    // plates: one mitered plate per quad, thickness deep, its sharp corners chamfered
    const std::vector<std::shared_ptr<FacePlanarElement>> plates =
        mitered_plates(mesh, guide.thickness, guide.chamfer, guide.chamfer, guide.chamfer_angle);
    const std::shared_ptr<TreeNode> plates_group = add_group("plates");

    for (const std::shared_ptr<FacePlanarElement>& plate : plates)
        add(plate, plates_group);

    // assembly: two by two plates make one patch of four, an odd last column and row patches of three and four; the patches put one
    // after another along the cross section, pair of rows after pair of rows
    const size_t across = guide.cross_section.point_count() - 1;
    const size_t along = guide.profile.point_count() - 1;
    std::vector<std::vector<size_t>> patches;
    for (size_t i = 0; i + 1 < along; i += 2)
        for (size_t j = 0; j + 1 < across; j += 2)
            patches.push_back({i * across + j, i * across + j + 1, (i + 1) * across + j, (i + 1) * across + j + 1});

    std::array<std::vector<size_t>, 2> odd;
    for (size_t i = 0; across % 2 == 1 && i + along % 2 < along; i++)
        odd[0].push_back(i * across + across - 1);
    for (size_t j = 0; along % 2 == 1 && j < across; j++)
        odd[1].push_back((along - 1) * across + j);
    for (const std::vector<size_t>& run : odd) {
        const size_t pieces = (run.size() + 3) / 4;
        for (size_t k = 0; k < pieces; k++)
            patches.emplace_back(run.begin() + static_cast<std::ptrdiff_t>(run.size() * k / pieces), run.begin() + static_cast<std::ptrdiff_t>(run.size() * (k + 1) / pieces));
    }

    std::vector<AssemblyModule> modules;
    for (size_t k = 0; k < patches.size(); k++) {
        std::vector<AssemblyModule> patch;
        for (const size_t plate : patches[k])
            patch.push_back(AssemblyModule::part(plates[plate]));
        modules.push_back(AssemblyModule::module(fmt::format("patch_{}", k), patch));
    }
    AssemblyModule whole = AssemblyModule::module(name, modules);
    whole.one_by_one = true;
    assemblies = {whole};
}

std::vector<Polyline> TranslationShell::compute_sections() const {

    std::vector<Polyline> sections;

    for (size_t i = 0; i < guide.profile.point_count(); i++) {
        const Vector step = guide.profile[i] - guide.profile[0];
        sections.push_back(guide.cross_section.translated(step));
    }

    return sections;
}

Mesh TranslationShell::compute_mesh(const std::vector<Polyline>& sections) {

    const size_t count = sections[0].point_count();
    std::vector<Point> points;
    std::vector<std::vector<size_t>> faces;

    for (const Polyline& section : sections)
        for (size_t j = 0; j < count; j++)
            points.push_back(section[j]);

    for (size_t i = 1; i < sections.size(); i++)
        for (size_t j = 0; j + 1 < count; j++) {
            const size_t row = i * count + j;
            const size_t previous = row - count;
            faces.push_back({row, previous, previous + 1, row + 1});
        }

    return Mesh::from_vertices_and_faces(points, faces);
}
