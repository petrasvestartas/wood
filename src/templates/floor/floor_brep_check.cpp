#include "pch.h"
#include "src/templates/floor/floor.h"
#include "wood_element_geometry.h"
#include "wood_brep_drill.h"
#include <chrono>

using namespace session_cpp;

namespace wood_floor {

// ═══════════════════════════════════════════════════════════════════════════
// Counting
// ═══════════════════════════════════════════════════════════════════════════

/// A dowel or a connector part: a child a connector draws on its own.
static bool is_connector_child(const std::shared_ptr<Element>& element) {

    return std::dynamic_pointer_cast<wood_session::Dowel>(element) || std::dynamic_pointer_cast<wood_session::ConnectorPart>(element);
}

/// A member its joints cut: not a joint, and its model differs from the uncut element.
static bool is_cut_member(const std::shared_ptr<Element>& element) {

    return !std::dynamic_pointer_cast<wood_session::Joint>(element) && element->model_geometry_mesh().number_of_vertices() != element->element_geometry_mesh().number_of_vertices();
}

/// The exact bores of a BRep: its rational surfaces, cylinders.
static size_t count_bores(const BRep& brep) {

    size_t bores = 0;

    for (const NurbsSurface& surface : brep.m_surfaces)
        if (surface.is_rational())
            bores++;

    return bores;
}

/// The bores the dowels ask for in one solid, already cut by its pockets: one per stretch of a dowel inside it, dowels meeting end to end on one axis joined into one.
static size_t bore_stretches(const Mesh& solid, const std::vector<wood_session::Drill>& drills) {

    size_t stretches = 0;

    for (const wood_session::Drill& drill : wood_session::merged_drills(drills))
        for (const std::array<double, 2>& stretch : wood_session::inside_stretches(solid, drill.axis))
            if (stretch[1] > 1e-6 && stretch[0] < drill.axis.length() - 1e-6)
                stretches++;

    return stretches;
}

/// The bores the dowels and screws ask for over the scene: every stretch of one inside a target's pocketed solid or inside its connector's own part.
static size_t dowel_stretches(const wood_session::WoodSession& session) {

    std::map<std::string, std::vector<wood_session::Drill>> drills;
    size_t stretches = 0;

    for (const std::shared_ptr<wood_session::Joint>& joint : session.get_elements<wood_session::Joint>()) {
        std::vector<wood_session::Drill> own;

        for (const Line& dowel : joint->drill_axes())
            own.push_back({dowel, joint->line_radius});

        for (const std::string& target : joint->targets)
            drills[target].insert(drills[target].end(), own.begin(), own.end());

        if (const std::shared_ptr<wood_session::JointBeam> connector = std::dynamic_pointer_cast<wood_session::JointBeam>(joint))
            for (size_t i = 0; i < connector->parts.size(); i++)
                stretches += bore_stretches(wood_session::apply_solid_cuts(connector->part_mesh(i), connector->solid_cuts, false), own);
    }

    for (const std::pair<const std::string, std::vector<wood_session::Drill>>& element : drills) {
        const std::shared_ptr<Element> target = session.get_element<Element>(element.first);
        const std::vector<wood_session::SolidCut>* cuts = wood_session::solid_cuts_of(*target);
        stretches += bore_stretches(cuts ? wood_session::apply_solid_cuts(target->element_geometry_mesh(), *cuts, false) : target->element_geometry_mesh(), element.second);
    }

    return stretches;
}

// ═══════════════════════════════════════════════════════════════════════════
// Check
// ═══════════════════════════════════════════════════════════════════════════

BrepCheck check_breps(const wood_session::WoodSession& session) {

    BrepCheck check;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    for (const std::shared_ptr<Element>& element : *session.objects.elements) {
        if (is_connector_child(element)) {
            if (std::dynamic_pointer_cast<wood_session::ConnectorPart>(element))
                check.part_bores += count_bores(element->model_geometry_brep());

            continue;
        }

        if (const std::shared_ptr<wood_session::JointBeam> connector = std::dynamic_pointer_cast<wood_session::JointBeam>(element)) {
            check.connectors += !connector->parts.empty() || !connector->drill_lines.empty();
            continue;
        }

        if (!is_cut_member(element))
            continue;

        const size_t bores = count_bores(element->model_geometry_brep());
        check.bores += bores;

        if (bores > 0)
            check.exact++;
        else
            check.faceted.push_back(element->name);
    }

    check.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    check.stretches = dowel_stretches(session);

    return check;
}

void compute_breps(wood_session::WoodSession& session) {

    for (const std::shared_ptr<Element>& element : *session.objects.elements)
        if (is_connector_child(element) || is_cut_member(element))
            element->compute_geometry_brep();
}

std::string BrepCheck::str() const {

    std::string text = fmt::format("BReps of the cut elements: {} with {} exact bores, {} faceted, and of {} connectors with {} exact bores through their parts, in {:.0f} ms\n", exact, bores, faceted.size(), connectors, part_bores, ms);
    text += fmt::format("Every dowel bores every element it passes: {} dowel stretches through members and parts, {} exact bores found", stretches, bores + part_bores);

    for (const std::string& name : faceted)
        text += "\n  faceted: " + name;

    return text;
}

}
