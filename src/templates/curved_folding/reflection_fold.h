#pragma once
#include "wood_session.h"
#include "development.h"
#include "primitives.h"

using namespace session_cpp;
using namespace wood_session;

/// A curved folding of a cylindrical sheet, the construction of the IBOIS curved folded plates and of the linear structure systems of
/// folded cylindrical surfaces, frames and arches. The sheet is a planar profile swept along one direction, a cylinder; at every fold its rulings turn about the folds' axis by the fold's angle, and the crease is the
/// section of the sheet by the fold's plane, a planar curve, an ellipse arc when the profile is an arc. A fold is made in one of two
/// ways, both with a planar crease. The reflection: the part of the sheet beyond the plane is mirrored in it, the plane bisecting
/// the angle between the old rulings and the new, so the sheet is turned over, its bulge flips from a vault to a trough, and the whole
/// sheet keeps one development; the crease is the long oval of a sharp kink. The mirror fold: the next
/// segment is the mirror image of the one before in the plane bisecting the fold's angle, so the bulge is kept, a vault folds onto a
/// vault; every segment is still an exact cylinder cut flat on its own, but the two sides of the crease
/// develop as mirror images, not as one sheet. A frame's ridge or an arch's crown is the symmetry plane: the sheet ends on it and the
/// folded half is mirrored in it, meeting itself along the plane's section, straight in plan.
///
/// Every part of the sheet between two creases, and between two consecutive curves of the profile, is one lamella, a PlateCurved: its
/// outer face the exact cylinder between the two creases, the second the first projected along the rulings onto the next plane. The
/// thickness is the sheet's own: the profile's section square to the rulings is offset once, every curve along its normal, the offsets
/// of two curves meeting at an angle ending where they cross, of two meeting tangent sharing their end, and the sheet's free edges cut
/// square to its chord, to either side of the sheet, and both are carried from plane to plane along the rulings exactly as the creases
/// are. The material lies under the sheet in the first segment; a reflection turns the sheet over, so beyond it the material lies on
/// the other offset, a mirror fold keeps it. At a mirror fold the two plates are each other's mirror image on either side of the plane,
/// so they meet on the plane: a planar mitre. At a reflection both lie on one side of it, so every inner ruling runs on to the inner
/// cylinder beyond: the mitre is the ruled face between the crease and that crossing, shared by the two. Every seam between
/// two rows is the plane through the rulings, and the lamellas tile the folded slab with no gap.
/// Every lamella develops flat without distortion, being a cylinder strip; its development is checked and drawn in the flat pattern
/// beside the model, every lamella where it lies on the unrolled sheet. A column is the sheet itself: a reflection whose plane
/// crosses the first segment higher in the middle of a barrel than at its cusps leaves a leg under every barrel that rises as an arch
/// to the crease, the columns under the troughs of the arch with a top hinge.
///
/// Fields: the profile, the rulings' direction and length, the folds' axis, the folds, the symmetry plane, the thickness and the
/// sampling; the creases, the lamellas and the flat pattern are in the session.
class ReflectionFold : public WoodSession {
public:
    /// One fold of the sheet: where the rulings turn, by how much, and how.
    struct Fold {
        double station; // mm along the rulings from the sheet's start, on the ruling through the profile's first point.
        double angle; // radians, the rulings' turn about the axis, positive by the right hand.
        bool reflected = true; // the reflection, the sheet turned over, when true; the mirror fold, the bulge kept, when false.
    };

    const std::vector<NurbsCurve> profile; // The sheet's section: planar curves one after another sharing their ends, running one way, each a row of lamellas.
    const Vector direction; // The rulings' unit direction before the first fold, the profile swept along it.
    const Vector axis; // The folds' unit axis, in every fold plane; the profile's plane is spanned by it and the sheet's width.
    const double length; // mm, the sheet along the rulings from its start to its end, when no symmetry plane ends it.
    const std::vector<Fold> folds; // The folds, in order along the rulings.
    const std::optional<Plane> symmetry; // The folded sheet's plane of symmetry, where the sheet ends and its mirror image begins; none for a sheet with two free ends.
    const double thickness; // mm, every lamella, on the sheet's physical underside: the side away from the profile's tangent crossed with the direction, kept through every fold.
    const size_t samples; // Rulings per profile curve the creases are checked on; the offsets and mitres are interpolated through 16 times as many.

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

