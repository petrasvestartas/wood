#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// A curved folding of a cylindrical sheet: a planar profile swept along its rulings and folded on planar creases by reflection
/// or mirror folds, one curved lamella between every two creases and two profile curves.
class ReflectionFold : public WoodSession {
public:
    /// One fold of the sheet: where the rulings turn, by how much, and how.
    struct Fold {
        double station; // mm along the rulings from the sheet's start, on the ruling through the profile's first point.
        double angle; // radians, the rulings' turn about the axis, positive by the right hand.
        bool reflected = true; // the reflection, the sheet turned over, when true; the mirror fold, the bulge kept, when false.
    };

    const std::vector<NurbsCurve> profile; // The sheet's section: planar curves end to end, each a row of lamellas.
    const Vector direction; // The rulings' unit direction before the first fold, the profile swept along it.
    const Vector axis; // The folds' unit axis, in every fold plane; the profile's plane is spanned by it and the sheet's width.
    const double length; // mm, the sheet along the rulings from its start to its end, when no symmetry plane ends it.
    const std::vector<Fold> folds; // The folds, in order along the rulings.
    const std::optional<Plane> symmetry; // The plane the sheet ends on and is mirrored in; none for two free ends.
    const double thickness; // mm, every lamella, on the sheet's underside, kept through every fold.
    const size_t samples; // Rulings per profile curve the creases are checked on, 16 times as many for the offsets.

private:
    double _distortion = 0.0; // The largest relative length error of any lamella's development.
    double _twist = 0.0; // The largest twist of any lamella's rulings out of the tangent planes.

public:
    /// The fold as the session named name.
    ReflectionFold(
        const std::vector<NurbsCurve>& profile,
        const Vector& direction,
        const Vector& axis,
        double length,
        const std::vector<Fold>& folds,
        const std::optional<Plane>& symmetry = std::nullopt,
        double thickness = 120.0,
        const std::string& name = "reflection_fold",
        size_t samples = 24
    );

    /// Not copied, as a session is not.
    ReflectionFold(const ReflectionFold&) = delete;

    /// Not assigned, as it is not copied.
    ReflectionFold& operator=(const ReflectionFold&) = delete;

    /// The largest relative length error of any lamella's development, the proof every lamella is cut flat.
    double distortion() const {
        return _distortion;
    }

    /// The largest twist of any lamella's rulings out of the tangent planes, 0 on a cylinder.
    double twist() const {
        return _twist;
    }

    /// A profile of count circular arcs side by side from start along across, each width wide and bulging rise toward up.
    static std::vector<NurbsCurve> arcs(
        const Point& start,
        const Vector& across,
        const Vector& up,
        size_t count,
        double width,
        double rise
    );

    /// A corrugated profile from start along across: half_waves half sine waves half_wavelength long and amplitude high, first up.
    static std::vector<NurbsCurve> waves(
        const Point& start,
        const Vector& across,
        const Vector& up,
        size_t half_waves,
        double half_wavelength,
        double amplitude
    );

private:
    /// One straight run of the sheet between two creases: its rulings and the planes it starts and ends on.
    struct Segment {
        Vector direction; // The rulings' unit direction.
        Plane start; // The plane the segment's first crease lies in.
        Plane end; // The plane its last crease lies in.
    };

    /// The profile's section square to the rulings and its offset, curve by curve, in the section's frame u, v.
    struct Offsets {
        std::vector<std::vector<std::array<double, 2>>> outer; // Every curve's section, densely sampled.
        std::vector<std::vector<std::array<double, 2>>> inner; // Every curve's offset, the same samples moved the thickness.
        std::vector<std::array<std::array<double, 2>, 2>> tangents; // Every curve's unit tangents at its start and end.
    };

    /// The profile's plane, from its most different tangents, through its first point.
    Plane compute_profile_plane() const;

    /// Every segment from the profile's plane to the end, the rulings turned about the axis at every fold.
    std::vector<Segment> compute_segments(const Plane& first) const;

    /// Whether rulings in this direction cross the symmetry plane square, the segment and its mirror image one cylinder.
    bool crosses_square(const Vector& rulings) const;

    /// What carries segment j's sections onto its end: the projection along its rulings, or at a crown the mirror.
    Xform carry(const std::vector<Segment>& segments, size_t j) const;

    /// Every crease, curve by curve: the profile carried from plane to plane, checked to run forward and never fold flat.
    std::vector<std::vector<NurbsCurve>> compute_creases(const std::vector<Segment>& segments) const;

    /// The profile offset the thickness under the sheet (side 1) or over it (-1), trimmed at seams and free edges.
    std::vector<NurbsCurve> compute_inner_profile(const Plane& first, double side) const;

    /// Every curve's section on the section plane and its offset the thickness to the side, in the frame u, v.
    Offsets compute_offsets(
        const Plane& section,
        const Vector& u,
        const Vector& v,
        double side
    ) const;

