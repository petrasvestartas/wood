#include "pch.h"
#include "wood_instance.h"
#include "wood_session.h"
#include "wood_element_geometry.h"

namespace wood_session {

using namespace session_cpp;

namespace {

// ═══════════════════════════════════════════════════════════════════════════
// Keys
// ═══════════════════════════════════════════════════════════════════════════

/// A number in thousandths, so values that round alike write alike and -0 writes as 0.
void write(std::string& key, double value) {
    key += std::to_string(std::llround(value * 1000.0));
    key += ',';
}

/// A point in frame coordinates.
void write(std::string& key, const Point& point, const Xform& local) {

    const Point moved = point.transformed(local);

    for (int i = 0; i < 3; i++)
        write(key, moved[i]);
}

/// A vector in frame coordinates.
void write(std::string& key, const Vector& vector, const Xform& local) {

    const Vector moved = vector.transformed(local);

    for (int i = 0; i < 3; i++)
        write(key, moved[i]);
}

/// A polyline in frame coordinates.
void write(std::string& key, const Polyline& polyline, const Xform& local) {

    for (const Point& point : polyline.get_points())
        write(key, point, local);

    key += ';';
}

/// A plane by its origin and normal in frame coordinates.
void write(std::string& key, const Plane& plane, const Xform& local) {
    write(key, plane.origin(), local);
    write(key, plane.z_axis(), local);
    key += ';';
}

/// Every item in frame coordinates, then a list end.
template <class T>
void write(std::string& key, const std::vector<T>& items, const Xform& local) {

    for (const T& item : items)
        write(key, item, local);

    key += '|';
}

/// The right-handed frame at origin with z along normal and x along direction made perpendicular to it; nullopt when either is degenerate.
std::optional<Xform> compute_frame(const Point& origin, const Vector& direction, const Vector& normal) {

    const Vector z = normal.normalized();
    const Vector along = direction.normalized();
    const Vector x = along - z * along.dot(z);

    if (z.is_zero() || x.magnitude() < Tolerance::RELATIVE)
        return std::nullopt;

    return Xform::frame_to_world(origin, x, z.cross(x), z);
}

/// Origin at the axis start, z along the axis, x from the first section edge.
std::optional<std::pair<std::string, Xform>> column_key(const Column& column) {

    if (column.section.point_count() < 2)
        return std::nullopt;

    const std::optional<Xform> frame = compute_frame(column.axis.start(), column.section.get_point(1) - column.section.get_point(0), column.axis.to_vector());

    if (!frame)
        return std::nullopt;

    const Xform local = *frame->inverse();
    std::string key = "Column:";
    write(key, column.section, local);
    write(key, column.axis.end(), local);
    write(key, column.cuts, local);
    write(key, column.profile, Xform::identity());

    return std::make_pair(key, *frame);
}

/// Origin at the first axis point, x along the first segment, z from the first direction or world z; every segment keys the up it is swept with, world z where it has none.
std::optional<std::pair<std::string, Xform>> beam_key(const Beam& beam) {

    if (beam.axis.point_count() < 2)
        return std::nullopt;

    const Vector along = (beam.axis.get_point(1) - beam.axis.get_point(0)).normalized();
    const Vector up = beam.has_direction(0) ? beam.directions[0] : Vector::z_axis();
    const std::optional<Xform> frame = compute_frame(beam.axis.get_point(0), along, up - along * up.dot(along));

    if (!frame)
        return std::nullopt;

    const Xform local = *frame->inverse();
    std::string key = "Beam:" + std::to_string(beam.allowed_type) + ":";
    write(key, beam.axis, local);

    for (const double radius : beam.radii)
        write(key, radius);

    for (int segment = 0; segment < static_cast<int>(beam.axis.segment_count()); segment++) {
        key += beam.has_direction(segment) ? 'd' : 'z';
        write(key, beam.has_direction(segment) ? beam.directions[segment] : Vector::z_axis(), local);
    }

    write(key, beam.cuts, local);
    write(key, beam.profile, Xform::identity());

    return std::make_pair(key, *frame);
}

/// Origin at the first point of the bottom loop, x along its first edge, z its normal.
std::optional<std::pair<std::string, Xform>> block_key(const Block& block) {

    if (block.loops.empty() || block.loops[0].point_count() < 3)
        return std::nullopt;

    const Polyline& bottom = block.loops[0];
    const std::optional<Xform> frame = compute_frame(bottom.get_point(0), bottom.get_point(1) - bottom.get_point(0), Vector::average_normal(bottom));

    if (!frame)
        return std::nullopt;

    const Xform local = *frame->inverse();
    std::string key = "Solid:";
    write(key, block.loops, local);
    write(key, block.cuts, local);

    return std::make_pair(key, *frame);
}

/// Origin at the first bottom point, x along the first bottom edge, z the bottom plane's normal.
std::optional<std::pair<std::string, Xform>> plate_key(const Plate& plate) {

    if (plate.polylines.size() < 2 || plate.planes.empty() || plate.polylines[0].point_count() < 2)
        return std::nullopt;

    const Polyline& bottom = plate.polylines[0];
    const std::optional<Xform> frame = compute_frame(bottom.get_point(0), bottom.get_point(1) - bottom.get_point(0), plate.planes[0].z_axis());

    if (!frame)
        return std::nullopt;

    const Xform local = *frame->inverse();
    std::string key = "Plate:" + std::string(plate.reversed ? "r:" : "n:");
    write(key, plate.polylines[0], local);
    write(key, plate.polylines[1], local);
    write(key, plate.thickness);

    for (const int type : plate.feature_types)
        key += std::to_string(type) + ",";

    write(key, plate.insertion_vectors(), local);
    write(key, plate.features.top, local);
    write(key, plate.features.bottom, local);

    return std::make_pair(key, *frame);
}

// ═══════════════════════════════════════════════════════════════════════════
// Copies
// ═══════════════════════════════════════════════════════════════════════════

/// T::transformed for a wood type, else a clone placed by xform, guids kept; nullptr for a mirror.
std::shared_ptr<Element> transformed(const std::shared_ptr<Element>& element, const Xform& xform) {

    if (const std::shared_ptr<Column> column = std::dynamic_pointer_cast<Column>(element))
        return column->transformed(xform);

    if (const std::shared_ptr<Beam> beam = std::dynamic_pointer_cast<Beam>(element))
        return beam->transformed(xform);

    if (const std::shared_ptr<Block> block = std::dynamic_pointer_cast<Block>(element))
        return block->transformed(xform);

    if (const std::shared_ptr<Plate> plate = std::dynamic_pointer_cast<Plate>(element))
        return plate->transformed(xform);

    if (is_mirror(xform))
        return nullptr;

    const std::shared_ptr<Element> copy = std::get<std::shared_ptr<Element>>(clone(Geometry(element)));
    copy->place(xform);

    return copy;
}

/// The element a lookup table holds under guid, nullptr when it holds none or other geometry.
std::shared_ptr<Element> element_of(const std::unordered_map<std::string, Geometry>& table, const std::string& guid) {

    const std::unordered_map<std::string, Geometry>::const_iterator found = table.find(guid);

    if (found == table.end())
        return nullptr;

    const std::shared_ptr<Element>* element = std::get_if<std::shared_ptr<Element>>(&found->second);

    return element ? *element : nullptr;
}

/// definition_keys rebuilt from the element definitions when it is empty.
void index_definitions(WoodSession& session) {

    if (!session.definition_keys.empty())
        return;

    for (const std::shared_ptr<Element>& definition : *session.definitions.elements)
        if (const std::optional<std::pair<std::string, Xform>> key = element_key(*definition))
            session.definition_keys.emplace(key->first, definition->guid());
}

/// The guid of the definition stored under key, "" when none is.
std::string find_definition(const WoodSession& session, const std::string& key) {

    const std::unordered_map<std::string, std::string>::const_iterator found = session.definition_keys.find(key);

    if (found == session.definition_keys.end() || !session.definition_lookup.count(found->second))
        return "";

    return found->second;
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Keys
// ═══════════════════════════════════════════════════════════════════════════

std::optional<std::pair<std::string, Xform>> element_key(const Element& element) {

    if (const Column* column = dynamic_cast<const Column*>(&element))
        return column_key(*column);

    if (const Beam* beam = dynamic_cast<const Beam*>(&element))
        return beam_key(*beam);

    if (const Block* block = dynamic_cast<const Block*>(&element))
        return block_key(*block);

    if (const Plate* plate = dynamic_cast<const Plate*>(&element))
        return plate_key(*plate);

    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════════════
// WoodSession - Instances
// ═══════════════════════════════════════════════════════════════════════════

std::string WoodSession::add_definition(std::shared_ptr<Element> definition, const std::string& key) {

    index_definitions(*this);
    const std::string stored = find_definition(*this, key);

    if (!stored.empty())
        return stored;

    const std::string guid = Session::add_definition(definition);

    if (!key.empty() && !guid.empty())
        definition_keys[key] = guid;

    return guid;
}

std::shared_ptr<TreeNode> WoodSession::add_instance(const std::string& definition_guid, const Xform& xform, const std::string& name, std::shared_ptr<TreeNode> parent) {

    const std::shared_ptr<Element> definition = element_of(definition_lookup, definition_guid);

    if (!definition || is_mirror(xform))
        return nullptr;

    std::shared_ptr<InstanceRef> instance = std::make_shared<InstanceRef>(definition_guid, Xform::identity());
    instance->name = name.empty() ? definition->name : name;

    return Session::add_instance(instance, xform, parent);
}

/// A view of a column, beam or block takes its outlines and planes from the stored mesh, lofted once; a plate answers from its own members.
std::shared_ptr<Element> WoodSession::world_view(const std::string& guid, const Xform& world) const {

    const std::unordered_map<std::string, std::shared_ptr<InstanceRef>>::const_iterator instance = instance_lookup.find(guid);
    const std::shared_ptr<Element> source = instance == instance_lookup.end() ? element_of(lookup, guid) : element_of(definition_lookup, instance->second->definition_guid);

    if (!source)
        return nullptr;

    const std::shared_ptr<Element> view = transformed(source, world);

    if (!view)
        return nullptr;

    if (!std::dynamic_pointer_cast<Plate>(view)) {
        source->geometry_mesh();
        view->set_polylines(transformed_list(source->polylines(), world));
        view->set_planes(transformed_list(source->planes(), world));
    }

    if (instance != instance_lookup.end()) {
        view->guid() = guid;
        view->name = instance->second->name;
        view->set_features(transformed_features(instance->second->features, world));
    }

    return view;
}

/// Tree order once there is an element definition, since to_instance, explode and undo put every object back at its tree position; objects outside the tree follow in list order.
std::vector<std::shared_ptr<Element>> WoodSession::world_elements(const std::function<bool(const Element&)>& keep) const {

    const std::unordered_map<std::string, Xform> world = world_xforms();
    std::vector<std::string> guids;

    if (!definitions.elements->empty() && tree.root())
        for (const TreeNode* node : tree.root()->traverse())
            guids.push_back(node->name);

    for (const std::shared_ptr<Element>& element : *objects.elements)
        guids.push_back(element->guid());

    for (const std::shared_ptr<InstanceRef>& instance : *objects.instances)
        guids.push_back(instance->guid());

    std::unordered_set<std::string> seen;
    std::vector<std::shared_ptr<Element>> out;

    for (const std::string& guid : guids) {

        if (!seen.insert(guid).second)
            continue;

        const std::shared_ptr<Element> stored = element_of(lookup, guid);
        const std::unordered_map<std::string, std::shared_ptr<InstanceRef>>::const_iterator instance = instance_lookup.find(guid);
        const std::shared_ptr<Element> source = instance == instance_lookup.end() ? stored : element_of(definition_lookup, instance->second->definition_guid);

        if (!source || (keep && !keep(*source)))
            continue;

        const std::unordered_map<std::string, Xform>::const_iterator placed = world.find(guid);
        const Xform xform = placed == world.end() ? Xform::identity() : placed->second;

        if (stored && xform.is_identity())
            out.push_back(stored);
        else if (const std::shared_ptr<Element> view = world_view(guid, xform))
            out.push_back(view);
    }

    return out;
}

void WoodSession::host_feature(const std::string& guid, ElementFeature feature) {

    const Xform world = xforms.empty() ? Xform::identity() : world_xform(guid);

    if (!world.is_identity())
        feature.outlines = transformed_list(feature.outlines, *world.inverse());

    const std::unordered_map<std::string, std::shared_ptr<InstanceRef>>::const_iterator instance = instance_lookup.find(guid);

    if (instance != instance_lookup.end())
        instance->second->features.push_back(std::move(feature));
    else if (const std::shared_ptr<Element> element = element_of(lookup, guid))
        element->add_feature(std::move(feature));
}

/// An element without features is not read, so it does not loft; a mirroring frame is skipped.
size_t WoodSession::instance_by_key(const std::function<std::optional<std::pair<std::string, Xform>>(const Element&)>& key_of) {

    index_definitions(*this);
    size_t made = 0;
    const std::vector<std::shared_ptr<Element>> elements = *objects.elements;

    for (const std::shared_ptr<Element>& element : elements) {

        const std::optional<std::pair<std::string, Xform>> key = key_of(*element);

        if (!key || is_mirror(key->second))
            continue;

        const Xform local = *key->second.inverse();
        std::string definition = find_definition(*this, key->first);

        if (definition.empty()) {
            const std::shared_ptr<Element> canonical = transformed(element, local);
            canonical->refresh_guid();
            canonical->set_features({});
            definition = add_definition(canonical, key->first);
        }

        std::vector<ElementFeature> carried = element->features_count() > 0 ? transformed_features(session_features(*element), local) : std::vector<ElementFeature>();

        if (!to_instance(element->guid(), definition, key->second))
            continue;

        instance_lookup.at(element->guid())->features = std::move(carried);
        made++;
    }

    return made;
}

void WoodSession::promote(const std::shared_ptr<Element>& view) {

    const std::string guid = view->guid();
    const std::unordered_map<std::string, Geometry>::const_iterator stored = lookup.find(guid);

    if (stored != lookup.end() && stored->second == Geometry(view))
        return;

    explode(guid);
    replace(guid, transformed(view, *world_xform(guid).inverse()));
}

} // namespace wood_session
