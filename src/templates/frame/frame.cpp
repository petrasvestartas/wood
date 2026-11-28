#include "pch.h"
#include "src/templates/frame/frame.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_frame {

namespace {

int family_rank(const std::string& name) {
    if (name == "support")
        return 0;
    if (name == "column")
        return 1;
    if (name == "slab_bottom")
        return 2;
    if (name == "wall")
        return 3;
    if (name == "chord_bottom")
        return 4;
    if (name == "post")
        return 5;
    if (name == "rafter" || name == "chord_top")
        return 6;
    if (name == "slab_top" || name == "slab")
        return 7;
    if (name == "purlin")
        return 8;
    return 9;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Frame
// ═══════════════════════════════════════════════════════════════════════════

Frame::Frame(const FrameGuide& guide_in, const std::string& name_in)
    : WoodSession(name_in),
      guide(guide_in) {

    // wireframe: construction axes at ground/datum
    std::shared_ptr<TreeNode> wire_grp = add_group("guide");
    for (const MemberSpec& m : guide.members())
        add_polyline(Polyline({m.axis.start(), m.axis.end()}), wire_grp);

    // supports: steel-and-pin base shoes on the foundation
    add_supports();

    // members: tapered legs, haunched rafters, Vierendeel chords and posts, roof and floor purlins
    add_members();

    // plates: bottom, wall, and top CLT slabs of cellular box frames
    add_plates();

    // contacts: face-to-face bearing and miter contacts across the frame
    compute_face_contacts();

    // assembly: stage-by-stage erection sequence from foundation up
    assemblies = {compute_assembly()};
}

void Frame::add_supports() {
    if (guide.supports().empty())
        return;

    std::shared_ptr<TreeNode> grp = add_group("supports");
    for (const SupportSpec& spec : guide.supports()) {
        std::shared_ptr<NodeSupportElement> shoe =
            std::make_shared<NodeSupportElement>(spec.plane, "support");
        shoe->head_plate_recess = 0.0;
        shoe->base_plate_size = spec.base_size;
        shoe->base_plate_hole_spacing = spec.base_size * 0.72;
        shoe->head_plate_diameter = spec.head_diameter;
        shoe->height = spec.height;
        shoe->invalidate_geometry();
        add_element(shoe, grp);
        _element_stages.push_back(-1);
    }
}

void Frame::add_members() {
    if (guide.members().empty())
        return;

    std::map<std::string, std::shared_ptr<TreeNode>> groups;
    auto group_for = [&](const std::string& fam) -> std::shared_ptr<TreeNode> {
        std::string gname = fam + "s";
        if (fam == "chord_bottom" || fam == "chord_top")
            gname = "chords";
        std::map<std::string, std::shared_ptr<TreeNode>>::iterator it = groups.find(gname);
        if (it != groups.end())
            return it->second;
        std::shared_ptr<TreeNode> g = add_group(gname);
        groups[gname] = g;
        return g;
    };

    for (const MemberSpec& spec : guide.members()) {
        if (spec.sections.empty())
            continue;

        std::shared_ptr<Element> elem;
        if (spec.sections.size() == 1) {
            std::shared_ptr<EdgeStraightElement> bar = std::make_shared<EdgeStraightElement>(
                Polyline({spec.axis.start(), spec.axis.end()}),
                spec.sections,
                std::vector<Vector>{spec.up},
                spec.family
            );
            bar->cuts = spec.cuts;
            for (const Mesh& cutter : spec.subtractions)
                bar->solid_features.emplace_back(cutter, SolidOperation::subtract);
            elem = bar;
        } else {
            std::shared_ptr<EdgeVariableElement> var_bar =
                std::make_shared<EdgeVariableElement>(spec.axis, spec.sections, spec.family);
            var_bar->cuts = spec.cuts;
            for (const Mesh& cutter : spec.subtractions)
                var_bar->solid_features.emplace_back(cutter, SolidOperation::subtract);
            elem = var_bar;
        }
        add_element(elem, group_for(spec.family));
        _element_stages.push_back(spec.stage);
    }
}

void Frame::add_plates() {
    if (guide.plates().empty())
        return;

    std::shared_ptr<TreeNode> slab_grp;
    std::shared_ptr<TreeNode> wall_grp;

    for (const PlateSpec& spec : guide.plates()) {
        std::shared_ptr<FacePlanarElement> plate =
            std::make_shared<FacePlanarElement>(spec.bottom, spec.top, spec.family);
        if (!spec.holes_bottom.empty() && spec.holes_bottom.size() == spec.holes_top.size()) {
            plate->features.bottom.push_back(spec.bottom);
            plate->features.top.push_back(spec.top);
            for (size_t i = 0; i < spec.holes_bottom.size(); ++i) {
                plate->features.bottom.push_back(spec.holes_bottom[i]);
                plate->features.top.push_back(spec.holes_top[i]);
            }
        }
        plate->invalidate_geometry();
        const bool is_wall = (spec.family == "wall");
        if (is_wall && !wall_grp)
            wall_grp = add_group("walls");
        if (!is_wall && !slab_grp)
            slab_grp = add_group("slabs");
        add_element(plate, is_wall ? wall_grp : slab_grp);
        _element_stages.push_back(spec.stage);
    }
}

AssemblyModule Frame::compute_assembly() const {
    const std::vector<std::shared_ptr<Element>>& elems = elements();
    std::vector<size_t> indices(elems.size());
    for (size_t i = 0; i < elems.size(); ++i)
        indices[i] = i;

    std::stable_sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        const int sa = a < _element_stages.size() ? _element_stages[a] : 0;
        const int sb = b < _element_stages.size() ? _element_stages[b] : 0;
        if (sa != sb)
            return sa < sb;
        const int ra = family_rank(elems[a]->name);
        const int rb = family_rank(elems[b]->name);
        if (ra != rb)
            return ra < rb;
        const AABB ba = AABB::from_mesh(elems[a]->model_geometry_mesh());
        const AABB bb = AABB::from_mesh(elems[b]->model_geometry_mesh());
        return (ba.cz - ba.hz) < (bb.cz - bb.hz - 1.0);
    });

    std::vector<AssemblyModule> parts;
    parts.reserve(indices.size());
    for (size_t idx : indices)
        parts.push_back(AssemblyModule::part(elems[idx]));

    return AssemblyModule::module(name, parts);
}

// ═══════════════════════════════════════════════════════════════════════════
// Frames
// ═══════════════════════════════════════════════════════════════════════════

Frames::Frames(const std::vector<FrameDesign>& designs, double gap, const std::string& name_in)
    : WoodSession(name_in) {

    double cursor = 0.0;
    for (const FrameDesign& design : designs) {
        FrameGuide g = design.make(cursor);
        const double w = std::max(4000.0, g.span_x());
        graft(wood_frame::Frame(g, design.name), add_group(design.name));
        cursor += w + gap;
    }
}

} // namespace wood_frame
