#pragma once
#include "src/templates/cylindrical_folding/cylindrical_folding_guide.h"
#include "src/templates/development.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_cylindrical_folding {

// ═══════════════════════════════════════════════════════════════════════════
// ReflectionFold
// ═══════════════════════════════════════════════════════════════════════════

/// A curved folding of a cylindrical sheet.
class ReflectionFold : public WoodSession {
public:
    const ReflectionFoldGuide
        guide; // The sheet the fold is built from; every element is in the session.

    /// The fold of the guide as the session named name.
    explicit ReflectionFold(
        const ReflectionFoldGuide& guide = ReflectionFoldGuide(),
        const std::string& name = "reflection_fold"
    );

    /// Not copied, as a session is not.
    ReflectionFold(const ReflectionFold&) = delete;

    /// Not assigned, as it is not copied.
    ReflectionFold& operator=(const ReflectionFold&) = delete;

    /// The largest relative length error of any lamella's development, the proof every lamella is cut flat.
    double distortion() const;

    /// The largest twist of any lamella's rulings out of the tangent planes, 0 on a cylinder.
    double twist() const;

private:
    /// One straight run of the sheet between two creases: its rulings and the planes it starts and ends on.
    struct Segment {
        Vector direction; // The rulings' unit direction.
        Plane start; // The plane the segment's first crease lies in.
        Plane end; // The plane its last crease lies in.
    };

    /// The profile's section square to the rulings and its offset, curve by curve, in the section's frame u, v.
    struct Offsets {
        std::vector<std::vector<std::array<double, 2>>>
            outer; // Every curve's section, densely sampled.
        std::vector<std::vector<std::array<double, 2>>>
            inner; // Every curve's offset, the same samples moved the thickness.
        std::vector<std::array<std::array<double, 2>, 2>>
            tangents; // Every curve's unit tangents at its start and end.
    };

    double _distortion = 0.0; // The largest relative length error of any lamella's development.
    double _twist = 0.0; // The largest twist of any lamella's rulings out of the tangent planes.

    /// The profile's plane, from its most different tangents, through its first point.
    Plane compute_profile_plane() const;

    /// Every segment from the profile's plane to the end, the rulings turned about the axis at every fold.
    std::vector<Segment> compute_segments(const Plane& first) const;

    /// Whether rulings in this direction cross the symmetry plane square, the segment and its mirror image one cylinder.
    bool crosses_square(const Vector& rulings) const;

    /// What carries segment j's sections onto its end: the projection along its rulings, or at a crown the mirror.
    Xform carry(const std::vector<Segment>& segments, size_t j) const;

    /// Every crease, curve by curve: the profile carried from plane to plane, checked to run forward and never fold flat.
    std::vector<std::vector<NurbsCurve>>
    compute_creases(const std::vector<Segment>& segments) const;

    /// The profile offset the thickness under the sheet (side 1) or over it (-1), trimmed at seams and free edges.
    std::vector<NurbsCurve> compute_inner_profile(const Plane& first, double side) const;

    /// Every curve's section on the section plane and its offset the thickness to the side, in the frame u, v.
    Offsets
    compute_offsets(const Plane& section, const Vector& u, const Vector& v, double side) const;

    /// Every two offsets meeting at an angle trimmed to end where they cross, on the seam through the cusp.
    void trim_seams(Offsets& offsets) const;

    /// The inner meridians on every cut, the one curve the two plates there share.
    std::vector<std::vector<NurbsCurve>> compute_inners(
        const std::vector<Segment>& segments, const std::array<std::vector<NurbsCurve>, 2>& profiles
    ) const;

    /// The offset's section on the plane as one dense chain across all rows, carried on past both free edges.
    std::vector<std::array<double, 2>>
    compute_chain(const std::vector<NurbsCurve>& section, const Plane& plane) const;

    /// The inner meridians beyond a reflection, where every inner ruling meets the mirror image of the chain's cylinder.
    std::vector<NurbsCurve> compute_crossing(
        const std::vector<NurbsCurve>& inner,
        const std::vector<std::array<double, 2>>& chain,
        const Segment& segment,
        size_t j
    ) const;

    /// Every crease on the folded sheet, crease_<j>, in the group creases.
    void add_creases(const std::vector<std::vector<NurbsCurve>>& creases);

    /// Every lamella, lamella_<segment>_<row>, in the group lamellas; its development returned for the flat pattern.
    std::vector<Development> add_lamellas(
        const std::vector<std::vector<NurbsCurve>>& creases,
        const std::vector<std::vector<NurbsCurve>>& inners
    );

    /// The flat cutting pattern beside the model, every lamella's development where it lies on the unrolled sheet, in the group flat.
    void add_flat(
        const std::vector<Development>& developments,
        const std::vector<Segment>& segments,
        const std::vector<std::vector<NurbsCurve>>& creases,
        double beside
    );

    /// The mirror image in the symmetry plane of every lamella, crease and flat outline, except at a crown the crossing segment.
    void add_mirrored(const std::vector<std::vector<NurbsCurve>>& creases, bool crown);

    /// The mirror image of every lamella, segment s becoming beyond - s, the crossing segment skipped at a crown.
    void add_mirrored_lamellas(size_t beyond, bool crown, size_t crossing);

    /// The symmetry plane's own crease unless at a crown, and the mirror image of every crease.
    void add_mirrored_creases(
        const std::vector<std::vector<NurbsCurve>>& creases, size_t beyond, bool crown
    );

    /// The mirrored half's flat outlines turned over beyond the first half's, meeting it along the symmetry plane's section.
    void add_mirrored_flat(size_t beyond, bool crown, size_t crossing);

    /// The reflection in the plane.
    static Xform reflection(const Plane& plane);

    /// The vector turned about axis by angle (radians).
    static Vector turned(const Vector& vector, const Vector& axis, double angle);

    /// The curve's parameter at the k-th of samples equal parameter steps.
    double parameter(const NurbsCurve& curve, size_t k) const;

    /// The offset's coordinates in the frame u, v.
    static std::array<double, 2> in_frame(const Vector& offset, const Vector& u, const Vector& v);

    /// Where the lines through a along da and through b along db cross, as the distance along the first; none when parallel.
    static std::optional<double> line_crossing(
        const std::array<double, 2>& a,
        const std::array<double, 2>& da,
        const std::array<double, 2>& b,
        const std::array<double, 2>& db
    );

    /// The offset trimmed or carried on along its end tangent to the line through a along da, at its end (last) or its start.
    static void meet_line(
        std::vector<std::array<double, 2>>& offset,
        bool last,
        const std::array<double, 2>& a,
        const std::array<double, 2>& da,
        const std::array<double, 2>& tangent
    );

    /// The chain resampled at count equal steps of its length.
    static std::vector<std::array<double, 2>>
    resampled(const std::vector<std::array<double, 2>>& offset, size_t count);
};

}