    /// A profile of count circular arcs side by side from start along across, each width wide and bulging rise toward up: rows of barrels.
    static std::vector<NurbsCurve> arcs(const Point& start, const Vector& across, const Vector& up, size_t count, double width, double rise);

    /// An undulating profile from start along across: half_waves half sine waves, each half_wavelength long and amplitude high toward up, first up: a corrugation.
    static std::vector<NurbsCurve> waves(const Point& start, const Vector& across, const Vector& up, size_t half_waves, double half_wavelength, double amplitude);

private:
    /// One straight run of the sheet between two creases: its rulings and the planes it starts and ends on.
    struct Segment {
        Vector direction; // The rulings' unit direction.
        Plane start; // The plane the segment's first crease lies in.
        Plane end; // The plane its last crease lies in.
    };

    double _distortion = 0.0;
    double _twist = 0.0;

    /// The profile's plane, from its most different tangents, through its first point.
    Plane compute_profile_plane() const;

    /// Every segment from the profile's plane to the end: at every fold the rulings turned about the axis, the fold's plane through the
    /// station on the ruling from the profile's first point, bisecting the turn for a mirror fold and perpendicular to that for a
    /// reflection; the end the symmetry plane or the profile's plane moved by length.
    std::vector<Segment> compute_segments(const Plane& first) const;

    /// Whether rulings in this direction cross the symmetry plane square: the segment and its mirror image are then one cylinder.
    bool crosses_square(const Vector& rulings) const;

    /// What carries segment j's sections from its start to its end: the projection along its rulings onto its end plane, or at a
    /// crown, the last segment's rulings crossing the symmetry plane square, the mirror in that plane, the segment running on across
    /// it to the mirror image of its start, one cylinder.
    Xform carry(const std::vector<Segment>& segments, size_t j) const;

    /// Every crease, curve by curve of the profile: the profile on the first plane, then each crease projected along its segment's
    /// rulings onto the next plane; checked to lie beyond the one before along every ruling, and no reflection's plane to stand on
    /// the sheet, where its fold would be flat and the sheet would cross itself.
    std::vector<std::vector<NurbsCurve>> compute_creases(const std::vector<Segment>& segments) const;

    /// An inner profile on the profile's plane: the profile's section square to the rulings offset the thickness to one side of the sheet,
    /// side 1 under it and -1 over it, every curve along its normal, the offsets of two curves meeting at an angle ending where they cross on the seam through
    /// the cusp, of two meeting tangent sharing their end, the first and the last cut square to the section's chord at the sheet's free
    /// edges; carried back onto the profile's plane along the rulings.
    std::vector<NurbsCurve> compute_inner_profile(const Plane& first, double side) const;

    /// The inner meridians on every cut, the one curve the two plates there share. The material lies under the sheet in the first
    /// segment and changes side at every reflection, which turns the sheet over, not at a mirror fold; both offsets are carried from
    /// plane to plane along the rulings as the creases are. At a mirror fold, a sheet end and the symmetry plane the plates lie on
    /// either side of the plane and meet on it; at a reflection both lie on one side, and every inner ruling of the plate before runs on
    /// to the plate beyond's inner cylinder, the mirror image of the other offset's.
    std::vector<std::vector<NurbsCurve>> compute_inners(const std::vector<Segment>& segments, const std::array<std::vector<NurbsCurve>, 2>& profiles) const;

    /// Every crease on the folded sheet, crease_<j>, in the group creases.
    void add_creases(const std::vector<std::vector<NurbsCurve>>& creases);

    /// Every lamella, lamella_<segment>_<row>, in the group lamellas; its development returned for the flat pattern.
    std::vector<Development> add_lamellas(const std::vector<std::vector<NurbsCurve>>& creases, const std::vector<std::vector<NurbsCurve>>& inners);

