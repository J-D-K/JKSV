#pragma once
#include <json-c/json.h>
#include <memory>

namespace JSON
{
    // Use this instead of default json_object
    using Object = std::unique_ptr<json_object, decltype(&json_object_put)>;

    // Use this instead of json_object_from_x. Pass the function and its arguments instead.
    template <typename... Args>
    static inline JSON::Object NewObject(json_object *(*Function)(Args...), Args... Arguments)
    {
        return JSON::Object((*Function)(Arguments...), json_object_put);
    }
} // namespace JSON
