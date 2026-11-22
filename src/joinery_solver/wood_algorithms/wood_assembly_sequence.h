#pragma once

#include "pch.h"
#include "wood_session.h"

using namespace session_cpp;

namespace wood_session {

/// One move of an assembly: the member, the unit direction it goes in along (zero for the base), the moves it makes after it
/// (in, then along the slot), the joints it engages and their other members.
struct AssemblyStep {
    std::string element;
    Vector insertion;
    std::vector<Vector> legs;
    std::vector<std::string> joints;
    std::vector<std::string> partners;
};

/// The assembly of the members a session's joints act on and the pegs and keys nested under them, found by taking it apart: again and again a member slides out along a
/// direction one of its joints gives, swept its own diagonal against every member still in without entering one, the base last.
/// The assembly is that order reversed, every member going in the opposite way; a member no direction frees is locked, and a
/// joint whose every direction one other member of the same element blocks is in conflict with that member's joint.
class AssemblySequence {
public:
    const std::string base; // The member that stays: placed first, taken out last.
    const double step; // mm a member moves between two checks.
    const double clearance; // mm3 two members may share and still be apart, the booleans' noise.

    /// The sequence of the members of session, base by name.
    AssemblySequence(const WoodSession& session, const std::string& base, double step = 10.0, double clearance = 10.0);

    /// The assembly, base first.
    const std::vector<AssemblyStep>& steps() const;

    /// The members no direction frees, by name; empty when the whole assembly comes apart.
    const std::vector<std::string>& locked() const;

    /// "<member>: <joint> blocked by <other member>", one per joint every direction of which the other member blocks.
    const std::vector<std::string>& conflicts() const;

    /// The order, the directions, every member's box, the locked members and the conflicts as JSON, for tools/render_joint_assembly_gif.py.
    void json_dump(const std::string& filename) const;

    /// The assembly one step per line, then the locked members and the conflicts.
    std::string str() const;

private:
    std::vector<std::shared_ptr<Element>> members; // Every element a joint acts on, in session order.
    std::map<std::string, std::vector<std::pair<std::shared_ptr<JointElement>, size_t>>> joints; // Per member guid, each joint on it and its index there.
    std::vector<AssemblyStep> sequence;
    std::vector<std::string> stuck;
    std::vector<std::string> blocked;

    /// The other members of a joint, by guid.
    static std::vector<std::string> partners(const JointElement& joint, size_t index);

    /// Whether member makes the moves legs, as long as each, then slides out along direction, its own diagonal, without
    /// entering any of others.
    bool slides_out(const Element& member, const std::vector<Vector>& legs, const Vector& direction, const std::vector<const Element*>& others) const;

    /// The moves out of a way in's legs: the last undone first, each turned round.
    static std::vector<Vector> out_legs(const std::vector<Vector>& legs);

    /// Takes the members apart one by one; returns the steps in removal order and fills stuck.
    std::vector<AssemblyStep> compute_disassembly();

    /// Per member and joint, the other members that block every direction the joint gives.
    std::vector<std::string> compute_conflicts() const;
};

}
