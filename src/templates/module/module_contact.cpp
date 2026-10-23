#include "src/templates/module/module_contact.h"

namespace wood_module {

// ═══════════════════════════════════════════════════════════════════════════
// Contacts of a framed module
// ═══════════════════════════════════════════════════════════════════════════

std::array<size_t, 2> add_module_contacts(WoodSession& session) {

    const std::shared_ptr<TreeNode> frame_group = session.group_named("contacts_frame");
    const std::shared_ptr<TreeNode> sheathing_group = session.group_named("contacts_sheathing");
    std::vector<std::shared_ptr<Element>> parts;
    std::vector<OBB> boxes;
    std::array<size_t, 2> found = {0, 0};

    for (const std::shared_ptr<Element>& element : *session.objects.elements)
        if (std::dynamic_pointer_cast<Beam>(element) || std::dynamic_pointer_cast<Plate>(element)) {
            parts.push_back(element);
            boxes.push_back(element->aabb());
        }

    for (size_t i = 0; i < parts.size(); i++)
        for (size_t j = i + 1; j < parts.size(); j++) {

            // boxes apart by more than a millimetre on any axis: no contact to search
            const Vector gap = boxes[i].center - boxes[j].center;
            bool apart = false;

            for (size_t axis = 0; axis < 3; axis++)
                if (std::abs(gap[axis]) > boxes[i].half_size[axis] + boxes[j].half_size[axis] + 1.0)
                    apart = true;

            if (apart)
                continue;

            const std::shared_ptr<Beam> beam_i = std::dynamic_pointer_cast<Beam>(parts[i]);
            const std::shared_ptr<Beam> beam_j = std::dynamic_pointer_cast<Beam>(parts[j]);

            // frame: the member that ends on the contact is its target, the one it stands on the source; side by side the first is the source
            if (beam_i && beam_j) {
                const std::shared_ptr<InteractionContactFace> probe = session.compute_face_contact(beam_i, beam_j);

                if (!probe)
                    continue;

                Point origin;
                Plane plane;
                probe->polygon.get_fast_plane(origin, plane);
                const Vector normal = plane.z_axis();
                const Vector along_i = (beam_i->axis[1] - beam_i->axis[0]).normalized();
                const Vector along_j = (beam_j->axis[1] - beam_j->axis[0]).normalized();
                const double end_i = std::abs(along_i.dot(normal));
                const double end_j = std::abs(along_j.dot(normal));

                // side by side, as a jamb and a stud next to it: neither ends on the other
                const bool beside = std::max(end_i, end_j) < 0.9;
                const std::shared_ptr<Beam> source = !beside && end_i > end_j ? beam_j : beam_i;
                const std::shared_ptr<Beam> target = !beside && end_i > end_j ? beam_i : beam_j;
                const std::shared_ptr<InteractionContactFace> face = session.compute_face_contact(source, target);
                face->name = beside ? fmt::format("{}_beside_{}", target->name, source->name) : fmt::format("{}_on_{}", target->name, source->name);
                session.add_interaction(source, target, face);
                Mesh patch = Mesh::from_polylines(std::vector<Polyline>{face->polygon});
                patch.set_objectcolor(Color::red());
                session.add_mesh(patch, frame_group);
                found[0]++;
                continue;
            }

            // sheathing: a plate on a member or on another plate, the plate the source
            const std::shared_ptr<Element> source = beam_i ? parts[j] : parts[i];
            const std::shared_ptr<Element> target = beam_i ? parts[i] : parts[j];
            const std::shared_ptr<InteractionContactFace> face = session.compute_face_contact(source, target);

            if (!face)
                continue;

            face->name = fmt::format("{}_on_{}", source->name, target->name);
            session.add_interaction(source, target, face);
            Mesh patch = Mesh::from_polylines(std::vector<Polyline>{face->polygon});
            patch.set_objectcolor(Color::red());
            session.add_mesh(patch, sheathing_group);
            found[1]++;
        }

    return found;
}

}
