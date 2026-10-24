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
/// outer face the exact cylinder between the two creases, the second the first projected along the rulings onto the next plane; its
/// inner face the thickness under it, on the one physical underside the material keeps through every fold, so a reflection turns the
/// sheet over but never the plate's thickness; its side faces at the creases the mitre where it meets its neighbour, one curve the two
/// plates share: the crease plane's section of the offset at a mirror fold, a sheet end and the symmetry plane, where the two offsets are
/// mirror images, and at a reflection the exact curve where the two plates' offset cylinders cross, found ruling by ruling; its caps the
/// rulings at the profile curves' ends, mitred between neighbouring curves where the profile has a kink.
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
    const size_t samples; // Rulings per profile curve the offsets and mitres are interpolated through.

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

    /// A meander from start along across: half_waves circular arcs, each half_wavelength long, bulging depth toward up and away in turn, first toward up, meeting tangent to each other: the smooth wave section of the IBOIS curved folded plate.
    static std::vector<NurbsCurve> meander(const Point& start, const Vector& across, const Vector& up, size_t half_waves, double half_wavelength, double depth);

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

    /// Every crease, curve by curve of the profile: the profile on the first plane, then each crease projected along its segment's
    /// rulings onto the next plane; checked to lie beyond the one before along every ruling, and no reflection's plane to stand on
    /// the sheet, where its fold would be flat and the sheet would cross itself.
    std::vector<std::vector<NurbsCurve>> compute_creases(const std::vector<Segment>& segments) const;

    /// Every segment's offset curves: its first crease moved the thickness to its underside along the sheet's normal, curve by curve
    /// through the sampled points, each curve's ends mitred with its neighbour where the profile has a kink.
    std::vector<std::vector<NurbsCurve>> compute_offsets(const std::vector<Segment>& segments, const std::vector<std::vector<NurbsCurve>>& creases) const;

    /// The inner meridian on every cut, curve by curve, shared by the two plates meeting there: the segment's offset projected along its
    /// rulings onto the plane at the sheet's ends, the symmetry plane and the mirror folds, where the two offsets are mirror images in the
    /// plane; at a reflection the curve where the two segments' offset cylinders cross, every ruling of the offset before the crease cut
    /// by the offset cylinder beyond it, which it meets where its projection along the rulings beyond crosses the offset beyond.
    std::vector<std::vector<NurbsCurve>> compute_inners(const std::vector<Segment>& segments, const std::vector<std::vector<NurbsCurve>>& creases, const std::vector<std::vector<NurbsCurve>>& offsets) const;

    /// Every crease on the folded sheet, crease_<j>, in the group creases.
    void add_creases(const std::vector<std::vector<NurbsCurve>>& creases);

    /// Every lamella, lamella_<segment>_<row>, in the group lamellas; its development returned for the flat pattern.
    std::vector<Development> add_lamellas(const std::vector<std::vector<NurbsCurve>>& creases, const std::vector<std::vector<NurbsCurve>>& inners);

    /// The flat cutting pattern beside the model, every lamella's development where it lies on the unrolled sheet, in the group flat.
    void add_flat(const std::vector<Development>& developments, const std::vector<Segment>& segments, const std::vector<std::vector<NurbsCurve>>& creases, double beside);

    /// The mirror image of every lamella and crease in the symmetry plane, the plane's own section of the sheet as the last crease,
    /// and the mirrored half's cutting pattern beyond the first's.
    void add_mirrored(const std::vector<std::vector<NurbsCurve>>& creases);

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

    // offsets: every segment's first crease moved the thickness to the sheet's underside, the plates' inner faces
    const std::vector<std::vector<NurbsCurve>> offsets = compute_offsets(segments, creases);

    // inners: on every cut the one inner meridian the two plates there share, the mitre
    const std::vector<std::vector<NurbsCurve>> inners = compute_inners(segments, creases, offsets);

    // creases drawn, then the lamellas: between two creases and two profile curves one curved plate, its inner meridians the shared ones
    add_creases(creases);
    const std::vector<Development> developments = add_lamellas(creases, inners);

    // flat: the cutting pattern beside the model, every lamella unfolded by its rulings
    add_flat(developments, segments, creases, 0.1 * length);

    // symmetry: the folded half mirrored in its plane of symmetry, meeting itself along the plane's section of the sheet
    if (symmetry)
        add_mirrored(creases);

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