    /// Every two offsets meeting at an angle trimmed to end where they cross, on the seam through the cusp.
    void trim_seams(Offsets& offsets) const;

    /// The inner meridians on every cut, the one curve the two plates there share.
    std::vector<std::vector<NurbsCurve>> compute_inners(
        const std::vector<Segment>& segments,
        const std::array<std::vector<NurbsCurve>, 2>& profiles
    ) const;

    /// The offset's section on the plane as one dense chain across all rows, carried on past both free edges.
    std::vector<std::array<double, 2>> compute_chain(const std::vector<NurbsCurve>& section, const Plane& plane) const;

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
    void add_mirrored_creases(const std::vector<std::vector<NurbsCurve>>& creases, size_t beyond, bool crown);

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
    static std::vector<std::array<double, 2>> resampled(const std::vector<std::array<double, 2>>& offset, size_t count);
};

inline ReflectionFold::ReflectionFold(
    const std::vector<NurbsCurve>& profile,
    const Vector& direction,
    const Vector& axis,
    double length,
    const std::vector<Fold>& folds,
    const std::optional<Plane>& symmetry,
    double thickness,
    const std::string& name,
    size_t samples
)
    : WoodSession(name),
      profile(profile),
      direction(direction.normalized()),
      axis(axis.normalized()),
      length(length),
      folds(folds),
      symmetry(symmetry),
      thickness(thickness),
      samples(std::max<size_t>(samples, 4)) {

    if (profile.empty() || length <= 0.0 || thickness <= 0.0)
        throw std::invalid_argument("A reflection fold needs a profile, a length and a thickness");

    // segments: the sheet's straight runs between the folds, each with its rulings and its two planes
    const Plane first = compute_profile_plane();
    const std::vector<Segment> segments = compute_segments(first);

    // creases: every fold plane's section of the sheet, the profile carried along the rulings from plane to plane
    const std::vector<std::vector<NurbsCurve>> creases = compute_creases(segments);

    // profiles: the profile's section offset the thickness under and over the sheet, seams through the cusps, free edges square
    const std::array<std::vector<NurbsCurve>, 2> profiles = {
        compute_inner_profile(first, 1.0),
        compute_inner_profile(first, -1.0)
    };

    // inners: the inner profile carried from plane to plane along the rulings, on every cut the one curve the two plates there share
    const std::vector<std::vector<NurbsCurve>> inners = compute_inners(segments, profiles);

    // creases drawn, then the lamellas: between two creases and two profile curves one curved plate, its inner meridians the shared ones
    add_creases(creases);
    const std::vector<Development> developments = add_lamellas(creases, inners);

    // flat: the cutting pattern beside the model, every lamella unfolded by its rulings
    add_flat(developments, segments, creases, 0.1 * length);

    // symmetry: the folded half mirrored in its plane of symmetry, or at a crown run on across it in the last segment's plates
    if (symmetry)
        add_mirrored(creases, crosses_square(segments.back().direction));

    // contacts: every two lamellas that share a face, found by the session's contact detection
    compute_face_contacts(0);
}

inline double ReflectionFold::parameter(const NurbsCurve& curve, size_t k) const {

    const double domain = curve.domain_end() - curve.domain_start();

    return curve.domain_start() + domain * static_cast<double>(k) / static_cast<double>(samples);
}

inline Plane ReflectionFold::compute_profile_plane() const {

    // the normal from the two most different tangents, or across the direction when the profile is straight
    std::vector<Vector> tangents;

    for (const NurbsCurve& curve : profile)
        for (size_t k = 0; k <= samples; k++)
            tangents.push_back(curve.tangent_at(parameter(curve, k)).normalized());

    Vector normal = tangents.front().cross(tangents.back());

    for (const Vector& tangent : tangents)
        if (tangents.front().cross(tangent).magnitude() > normal.magnitude())
            normal = tangents.front().cross(tangent);

    if (normal.magnitude() < 1e-6)
        normal = direction - tangents.front() * direction.dot(tangents.front());

    normal.normalize_self();

    if (std::abs(normal.dot(direction)) < 1e-6)
        throw std::invalid_argument("The profile's plane contains the direction, so the sheet has no width");

    const Point origin = profile.front().point_at(profile.front().domain_start());

    return Plane::from_point_normal(origin, normal);
}

