#pragma once
#include "src/templates/cylindrical_folding/cylindrical_folding_guide.h"
#include "src/templates/development.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_cylindrical_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CylinderCarving
// ═══════════════════════════════════════════════════════════════════════════

/// A cylinder carved by cylinders in sequence, ((a - b) - c) - d, one concave plate per carved patch.
class CylinderCarving : public WoodSession {
public:
    const CylinderCarvingGuide
        guide; // The stock and carvers the carving is built from; every element is in the session.

    /// The carving of the guide as the session named name.
    explicit CylinderCarving(
        const CylinderCarvingGuide& guide = CylinderCarvingGuide(),
        const std::string& name = "cylinder_carving"
    );

    /// Not copied, as a session is not.
    CylinderCarving(const CylinderCarving&) = delete;

    /// Not assigned, as it is not copied.
    CylinderCarving& operator=(const CylinderCarving&) = delete;

    /// The largest relative length error of any plate's development, 0 for an exact cylinder.
    double distortion() const;

private:
    /// One piece of a generator a plate keeps, with what ends it at each side.
    struct Part {
        double low; // mm, along the generator, where the piece starts.
        double high; // mm, where it ends.
        int low_by; // What ends it at the start: 0 the stock, 1 and 2 its end planes, 3 + j carver j.
        int high_by; // What ends it at the end, the same way.
    };

    /// One carved patch of a carver: the angles of its generators and, per fine step, the piece it keeps.
    struct Patch {
        double first; // radians, its first generator.
        double last; // radians, its last generator.
        double
            behind_first; // radians, the first generator of the plate behind it, a thickness deep.
        double behind_last; // radians, the last.
        std::vector<std::pair<double, Part>>
            track; // Per fine step its angle and its piece on the surface.
    };

    static constexpr double LEAST =
        0.02; // mm, the shortest piece of a generator a plate keeps, so it reaches its corners.

    double _distortion = 0.0; // The largest relative length error of any plate's development.

    /// Every carver's patches, every patch a plate, plate_<k>_<p> in the group plates.
    void add_plates();

    /// The generator of carver k at angle on the circle grown by offset: its point and its direction.
    std::pair<Point, Vector> generator(size_t k, double angle, double offset) const;

    /// The pieces of carver k's generator at angle grown by offset a plate keeps: inside the stock where the carved surface is,
    /// so a free edge is cut square, between the stock's end planes and outside every other carver grown by offset.
    std::vector<Part> parts(size_t k, double angle, double offset) const;

    /// The parameters along the line between which it lies inside the cylinder grown by offset; none when it misses it.
    static std::optional<std::pair<double, double>>
    inside(const Cylinder& cylinder, double offset, const Point& point, const Vector& direction);

    /// The piece of the generator at angle grown by offset that overlaps the reference most, the least long; none when none does.
    std::optional<Part> pick(size_t k, double angle, double offset, const Part& reference) const;

    /// The last angle from angle towards side (+1 or -1) whose generator grown by offset keeps a piece overlapping the reference:
    /// stepped to the first that does not, or back to the first that does, then bisected.
    double reach(size_t k, double angle, double side, double offset, const Part& reference) const;

    /// Carver k's patches: its pieces followed from generator to generator by their overlap, the ends of every one exact on the
    /// carved surface and behind it, where a crease behind reaches an end plane at another angle than the crease on the surface.
    std::vector<Patch> compute_patches(size_t k) const;

    /// The patch's piece nearest the angle, the reference for every pick inside it.
    static const Part& reference(const Patch& patch, double angle);

    /// What ends the patch's piece at angle grown by offset at its start and at its end; -1 where it keeps none.
    std::array<int, 2> enders(size_t k, const Patch& patch, double angle, double offset) const;

    /// The angles from first to last where an end of the patch's piece grown by offset changes what ends it.
    std::vector<double>
    compute_breaks(size_t k, const Patch& patch, double offset, double first, double last) const;

    /// Throws when two carved patches meet at less than min_angle, or, closed, when a patch ends on the stock's surface: the point
    /// it ends at lies in no other carver, so the stock's surface beside it is left uncarved, a gap between the patches.
    void validate(size_t k, const Patch& patch) const;

    /// The unit normal of cylinder c at point, away from its axis.
    static Vector radial(const Cylinder& cylinder, const Point& point);
};

}