inline std::vector<std::vector<NurbsCurve>> ReflectionFold::compute_creases(const std::vector<Segment>& segments) const {

    std::vector<std::vector<NurbsCurve>> creases = {profile};

    for (size_t j = 0; j < segments.size(); j++) {
        const Segment& segment = segments[j];

        if (std::abs(segment.end.z_axis().dot(segment.direction)) < 1e-9)
            throw std::invalid_argument(fmt::format("The plane ending segment {} runs along its rulings", j));

        const Xform to_end = Xform::project_to_plane_by_axis(segment.end, segment.direction);
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

inline std::vector<std::vector<NurbsCurve>> ReflectionFold::compute_offsets(const std::vector<Segment>& segments, const std::vector<std::vector<NurbsCurve>>& creases) const {

    std::vector<std::vector<NurbsCurve>> offsets;

    for (size_t j = 0; j < segments.size(); j++) {
        // the sheet's normal at every sample of every curve of the segment's first crease: the tangent crossed with the rulings
        std::vector<std::vector<Point>> points(profile.size());
        std::vector<std::vector<Vector>> normals(profile.size());

        for (size_t i = 0; i < profile.size(); i++)
            for (size_t k = 0; k <= samples; k++) {
                const double t = parameter(creases[j][i], k);
                points[i].push_back(creases[j][i].point_at(t));
                normals[i].push_back(creases[j][i].tangent_at(t).cross(segments[j].direction).normalized());
            }

        // every curve moved along its normals to the underside, its ends along the mitre of its own and its neighbour's normal
        std::vector<NurbsCurve> offset;

        for (size_t i = 0; i < profile.size(); i++) {
            std::vector<Point> moved;

            for (size_t k = 0; k <= samples; k++) {
                const Vector& own = normals[i][k];
                const Vector& neighbour = k == 0 && i > 0 ? normals[i - 1].back() : k == samples && i + 1 < profile.size() ? normals[i + 1].front() : own;
                const Vector mitre = (own + neighbour) * (1.0 / (1.0 + own.dot(neighbour)));
                moved.push_back(points[i][k] - mitre * thickness);
            }

            offset.push_back(NurbsCurve::create_interpolated(moved, CurveNurbsKnotStyle::Uniform));
        }

        offsets.push_back(offset);
    }

    return offsets;
}

inline std::vector<std::vector<NurbsCurve>> ReflectionFold::compute_inners(const std::vector<Segment>& segments, const std::vector<std::vector<NurbsCurve>>& creases, const std::vector<std::vector<NurbsCurve>>& offsets) const {

    std::vector<std::vector<NurbsCurve>> inners(creases.size());

    for (size_t c = 0; c < creases.size(); c++) {
        // the plane's own section of the offset before it, or of the offset after it at the sheet's start
        const size_t before = c == 0 ? 0 : c - 1;
        const Plane& plane = c == 0 ? segments[0].start : segments[before].end;
        const bool reflected = c > 0 && c < folds.size() + 1 && folds[c - 1].reflected;

        for (size_t i = 0; i < profile.size(); i++) {
            if (!reflected) {
                inners[c].push_back(offsets[before][i].transformed(Xform::project_to_plane_by_axis(plane, segments[before].direction)));
                continue;
            }

            // beyond the crease: every row's offset seen along the rulings beyond, planar curves the row's shadow may cross in its own row or a neighbour's
            const Vector& mine = segments[before].direction;
            const Vector& theirs = segments[c].direction;
            const Xform along_theirs = Xform::project_to_plane_by_axis(Plane::from_point_normal(Point(0.0, 0.0, 0.0), theirs), theirs);
            std::vector<NurbsCurve> others;

            for (const NurbsCurve& offset : offsets[c])
                others.push_back(offset.transformed(along_theirs));

            std::vector<Point> points;

            for (size_t k = 0; k <= samples; k++) {
                // my offset ruling, and where its shadow along their rulings crosses their offset: the point on both offset cylinders nearest the crease
                const Point start = offsets[before][i].point_at(parameter(offsets[before][i], k));
                const Point on_crease = creases[c][i].point_at(parameter(creases[c][i], k));
                const double expected = (on_crease - start).dot(mine);
                const Point shadow = along_theirs.transform_point(start);
                const Vector shadow_direction = along_theirs.transform_vector(mine);
                const Plane sweep = Plane::from_point_normal(shadow, shadow_direction.cross(theirs));
                const auto along = [&](const Point& on_other) { return (on_other - shadow).dot(shadow_direction) / shadow_direction.dot(shadow_direction); };
                std::optional<double> nearest;

                for (const NurbsCurve& other : others)
                    for (const double root : Intersection::curve_plane(other, sweep, 1e-7))
                        if (const double at = along(other.point_at(root)); !nearest || std::abs(at - expected) < std::abs(*nearest - expected))
                            nearest = at;

                // past the sheet's free edge the row's offset goes on along its end tangent
                if (!nearest) {
                    const NurbsCurve& edge = others[i];
                    const double end = k < samples / 2 ? edge.domain_start() : edge.domain_end();
                    const Vector tangent = edge.tangent_at(end);
                    const double speed = tangent.dot(sweep.z_axis());

                    if (std::abs(speed) < 1e-12)
                        throw std::runtime_error(fmt::format("The two plates at crease {} do not meet on ruling {} of curve {}", c, k, i));

                    nearest = along(edge.point_at(end) - tangent * (sweep.signed_distance(edge.point_at(end)) / speed));
                }

                points.push_back(start + mine * *nearest);
            }

            inners[c].push_back(NurbsCurve::create_interpolated(points, CurveNurbsKnotStyle::Uniform));
        }
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

inline void ReflectionFold::add_mirrored(const std::vector<std::vector<NurbsCurve>>& creases) {

    const Xform mirror = reflection(*symmetry);
    const size_t folded = creases.size() - 1;

    // the lamellas: the mirror image of every plate, the pair at the symmetry plane sharing its side face there
    const std::shared_ptr<TreeNode> lamellas = group_named("lamellas");
    const std::vector<std::shared_ptr<PlateCurved>> plates = get_elements_placed<PlateCurved>("lamella");

    for (const std::shared_ptr<PlateCurved>& plate : plates) {
        const size_t segment = std::stoul(plate->name.substr(8, plate->name.find('_', 8) - 8));
        const std::string row = plate->name.substr(plate->name.find('_', 8) + 1);
        add(std::make_shared<PlateCurved>(plate->outer_a.transformed(mirror), plate->outer_b.transformed(mirror), plate->inner_a.transformed(mirror), plate->inner_b.transformed(mirror), fmt::format("lamella_{}_{}", 2 * folded - 1 - segment, row)), lamellas);
    }

    // the creases: the symmetry plane's own section of the sheet, then every crease mirrored
    const std::shared_ptr<TreeNode> group = group_named("creases");
    std::vector<Point> ridge;

    for (const NurbsCurve& curve : creases.back())
        for (size_t k = 0; k <= samples; k++)
            ridge.push_back(curve.point_at(parameter(curve, k)));

    Polyline drawn(ridge);
    drawn.name = fmt::format("crease_{}", folded);
    add_polyline(drawn, group);
    std::vector<Polyline> folded_creases;

    for (const std::shared_ptr<Polyline>& polyline : *objects.polylines)
        if (polyline && polyline->name.rfind("crease_", 0) == 0 && polyline->name != drawn.name)
            folded_creases.push_back(*polyline);

    for (const Polyline& crease : folded_creases) {
        Polyline mirrored = crease.transformed(mirror);
        mirrored.name = fmt::format("crease_{}", 2 * folded - std::stoul(crease.name.substr(7)));
        add_polyline(mirrored, group);
    }

    // the flat pattern: the mirrored half's developments beyond the first's, turned over so the two halves meet along the symmetry plane's section
    const std::shared_ptr<TreeNode> flat = group_named("flat");
    std::vector<Polyline> outlines;
    double top = -std::numeric_limits<double>::infinity();

    for (const std::shared_ptr<Polyline>& polyline : *objects.polylines)
        if (polyline && polyline->name.rfind("flat_", 0) == 0) {
            outlines.push_back(*polyline);

            for (size_t k = 0; k < polyline->point_count(); k++)
                top = std::max(top, polyline->get_point(k)[1]);
        }

    const Xform turn_over = Xform::scale_non_uniform(Point(0.0, top + 0.05 * length, 0.0), 1.0, -1.0, 1.0);

    for (const Polyline& outline : outlines) {
        const size_t segment = std::stoul(outline.name.substr(5, outline.name.find('_', 5) - 5));
        Polyline mirrored = outline.transformed(turn_over);
        mirrored.name = fmt::format("flat_{}_{}", 2 * folded - 1 - segment, outline.name.substr(outline.name.find('_', 5) + 1));
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

inline std::vector<NurbsCurve> ReflectionFold::meander(const Point& start, const Vector& across, const Vector& up, size_t half_waves, double half_wavelength, double depth) {

    const Vector a = across.normalized();
    const Vector u = up.normalized();
    std::vector<NurbsCurve> curves;

    for (size_t i = 0; i < half_waves; i++) {
        const double sign = i % 2 == 0 ? 1.0 : -1.0;
        const Point from = start + a * (half_wavelength * static_cast<double>(i));
        curves.push_back(Primitives::arc(from, from + a * (0.5 * half_wavelength) + u * (sign * depth), from + a * half_wavelength));
    }

    return curves;
}
