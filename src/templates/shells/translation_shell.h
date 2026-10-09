#pragma once
#include "wood_session.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// TranslationShell
// ═══════════════════════════════════════════════════════════════════════════

/// A shell of mitred plates, a cross section swept along a profile.
///
/// Fields: the two input curves and the plate sizes; every plate is in the session as `plate_<i>`.
class TranslationShell : public WoodSession {
public:
    const Polyline cross_section; // The curve that is swept.
    const Polyline profile; // The path it is swept along.
    const double thickness; // Plate thickness.
    const double chamfer; // How far a sharp corner is cut back.
    const double chamfer_angle; // Corners sharper than this, in degrees, are chamfered.

    /// The shell of cross_section swept along profile, as the session named name.
    explicit TranslationShell(
        const Polyline& cross_section = default_cross_section(),
        const Polyline& profile = default_profile(),
        double thickness = 10.0,
        double chamfer = 1.0,
        double chamfer_angle = 180.0,
        const std::string& name = "translation_shell"
    );

    /// The arch the default shell is swept from, in the xz plane.
    static Polyline default_cross_section();

    /// The arch the default shell is swept along, in the yz plane.
    static Polyline default_profile();

private:
    /// The cross section moved to every profile point.
    std::vector<Polyline> compute_sections() const;

    /// One quad between every two neighbouring points of two neighbouring sections.
    static Mesh compute_mesh(const std::vector<Polyline>& sections);
};

inline TranslationShell::TranslationShell(
    const Polyline& cross_section,
    const Polyline& profile,
    double thickness,
    double chamfer,
    double chamfer_angle,
    const std::string& name
)
    : WoodSession(name),
      cross_section(cross_section),
      profile(profile),
      thickness(thickness),
      chamfer(chamfer),
      chamfer_angle(chamfer_angle) {

    // curves: the cross section and the profile it is swept along
    const std::shared_ptr<TreeNode> curves = add_group("curves");
    add_polyline(cross_section, curves);
    add_polyline(profile, curves);

    // sections: the cross section moved to every profile point
    const std::vector<Polyline> sections = compute_sections();
    const std::shared_ptr<TreeNode> sections_group = add_group("sections");

    for (const Polyline& section : sections)
        add_polyline(section, sections_group);

    // mesh: a quad between every two neighbouring sections
    const Mesh mesh = compute_mesh(sections);
    add_mesh(mesh, add_group("mesh"));

    // plates: one mitred plate per quad, thickness deep, its sharp corners chamfered
    const std::vector<std::shared_ptr<Plate>> plates = mitred_plates(
        mesh,
        thickness,
        chamfer,
        chamfer,
        chamfer_angle
    );
    const std::shared_ptr<TreeNode> plates_group = add_group("plates");

    for (const std::shared_ptr<Plate>& plate : plates)
        add(plate, plates_group);
}

inline std::vector<Polyline> TranslationShell::compute_sections() const {

    std::vector<Polyline> sections;

    for (size_t i = 0; i < profile.point_count(); i++) {
        const Vector step = profile[i] - profile[0];
        sections.push_back(cross_section.translated(step));
    }

    return sections;
}

inline Mesh TranslationShell::compute_mesh(const std::vector<Polyline>& sections) {

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

inline Polyline TranslationShell::default_cross_section() {

    return Polyline(std::vector<Point>{
        {1343.472686, 0.0, 0.0},
        {1237.791431, 0.0, 121.964275},
        {1117.698859, 0.0, 229.686024},
        {981.901426, 0.0, 316.618767},
        {831.471484, 0.0, 374.347649},
        {671.736343, 0.0, 394.768581},
        {512.001202, 0.0, 374.347649},
        {361.571259, 0.0, 316.618767},
        {225.773829, 0.0, 229.686026},
        {105.681255, 0.0, 121.964275},
        {0.0, 0.0, 0.0},
    });
}

inline Polyline TranslationShell::default_profile() {

    return Polyline(std::vector<Point>{
        {0.0, 0.0, 0.0},
        {0.0, -154.339980, 121.873175},
        {0.0, -321.617334, 225.228387},
        {0.0, -500.643329, 306.496555},
        {0.0, -689.070490, 362.559337},
        {0.0, -883.558001, 391.165838},
        {0.0, -1080.134636, 391.165838},
        {0.0, -1274.622136, 362.559339},
        {0.0, -1463.049306, 306.496555},
        {0.0, -1642.075299, 225.228388},
        {0.0, -1809.352653, 121.873177},
        {0.0, -1963.692636, 0.0},
    });
}
