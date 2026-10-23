#pragma once
#include "src/templates/module/module_contact.h"
#include "src/templates/module/wall_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// CassetteContacts
// ═══════════════════════════════════════════════════════════════════════════

/// The contacts of a cassette shell.
///
/// - `inside`: within a cassette, every two of its plates and beams that touch.
/// - `between`: across every interior mesh edge, every two parts of the two cassettes that touch.
struct CassetteContacts {
    std::vector<Contact> inside; // `<a>_on_<b>` within every cassette.
    std::vector<Contact> between; // `<a>_on_<b>` across every interior edge, `a` a part of the edge's lower face.
};

// ═══════════════════════════════════════════════════════════════════════════
// CassetteShell
// ═══════════════════════════════════════════════════════════════════════════

/// A shell of hollow timber cassettes after the BUGA Wood Pavilion 2019, a session: per mesh face one cassette, a top and a bottom
/// plate over the whole face with its openings cut out, and between them an edge beam on every face edge, studs at the stud
/// spacing and a frame around every opening. Two cassettes meet either on a miter, the bisector plane of their faces, as on the hex
/// shell, or with ranks in a T: the cassette of lower rank runs through with a square end and the other is cut on its face, so the
/// fewest cuts are inclined. The edge beams of one cassette meet the same way: mitered, or with ranks butted, the more level beam
/// running through. The contacts are detected, no connector is made.
///
/// Fields: the welded mesh, the cassette sizes, the openings and the stud spacing.
/// Every cassette is a group `cassette_<f>` of `top_plate_<f>`, `bottom_plate_<f>`, `edge_beam_<f>_<j>`, `stud_<f>_<k>`,
/// `jamb_<f>_<o>_<side>`, `header_<f>_<o>` and `sill_<f>_<o>`, read back by name; the polygon of every contact between cassettes is
/// drawn in `contacts` as a red mesh.
class CassetteShell : public WoodSession {
public:
    const Mesh mesh; // The input mesh of planar faces, welded at 0.01.
    const double depth; // Cassette depth along the face normal, plates included.
    const double plate_thickness; // Top and bottom plate thickness.
    const double beam_width; // Every edge beam's, stud's and frame member's width, one section for all.
    const std::vector<std::vector<Opening>> openings; // Per face its windows in the face frame: x along its first edge, y up the face; a door is a notch in the face itself.
    const double stud_spacing; // Largest distance between stud centres along the first edge of every face, 0 for none.
    const std::vector<int> ranks; // Per face its rank at a T joint, the lower running through; empty for miters on the bisector planes.

    /// The cassettes of the mesh as the session named name.
    explicit CassetteShell(
        const Mesh& mesh = default_mesh(),
        double depth = 200.0,
        double plate_thickness = 20.0,
        double beam_width = 60.0,
        const std::vector<std::vector<Opening>>& openings = {},
        double stud_spacing = 0.0,
        const std::vector<int>& ranks = {},
        const std::string& name = "cassette_shell"
    );

    /// Not copied, as a session is not.
    CassetteShell(const CassetteShell&) = delete;

    /// Not assigned, as it is not copied.
    CassetteShell& operator=(const CassetteShell&) = delete;

    /// The fifteen-face dome of hexagons and pentagons of the VDA mesh.
    static Mesh default_mesh();

    /// A gable house, one face per cassette, the floor open: the front with its door a notch from the ground, the right gable end, the
    /// back, the left gable end, the front and the back roof slope, each face's first edge along its foot or its eaves.
    static Mesh house_mesh(
        double length = 6000.0,
        double width = 4000.0,
        double height = 2700.0,
        double ridge = 4500.0
    );

    /// The ranks of house_mesh per face: the roof slopes over everything, the front slope over the back one at the ridge, the long walls
    /// past the gable ends.
    static std::vector<int> house_ranks();

    /// The windows of house_mesh per face: one in the front, one in the back and one in each gable end.
    static std::vector<std::vector<Opening>> house_windows();

    /// Per face its edge planes, each through its edge with its normal out of the face, shared by the two faces of an interior edge.
    const std::vector<std::vector<Plane>>& edge_planes() const;

    /// Per face corner the miter plane between edge j and edge j + 1, through their common vertex.
    const std::vector<std::vector<Plane>>& miter_planes() const;

private:
    std::vector<Plane> _face_planes;
    std::vector<Plane> _face_frames;
    std::vector<std::vector<Plane>> _edge_planes;
    std::vector<std::vector<Plane>> _miter_planes;
    std::vector<std::vector<long long>> _neighbours;

    /// The plane of every face at its centroid along its normal.
    std::vector<Plane> compute_face_planes() const;

    /// The frame of every face: origin at its first corner, x along its first edge, y up the face, z its normal.
    std::vector<Plane> compute_face_frames() const;

    /// Per face edge the face on its other side, -1 on a naked edge.
    std::vector<std::vector<long long>> compute_neighbours() const;

    /// Per face edge the plane its cassette ends on, its normal out of the face: without ranks the bisector of the two faces; with ranks
    /// the face of the neighbour that runs through, or square to the face and past the neighbour's far side where this one runs through.
    std::vector<std::vector<Plane>> compute_edge_planes() const;

    /// Per face corner the plane halfway between the edge planes that meet there.
    std::vector<std::vector<Plane>> compute_miter_planes() const;

    /// Adds per face its top and bottom plate under cassette_<f>, the openings cut out of both.
    void add_plates();

    /// Adds per face edge its beam between the plates under cassette_<f>.
    void add_edge_beams();

    /// Adds per opening its two jambs, its header and its sill, between the plates under cassette_<f>.
    void add_opening_frames();

    /// Adds per face, between every two posts, the edge beams running up the face and the jambs, as few studs as keep the stud spacing,
    /// evenly apart, their ends on the inner faces of the beams over and under them, split by the openings they stand in.
    void add_studs();

    /// The rectangle from (u, v) width along x and height along y of face f's frame, level along its normal.
    Polyline face_rectangle(
        size_t f,
        double level,
        double u,
        double v,
        double width,
        double height
    ) const;

    /// Searches the contacts inside every cassette and across every interior edge, stores them and returns them.
    CassetteContacts add_contacts();

    /// The contact between a and b stored as their interaction, null when they do not touch.
    std::shared_ptr<InteractionContactFace> add_contact(const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b);
};

}
