#pragma once
#include <json-c/json.h>
#include <memory>

namespace JSON
{
    // These are just to make this process quicker and avoid memory leaks.
    using Object = std::unique_ptr<json_object, decltype(&json_object_put)>;
    // I don't feel like typing this over and over.
    template <typename... Args>
    JSON::Object NewObject(json_object *(*Function)(Args...), Args... Arguments)
    {
        JSON::Object Object((*Function)(Arguments...), json_object_put);
        return std::move(Object);
    }
} // namespace JSON
