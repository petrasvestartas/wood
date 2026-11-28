#pragma once
#include "src/templates/developable_folding/developable_folding_guide.h"
#include "src/templates/development.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CurvedCreaseTube
// ═══════════════════════════════════════════════════════════════════════════

/// Flat-foldable cellular curved-crease tube vault on a space curve of harmonic torsion (Tachi 2013): mid-surface walls, then plates.
class CurvedCreaseTube : public WoodSession {
public:
    const CurvedCreaseTubeGuide
        guide; // The arch, torsion, walls, stages and thickness; every wall, plate and piece is in the session.

    /// The tube vault of the guide as the session named name: its walls, creases and crease pattern, with a thickness its plates and flat pieces.
    explicit CurvedCreaseTube(
        const CurvedCreaseTubeGuide& guide = CurvedCreaseTubeGuide(),
        const std::string& name = "curved_crease_tube"
    );

    /// Not copied, as a session is not.
    CurvedCreaseTube(const CurvedCreaseTube&) = delete;

    /// Not assigned, as it is not copied.
    CurvedCreaseTube& operator=(const CurvedCreaseTube&) = delete;

    /// Largest relative diagonal error of any wall's flat development.
    double distortion() const {
        return _distortion;
    }

    /// Smallest ruling arclength speed ds_{u,v} / ds across the grid; strictly positive away from regression.
    double min_speed() const {
        return _min_speed;
    }

    /// Mid-arch cross-section width taper w_R(0.5 * s_0) / w_R(0) induced by the space curve's torsion.
    double midspan_taper() const {
        return _midspan_taper;
    }

private:
    /// Sampled Frenet-Serret frame along a constant-curvature harmonic-torsion space curve.
    struct FrenetSamples {
        std::vector<Point> points;
        std::vector<Vector> tangents;
        std::vector<Vector> normals;
        std::vector<Vector> binormals;
    };

    /// Alpha-invariant strip widths and ruling sector angles along s in [0, s_0].
    struct IntrinsicProfile {
        double arc_angle =
            0.0; // Dimensionless total curvature kappa_0 * s_0 satisfying T(0.5 * s_0) . T(0) = 0.
        double arc_length = 0.0; // mm, total reference space-curve arclength s_0.
        double curvature = 0.0; // 1/mm, reference space-curve curvature kappa_0.
        std::vector<double> cos_beta;
        std::vector<double> sin_beta;
        std::vector<double> width_l;
        std::vector<double> width_r;
        std::vector<double> length_l;
        std::vector<double> length_r;
        double min_speed = 1.0;
        double midspan_taper = 1.0;
    };

    /// One wall of a tube without thickness: its rulings from one crease to the next.
    struct Wall {
        std::string suffix; // bottom_left, top_left, bottom_right or top_right.
        size_t tube = 0;
        std::vector<Point> start; // The crease it starts on.
        std::vector<Point> end; // The crease it ends on.
        std::vector<Vector> rulings; // Unit rulings from start to end.
        std::vector<Vector> normals; // Unit wall normals.
        std::vector<double> widths; // Perpendicular strip width per station.
        std::vector<double> sin_beta; // Sine of the ruling against the arch per station.
        Vector tangent; // The arch's unit tangent at the middle station.
        std::array<double, 3> sides = {
            0.0, 0.0, 0.0
        }; // Per half thickness: the first face along the normal, the miter shifts at start and at end.
    };

    /// The walls and creases of one deployed stage.
    struct DeployedVault {
        std::vector<Wall> walls;
        std::vector<std::vector<Point>> creases;
        double cos_2a = 0.0; // Of the dihedral angle 2 alpha between the walls.
        double sin_2a = 1.0;
    };

    double _distortion = 0.0; // Largest relative diagonal error of any wall's flat development.
    double _min_speed = 1.0; // Smallest arclength speed ds_{u,v} / ds > 0 (regression margin).
    double _midspan_taper = 1.0; // Width taper ratio w_R(0.5 * s_0) / w_R(0) at mid-arch.

    /// Midpoint Darboux exponential integration of a space curve on [0, length].
    static FrenetSamples integrate_frenet(
        double length,
        double curvature,
        double torsion_amplitude,
        double period_length,
        size_t steps
    );

    /// Solves T(0.5 * s_0) . T(0) = 0 for kappa_0 * s_0 and computes the alpha-invariant strip profile.
    IntrinsicProfile compute_profile() const;

    /// The deployed Frenet frame at alpha = w * opening and every tube's four walls and its creases.
    DeployedVault compute_stage(const IntrinsicProfile& profile, double deployment) const;

    /// One wall of a parallelogram tube from corner (u, v) along its rulings, its faces and miters to come on sides.
    Wall compute_wall(
        const IntrinsicProfile& profile,
        const std::vector<Point>& p1,
        const std::vector<Vector>& tangents,
        const std::vector<Vector>& rl,
        const std::vector<Vector>& rr,
        const std::vector<double>& widths,
        const std::vector<Vector>& rulings,
        const std::vector<Vector>& wall_normals,
        size_t u,
        size_t v,
        const std::array<double, 3>& sides,
        size_t tube,
        const char* suffix
    ) const;

    /// Per stage its shift along y, the stages side by side.
    std::vector<Vector> compute_shifts(const std::vector<DeployedVault>& stages) const;

    /// Adds every stage's walls as quad meshes and its creases, moved by its shift, and returns the box around the walls.
    AABB add_walls(const std::vector<DeployedVault>& stages, const std::vector<Vector>& shifts);

    /// Adds every stage's walls as plates of the thickness, centred on the walls and mitered on the creases.
    void add_plates(const std::vector<DeployedVault>& stages, const std::vector<Vector>& shifts);

    /// Adds every plate unrolled into its flat piece on a cutting sheet, group flat.
    void add_flat();

    /// The assembly: four walls per tube, then the tubes side by side.
    void add_assembly();

    /// The plate name prefix of stage s: tube, or stage_s_tube for a sequence.
    std::string prefix(size_t s) const;
};

}
