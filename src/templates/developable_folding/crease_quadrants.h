#pragma once
#include "src/templates/developable_folding/developable_folding_guide.h"
#include "src/templates/development.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CreaseQuadrants
// ═══════════════════════════════════════════════════════════════════════════

/// The four quadrants Q1-Q4 of a planar curved crease: CCF single-sheet folding (Q1, Q3) and CCU two-sheet mirror unfolding (Q2, Q4).
class CreaseQuadrants : public WoodSession {
public:
    const CreaseQuadrantsGuide
        guide; // The crease, opening, width, deployment and thickness; every strip, plate and piece is in the session.

    /// The four quadrants of the guide side by side as the session named name, with a thickness their plates and flat pieces.
    explicit CreaseQuadrants(
        const CreaseQuadrantsGuide& guide = CreaseQuadrantsGuide(),
        const std::string& name = "crease_quadrants"
    );

    /// Not copied, as a session is not.
    CreaseQuadrants(const CreaseQuadrants&) = delete;

    /// Not assigned, as it is not copied.
    CreaseQuadrants& operator=(const CreaseQuadrants&) = delete;

    /// Largest relative length error of any strip's flat development, 0 on exact cones and cylinders.
    double distortion() const {
        return _distortion;
    }

    /// Vertex sector angle sums Q1..Q4 in radians: 2 pi on the developable Q1 and Q3, less on the synclastic Q2, more on the anticlastic Q4.
    const std::array<double, 4>& sector_sums() const {
        return _sector_sums;
    }

private:
    /// Per quadrant and strip the signs of the ruling's inward and normal parts: Q1 (r, -Mr), Q2 (r, Mr), Q3 (Mr, -r), Q4 (-r, -Mr).
    static constexpr std::array<std::array<std::array<double, 2>, 2>, 4> SIGNS = {{
        {{{1.0, 1.0}, {-1.0, 1.0}}},
        {{{1.0, 1.0}, {1.0, -1.0}}},
        {{{1.0, -1.0}, {-1.0, -1.0}}},
        {{{-1.0, -1.0}, {-1.0, 1.0}}},
    }};

    /// Sampled osculating frame of the planar crease.
    struct Frame {
        std::vector<Point> points; // Sampled points on the crease.
        std::vector<Vector> tangents; // Unit tangents.
        std::vector<Vector> inward; // Unit normals in the crease plane toward its concave side.
        Plane plane; // Osculating plane of the crease.
        Vector normal; // Osculating plane unit normal.
    };

    /// One strip without thickness: rulings from the crease, the side its body takes and its miter per mm of thickness.
    struct Strip {
        std::vector<Point> hinges; // The crease, moved to the quadrant's place.
        std::vector<Vector> rulings; // Unit rulings at the deployment.
        std::vector<Point> ends; // The far edge, width along the rulings.
        std::vector<Vector>
            normals; // Unit normals into the body at the deployment, away from the hinge face.
        std::vector<double>
            bevels; // Miter reach along the ruling on the body face, per mm of thickness, at the target.
    };

    double _distortion = 0.0; // Largest relative diagonal error of any strip's flat development.
    std::array<double, 4> _sector_sums = {
        0.0, 0.0, 0.0, 0.0
    }; // Sum of the four vertex sector angles per quadrant.

    /// Sampled frame of the crease, uncurled when the deployment is below 1.
    Frame compute_frame() const;

    /// Per quadrant its two strips from the crease, the quadrants side by side along the chord.
    std::array<std::array<Strip, 2>, 4> compute_strips(const Frame& frame) const;

    /// Per quadrant the four sector angles at the crease's middle, between its target rulings and the crease.
    std::array<double, 4> compute_sector_sums(const Frame& frame) const;

    /// Adds every strip as a quad mesh and every quadrant's crease, and returns the box around the strips.
    AABB add_strips(const std::array<std::array<Strip, 2>, 4>& strips);

    /// Adds every strip as a plate of the thickness on its body side, the hinge face on the strip and the miter at the crease.
    void add_plates(const std::array<std::array<Strip, 2>, 4>& strips);

    /// Adds every plate unrolled into its flat piece on a cutting sheet, group flat.
    void add_flat();
};

}
