#pragma once

#include "pch.h"
#include "wood_session.h"
#include "wood_assembly_module.h"
#include "wood_solid_body.h"

using namespace laika_cpp;

namespace wood_session {

/// One move of a module's assembly: the piece, its elements, the unit direction it goes in along (zero for the piece that
/// stands first), the moves it makes after it (turns and slides), how far out it is clear of every other's bounds, how far it
/// was swept, its wave (the pieces of one wave can move at the same time) and the pieces it meets.
struct ModuleMove {
    std::string piece;
    std::vector<std::string> elements;
    Vector insertion;
    std::vector<JointMove> legs;
    double travel = 0.0;
    double swept = 0.0;
    size_t wave = 0;
    std::vector<std::string> meets;
};

/// The assembly of one module: its pieces in the order they go in, and the pieces no move frees.
struct ModuleSteps {
    std::string name;
    std::vector<ModuleMove> moves;
    std::vector<std::string> locked;
};

/// Two pieces' solids sharing wood where they stand, before anything moves.
struct ModuleOverlap {
    std::string first;
    std::string second;
    double thickness; // mm, the mean thickness of the wood shared.
};

/// The assembly of a session's modules, every module on its own and then as one piece of the module it belongs to, found by
/// taking it apart: again and again the last piece a move frees is taken out, swept as a solid against every piece still in.
/// A piece is tried along its own ways first, then straight off the one other it touches, up and away from its neighbours,
/// each turned as little as the faces it touches ask (the nearest direction that leaves every contact face or slides on it), then along the contact normals
/// and the axes, and last with a turn before the slide. The assembly is that order reversed.
class ModuleSequence {
public:
    const AssemblyModule root; // The whole, its modules nested in it.
    const double step; // mm a piece moves between two checks.
    const double play; // mm, the mean thickness of shared wood under which two solids only touch.

    /// The sequence of the modules of session under root.
    ModuleSequence(const WoodSession& session, const AssemblyModule& root, double step = 10.0, double play = 0.1);

    /// Every module's assembly, the inner ones first, root last.
    const std::vector<ModuleSteps>& modules() const;

    /// The pieces no move frees, as `<module>: <piece>`; empty when everything goes together.
    const std::vector<std::string>& locked() const;

    /// The elements' solids that share wood where they stand.
    const std::vector<ModuleOverlap>& overlaps() const;

    /// The elements whose solid is not closed, so no sweep checks them.
    const std::vector<std::string>& open_solids() const;

    /// The modules, moves, waves and locked pieces as JSON, for tools/render_template_assembly.py.
    void json_dump(const std::string& filename) const;

    /// The counts, then every kind of module once with its moves, then what is locked, overlaps or is open.
    std::string str() const;

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const ModuleSequence& sequence);

private:
    /// A face of a solid: its outline, its outward normal and its bounds.
    struct BodyFace {
        Polyline outline;
        Vector normal;
        Point origin;
        AABB box;
    };

    /// An element's solid.
    struct Body {
        std::string guid;
        std::string name;
        std::shared_ptr<SolidBody> solid;
        std::vector<BodyFace> faces;
        bool pin = false; // A pin, driven into the wood it stands in.
    };

    /// Where two solids touch: the normal out of the other solid, and the corners and the size of the area they share.
    struct Touch {
        Vector normal;
        std::vector<Point> points;
        double area = 0.0;
    };

    /// A way out of a module: the slide's unit direction after the moves legs, and how far it clears everything.
    struct Way {
        Vector out;
        std::vector<JointMove> legs;
        double travel = 0.0;
        double swept = 0.0;
    };

    /// A piece while its module is taken apart.
    struct Piece {
        std::string name;
        std::vector<size_t> bodies;
        std::vector<Vector> ways;
        double height = 0.0; // Of its bounds' centre, to a millimetre.
    };

