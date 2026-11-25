#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_timber_floor {

// ═══════════════════════════════════════════════════════════════════════════
// EarthFloorGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The geometry of a floor element of timber beams with rammed earth between them in mm.
struct EarthFloorGuide {
    double length = 4200.0; // Along the beams, x.
    double width = 2100.0; // Across them, y: a whole number of bays.
    double pitch = 700.0; // Between the middles of two beams; a beam is two halves but the first and the last, one.
    double depth = 200.0; // Depth of the beams.
    double half_top = 75.0; // Width of half a beam at its top.
    double half_bottom = 100.0; // Width of half a beam at its foot, the wider one the earth rests on.
    double flank = 120.0; // Height over which the beam widens, the earth's depth along it.
    double rise = 18.0; // Rise of the arch under the earth, 0 for a flat one.
    size_t chords = 24; // Sides of the arch.
    double panel = 27.0; // Three-layer panel on the beams, pinned to them.
    double lining = 0.0; // Board under the beams that hides the earth, 0 for none.
    double pin_spacing = 300.0; // Pins of the panel along every half beam, 0 for none.
    size_t ties = 0; // Rods across all beams that press the earth between them.
    double bearing_width = 160.0; // Width of the two beams the element lies on.
    double bearing_depth = 280.0; // Depth of those beams.

    /// Depth of the element.
    double height() const {
        return lining + depth + panel;
    }

    /// Bays of earth across.
    size_t bays() const {
        return static_cast<size_t>(std::round(width / pitch));
    }

    /// The earth arched between beams that run on below it.
    static EarthFloorGuide arch() {
        return EarthFloorGuide();
    }

    /// The earth flat between beams that run on below it.
    static EarthFloorGuide straight() {

        EarthFloorGuide guide;
        guide.rise = 0.0;

        return guide;
    }

    /// The earth as deep as the beams.
    static EarthFloorGuide flush() {

        EarthFloorGuide guide;
        guide.half_top = 93.0;
        guide.half_bottom = 119.0;
        guide.flank = guide.depth;
        guide.rise = 0.0;

        return guide;
    }

    /// The earth as deep as the beams, over a board.
    static EarthFloorGuide concealed() {

        EarthFloorGuide guide = flush();
        guide.lining = 20.0;

        return guide;
    }
};

}
