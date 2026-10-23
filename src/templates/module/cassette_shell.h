#pragma once
#include "src/templates/module/module_contact.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// CassetteContacts
// ═══════════════════════════════════════════════════════════════════════════

/// The contacts of a cassette shell.
///
/// - `inside`: within a cassette, each plate on each of its edge beams and every two neighbouring edge beams at their miter.
/// - `between`: across every interior mesh edge, the edge beams of the two cassettes face to face on the plane they share.
struct CassetteContacts {
    std::vector<Contact> inside; // `<a>_on_<b>` within every cassette.
    std::vector<Contact> between; // `<a>_on_<b>` across every interior edge, `a` the beam of the edge's lower face.
};

// ═══════════════════════════════════════════════════════════════════════════
// CassetteShell
// ═══════════════════════════════════════════════════════════════════════════

/// A segmented shell of hollow timber cassettes after the BUGA Wood Pavilion 2019, a session: per mesh face a thin top and bottom
/// plate with an edge beam on every face edge between them, the outer face of each beam on the plane its edge shares with the
/// neighbouring cell, so it is cut at the angle between the two cells, its ends mitered with the next beam. Neighbouring cassettes
/// meet beam to beam; the contacts are detected, no connector is made yet.
///
/// Fields: the welded mesh and the cassette sizes.
/// Every cassette is a group `cassette_<f>` of `top_plate_<f>`, `bottom_plate_<f>` and `edge_beam_<f>_<j>`, read back by name; the
/// polygon of every contact between cassettes is drawn in `contacts`.
class CassetteShell : public WoodSession {
public:
    const Mesh mesh; // The input mesh of planar faces, welded at 0.01.
    const double depth; // Cassette depth along the face normal, plates included.
    const double plate_thickness; // Top and bottom plate thickness.
    const double beam_width; // Edge beam width square to its edge plane.

    /// The cassettes of the mesh as the session named name.
    explicit CassetteShell(
        const Mesh& mesh = default_mesh(),
        double depth = 200.0,
        double plate_thickness = 20.0,
        double beam_width = 60.0,
        const std::string& name = "cassette_shell"
    );

    /// Not copied, as a session is not.
    CassetteShell(const CassetteShell&) = delete;

    /// Not assigned, as it is not copied.
    CassetteShell& operator=(const CassetteShell&) = delete;

    /// The fifteen-face dome of hexagons and pentagons of the VDA mesh.
    static Mesh default_mesh();

    /// A gable house: four walls split into panels with a door and four windows left open, a triangle over each end wall and two roof
    /// slopes to the ridge, one face per cassette, the floor open.
    static Mesh house_mesh(
        double length = 6000.0,
        double width = 4000.0,
        double height = 2700.0,
        double ridge = 4500.0
    );

    /// Per face its edge planes, each through its edge with its normal out of the face, shared by the two faces of an interior edge.
    const std::vector<std::vector<Plane>>& edge_planes() const;

    /// Per face corner the miter plane between edge j and edge j + 1, through their common vertex.
    const std::vector<std::vector<Plane>>& miter_planes() const;

private:
    std::vector<Plane> _face_planes;
    std::vector<std::vector<Plane>> _edge_planes;
    std::vector<std::vector<Plane>> _miter_planes;
    std::vector<std::vector<long long>> _neighbours;

    /// The plane of every face at its centroid along its normal.
    std::vector<Plane> compute_face_planes() const;

    /// Per face edge the face on its other side, -1 on a naked edge.
    std::vector<std::vector<long long>> compute_neighbours() const;

    /// Per face edge the plane through the edge leaning on the average normal of its faces, its normal out of the face.
    std::vector<std::vector<Plane>> compute_edge_planes() const;

    /// Per face corner the plane halfway between the edge planes that meet there.
    std::vector<std::vector<Plane>> compute_miter_planes() const;

    /// Adds per face its top and bottom plate under cassette_<f>.
    void add_plates();

    /// Adds per face edge its beam between the plates under cassette_<f>.
    void add_edge_beams();

    /// Searches the contacts inside every cassette and across every interior edge, stores them and returns them.
    CassetteContacts add_contacts();

    /// The contact between a and b stored as their interaction, null when they do not touch.
    std::shared_ptr<InteractionContactFace> add_contact(const std::shared_ptr<Element>& a, const std::shared_ptr<Element>& b);
};

}
