#pragma once
#include "src/templates/developable_folding/developable_folding_guide.h"
#include "src/templates/development.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CurvedCreaseUnfolding
// ═══════════════════════════════════════════════════════════════════════════

/// Flat-foldable curved-crease unfolding (CCU): mid-surface strips mirrored from plane to plane and deployed isometrically, then made plates.
class CurvedCreaseUnfolding : public WoodSession {
public:
    const CurvedCreaseUnfoldingGuide
        guide; // The crease, planes, stages and thickness; every strip, plate and piece is in the session.

    /// The unfolding of the guide as the session named name: its strips, creases and crease pattern, with a thickness its plates and flat pieces.
    explicit CurvedCreaseUnfolding(
        const CurvedCreaseUnfoldingGuide& guide = CurvedCreaseUnfoldingGuide(),
        const std::string& name = "curved_crease_unfolding"
    );

    /// Not copied, as a session is not.
    CurvedCreaseUnfolding(const CurvedCreaseUnfolding&) = delete;

    /// Not assigned, as it is not copied.
    CurvedCreaseUnfolding& operator=(const CurvedCreaseUnfolding&) = delete;

    /// Largest relative length error of any strip's flat development, 0 on exact developables.
    double distortion() const {
        return _distortion;
    }

    /// mm, how far the target strips' feet rise above or sink below the crease's floor.
    double drift() const {
        return _drift;
    }

private:
    /// One mid-surface strip of the target between plane from and plane to.
    struct TargetStep {
        size_t from = 0;
        size_t to = 0;
        Vector plane_normal; // Unit normal of plane from pointing toward plane to.
        std::vector<Point> neutral_start; // The crease on plane from.
        std::vector<Point> neutral_end; // The crease on plane to.
        std::vector<Vector> rulings; // Unit rulings from the crease on plane from.
        std::vector<double> cos_gamma; // Ruling cosine against the crease tangent.
        std::vector<double> sin_gamma; // Ruling sine against the crease tangent.
        double inward_sign = 1.0; // Sign of the ruling's projection on plane_normal x tangent.
        std::vector<double> sin_alpha; // Ruling sine against plane from along the crease.
        std::vector<double> neutral_reach; // Ruling length from plane from to plane to.
    };

    /// Where the rulings of one strip reach the planes on its two faces, half a thickness off the mid-surface.
    struct FaceReaches {
        double half = 0.0; // Half the thickness, 0 on the mid-surface.
        std::vector<double> minus_start; // On the minus face to plane from, the hinge at from.
        std::vector<double> minus_end; // On the minus face to plane to, the outer miter tip.
        std::vector<double> plus_start; // On the plus face to plane from, the outer miter tip.
        std::vector<double> plus_end; // On the plus face to plane to, the hinge at to.
    };

    /// One deployed strip: its mid-surface between its two creases and its two faces.
    struct DeployedPlate {
        std::vector<Point> minus_a;
        std::vector<Point> minus_b;
        std::vector<Point> plus_a;
        std::vector<Point> plus_b;
        std::vector<Point> neutral_a;
        std::vector<Point> neutral_b;
        std::vector<Point> crease_to; // The hinge on plane to, on the plus face.
        size_t to_index = 0;
    };

    /// One deployed stage: every strip and the leading crease.
    struct DeployedStage {
        std::vector<DeployedPlate> plates;
        std::vector<Point> start_crease;
        size_t start_index = 0;
    };

    double _distortion = 0.0; // Largest relative diagonal error of any strip's flat development.
    double _drift = 0.0; // mm, largest vertical drift of the target strips' feet from the floor.

    /// Index of the plane nearest the crease's ends and middle.
    size_t compute_start() const;

    /// The mid-surface strips on both sides of start at full opening.
    std::vector<TargetStep> compute_target(size_t start) const;

    /// Traces the target strips from start toward one end (step = +1 or -1), reflecting the rulings at every plane.
    void trace_target_side(
        std::vector<Point> neutral,
        std::vector<Vector> rulings,
        size_t from,
        int step,
        std::vector<TargetStep>& steps
    ) const;

    /// Where every target ruling reaches the planes on the faces thickness apart: the axis shift, the mid-surface itself at 0.
    std::vector<FaceReaches>
    compute_reaches(const std::vector<TargetStep>& target, double thickness) const;

    /// Every deployment's strips, hinged where reaches puts them.
    std::vector<DeployedStage> compute_stages(
        size_t start, const std::vector<TargetStep>& target, const std::vector<FaceReaches>& reaches
    ) const;

    /// All strips rebuilt isometrically at deployment fraction w in [0, 1] from the uncurled leading crease.
    DeployedStage compute_deployed(
        size_t start,
        const std::vector<TargetStep>& target,
        const std::vector<FaceReaches>& reaches,
        double fraction
    ) const;

    /// Uncurls a planar crease on plane for reflection angle alpha = fraction * opening (Eq. 5 and 8).
    std::vector<Point>
    uncurl_crease(const std::vector<Point>& points, const Plane& plane, double fraction) const;

    /// Rulings and thickness normals along a curve for an osculating normal and a deployment fraction.
    void compute_strip_frame(
        const std::vector<Point>& curve,
        const Vector& plane_normal,
        const TargetStep& info,
        double fraction,
        std::vector<Vector>& rulings,
        std::vector<Vector>& normals
    ) const;

    /// Per stage its shift along the unfolding direction, the stages side by side.
    std::vector<Vector>
    compute_shifts(size_t start, const std::vector<DeployedStage>& stages) const;

    /// Every strip of every stage developed flat.
    std::vector<std::vector<Development>>
    compute_developments(const std::vector<DeployedStage>& stages) const;

    /// mm, the largest height of the target strips' feet over the floor of the crease.
    double compute_drift(const std::vector<DeployedStage>& stages) const;

    /// Adds every stage's strips as quad meshes and its creases, moved by its shift, and returns the box around the strips.
    AABB add_strips(const std::vector<DeployedStage>& stages, const std::vector<Vector>& shifts);

    /// Adds every stage's plates as curved solids between their faces, moved by its shift.
    void add_plates(const std::vector<DeployedStage>& stages, const std::vector<Vector>& shifts);

    /// Adds the plates of the first stage unrolled into flat pieces side by side in the xy plane, group flat.
    void add_flat();

    /// The assembly: neighbours in pairs on their crease when mitered at full opening, plate by plate when not.
    void add_assembly();

    /// The group and name prefix of stage s: plates and plate, or stage_s for a sequence.
    std::array<std::string, 2> stage_names(size_t s) const;

    /// Unit tangents at uniform samples of an interpolated cubic through points.
    static std::vector<Vector> sample_tangents(const std::vector<Point>& points);

    /// Reflection of vector in the plane with unit normal.
    static Vector reflected(const Vector& vector, const Vector& normal);

    /// Signed distance along direction from point to plane; NaN when parallel.
    static double cut(const Point& point, const Vector& direction, const Plane& plane);
};

}