inline std::vector<ReflectionFold::Segment> ReflectionFold::compute_segments(const Plane& first) const {

    std::vector<Segment> segments;
    Plane start = first;
    Vector rulings = direction;
    Point station = first.origin();
    double before = 0.0;

    for (size_t j = 0; j < folds.size(); j++) {
        if (folds[j].station <= before)
            throw std::invalid_argument(fmt::format("Fold {} is not beyond the fold before it", j));

        // the fold's plane through its station: bisecting the turn for a mirror fold, across the bisector for a reflection
        const Vector next = turned(rulings, axis, folds[j].angle);
        const Vector normal = folds[j].reflected ? rulings - next : rulings + next;

        if (normal.magnitude() < 1e-9)
            throw std::invalid_argument(fmt::format("Fold {} turns by nothing or by a half turn", j));

        station = station + rulings * (folds[j].station - before);
        const Plane plane = Plane::from_point_normal(station, normal);
        segments.push_back({rulings, start, plane});
        start = plane;
        rulings = next;
        before = folds[j].station;
    }

    // the last segment to the symmetry plane, or to the sheet's end across its rulings
    const Point end = station + rulings * (length - before);

    if (symmetry)
        segments.push_back({rulings, start, *symmetry});
    else
        segments.push_back({rulings, start, Plane::from_point_normal(end, rulings)});

    return segments;
}

inline bool ReflectionFold::crosses_square(const Vector& rulings) const {

    if (!symmetry)
        return false;

    const Vector normal = symmetry->z_axis().normalized();

    return std::abs(rulings.normalized().dot(normal)) > 1.0 - 1e-9;
}

inline Xform ReflectionFold::carry(const std::vector<Segment>& segments, size_t j) const {

    if (j + 1 == segments.size() && crosses_square(segments[j].direction))
        return reflection(*symmetry);

    return Xform::project_to_plane_by_axis(segments[j].end, segments[j].direction);
}

inline std::vector<std::vector<NurbsCurve>> ReflectionFold::compute_creases(const std::vector<Segment>& segments) const {

    std::vector<std::vector<NurbsCurve>> creases = {profile};

    for (size_t j = 0; j < segments.size(); j++) {
        const Segment& segment = segments[j];
        const Vector end_normal = segment.end.z_axis().normalized();
        const bool reflects = j + 1 < segments.size() && folds[j].reflected;

        if (std::abs(segment.end.z_axis().dot(segment.direction)) < 1e-9)
            throw std::invalid_argument(fmt::format("The plane ending segment {} runs along its rulings", j));

        const Xform to_end = carry(segments, j);
        std::vector<NurbsCurve> next;

        for (size_t i = 0; i < profile.size(); i++) {
            const NurbsCurve& curve = creases[j][i];
            next.push_back(curve.transformed(to_end));

            // every ruling runs forward from this crease to the next, and a reflection's plane never stands on the sheet
            for (size_t k = 0; k <= samples; k++) {
                const double t = parameter(curve, k);
                const Point from = curve.point_at(t);
                const Point to = next.back().point_at(t);
                const double along = (to - from).dot(segment.direction);

                if (along < -1e-6)
                    throw std::invalid_argument(fmt::format("The plane ending segment {} lies behind its start at ruling {} of curve {}", j, k, i));

                const Vector sheet_normal = curve.tangent_at(t).cross(segment.direction).normalized();
                const double standing = std::abs(end_normal.dot(sheet_normal));

                if (reflects && standing < 0.05)
                    throw std::invalid_argument(fmt::format("Fold {} stands on the sheet at ruling {} of curve {}, where its reflection would be flat", j, k, i));
            }
        }

        creases.push_back(next);
    }

    return creases;
}

inline std::vector<NurbsCurve> ReflectionFold::compute_inner_profile(const Plane& first, double side) const {

    // the section square to the rulings, through the profile's first point, with a frame u, v in it
    const Plane section = Plane::from_point_normal(first.origin(), direction);
    const Vector u = section.x_axis().normalized();
    const Vector v = direction.cross(u).normalized();

    // offsets: every curve's section and its offset the thickness to the side
    Offsets offsets = compute_offsets(
        section,
        u,
        v,
        side
    );

    // seams: two offsets meeting at an angle end where they cross
    trim_seams(offsets);

    // free edges: the sheet's two ends cut square to its chord across the rulings, vertical for a level sheet
    const std::array<double, 2>& first_point = offsets.outer.front().front();
    const std::array<double, 2>& last_point = offsets.outer.back().back();
    const std::array<double, 2> chord = {last_point[0] - first_point[0], last_point[1] - first_point[1]};
    const std::array<double, 2> square = {-chord[1], chord[0]};
    meet_line(
        offsets.inner.front(),
        false,
        first_point,
        square,
        offsets.tangents.front()[0]
    );
    meet_line(
        offsets.inner.back(),
        true,
        last_point,
        square,
        offsets.tangents.back()[1]
    );

    // curves: every offset resampled at 16 samples + 1 equal steps of its length, so all share one knot vector, back on the profile's plane
    const Xform to_first = Xform::project_to_plane_by_axis(first, direction);
    std::vector<NurbsCurve> curves;

    for (const std::vector<std::array<double, 2>>& offset : offsets.inner) {
        std::vector<Point> points;

        for (const std::array<double, 2>& point : resampled(offset, 16 * samples)) {
            const Point lifted = section.origin() + u * point[0] + v * point[1];
            points.push_back(to_first.transform_point(lifted));
        }

        curves.push_back(NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform));
    }

    return curves;
}

