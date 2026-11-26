#pragma once

#include "src/templates/grid/grid_guide.h"

namespace wood_grid {

// ═══════════════════════════════════════════════════════════════════════════
// Grid
// ═══════════════════════════════════════════════════════════════════════════

/// Timber framing of a `Plan` or a `Stack`: its elements and their face-to-face contacts.
class Grid : public wood_session::WoodSession {
public:
    /// The plan framed as the session named name.
    explicit Grid(const Plan& plan, const Framing& framing = {}, std::string name = "grid");

    /// Every storey and facade of the stack framed as the session named name.
    explicit Grid(const Stack& stack, std::string name = "grid");
};

// ═══════════════════════════════════════════════════════════════════════════
// Grids
// ═══════════════════════════════════════════════════════════════════════════

/// Grids side by side along x, each in the group of its design.
class Grids : public wood_session::WoodSession {
public:
    /// One grid per design as the session named name, gap between two.
    Grids(const std::vector<Design>& designs, double gap, const std::string& name);
};

} // namespace wood_grid
