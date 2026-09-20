#include "pch.h"
#include "wood_serialization.h"

#include <google/protobuf/json/json.h>

namespace wood_session {

nlohmann::ordered_json json_of(const google::protobuf::Message& message) {

    google::protobuf::json::PrintOptions options;
    options.preserve_proto_field_names = true;
    options.always_print_fields_with_no_presence = true;
    options.unquote_int64_if_possible = true;

    std::string text;
    google::protobuf::json::MessageToJsonString(message, &text, options).IgnoreError();

    nlohmann::ordered_json data = nlohmann::ordered_json::object();
    data["type"] = message.GetDescriptor()->name();
    for (auto& [key, value] : nlohmann::ordered_json::parse(text).items())
        data[key] = std::move(value);

    return data;
}

void message_from_json(const nlohmann::json& data, google::protobuf::Message& message) {

    google::protobuf::json::ParseOptions options;
    options.ignore_unknown_fields = true;

    google::protobuf::json::JsonStringToMessage(data.dump(), &message, options).IgnoreError();
}

} // namespace wood_session