inline ReflectionFold::Offsets ReflectionFold::compute_offsets(
    const Plane& section,
    const Vector& u,
    const Vector& v,
    double side
) const {

    // every sample moved the thickness to the side along the section's normal, dense enough to trim and resample on its chords
    const Xform to_section = Xform::project_to_plane_by_axis(section, direction);
    const size_t dense = 64 * samples;
    Offsets offsets;
    offsets.outer.resize(profile.size());
    offsets.inner.resize(profile.size());
    offsets.tangents.resize(profile.size());

    for (size_t i = 0; i < profile.size(); i++) {
        const NurbsCurve section_curve = profile[i].transformed(to_section);
        const double domain = section_curve.domain_end() - section_curve.domain_start();

        for (size_t k = 0; k <= dense; k++) {
            const double t = section_curve.domain_start() + domain * static_cast<double>(k) / static_cast<double>(dense);
            const Point point = section_curve.point_at(t);
            const Vector normal = section_curve.tangent_at(t).cross(direction).normalized();
            const Point moved = point - normal * (side * thickness);
            offsets.outer[i].push_back(in_frame(point - section.origin(), u, v));
            offsets.inner[i].push_back(in_frame(moved - section.origin(), u, v));
        }

        // the unit tangents at both ends
        const Point start = section.origin() + section_curve.tangent_at(section_curve.domain_start()).normalized();
        const Point end = section.origin() + section_curve.tangent_at(section_curve.domain_end()).normalized();
        offsets.tangents[i] = {in_frame(start - section.origin(), u, v), in_frame(end - section.origin(), u, v)};
    }

    return offsets;
}

inline void ReflectionFold::trim_seams(Offsets& offsets) const {

    for (size_t i = 0; i + 1 < profile.size(); i++) {
        // where the two curves meet tangent the offsets already share their end
        const std::array<double, 2>& before = offsets.tangents[i][1];
        const std::array<double, 2>& after = offsets.tangents[i + 1][0];
        const double sine = before[0] * after[1] - before[1] * after[0];
        const double cosine = before[0] * after[0] + before[1] * after[1];

        if (std::abs(sine) < 1e-3 && cosine > 0.0)
            continue;

        // the seam's line: from the cusp through the crossing of the two offsets, carried on along their end tangents
        const std::array<double, 2>& cusp = offsets.outer[i].back();
        const std::array<double, 2>& a = offsets.inner[i].back();
        const std::array<double, 2>& b = offsets.inner[i + 1].front();
        const std::optional<double> along = line_crossing(
            a,
            before,
            b,
            after
        );

        if (!along)
            continue;

        const std::array<double, 2> corner = {a[0] + before[0] * *along, a[1] + before[1] * *along};
        const std::array<double, 2> seam = {corner[0] - cusp[0], corner[1] - cusp[1]};
        meet_line(
            offsets.inner[i],
            true,
            cusp,
            seam,
            before
        );
        meet_line(
            offsets.inner[i + 1],
            false,
            cusp,
            seam,
            after
        );
    }
}

inline std::vector<std::vector<NurbsCurve>> ReflectionFold::compute_inners(
    const std::vector<Segment>& segments,
    const std::array<std::vector<NurbsCurve>, 2>& profiles
) const {

    // the two offsets carried as planar sections, and the material's side: under the sheet first, flipped at every reflection
    std::array<std::vector<NurbsCurve>, 2> sections = profiles;
    size_t side = 0;
    std::vector<std::vector<NurbsCurve>> inners = {profiles[0]};

    for (size_t j = 0; j < segments.size(); j++) {
        const Xform to_end = carry(segments, j);

        for (std::vector<NurbsCurve>& section : sections)
            for (NurbsCurve& curve : section)
                curve = curve.transformed(to_end);

        // a mirror fold, a sheet end or the symmetry plane: this segment's inner meridians carried along its rulings onto the plane
        const bool reflected = j + 1 < segments.size() && folds[j].reflected;

        if (!reflected) {
            std::vector<NurbsCurve> next;

            for (const NurbsCurve& curve : inners.back())
                next.push_back(curve.transformed(to_end));

            inners.push_back(next);
            continue;
        }

        // a reflection: every inner ruling runs on to the plate beyond's inner cylinder, on the other offset
        side = 1 - side;
        const std::vector<std::array<double, 2>> chain = compute_chain(sections[side], segments[j].end);
        const std::vector<NurbsCurve> crossing = compute_crossing(
            inners.back(),
            chain,
            segments[j],
            j
        );
        inners.push_back(crossing);
    }

    return inners;
}

