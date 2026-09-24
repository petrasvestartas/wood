#pragma once

#include "pch.h"

#include "wood_interaction_contact_face.h"
#include "wood_interaction_contact_axis.h"
#include "wood_interaction_contact_cross.h"

namespace wood_session {

class WoodSession;

/// One place where the edge's two elements touch, exactly one kind: a face overlap, a closest segment or a crossing.
struct InteractionContact {
    WoodSession* _session = nullptr; // The scene this record was stored in; null until it is added, never written.
    std::string guid; // Key of this contact; minted when the interaction stores it.
    std::variant<ContactFace, ContactAxis, ContactCross> data; // The kind, one at a time.

    InteractionContact() = default;

    /// A face contact.
    explicit InteractionContact(ContactFace face);

    /// An axis contact.
    explicit InteractionContact(ContactAxis axis);

    /// A cross contact.
    explicit InteractionContact(ContactCross cross);


    /// The scene this record belongs to; throws std::logic_error before the record is added to one.
    WoodSession& session() const;

    /// True once the record has been stored in a scene.
    bool has_session() const {
        return _session != nullptr;
    }

    /// Stores the scene on this record and on the kind it holds.
    void set_session(WoodSession* scene);

    // ═══════════════════════════════════════════════════════════════════════════
    // Operators
    // ═══════════════════════════════════════════════════════════════════════════

    /// str() onto a stream.
    friend std::ostream& operator<<(std::ostream& os, const InteractionContact& contact);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The face contact, or null when this is another kind.
    const ContactFace* face() const {
        return std::get_if<ContactFace>(&data);
    }

    /// The axis contact, or null when this is another kind.
    const ContactAxis* axis() const {
        return std::get_if<ContactAxis>(&data);
    }

    /// The cross contact, or null when this is another kind.
    const ContactCross* cross() const {
        return std::get_if<ContactCross>(&data);
    }

    /// "face", "axis" or "cross".
    std::string_view kind() const;

    /// The kind flipped, same guid: the contact read from the other end of the edge.
    InteractionContact flipped() const;

    /// Same kind and the kind's coincides(): what detection reuses instead of appending twice.
    bool coincides(const InteractionContact& other) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The contact as JSON: guid, kind, data.
    nlohmann::ordered_json jsondump() const;

    /// A contact from its JSON.
    static InteractionContact jsonload(const nlohmann::json& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The contact as wood_proto.InteractionContact bytes.
    std::string pb_dumps() const;

    /// A contact from wood_proto.InteractionContact bytes.
    static InteractionContact pb_loads(const std::string& data);

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "InteractionContact(guid, kind)".
    std::string str() const;
};

} // namespace wood_session