    /// The flat cutting pattern beside the model, every lamella's development where it lies on the unrolled sheet, in the group flat.
    void add_flat(const std::vector<Development>& developments, const std::vector<Segment>& segments, const std::vector<std::vector<NurbsCurve>>& creases, double beside);

    /// The mirror image of every lamella and crease in the symmetry plane, the plane's own section of the sheet as the last crease,
    /// and the mirrored half's cutting pattern beyond the first's; at a crown the last segment's plates already run across the plane,
    /// so they are not mirrored and the plane is no crease.
    void add_mirrored(const std::vector<std::vector<NurbsCurve>>& creases, bool crown);

    /// The reflection in the plane.
    static Xform reflection(const Plane& plane);

    /// The vector turned about axis by angle (radians).
    static Vector turned(const Vector& vector, const Vector& axis, double angle);

    /// The curve's point at the k-th of samples equal parameter steps.
    double parameter(const NurbsCurve& curve, size_t k) const;
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
    const std::array<std::vector<NurbsCurve>, 2> profiles = {compute_inner_profile(first, 1.0), compute_inner_profile(first, -1.0)};

    // inners: the inner profile carried from plane to plane along the rulings, on every cut the one curve the two plates there share
    const std::vector<std::vector<NurbsCurve>> inners = compute_inners(segments, profiles);

    // creases drawn, then the lamellas: between two creases and two profile curves one curved plate, its inner meridians the shared ones
    add_creases(creases);
    const std::vector<Development> developments = add_lamellas(creases, inners);

    // flat: the cutting pattern beside the model, every lamella unfolded by its rulings
    add_flat(developments, segments, creases, 0.1 * length);

    // symmetry: the folded half mirrored in its plane of symmetry, meeting itself along the plane's section of the sheet, or at a
    // crown, where the last segment's rulings cross the plane square, running on in the last segment's plates, one across the plane
    if (symmetry)
        add_mirrored(creases, crosses_square(segments.back().direction));

    // contacts: every two lamellas that share a face, found by the session's contact detection
    compute_face_contacts(0);
}

inline double ReflectionFold::parameter(const NurbsCurve& curve, size_t k) const {
    return curve.domain_start() + (curve.domain_end() - curve.domain_start()) * static_cast<double>(k) / static_cast<double>(samples);
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

    return Plane::from_point_normal(profile.front().point_at(profile.front().domain_start()), normal);
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
    if (symmetry)
        segments.push_back({rulings, start, *symmetry});
    else
        segments.push_back({rulings, start, Plane::from_point_normal(station + rulings * (length - before), rulings)});

    return segments;
}