inline std::vector<std::array<double, 2>> ReflectionFold::compute_chain(
    const std::vector<NurbsCurve>& section,
    const Plane& plane
) const {

    // the plane's frame u, w
    const Vector n = plane.z_axis().normalized();
    const Vector u = plane.x_axis().normalized();
    const Vector w = n.cross(u).normalized();

    // the section's samples across all rows, no two closer than a micrometre
    const size_t dense = 64 * samples;
    std::vector<std::array<double, 2>> chain;

    for (const NurbsCurve& curve : section) {
        const double domain = curve.domain_end() - curve.domain_start();

        for (size_t k = 0; k <= dense; k++) {
            const double t = curve.domain_start() + domain * static_cast<double>(k) / static_cast<double>(dense);
            const std::array<double, 2> point = in_frame(curve.point_at(t) - plane.origin(), u, w);

            if (chain.empty() || std::hypot(point[0] - chain.back()[0], point[1] - chain.back()[1]) > 1e-6)
                chain.push_back(point);
        }
    }

    // carried on past the sheet's two free edges along its end tangents
    const double far = 1000.0 * thickness;

    for (const bool last : {false, true}) {
        const std::array<double, 2>& end = last ? chain.back() : chain.front();
        const std::array<double, 2>& inside = last ? chain[chain.size() - 2] : chain[1];
        const double reach = std::hypot(end[0] - inside[0], end[1] - inside[1]);
        const std::array<double, 2> beyond = {
            end[0] + (end[0] - inside[0]) / reach * far,
            end[1] + (end[1] - inside[1]) / reach * far
        };

        if (last)
            chain.push_back(beyond);
        else
            chain.insert(chain.begin(), beyond);
    }

    return chain;
}

inline std::vector<NurbsCurve> ReflectionFold::compute_crossing(
    const std::vector<NurbsCurve>& inner,
    const std::vector<std::array<double, 2>>& chain,
    const Segment& segment,
    size_t j
) const {

    // the mirror in the plane, its frame u, w, and the image rulings' direction on it
    const Plane& plane = segment.end;
    const Xform mirror = reflection(plane);
    const Vector n = plane.z_axis().normalized();
    const Vector u = plane.x_axis().normalized();
    const Vector w = n.cross(u).normalized();
    const Vector image_rulings = mirror.transform_vector(segment.direction);
    const Vector along = image_rulings - segment.direction * (image_rulings.dot(n) / segment.direction.dot(n));
    const std::array<double, 2> image_direction = in_frame(along, u, w);
    const size_t count = 16 * samples;
    std::vector<NurbsCurve> crossing;

    for (const NurbsCurve& curve : inner) {
        const double domain = curve.domain_end() - curve.domain_start();
        std::vector<Point> points;

        for (size_t k = 0; k <= count; k++) {
            // the ruling's start, its distance to the plane, and its mirror image's line on the plane
            const double t = curve.domain_start() + domain * static_cast<double>(k) / static_cast<double>(count);
            const Point start = curve.point_at(t);
            const double expected = (plane.origin() - start).dot(n) / segment.direction.dot(n);
            const Point image = mirror.transform_point(start);
            const double reach = (plane.origin() - image).dot(n) / segment.direction.dot(n);
            const std::array<double, 2> origin = in_frame(image + segment.direction * reach - plane.origin(), u, w);
            std::optional<double> nearest;

            // the image line origin + s image_direction against every link of the chain, the crossing nearest the crease
            for (size_t c = 0; c + 1 < chain.size(); c++) {
                const std::array<double, 2> link = {chain[c + 1][0] - chain[c][0], chain[c + 1][1] - chain[c][1]};
                const double determinant = image_direction[0] * link[1] - image_direction[1] * link[0];

                if (std::abs(determinant) < 1e-12)
                    continue;

                const std::array<double, 2> gap = {chain[c][0] - origin[0], chain[c][1] - origin[1]};
                const double s = (gap[0] * link[1] - gap[1] * link[0]) / determinant;
                const double r = (gap[0] * image_direction[1] - gap[1] * image_direction[0]) / determinant;

                if (r >= 0.0 && r <= 1.0 && (!nearest || std::abs(s - expected) < std::abs(*nearest - expected)))
                    nearest = s;
            }

            if (!nearest)
                throw std::invalid_argument(fmt::format("The plates at the reflection after segment {} do not meet on ruling {}", j, k));

            points.push_back(start + segment.direction * *nearest);
        }

        crossing.push_back(NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform));
    }

    return crossing;
}

