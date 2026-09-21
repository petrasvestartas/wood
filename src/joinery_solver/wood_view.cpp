#include "pch.h"
#include "wood_view.h"
#include "wood_session.h"
#include "wood_element_geometry.h"
using namespace session_cpp;

namespace wood_session {

namespace {

/// First block of a guid - enough to tell two elements apart in an object name.
std::string short_guid(const std::string& guid) {
    return guid.substr(0, guid.find('-'));
}

/// The ring of a contact or a joint, as a wide line loop in its own colour.
std::shared_ptr<Polyline> ring(const Polyline& area, const Color& color, const std::string& name) {

    std::shared_ptr<Polyline> outline = std::make_shared<Polyline>(area);
    outline->linecolor = color;
    outline->width = 3.0;
    outline->name = name;

    return outline;
}

/// The tree node named by this guid, wherever it hangs; null when the element has none yet.
std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodes_by_guid(const session_cpp::Tree& tree) {

    std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodes;
    if (!tree.root())
        return nodes;

    for (TreeNode* node : tree.root()->descendants())
        nodes.emplace(node->name, node->shared_from_this());

    return nodes;
}

/// The child group of `parent` called `name`, made on first use.
std::shared_ptr<TreeNode> child_group(WoodSession& scene, std::map<std::string, std::shared_ptr<TreeNode>>& made, const std::shared_ptr<TreeNode>& parent, const std::string& name) {

    const std::string key = parent->name + "/" + name;
    const auto it = made.find(key);
    if (it != made.end())
        return it->second;

    const std::shared_ptr<TreeNode> group = std::make_shared<TreeNode>(name);
    scene.add(group, parent);
    made[key] = group;

    return group;
}

}  // namespace

/// Every contact under the `contacts` group of its first element: a face or cross contact its polygon as a region, an axis contact its segment as a line.
static void add_contacts_to(WoodSession& scene, const std::map<std::string, std::shared_ptr<TreeNode>>& groups, std::map<std::string, std::shared_ptr<TreeNode>>& children) {

    const std::vector<std::string> guids = scene.element_guids();
    std::unordered_map<std::string, int> index;
    for (size_t i = 0; i < guids.size(); ++i)
        index[guids[i]] = static_cast<int>(i);

    for (const auto& [guid, interaction] : scene.interactions) {

        const std::pair<std::string, std::string> ends = scene.edge_of(interaction);
        const auto owner = groups.find(ends.first);
        if (owner == groups.end() || !index.count(ends.first) || !index.count(ends.second))
            continue;

        const std::shared_ptr<TreeNode> group = child_group(scene, children, owner->second, "contacts");
        const std::string prefix = fmt::format("contact_{}_{}", index[ends.first], index[ends.second]);
        for (const InteractionContact& contact : interaction.contacts) {

            if (const ContactAxis* axis = contact.axis()) {
                const std::string name = fmt::format("{}_s{}_{}_axis", prefix, axis->segment_a, axis->segment_b);
                scene.add_polyline(ring(Polyline({axis->segment.start(), axis->segment.end()}), Color(0.086f, 0.635f, 0.667f, 1.0f, "axis_teal"), name), group);
                continue;
            }

            const ContactFace* face = contact.face();
            const ContactCross* cross = contact.cross();
            const std::string name = face
                ? fmt::format("{}_f{}_{}_{}", prefix, face->face_a, face->face_b, contact_type_name(face->type))
                : fmt::format("{}_f{}_{}_cross", prefix, cross->faces_a[0], cross->faces_b[0]);
            std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(Mesh::from_polylines(std::vector<Polyline>{face ? face->polygon : cross->polygon}));
            mesh->name = name;
            mesh->set_objectcolor(face ? contact_color(face->type) : joint_color(30));
            scene.add_mesh(mesh, group);
        }
    }
}

/// Every joint's area, volumes, lines and male cuts under the `joints` group of its male element, the female cuts under the female's; beam features as their four volume rings.
static void add_joints_to(WoodSession& scene, const std::map<std::string, std::shared_ptr<TreeNode>>& groups, std::map<std::string, std::shared_ptr<TreeNode>>& children) {

    for (const auto& [guid, interaction] : scene.interactions) {

        const std::pair<std::string, std::string> ends = scene.edge_of(interaction);
        const auto owner = groups.find(ends.first);
        if (owner == groups.end())
            continue;

        for (const InteractionFeature& feature : interaction.features) {

            const FeatureBeam* beam = feature.beam();
            if (!beam)
                continue;

            const std::shared_ptr<TreeNode> group = child_group(scene, children, owner->second, "joints");
            const std::string name = fmt::format("beam_{}_{}_{}", short_guid(ends.first), short_guid(ends.second), beam->end_type);
            for (const Polyline& volume : beam->volumes)
                scene.add_polyline(ring(volume, Color(0.86f, 0.31f, 0.70f, 1.0f, "magenta"), name + "_volume"), group);
        }
    }

    for (const FeaturePlate& joint : scene.get_plate_features()) {

        const auto male = groups.find(joint.element_a);
        const auto female = groups.find(joint.element_b);
        if (male == groups.end() || female == groups.end())
            continue;

        const Color color = joint_color(joint.joint_type);
        const std::string name = fmt::format("joint_{}_{}_{}", short_guid(joint.element_a), short_guid(joint.element_b), joint_type_name(joint.joint_type));
        const std::shared_ptr<TreeNode> group = child_group(scene, children, male->second, "joints");

        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(Mesh::from_polylines(std::vector<Polyline>{joint.contact.polygon}));
        mesh->name = name;
        mesh->set_objectcolor(color);
        scene.add_mesh(mesh, group);

        for (const std::optional<Polyline>& volume : joint.joint_volumes)
            if (volume.has_value())
                scene.add_polyline(ring(*volume, color, name + "_volume"), group);

        for (int k = 0; k < 2; ++k) {
            std::shared_ptr<Line> line = std::make_shared<Line>(joint.joint_lines[k]);
            line->linecolor = color;
            line->width = 3.0;
            line->name = fmt::format("{}_line{}", name, k);
            scene.add_line(line, group);
        }

        for (const Polyline& outline : joint.male_outlines[0])
            scene.add_polyline(ring(outline, color, name + "_male_bottom_cut"), group);
        for (const Polyline& outline : joint.male_outlines[1])
            scene.add_polyline(ring(outline, color, name + "_male_top_cut"), group);

        const std::shared_ptr<TreeNode> other = child_group(scene, children, female->second, "joints");
        for (const Polyline& outline : joint.female_outlines[0])
            scene.add_polyline(ring(outline, color, name + "_female_bottom_cut"), other);
        for (const Polyline& outline : joint.female_outlines[1])
            scene.add_polyline(ring(outline, color, name + "_female_top_cut"), other);
    }
}

/// The group of every element that has one: the parent of the element's guid node when that parent is not the root.
static std::map<std::string, std::shared_ptr<TreeNode>> element_groups(WoodSession& scene) {

    std::map<std::string, std::shared_ptr<TreeNode>> groups;
    const std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodes = nodes_by_guid(scene.tree);
    for (const std::shared_ptr<Element>& element : *scene.objects.elements) {

        if (!element)
            continue;

        const std::unordered_map<std::string, std::shared_ptr<TreeNode>>::const_iterator found = nodes.find(element->guid());
        if (found == nodes.end())
            continue;

        const std::shared_ptr<TreeNode> parent = found->second->parent();
        if (parent && !parent->is_root())
            groups[element->guid()] = parent;
    }

    return groups;
}

/// The `attributes` child of a group, or null.
static std::shared_ptr<TreeNode> attributes_of(const std::shared_ptr<TreeNode>& group) {

    for (TreeNode* child : group->children())
        if (child->name == "attributes")
            return child->shared_from_this();

    return nullptr;
}

void show_attributes(WoodSession& scene, bool on) {

    const std::map<std::string, std::shared_ptr<TreeNode>> groups = element_groups(scene);

    if (!on) {
        for (const auto& [guid, group] : groups) {

            const std::shared_ptr<TreeNode> attributes = attributes_of(group);
            if (!attributes)
                continue;

            for (TreeNode* child : attributes->children())
                scene.remove_object(child->name);
            group->remove(attributes);
        }
        return;
    }

    scene.sync_geometry();
    for (const auto& [guid, group] : groups) {

        if (attributes_of(group))
            continue;

        const std::shared_ptr<Element> element = scene.get_element<Element>(guid);
        const std::shared_ptr<TreeNode> attributes = std::make_shared<TreeNode>("attributes");
        scene.add(attributes, group);
        for (const ElementFeature& feature : element->features()) {

            if (!is_geometry_feature(feature.feature_type))
                continue;

            const std::string name = fmt::format("{}_{}", element->name, feature.feature_type);
            for (const Polyline& outline : feature.outlines) {

                if (outline.point_count() == 1) {
                    std::shared_ptr<Point> point = std::make_shared<Point>(outline[0]);
                    point->name = name;
                    scene.add_point(point, attributes);
                    continue;
                }

                std::shared_ptr<Polyline> copy = std::make_shared<Polyline>(outline);
                copy->name = name;
                scene.add_polyline(copy, attributes);
            }
        }
    }
}

void add_to_tree(WoodSession& scene, bool with_geometry, bool with_attributes, bool with_contacts, bool with_joints) {

    std::map<std::string, std::shared_ptr<TreeNode>> groups;
    std::map<std::string, std::shared_ptr<TreeNode>> children;

    const std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodes = nodes_by_guid(scene.tree);

    size_t index = 0;
    for (const std::shared_ptr<Element>& element : *scene.objects.elements) {

        if (!element)
            continue;

        const std::shared_ptr<TreeNode> group = scene.add_group(fmt::format("{}_{}", element->name, index++));
        groups[element->guid()] = group;

        if (!with_geometry)
            continue;

        const std::unordered_map<std::string, std::shared_ptr<TreeNode>>::const_iterator found = nodes.find(element->guid());
        if (found == nodes.end())
            scene.add(std::make_shared<TreeNode>(element->guid()), group);
        else if (const std::shared_ptr<TreeNode> parent = found->second->parent()) {
            parent->remove(found->second);
            group->add(found->second);
        }
    }

    if (with_attributes)
        show_attributes(scene, true);

    if (with_contacts)
        add_contacts_to(scene, groups, children);

    if (with_joints)
        add_joints_to(scene, groups, children);
}

// ═══════════════════════════════════════════════════════════════════════════
// Colours
// ═══════════════════════════════════════════════════════════════════════════

std::string_view contact_type_name(ContactType type) {

    switch (type) {
        case ContactType::side_side: return "side_side";
        case ContactType::side_top: return "side_top";
        case ContactType::top_top: return "top_top";
        case ContactType::unknown: break;
    }

    return "unknown";
}

std::string joint_type_name(int joint_type) {
    switch (joint_type) {
        case 11: return "ss_op_11";
        case 12: return "ss_ip_12";
        case 13: return "ss_rot_13";
        case 20: return "ts_20";
        case 30: return "cross_30";
        case 40: return "tt_40";
        default: return fmt::format("type_{}", joint_type);
    }
}

/// A contact takes the colour of the joint class it refines to; an unclassified one is BRG's zero-grey.
Color contact_color(ContactType type) {

    switch (type) {
        case ContactType::side_side: return joint_color(12);
        case ContactType::side_top: return joint_color(20);
        case ContactType::top_top: return joint_color(40);
        case ContactType::unknown: break;
    }

    return joint_color(-1);
}

/// The BRG equilibrium palette (brg-teaching.github.io): navy, pink and green carry the meaning, grey is anything inert.
Color joint_color(int joint_type) {
    switch (joint_type) {
        case 12: return Color(0.102f, 0.118f, 0.698f, 1.0f, "ss_ip_navy");
        case 11: return Color(0.878f, 0.478f, 0.149f, 1.0f, "ss_op_orange");
        case 13: return Color(0.659f, 0.192f, 0.475f, 1.0f, "ss_rot_deep_pink");
        case 20: return Color(0.808f, 0.251f, 0.584f, 1.0f, "ts_pink");
        case 40: return Color(0.247f, 0.612f, 0.125f, 1.0f, "tt_green");
        case 30: return Color(0.910f, 0.675f, 0.000f, 1.0f, "cross_yellow");
        default: return Color(0.725f, 0.725f, 0.741f, 1.0f, "unknown_zero");
    }
}

} // namespace wood_session
