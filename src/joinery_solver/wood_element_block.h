#pragma once

#include "../src/element.h"
#include "../src/mesh.h"
#include "../src/polyline.h"

#include <memory>
#include <string>
#include <vector>

namespace wood_session {

/// A solid for contact detection only: one face per closed loop, no plate convention.
class Block : public session_cpp::Element {
public:
    static constexpr const char* ELEMENT_TYPE = "Solid";
    static constexpr const char* LEGACY_ELEMENT_TYPE = "BlockElement";

    Block();
    /// Mesh::from_polylines: one n-gon face per loop, loops with fewer than 3 points dropped. `name` is the type flag face_contacts() filters on.
    explicit Block(const std::vector<session_cpp::Polyline>& loops, const std::string& name = "block");

    std::string element_type_name() const override { return ELEMENT_TYPE; }
    std::shared_ptr<session_cpp::Element> clone() const override { return std::make_shared<Block>(*this); }
    /// The block an Element describes, same guid: any element whose geometry is a mesh.
    static std::shared_ptr<Block> from_element(const session_cpp::Element& element);
    static void register_type();

    std::string str() const override;
};

} // namespace wood_session
