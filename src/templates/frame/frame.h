#pragma once

#include "src/templates/frame/frame_guide.h"

namespace wood_frame {

// ═══════════════════════════════════════════════════════════════════════════
// Frame
// ═══════════════════════════════════════════════════════════════════════════

/// Section-active timber frame or cellular box-frame session built step by step from a `FrameGuide`.
class Frame : public wood_session::WoodSession {
public:
    const FrameGuide guide; // Geometric guide of the frame; every built element lives in the session.

    /// Builds the frame's supports, members, plates, face contacts, and erection assembly sequence.
    explicit Frame(const FrameGuide& guide, const std::string& name = "frame");

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    /// Move constructor so a Frame can be grafted into `Frames`.
    Frame(Frame&&) noexcept = default;

private:
    std::vector<int> _element_stages;

    /// Adds every `NodeSupportElement` base shoe under pinned frame legs.
    void add_supports();

    /// Adds every straight or variable-depth timber bar member (`column`, `rafter`, `chord`, `post`, `purlin`).
    void add_members();

    /// Adds every planar CLT floor, wall, and roof plate of a cellular box frame (`Plattenrahmen`).
    void add_plates();

    /// Builds the stage-ordered erection `AssemblyModule` from bottom up.
    wood_session::AssemblyModule compute_assembly() const;
};

// ═══════════════════════════════════════════════════════════════════════════
// Frames
// ═══════════════════════════════════════════════════════════════════════════

/// Row of `Frame` models placed side by side along world X, each under its design's group.
class Frames : public wood_session::WoodSession {
public:
    /// Builds one `Frame` per design with `gap` clear spacing along X.
    Frames(const std::vector<FrameDesign>& designs, double gap, const std::string& name);
};

} // namespace wood_frame