inline bool ReflectionFold::crosses_square(const Vector& rulings) const {

    return symmetry && std::abs(rulings.normalized().dot(symmetry->z_axis().normalized())) > 1.0 - 1e-9;
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

        if (std::abs(segment.end.z_axis().dot(segment.direction)) < 1e-9)
            throw std::invalid_argument(fmt::format("The plane ending segment {} runs along its rulings", j));

        const Xform to_end = carry(segments, j);
        std::vector<NurbsCurve> next;

        for (size_t i = 0; i < profile.size(); i++) {
            next.push_back(creases[j][i].transformed(to_end));

            // every ruling runs forward from this crease to the next, and a reflection's plane never stands on the sheet
            for (size_t k = 0; k <= samples; k++) {
                const double t = parameter(creases[j][i], k);
                const Point from = creases[j][i].point_at(t);
                const Point to = next.back().point_at(t);
                const double along = (to - from).dot(segment.direction);

                if (along < -1e-6)
                    throw std::invalid_argument(fmt::format("The plane ending segment {} lies behind its start at ruling {} of curve {}", j, k, i));

                const Vector sheet_normal = creases[j][i].tangent_at(t).cross(segment.direction).normalized();

                if (j + 1 < segments.size() && folds[j].reflected && std::abs(segment.end.z_axis().normalized().dot(sheet_normal)) < 0.05)
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
    const Xform to_section = Xform::project_to_plane_by_axis(section, direction);
    const Vector u = section.x_axis().normalized();
    const Vector v = direction.cross(u).normalized();
    const std::function<std::array<double, 2>(const Point&)> flat = [&](const Point& point) {
        const Vector offset = point - section.origin();
        return std::array<double, 2>{offset.dot(u), offset.dot(v)};
    };
    const std::function<Point(const std::array<double, 2>&)> lifted = [&](const std::array<double, 2>& point) {
        return section.origin() + u * point[0] + v * point[1];
    };

    // where the lines through a along da and through b along db cross, as the distance along the first
    const std::function<std::optional<double>(const std::array<double, 2>&, const std::array<double, 2>&, const std::array<double, 2>&, const std::array<double, 2>&)> cross = [](const std::array<double, 2>& a, const std::array<double, 2>& da, const std::array<double, 2>& b, const std::array<double, 2>& db) -> std::optional<double> {
        const double determinant = da[0] * db[1] - da[1] * db[0];

        if (std::abs(determinant) < 1e-12)
            return std::nullopt;

        return ((b[0] - a[0]) * db[1] - (b[1] - a[1]) * db[0]) / determinant;
    };

    // every curve's section and its offset: every sample moved the thickness to the underside along the section's normal, sampled
    // densely enough that the chords the offset is trimmed and resampled on lie within a micrometre of it
    const size_t dense = 64 * samples;
    std::vector<std::vector<std::array<double, 2>>> outer(profile.size());
    std::vector<std::vector<std::array<double, 2>>> inner(profile.size());
    std::vector<std::array<std::array<double, 2>, 2>> tangents(profile.size());

    for (size_t i = 0; i < profile.size(); i++) {
        const NurbsCurve section_curve = profile[i].transformed(to_section);

        for (size_t k = 0; k <= dense; k++) {
            const double t = section_curve.domain_start() + (section_curve.domain_end() - section_curve.domain_start()) * static_cast<double>(k) / static_cast<double>(dense);
            const Point point = section_curve.point_at(t);
            const Vector normal = section_curve.tangent_at(t).cross(direction).normalized();
            outer[i].push_back(flat(point));
            inner[i].push_back(flat(point - normal * (side * thickness)));
        }

        const std::array<double, 2> start = flat(section.origin() + section_curve.tangent_at(section_curve.domain_start()).normalized());
        const std::array<double, 2> end = flat(section.origin() + section_curve.tangent_at(section_curve.domain_end()).normalized());
        tangents[i] = {start, end};
    }

    // where an offset meets a line through a along da: the first crossing walking in from its end (last) or its start, the offset
    // trimmed there; past its end, the offset carried on along its end tangent to the line
    const std::function<void(std::vector<std::array<double, 2>>&, bool, const std::array<double, 2>&, const std::array<double, 2>&, const std::array<double, 2>&)> meet_line = [&](std::vector<std::array<double, 2>>& offset, bool last, const std::array<double, 2>& a, const std::array<double, 2>& da, const std::array<double, 2>& tangent) {
        const size_t m = offset.size();

        for (size_t step = 0; step + 1 < m / 2; step++) {
            const std::array<double, 2>& p = offset[last ? m - 1 - step : step];
            const std::array<double, 2>& q = offset[last ? m - 2 - step : step + 1];
            const std::array<double, 2> pq = {q[0] - p[0], q[1] - p[1]};
            const std::optional<double> along = cross(p, pq, a, da);

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

        const std::array<double, 2>& end = last ? offset.back() : offset.front();
        const std::optional<double> along = cross(end, tangent, a, da);

        if (!along)
            return;

        const std::array<double, 2> point = {end[0] + tangent[0] * *along, end[1] + tangent[1] * *along};

        if (last)
            offset.push_back(point);
        else
            offset.insert(offset.begin(), point);
    };

    // between two curves meeting at an angle, both offsets end where they cross: the plane through the cusp, the rulings and that
    // point is the seam between the two rows; where they meet tangent the offsets already share their end
    for (size_t i = 0; i + 1 < profile.size(); i++) {
        const std::array<double, 2>& before = tangents[i][1];
        const std::array<double, 2>& after = tangents[i + 1][0];

        if (std::abs(before[0] * after[1] - before[1] * after[0]) < 1e-3 && before[0] * after[0] + before[1] * after[1] > 0.0)
            continue;

        // the seam's line: from the cusp through the crossing of the two offsets, carried on along their end tangents
        const std::array<double, 2>& cusp = outer[i].back();
        const std::array<double, 2>& a = inner[i].back();
        const std::array<double, 2>& b = inner[i + 1].front();
        const std::optional<double> along = cross(a, before, b, after);

        if (!along)
            continue;

        const std::array<double, 2> corner = {a[0] + before[0] * *along, a[1] + before[1] * *along};
        const std::array<double, 2> seam = {corner[0] - cusp[0], corner[1] - cusp[1]};
        meet_line(inner[i], true, cusp, seam, before);
        meet_line(inner[i + 1], false, cusp, seam, after);
    }

    // the sheet's two free edges cut square to its chord across the rulings, vertical for a level sheet
    const std::array<double, 2> chord = {outer.back().back()[0] - outer.front().front()[0], outer.back().back()[1] - outer.front().front()[1]};
    const std::array<double, 2> square = {-chord[1], chord[0]};
    meet_line(inner.front(), false, outer.front().front(), square, tangents.front()[0]);
    meet_line(inner.back(), true, outer.back().back(), square, tangents.back()[1]);

    // every offset back on the profile's plane along the rulings
    const Xform to_first = Xform::project_to_plane_by_axis(first, direction);
    std::vector<NurbsCurve> curves;

    for (const std::vector<std::array<double, 2>>& offset : inner) {
        // resampled at 16 samples + 1 equal steps of its length, so every inner curve, carried or crossed, has one knot vector
        std::vector<double> lengths = {0.0};

        for (size_t k = 1; k < offset.size(); k++)
            lengths.push_back(lengths.back() + std::hypot(offset[k][0] - offset[k - 1][0], offset[k][1] - offset[k - 1][1]));

        std::vector<Point> points;
        size_t link = 0;

        for (size_t k = 0; k <= 16 * samples; k++) {
            const double length = lengths.back() * static_cast<double>(k) / static_cast<double>(16 * samples);

            while (link + 2 < lengths.size() && lengths[link + 1] < length)
                link++;

            const double f = std::clamp((length - lengths[link]) / std::max(lengths[link + 1] - lengths[link], 1e-12), 0.0, 1.0);
            const std::array<double, 2> point = {offset[link][0] + (offset[link + 1][0] - offset[link][0]) * f, offset[link][1] + (offset[link + 1][1] - offset[link][1]) * f};
            points.push_back(to_first.transform_point(lifted(point)));
        }

        curves.push_back(NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform));
    }

    return curves;
}

inline std::vector<std::vector<NurbsCurve>> ReflectionFold::compute_inners(const std::vector<Segment>& segments, const std::array<std::vector<NurbsCurve>, 2>& profiles) const {

    // the two offsets carried as planar sections from plane to plane, and the side the material lies on in each segment: the first
    // segment under the sheet, flipped at every reflection, which turns the sheet over, kept at a mirror fold
    std::array<std::vector<NurbsCurve>, 2> sections = profiles;
    size_t side = 0;
    std::vector<std::vector<NurbsCurve>> inners = {profiles[0]};

    for (size_t j = 0; j < segments.size(); j++) {
        const Segment& segment = segments[j];
        const Xform to_end = carry(segments, j);

        for (std::vector<NurbsCurve>& section : sections)
            for (NurbsCurve& curve : section)
                curve = curve.transformed(to_end);

        // a mirror fold, a sheet end or the symmetry plane: the two plates are each other's mirror image on either side of the plane
        // and meet on it; this segment's inner meridians carried along its rulings onto it
        const bool reflected = j + 1 < segments.size() && folds[j].reflected;

        if (!reflected) {
            std::vector<NurbsCurve> next;

            for (const NurbsCurve& curve : inners.back())
                next.push_back(curve.transformed(to_end));

            inners.push_back(next);
            continue;
        }

        // a reflection: the plate beyond is the mirror image of this one's continuation, its material on the other offset, and both
        // lie on one side of the plane; every inner ruling of this plate runs on to the plate beyond's inner cylinder, which it meets
        // where its mirror image meets the other offset's cylinder, where the image carried along the rulings onto the plane crosses
        // that offset's section, a straight line across the plane
        side = 1 - side;
        const Xform mirror = reflection(segment.end);
        const Vector n = segment.end.z_axis().normalized();
        const Vector u = segment.end.x_axis().normalized();
        const Vector w = n.cross(u).normalized();
        const std::function<std::array<double, 2>(const Vector&)> flat = [&](const Vector& offset) {
            return std::array<double, 2>{offset.dot(u), offset.dot(w)};
        };
        const std::function<Vector(const Vector&)> along_rulings = [&](const Vector& vector) {
            return vector - segment.direction * (vector.dot(n) / segment.direction.dot(n));
        };

        // the other offset's section as one dense chain across all rows, carried on past the sheet's two free edges along its end tangents
        const size_t dense = 64 * samples;
        std::vector<std::array<double, 2>> chain;

        for (const NurbsCurve& curve : sections[side])
            for (size_t k = 0; k <= dense; k++) {
                const std::array<double, 2> point = flat(curve.point_at(curve.domain_start() + (curve.domain_end() - curve.domain_start()) * static_cast<double>(k) / static_cast<double>(dense)) - segment.end.origin());

                if (chain.empty() || std::hypot(point[0] - chain.back()[0], point[1] - chain.back()[1]) > 1e-6)
                    chain.push_back(point);
            }

        const double far = 1000.0 * thickness;

        for (const bool last : {false, true}) {
            const std::array<double, 2>& end = last ? chain.back() : chain.front();
            const std::array<double, 2>& inside = last ? chain[chain.size() - 2] : chain[1];
            const double length = std::hypot(end[0] - inside[0], end[1] - inside[1]);
            const std::array<double, 2> beyond = {end[0] + (end[0] - inside[0]) / length * far, end[1] + (end[1] - inside[1]) / length * far};

            if (last)
                chain.push_back(beyond);
            else
                chain.insert(chain.begin(), beyond);
        }

        const std::array<double, 2> image_direction = flat(along_rulings(mirror.transform_vector(segment.direction)));
        std::vector<NurbsCurve> crossing;

        for (const NurbsCurve& curve : inners.back()) {
            std::vector<Point> points;

            for (size_t k = 0; k <= 16 * samples; k++) {
                const Point start = curve.point_at(curve.domain_start() + (curve.domain_end() - curve.domain_start()) * static_cast<double>(k) / static_cast<double>(16 * samples));
                const double expected = (segment.end.origin() - start).dot(n) / segment.direction.dot(n);
                const Point image = mirror.transform_point(start);
                const std::array<double, 2> origin = flat(image + segment.direction * ((segment.end.origin() - image).dot(n) / segment.direction.dot(n)) - segment.end.origin());
                std::optional<double> nearest;

                // the image line origin + s image_direction against every link of the chain, the crossing nearest the crease
                for (size_t c = 0; c + 1 < chain.size(); c++) {
                    const std::array<double, 2> link = {chain[c + 1][0] - chain[c][0], chain[c + 1][1] - chain[c][1]};
                    const double determinant = image_direction[0] * link[1] - image_direction[1] * link[0];

                    if (std::abs(determinant) < 1e-12)
                        continue;

                    const std::array<double, 2> gap = {chain[c][0] - origin[0], chain[c][1] - origin[1]};
                    const double s = (gap[0] * link[1] - gap[1] * link[0]) / determinant;
                    const double t = (gap[0] * image_direction[1] - gap[1] * image_direction[0]) / determinant;

                    if (t >= 0.0 && t <= 1.0 && (!nearest || std::abs(s - expected) < std::abs(*nearest - expected)))
                        nearest = s;
                }

                if (!nearest)
                    throw std::invalid_argument(fmt::format("The plates at the reflection after segment {} do not meet on ruling {}", j, k));

                points.push_back(start + segment.direction * *nearest);
            }

            crossing.push_back(NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform));
        }

        inners.push_back(crossing);
    }

    return inners;
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

inline std::vector<Development> ReflectionFold::add_lamellas(const std::vector<std::vector<NurbsCurve>>& creases, const std::vector<std::vector<NurbsCurve>>& inners) {

    const std::shared_ptr<TreeNode> lamellas = add_group("lamellas");
    std::vector<Development> developments;

    for (size_t j = 0; j + 1 < creases.size(); j++)
        for (size_t i = 0; i < profile.size(); i++) {
            // the outer meridians the two creases, the inner the shared ones on the same two cuts
            const NurbsCurve& outer_a = creases[j][i];
            const NurbsCurve& outer_b = creases[j + 1][i];
            add(std::make_shared<PlateCurved>(outer_a, outer_b, inners[j][i], inners[j + 1][i], fmt::format("lamella_{}_{}", j, i)), lamellas);

            // its development, the proof it is cut flat
            developments.emplace_back(outer_a, outer_b, samples);
            _distortion = std::max(_distortion, developments.back().distortion);
            _twist = std::max(_twist, developments.back().twist);
        }

    return developments;
}

inline void ReflectionFold::add_flat(const std::vector<Development>& developments, const std::vector<Segment>& segments, const std::vector<std::vector<NurbsCurve>>& creases, double beside) {

    const std::shared_ptr<TreeNode> flat = add_group("flat");

    // the pattern starts beside the model's largest x; its x runs along the profile's development, its y along the rulings
    double largest = -std::numeric_limits<double>::infinity();

    for (const std::vector<NurbsCurve>& crease : creases)
        for (const NurbsCurve& curve : crease)
            for (size_t k = 0; k <= samples; k++)
                largest = std::max(largest, curve.point_at(parameter(curve, k))[0]);

    // every profile curve's start along the development: the lengths of the curves before it across the rulings
    const Xform across = Xform::project_to_plane_by_axis(Plane::from_point_normal(Point(0.0, 0.0, 0.0), direction), direction);
    std::vector<double> starts = {0.0};

    for (const NurbsCurve& curve : profile)
        starts.push_back(starts.back() + curve.transformed(across).length());

    // every lamella's outline moved to its place: its row's start along x, the rulings run before its first crease along y
    size_t index = 0;

    for (size_t j = 0; j < segments.size(); j++)
        for (size_t i = 0; i < profile.size(); i++, index++) {
            double along = 0.0;

            for (size_t m = 0; m < j; m++)
                along += (creases[m + 1][i].point_at(creases[m + 1][i].domain_start()) - creases[m][i].point_at(creases[m][i].domain_start())).dot(segments[m].direction);

            Polyline outline = developments[index].outline.transformed(Xform::translation(largest + beside + starts[i], along, 0.0));
            outline.name = fmt::format("flat_{}_{}", j, i);
            add_polyline(outline, flat);
        }
}

inline void ReflectionFold::add_mirrored(const std::vector<std::vector<NurbsCurve>>& creases, bool crown) {

    const Xform mirror = reflection(*symmetry);
    const size_t folded = creases.size() - 1;
    const size_t crossing = folded - 1; // the last segment, at a crown already across the plane
    const size_t beyond = crown ? 2 * folded - 2 : 2 * folded - 1; // segment s mirrored is segment beyond - s

    // the lamellas: the mirror image of every plate, the pair at the symmetry plane sharing its side face there
    const std::shared_ptr<TreeNode> lamellas = group_named("lamellas");
    const std::vector<std::shared_ptr<PlateCurved>> plates = get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& plate : plates) {
        const size_t segment = std::stoul(plate->name.substr(8, plate->name.find('_', 8) - 8));
        const std::string row = plate->name.substr(plate->name.find('_', 8) + 1);

        if (crown && segment == crossing)
            continue;

        add(std::make_shared<PlateCurved>(plate->outer_a.transformed(mirror), plate->outer_b.transformed(mirror), plate->inner_a.transformed(mirror), plate->inner_b.transformed(mirror), fmt::format("lamella_{}_{}", beyond - segment, row)), lamellas);
    }

    // the creases: the symmetry plane's own section of the sheet unless a plate runs across it, then every crease mirrored
    const std::shared_ptr<TreeNode> group = group_named("creases");
    std::vector<Polyline> folded_creases;

    for (const std::shared_ptr<Polyline>& polyline : *objects.polylines)
        if (polyline && polyline->name.rfind("crease_", 0) == 0)
            folded_creases.push_back(*polyline);

    if (!crown) {
        std::vector<Point> ridge;

        for (const NurbsCurve& curve : creases.back())
            for (size_t k = 0; k <= samples; k++)
                ridge.push_back(curve.point_at(parameter(curve, k)));

        Polyline drawn(ridge);
        drawn.name = fmt::format("crease_{}", folded);
        add_polyline(drawn, group);
    }

    for (const Polyline& crease : folded_creases) {
        Polyline mirrored = crease.transformed(mirror);
        mirrored.name = fmt::format("crease_{}", beyond + 1 - std::stoul(crease.name.substr(7)));
        add_polyline(mirrored, group);
    }

    // the flat pattern: the mirrored half's developments beyond the first's, turned over so the two halves meet along the symmetry
    // plane's section; at a crown that section is the middle of the crossing plates' first ruling, the turn closing on them
    const std::shared_ptr<TreeNode> flat = group_named("flat");
    std::vector<Polyline> outlines;
    double top = -std::numeric_limits<double>::infinity();
    std::optional<double> middle;

    for (const std::shared_ptr<Polyline>& polyline : *objects.polylines)
        if (polyline && polyline->name.rfind("flat_", 0) == 0) {
            const size_t segment = std::stoul(polyline->name.substr(5, polyline->name.find('_', 5) - 5));

            if (crown && segment == crossing) {
                middle = 0.5 * (polyline->get_point(0)[1] + polyline->get_point(polyline->point_count() - 2)[1]);
                continue;
            }

            outlines.push_back(*polyline);

            for (size_t k = 0; k < polyline->point_count(); k++)
                top = std::max(top, polyline->get_point(k)[1]);
        }

    const Xform turn_over = Xform::scale_non_uniform(Point(0.0, middle ? *middle : top + 0.05 * length, 0.0), 1.0, -1.0, 1.0);

    for (const Polyline& outline : outlines) {
        const size_t segment = std::stoul(outline.name.substr(5, outline.name.find('_', 5) - 5));
        Polyline mirrored = outline.transformed(turn_over);
        mirrored.name = fmt::format("flat_{}_{}", beyond - segment, outline.name.substr(outline.name.find('_', 5) + 1));
        add_polyline(mirrored, flat);
    }
}

inline Xform ReflectionFold::reflection(const Plane& plane) {

    const Vector n = plane.z_axis().normalized();
    const double h = n.dot(Vector(plane.origin()[0], plane.origin()[1], plane.origin()[2]));
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

    return vector * std::cos(angle) + k.cross(vector) * std::sin(angle) + k * (k.dot(vector) * (1.0 - std::cos(angle)));
}

inline std::vector<NurbsCurve> ReflectionFold::arcs(const Point& start, const Vector& across, const Vector& up, size_t count, double width, double rise) {

    const Vector a = across.normalized();
    const Vector u = up.normalized();
    std::vector<NurbsCurve> curves;

    for (size_t i = 0; i < count; i++) {
        const Point from = start + a * (width * static_cast<double>(i));
        curves.push_back(Primitives::arc(from, from + a * (0.5 * width) + u * rise, from + a * width));
    }

    return curves;
}

inline std::vector<NurbsCurve> ReflectionFold::waves(const Point& start, const Vector& across, const Vector& up, size_t half_waves, double half_wavelength, double amplitude) {

    const Vector a = across.normalized();
    const Vector u = up.normalized();
    const size_t points = 12;
    std::vector<NurbsCurve> curves;

    for (size_t i = 0; i < half_waves; i++) {
        const double sign = i % 2 == 0 ? 1.0 : -1.0;
        std::vector<Point> wave;

        for (size_t k = 0; k <= points; k++) {
            const double s = static_cast<double>(k) / static_cast<double>(points);
            wave.push_back(start + a * (half_wavelength * (static_cast<double>(i) + s)) + u * (sign * amplitude * std::sin(Tolerance::PI * s)));
        }

        curves.push_back(NurbsCurve::create_interpolated(wave));
    }

    return curves;
}

