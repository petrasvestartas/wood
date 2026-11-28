#pragma once
#include "src/templates/folding/folding_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// PrismaticFold
// ═══════════════════════════════════════════════════════════════════════════

/// A singly folded plate: a fold profile carried straight along the span, diaphragms under the folds.
class PrismaticFold : public WoodSession {
public:
    const PrismaticFoldGuide guide; // What it is built from; every element is in the session.

    /// The folded plate of the guide as the session named name.
    explicit PrismaticFold(
        const PrismaticFoldGuide& guide = PrismaticFoldGuide(),
        const std::string& name = "prismatic_fold"
    );

    /// Not copied, as a session is not.
    PrismaticFold(const PrismaticFold&) = delete;

    /// Not assigned, as it is not copied.
    PrismaticFold& operator=(const PrismaticFold&) = delete;

    /// The profile with its edge flanges, the cross section of every plate.
    const Polyline& folded_profile() const;

    /// The folded quad mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Polyline _folded; // The profile with its edge flanges.
    Mesh _mesh; // The folded quad mesh, the plates' outer faces.

    /// The profile with a flange at each free edge: down beside a shallow fold, outwards beside a steep one.
    Polyline compute_folded_profile() const;

    /// The folded profile's points moved onto the plates' inner faces.
    std::vector<Point> compute_inner_profile() const;

    /// One quad per profile segment between its two fold lines, facing in, so the plates grow inwards from it.
    Mesh compute_mesh() const;

    /// The diaphragm's outline: the inner profile closed along the folded profile's lowest level.
    Polyline compute_diaphragm_outline(const std::vector<Point>& inner) const;
};

}
