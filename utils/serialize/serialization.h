#ifndef __SERIALIZATION_H__
#define __SERIALIZATION_H__

#include "static_reflection.h"
#include "daqi/da4qi4.hpp"

namespace {

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

template <typename T>
constexpr bool is_optional_v = is_optional<std::decay_t<T>>::value;

template <typename T>
constexpr bool has_schema = std::tuple_size<decltype(StructSchema<T>())>::value;

}  // namespace

namespace nlohmann {

template <typename T>
struct adl_serializer<T, std::enable_if_t<::has_schema<T>>> {
    template <typename BasicJsonType>
    static void to_json(BasicJsonType& j, const T& value) {
        ForEachField(value, [&j](auto&& field, auto&& name) { j[name] = field; });
    }

    template <typename BasicJsonType>
    static void from_json(const BasicJsonType& j, T& value) {
        ForEachField(value, [&j](auto&& field, auto&& name) {
            // ignore missing field of optional
            if (::is_optional_v<decltype(field)> && j.find(name) == j.end())
                return;

            j.at(name).get_to(field);
        });
    }
};

template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
        j = opt ? json(*opt) : json(std::nullopt);
    }

    static void from_json(const json& j, std::optional<T>& opt) {
        opt = !j.is_null() ? std::make_optional<T>(j.get<T>()) : std::nullopt;
    }
};

}  // namespace nlohmann


#endif /* __SERIALIZATION_H__ */