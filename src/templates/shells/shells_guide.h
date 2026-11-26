#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// TranslationShellGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a translation shell: a cross section, the profile it is swept along and the plates on it.
struct TranslationShellGuide {
    Polyline cross_section = default_cross_section(); // The curve that is swept.
    Polyline profile = default_profile(); // The path it is swept along.
    double thickness = 10.0; // Plate thickness.
    double chamfer = 1.0; // How far a sharp corner is cut back.
    double chamfer_angle = 180.0; // Corners sharper than this, in degrees, are chamfered.

    /// The arch the default shell is swept from, in the xz plane.
    static Polyline default_cross_section() {

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

    /// The arch the default shell is swept along, in the yz plane.
    static Polyline default_profile() {

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

    /// The default arch swept along the default arch: 10 mm plates, their corners cut back 1 mm.
    static TranslationShellGuide arch() {
        return TranslationShellGuide();
    }
};
