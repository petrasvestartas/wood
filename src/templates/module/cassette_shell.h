#pragma once
#include "src/templates/module/module_contact.h"
#include "src/templates/module/wall_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// CassetteContacts
// ═══════════════════════════════════════════════════════════════════════════

/// The contacts of a cassette shell. - `inside`.
struct CassetteContacts {
    std::vector<Contact> inside; // `<a>_on_<b>` within every cassette.
    std::vector<Contact>
        between; // `<a>_on_<b>` across every interior edge, `a` a part of the edge's lower face.
};

// ═══════════════════════════════════════════════════════════════════════════
// CassetteShell
// ═══════════════════════════════════════════════════════════════════════════

/// A shell of hollow timber cassettes.
class CassetteShell : public WoodSession {
public:
    const Mesh mesh; // The input mesh of planar faces, welded at 0.01.
    const double depth; // Cassette depth along the face normal, plates included.
    const double plate_thickness; // Top and bottom plate thickness.
    const double
        beam_width; // Every edge beam's, stud's and frame member's width, one section for all.
    const std::vector<std::vector<Opening>> openings; // Per face its windows in the face frame.
    const double
        stud_spacing; // Stud centres from the first corner of every face along its first edge.
    const std::vector<int>
        ranks; // Per face its rank at a T joint, the lower running through; empty for miters on the bisector planes.

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

    /// A gable house.
    static Mesh house_mesh(
        double length = 6000.0, double width = 4000.0, double height = 2700.0, double ridge = 4500.0
    );

    /// The ranks of house_mesh per face.
    static std::vector<int> house_ranks();

    /// The windows of house_mesh per face: one in the front, one in the back and one in each gable end.
    static std::vector<std::vector<Opening>> house_windows();

    /// Per face its edge planes.
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

    /// Per face edge the plane its cassette ends on.
    std::vector<std::vector<Plane>> compute_edge_planes() const;

    /// Per face corner the plane halfway between the edge planes that meet there.
    std::vector<std::vector<Plane>> compute_miter_planes() const;

    /// Adds per face its top and bottom plate under cassette_<f>, the openings cut out of both.
    void add_plates();

    /// Adds per face edge its beam between the plates under cassette_<f>.
    void add_edge_beams();

    /// Adds per window its header and its sill between the jamb studs, under cassette_<f>.
    void add_opening_frames();

    /// Adds jamb, grid and cripple studs around each face opening.
    void add_studs();

    /// The rectangle from (u, v) width along x and height along y of face f's frame, level along its normal.
    Polyline
    face_rectangle(size_t f, double level, double u, double v, double width, double height) const;

    /// Searches the contacts inside every cassette and across every interior edge, stores them and returns them.
    CassetteContacts add_contacts();

    /// The contact between a and b stored as their interaction, null when they do not touch.
    std::shared_ptr<InteractionContactFace>
    add_contact(const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b);
};

}
