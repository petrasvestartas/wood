#include "wood_element_column.h"

namespace wood_session {

// ═══════════════════════════════════════════════════════════════════════════
// WoodColumn
// ═══════════════════════════════════════════════════════════════════════════

WoodColumn::WoodColumn()
    : element(std::make_shared<wood_session::TaggedElement>("column", ELEMENT_TYPE))
    , axis(Line::from_points(Point(0, 0, 0), Point(0, 0, 0)))
    , section(Polyline(std::vector<Point>{})) {}

Mesh WoodColumn::mesh() const {
    if (const Mesh* m = std::get_if<Mesh>(&element->geometry())) { return *m; }
    return Mesh{};
}

void WoodColumn::sync_faces() {
    polylines = element->polylines();
    planes    = element->planes();
}

std::shared_ptr<Element> WoodColumn::to_element() const {
    nlohmann::ordered_json payload{
        {"type", ELEMENT_TYPE},
        {"axis", axis.jsondump()},
    };
    if (section.point_count() > 0) { payload["section"] = section.jsondump(); }
    // The same object, payload refreshed - not a copy.
    element->set_element_data(payload.dump());
    return element;
}

WoodColumn WoodColumn::from_element(const Element& e) {
    WoodColumn out;
    // Wrapped once, here, and shared from now on: the kernel has no public setter for
    // element_type / element_data, so owning a tagged element means deriving one. The
    // TaggedElement ctor carries the guid across, so this is the same element, not a new one.
    out.element = std::make_shared<wood_session::TaggedElement>(e, ELEMENT_TYPE, e.element_data_dumps());

    if (!std::holds_alternative<Mesh>(e.geometry())) {
        fprintf(stderr, "  WARNING: WoodColumn::from_element: element '%s' carries %s, not a "
                        "Mesh - column left empty.\n", e.name.c_str(), e.geometry_type_name().c_str());
        fflush(stderr);
        return out;
    }
    out.sync_faces();

    // The payload is optional: without it the column is still a usable solid, it just
    // cannot say where its axis runs.
    const std::string data = e.element_data_dumps();
    if (data.empty()) { return out; }
    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(data);
    } catch (const std::exception& ex) {
        fprintf(stderr, "  WARNING: WoodColumn::from_element: element '%s' has unparseable "
                        "element_data (%s) - axis and section left empty.\n", e.name.c_str(), ex.what());
        fflush(stderr);
        return out;
    }
    if (payload.contains("axis") && !payload["axis"].is_null()) {
        out.axis = Line::jsonload(payload["axis"]);
    }
    if (payload.contains("section") && !payload["section"].is_null()) {
        out.section = Polyline::jsonload(payload["section"]);
    }
    return out;
}

std::string WoodColumn::str() const {
    std::ostringstream os;
    os << "WoodColumn(name=" << element->name
       << ", faces=" << polylines.size()
       << ", axis_length=" << axis.length()
       << ", section_pts=" << section.point_count() << ")";
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const WoodColumn& e) { return os << e.str(); }

} // namespace wood_session