inline void ReflectionFold::add_creases(const std::vector<std::vector<NurbsCurve>>& creases) {

    const std::shared_ptr<TreeNode> group = add_group("creases");

    for (size_t j = 1; j + 1 < creases.size(); j++) {
        std::vector<Point> crease;

        for (const NurbsCurve& curve : creases[j])
            for (size_t k = 0; k <= samples; k++)
                crease.push_back(curve.point_at(parameter(curve, k)));

        Polyline drawn(crease);
        drawn.name = fmt::format("crease_{}", j);
        add_polyline(drawn, group);
    }
}

inline std::vector<Development> ReflectionFold::add_lamellas(
    const std::vector<std::vector<NurbsCurve>>& creases,
    const std::vector<std::vector<NurbsCurve>>& inners
) {

    const std::shared_ptr<TreeNode> lamellas = add_group("lamellas");
    std::vector<Development> developments;

    for (size_t j = 0; j + 1 < creases.size(); j++)
        for (size_t i = 0; i < profile.size(); i++) {
            // the outer meridians the two creases, the inner the shared ones on the same two cuts
            const NurbsCurve& outer_a = creases[j][i];
            const NurbsCurve& outer_b = creases[j + 1][i];
            const std::shared_ptr<PlateCurved> lamella = std::make_shared<PlateCurved>(
                outer_a,
                outer_b,
                inners[j][i],
                inners[j + 1][i],
                fmt::format("lamella_{}_{}", j, i)
            );
            add(lamella, lamellas);

            // its development, the proof it is cut flat
            developments.emplace_back(outer_a, outer_b, samples);
            _distortion = std::max(_distortion, developments.back().distortion);
            _twist = std::max(_twist, developments.back().twist);
        }

    return developments;
}

inline void ReflectionFold::add_flat(
    const std::vector<Development>& developments,
    const std::vector<Segment>& segments,
    const std::vector<std::vector<NurbsCurve>>& creases,
    double beside
) {

    const std::shared_ptr<TreeNode> flat = add_group("flat");

    // the pattern starts beside the model's largest x; its x runs along the profile's development, its y along the rulings
    double largest = -std::numeric_limits<double>::infinity();

    for (const std::vector<NurbsCurve>& crease : creases)
        for (const NurbsCurve& curve : crease)
            for (size_t k = 0; k <= samples; k++)
                largest = std::max(largest, curve.point_at(parameter(curve, k))[0]);

    // every profile curve's start along the development: the lengths of the curves before it across the rulings
    const Plane square = Plane::from_point_normal(Point(0.0, 0.0, 0.0), direction);
    const Xform across = Xform::project_to_plane_by_axis(square, direction);
    std::vector<double> starts = {0.0};

    for (const NurbsCurve& curve : profile)
        starts.push_back(starts.back() + curve.transformed(across).length());

    // every lamella's outline moved to its place: its row's start along x, the rulings run before its first crease along y
    size_t index = 0;

    for (size_t j = 0; j < segments.size(); j++)
        for (size_t i = 0; i < profile.size(); i++, index++) {
            double along = 0.0;

            for (size_t m = 0; m < j; m++) {
                const NurbsCurve& from = creases[m][i];
                const NurbsCurve& to = creases[m + 1][i];
                const Vector run = to.point_at(to.domain_start()) - from.point_at(from.domain_start());
                along += run.dot(segments[m].direction);
            }

            const Xform place = Xform::translation(largest + beside + starts[i], along, 0.0);
            Polyline outline = developments[index].outline.transformed(place);
            outline.name = fmt::format("flat_{}_{}", j, i);
            add_polyline(outline, flat);
        }
}

inline void ReflectionFold::add_mirrored(const std::vector<std::vector<NurbsCurve>>& creases, bool crown) {

    // segment s mirrored is segment beyond - s; at a crown the last segment already runs across the plane
    const size_t folded = creases.size() - 1;
    const size_t crossing = folded - 1;
    const size_t beyond = crown ? 2 * folded - 2 : 2 * folded - 1;

    // lamellas: the mirror image of every plate, the pair at the symmetry plane sharing its side face there
    add_mirrored_lamellas(beyond, crown, crossing);

    // creases: the symmetry plane's own section of the sheet unless a plate runs across it, then every crease mirrored
    add_mirrored_creases(creases, beyond, crown);

    // flat: the mirrored half's developments beyond the first's, turned over so the two halves meet along the plane's section
    add_mirrored_flat(beyond, crown, crossing);
}