    double reach = 0.0; // mm every piece is swept at least, a share of the whole's size.
    std::vector<Body> bodies;
    std::vector<size_t> piece_of; // Per body, its piece in the module being taken apart.
    std::map<std::string, size_t> body_of; // Per guid, its place in bodies.
    std::map<std::pair<size_t, size_t>, std::vector<Touch>> touches; // Per pair of bodies, the lower first, the normals out of the second.
    std::map<std::pair<size_t, size_t>, SharedWood> standing; // Per pair of bodies, the wood they share where they stand.
    std::vector<ModuleSteps> every;
    std::vector<std::string> stuck;
    std::vector<ModuleOverlap> overlapping;
    std::vector<std::string> open;

    /// The solid of every element under root, with its faces.
    std::vector<Body> compute_bodies(const WoodSession& session) const;

    /// The faces of a solid's mesh, facing out.
    static std::vector<BodyFace> compute_faces(const Mesh& mesh);

    /// Where the first body's faces lie on the second's, within a millimetre and a half.
    std::vector<Touch> compute_touches(size_t first, size_t second) const;

    /// The touches of the pieces' bodies with others, the normals pointing at the piece.
    std::vector<Touch> touches_of(const std::vector<size_t>& piece, const std::vector<size_t>& others) const;

    /// The wood two bodies share where they stand.
    const SharedWood& standing_wood(size_t first, size_t second);

    /// Whether left shares thinner wood than right.
    static bool thinner_overlap(const ModuleOverlap& left, const ModuleOverlap& right);

    /// Whether left stands lower than right.
    static bool lower_piece(const Piece& left, const Piece& right);

    /// Adds the assembly of module to every, its inner modules first.
    void add_module(const AssemblyModule& module);

    /// The pieces taken out one by one, in removal order; the ones left locked in steps.
    std::vector<ModuleMove> compute_removal(const std::vector<Piece>& pieces, ModuleSteps& steps);

    /// A way out of others for piece, downwards only when it may, false when none is found; tried remembers what blocked each direction.
    bool find_way(const Piece& piece, const std::vector<size_t>& others, bool downwards, std::map<std::string, size_t>& tried, Way& found);

    /// The thickest wood piece shares with others on its first steps along direction.
    double bound_by(const std::vector<size_t>& piece, const std::vector<size_t>& others, const Vector& direction);

    /// The direction near one of the directions, turned by ever smaller angles to share less and less, that piece leaves others
    /// along without sharing more than play; zero when there is none.
    Vector refined_direction(const std::vector<size_t>& piece, const std::vector<size_t>& others, const std::vector<Vector>& directions);

    /// The unit directions piece is tried to leave others along, the best first.
    std::vector<Vector> candidate_directions(const Piece& piece, const std::vector<size_t>& others, const std::vector<Touch>& touching) const;

    /// The turns piece is tried to leave others by, as a line each and the sense the angle takes.
    std::vector<Line> candidate_turns(const Piece& piece, const std::vector<Vector>& directions, const std::vector<Touch>& touching) const;

    /// Whether the bodies placed by placed enter one of others, by the index of the one entered.
    bool enters(const std::vector<size_t>& piece, const std::vector<size_t>& others, const Xform& placed, size_t& blocker);

    /// Whether the bodies, once placed by reached, slide out along direction clear of others; found takes how far that is.
    bool slides_out(const std::vector<size_t>& piece, const std::vector<size_t>& others, const Xform& reached, const Vector& direction, Way& found, size_t& blocker);

    /// Puts the moves that can go at the same time in one wave.
    void compute_waves(ModuleSteps& steps);

    /// The placement of a piece share of its way out, 0 in place, 1 out.
    static Xform placement(const ModuleMove& move, double share);

    /// The unit direction nearest hint that leaves every touch or slides on it, zero when none does.
    static Vector nearest_direction(const Vector& hint, const std::vector<Touch>& touching);

    /// The bounds of the bodies where they stand.
    AABB bounds_of(const std::vector<size_t>& piece) const;

    /// The kind of a module: its name without its numbers.
    static std::string kind_of(const std::string& name);
};

}
