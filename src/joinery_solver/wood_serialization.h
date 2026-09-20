#pragma once

#include "pch.h"

#include <google/protobuf/message.h>

namespace wood_session {

/// A message as ordered JSON: "type" with the message name first, then every field under its proto name, defaults included.
nlohmann::ordered_json json_of(const google::protobuf::Message& message);

/// A message filled from JSON written by json_of; keys the message does not know, such as "type", are skipped.
void message_from_json(const nlohmann::json& data, google::protobuf::Message& message);

} // namespace wood_session