inline void ReflectionFold::add_mirrored_lamellas(size_t beyond, bool crown, size_t crossing) {

    const Xform mirror = reflection(*symmetry);
    const std::shared_ptr<TreeNode> lamellas = group_named("lamellas");
    const std::vector<std::shared_ptr<PlateCurved>> plates = get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& plate : plates) {
        const size_t underscore = plate->name.find('_', 8);
        const size_t segment = std::stoul(plate->name.substr(8, underscore - 8));
        const std::string row = plate->name.substr(underscore + 1);

        if (crown && segment == crossing)
            continue;

        const std::shared_ptr<PlateCurved> image = std::make_shared<PlateCurved>(
            plate->outer_a.transformed(mirror),
            plate->outer_b.transformed(mirror),
            plate->inner_a.transformed(mirror),
            plate->inner_b.transformed(mirror),
            fmt::format("lamella_{}_{}", beyond - segment, row)
        );
        add(image, lamellas);
    }
}

inline void ReflectionFold::add_mirrored_creases(const std::vector<std::vector<NurbsCurve>>& creases, size_t beyond, bool crown) {

    const Xform mirror = reflection(*symmetry);
    const size_t folded = creases.size() - 1;
    const std::shared_ptr<TreeNode> group = group_named("creases");
    std::vector<Polyline> folded_creases;

    for (const std::shared_ptr<Polyline>& polyline : *objects.polylines)
        if (polyline && polyline->name.rfind("crease_", 0) == 0)
            folded_creases.push_back(*polyline);

    // the symmetry plane's own section of the sheet
    if (!crown) {
        std::vector<Point> ridge;

        for (const NurbsCurve& curve : creases.back())
            for (size_t k = 0; k <= samples; k++)
                ridge.push_back(curve.point_at(parameter(curve, k)));

        Polyline drawn(ridge);
        drawn.name = fmt::format("crease_{}", folded);
        add_polyline(drawn, group);
    }

    // every crease mirrored
    for (const Polyline& crease : folded_creases) {
        Polyline mirrored = crease.transformed(mirror);
        mirrored.name = fmt::format("crease_{}", beyond + 1 - std::stoul(crease.name.substr(7)));
        add_polyline(mirrored, group);
    }
}

inline void ReflectionFold::add_mirrored_flat(size_t beyond, bool crown, size_t crossing) {

    // the first half's outlines and their top; at a crown the middle of the crossing plates' first ruling instead
    const std::shared_ptr<TreeNode> flat = group_named("flat");
    std::vector<Polyline> outlines;
    double top = -std::numeric_limits<double>::infinity();
    std::optional<double> middle;

    for (const std::shared_ptr<Polyline>& polyline : *objects.polylines)
        if (polyline && polyline->name.rfind("flat_", 0) == 0) {
            const size_t underscore = polyline->name.find('_', 5);
            const size_t segment = std::stoul(polyline->name.substr(5, underscore - 5));

            if (crown && segment == crossing) {
                const double first = polyline->get_point(0)[1];
                const double last = polyline->get_point(polyline->point_count() - 2)[1];
                middle = 0.5 * (first + last);
                continue;
            }

            outlines.push_back(*polyline);

            for (size_t k = 0; k < polyline->point_count(); k++)
                top = std::max(top, polyline->get_point(k)[1]);
        }

    // every outline turned over about the line the two halves meet on
    const double hinge = middle ? *middle : top + 0.05 * length;
    const Xform turn_over = Xform::scale_non_uniform(
        Point(0.0, hinge, 0.0),
        1.0,
        -1.0,
        1.0
    );

    for (const Polyline& outline : outlines) {
        const size_t underscore = outline.name.find('_', 5);
        const size_t segment = std::stoul(outline.name.substr(5, underscore - 5));
        Polyline mirrored = outline.transformed(turn_over);
        mirrored.name = fmt::format("flat_{}_{}", beyond - segment, outline.name.substr(underscore + 1));
        add_polyline(mirrored, flat);
    }
}

inline Xform ReflectionFold::reflection(const Plane& plane) {

    const Vector n = plane.z_axis().normalized();
    const Point origin = plane.origin();
    const double h = n.dot(Vector(origin[0], origin[1], origin[2]));
    std::array<double, 16> m = {};

    for (int column = 0; column < 3; column++)
        for (int row = 0; row < 3; row++)
            m[column * 4 + row] = (row == column ? 1.0 : 0.0) - 2.0 * n[row] * n[column];

    for (int row = 0; row < 3; row++)
        m[12 + row] = 2.0 * h * n[row];

    m[15] = 1.0;

    return Xform::from_matrix(m);
}

inline Vector ReflectionFold::turned(const Vector& vector, const Vector& axis, double angle) {

    const Vector k = axis.normalized();
    const Vector parallel = k * (k.dot(vector) * (1.0 - std::cos(angle)));

    return vector * std::cos(angle) + k.cross(vector) * std::sin(angle) + parallel;
}

inline std::array<double, 2> ReflectionFold::in_frame(const Vector& offset, const Vector& u, const Vector& v) {

    return std::array<double, 2>{offset.dot(u), offset.dot(v)};
}

