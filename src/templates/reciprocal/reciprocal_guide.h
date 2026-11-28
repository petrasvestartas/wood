#pragma once
#include "wood_session.h"
#include "nurbssurface.h"

#include <optional>

using namespace laika_cpp;
using namespace wood_session;

namespace wood_reciprocal {

// ═══════════════════════════════════════════════════════════════════════════
// ReciprocalRotationGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a reciprocal frame by rotation: the mesh, how every beam turns and grows, the beams and the frame.
struct ReciprocalRotationGuide {
    Mesh mesh; // The mesh whose interior edges become beams.
    double angle = 0.35; // Radians every beam turns about its midpoint.
    double scale = 1.4; // How much every beam is lengthened about its midpoint before the cuts.
    double width = 100.0; // mm across every beam.
    double height = 0.0; // mm along every beam's up, twice the width when zero.
    double extend_factor = 5.0; // Beam widths a beam may run past its ends to reach the beam it bears on.
    double cut_offset_factor = 1.0; // Where an end stops from the crossing beam's axis, 1 flush with its side face.
    std::map<std::pair<size_t, size_t>, Vector> frame_ups; // Up per naked edge key; the others are transported.
    bool butt = false; // Frame beams butted at the corners whose sections do not mirror, else mitered.
    std::map<std::pair<size_t, size_t>, int> through_priority; // Per naked edge key, the higher runs through a butt corner.

    /// The height of every beam: height, or twice the width when zero.
    double beam_height() const;

    /// The defaults on the mesh: 100 x 200 beams turned 0.35 rad, a mitered frame, its ups transported.
    static ReciprocalRotationGuide defaults(const Mesh& mesh);

    /// The examples' frame: 100 x 400 beams in a butted frame, its ups tilted per side of the surface, or per side at 45 degree bends, upright when vertical.
    static ReciprocalRotationGuide butted(
        const Mesh& mesh, const std::optional<NurbsSurface>& surface = std::nullopt, bool vertical = false
    );
};

// ═══════════════════════════════════════════════════════════════════════════
// ReciprocalMoveGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The inputs of a reciprocal frame by translation: the mesh, how far every beam moves, the beams and the frame.
struct ReciprocalMoveGuide {
    Mesh mesh; // The mesh whose edges become beams.
    double shift = 50.0; // mm every beam moves sideways; at least half the width for its ends to bear on their neighbours.
    double width = 100.0; // mm across every beam.
    double height = 0.0; // mm along every beam's up, twice the width when zero.
    double cut_offset_factor = 1.0; // Where an end stops from the crossing beam's axis, 1 flush with its side face.
    std::map<std::pair<size_t, size_t>, Vector> frame_ups; // Up per naked edge key; the others are transported.
    bool edge_up = false; // Every beam's up the average normal of the faces of its edge, else its own face's normal.
    bool butt = false; // Frame beams butted at the corners whose sections do not mirror, else mitered.
    std::map<std::pair<size_t, size_t>, int> through_priority; // Per naked edge key, the higher runs through a butt corner.

    /// The height of every beam: height, or twice the width when zero.
    double beam_height() const;

    /// The defaults on the mesh: 100 x 200 beams moved 50, their ups from their faces, a mitered frame.
    static ReciprocalMoveGuide defaults(const Mesh& mesh);

    /// The examples' frame: 100 x 400 beams moved 100, their ups from their edges, in a butted frame tilted like ReciprocalRotationGuide::butted.
    static ReciprocalMoveGuide butted(
        const Mesh& mesh, const std::optional<NurbsSurface>& surface = std::nullopt, bool vertical = false
    );
};

}
