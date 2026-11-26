#pragma once
#include "src/templates/folding/folding_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CounterFold
// ═══════════════════════════════════════════════════════════════════════════

/// A folded plate whose profile changes along the span, every warped strip folded on one diagonal.
class CounterFold : public WoodSession {
public:
    const CounterFoldGuide guide; // What it is built from; every element is in the session.

    /// The fold through the stations of the guide as the session named name.
    explicit CounterFold(
        const CounterFoldGuide& guide = CounterFoldGuide(), const std::string& name = "counter_fold"
    );

    /// Not copied, as a session is not.
    CounterFold(const CounterFold&) = delete;

    /// Not assigned, as it is not copied.
    CounterFold& operator=(const CounterFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.
};

}
