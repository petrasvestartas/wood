#pragma once

#include "../src/element.h"
#include "../src/line.h"
#include "../src/mesh.h"
#include "../src/polyline.h"

#include <memory>
#include <string>

namespace wood_session {

/// A column: a solid that knows its own axis and the section it is cut from.
class Column : public session_cpp::Element {
public:
    static constexpr const char* ELEMENT_TYPE = "Column";

    Column();
    Column(const session_cpp::Mesh& solid, const session_cpp::Line& axis, const session_cpp::Polyline& section,
           const std::string& name = "column");

    session_cpp::Line axis;          ///< centreline, base to head, in world space
    session_cpp::Polyline section;   ///< closed cross-section about the axis base; empty when unknown

    std::string element_type_name() const override { return ELEMENT_TYPE; }
    std::string element_data_dumps() const override;
    std::shared_ptr<session_cpp::Element> clone() const override { return std::make_shared<Column>(*this); }
    /// The column an Element tagged "Column" describes, same guid; a missing payload leaves axis and section default.
    static std::shared_ptr<Column> from_element(const session_cpp::Element& element);
    static void register_type();

    std::string str() const override;
};

} // namespace wood_session