inline std::optional<double> ReflectionFold::line_crossing(
    const std::array<double, 2>& a,
    const std::array<double, 2>& da,
    const std::array<double, 2>& b,
    const std::array<double, 2>& db
) {

    const double determinant = da[0] * db[1] - da[1] * db[0];

    if (std::abs(determinant) < 1e-12)
        return std::nullopt;

    return ((b[0] - a[0]) * db[1] - (b[1] - a[1]) * db[0]) / determinant;
}

inline void ReflectionFold::meet_line(
    std::vector<std::array<double, 2>>& offset,
    bool last,
    const std::array<double, 2>& a,
    const std::array<double, 2>& da,
    const std::array<double, 2>& tangent
) {

    // the first crossing walking in from the end or the start, the offset trimmed there
    const size_t m = offset.size();

    for (size_t step = 0; step + 1 < m / 2; step++) {
        const std::array<double, 2>& p = offset[last ? m - 1 - step : step];
        const std::array<double, 2>& q = offset[last ? m - 2 - step : step + 1];
        const std::array<double, 2> pq = {q[0] - p[0], q[1] - p[1]};
        const std::optional<double> along = line_crossing(
            p,
            pq,
            a,
            da
        );

        if (along && *along >= 0.0 && *along <= 1.0) {
            const std::array<double, 2> point = {p[0] + pq[0] * *along, p[1] + pq[1] * *along};

            if (last) {
                offset.resize(m - 1 - step);
                offset.push_back(point);
            } else {
                offset.erase(offset.begin(), offset.begin() + static_cast<long>(step) + 1);
                offset.insert(offset.begin(), point);
            }

            return;
        }
    }

    // none: the offset carried on past its end along its end tangent to the line
    const std::array<double, 2>& end = last ? offset.back() : offset.front();
    const std::optional<double> along = line_crossing(
        end,
        tangent,
        a,
        da
    );

    if (!along)
        return;

    const std::array<double, 2> point = {end[0] + tangent[0] * *along, end[1] + tangent[1] * *along};

    if (last)
        offset.push_back(point);
    else
        offset.insert(offset.begin(), point);
}

inline std::vector<std::array<double, 2>> ReflectionFold::resampled(
    const std::vector<std::array<double, 2>>& offset,
    size_t count
) {

    // the length along the chain at every point
    std::vector<double> lengths = {0.0};

    for (size_t k = 1; k < offset.size(); k++) {
        const double link_length = std::hypot(offset[k][0] - offset[k - 1][0], offset[k][1] - offset[k - 1][1]);
        lengths.push_back(lengths.back() + link_length);
    }

    // count equal steps of the length, each interpolated on the link it falls in
    std::vector<std::array<double, 2>> points;
    size_t link = 0;

    for (size_t k = 0; k <= count; k++) {
        const double distance = lengths.back() * static_cast<double>(k) / static_cast<double>(count);

        while (link + 2 < lengths.size() && lengths[link + 1] < distance)
            link++;

        const double span = std::max(lengths[link + 1] - lengths[link], 1e-12);
        const double f = std::clamp((distance - lengths[link]) / span, 0.0, 1.0);
        const std::array<double, 2>& from = offset[link];
        const std::array<double, 2>& to = offset[link + 1];
        points.push_back({from[0] + (to[0] - from[0]) * f, from[1] + (to[1] - from[1]) * f});
    }

    return points;
}

inline std::vector<NurbsCurve> ReflectionFold::arcs(
    const Point& start,
    const Vector& across,
    const Vector& up,
    size_t count,
    double width,
    double rise
) {

    const Vector a = across.normalized();
    const Vector u = up.normalized();
    std::vector<NurbsCurve> curves;

    for (size_t i = 0; i < count; i++) {
        const Point from = start + a * (width * static_cast<double>(i));
        const Point top = from + a * (0.5 * width) + u * rise;
        const Point to = from + a * width;
        curves.push_back(Primitives::arc(from, top, to));
    }

    return curves;
}

inline std::vector<NurbsCurve> ReflectionFold::waves(
    const Point& start,
    const Vector& across,
    const Vector& up,
    size_t half_waves,
    double half_wavelength,
    double amplitude
) {

    const Vector a = across.normalized();
    const Vector u = up.normalized();
    const size_t points = 12;
    std::vector<NurbsCurve> curves;

    for (size_t i = 0; i < half_waves; i++) {
        const double sign = i % 2 == 0 ? 1.0 : -1.0;
        std::vector<Point> wave;

        for (size_t k = 0; k <= points; k++) {
            const double s = static_cast<double>(k) / static_cast<double>(points);
            const Vector run = a * (half_wavelength * (static_cast<double>(i) + s));
            const Vector lift = u * (sign * amplitude * std::sin(Tolerance::PI * s));
            wave.push_back(start + run + lift);
        }

        curves.push_back(NurbsCurve::create_interpolated(wave));
    }

    return curves;
}
