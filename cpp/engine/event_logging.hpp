#pragma once

#include <sstream>
#include <string>
#include <type_traits>

#include "game.hpp"

namespace pyrisk {

inline std::string escape_json(const std::string& in) {
    std::ostringstream oss;
    for (char c : in) {
        switch (c) {
            case '\\':
                oss << "\\\\";
                break;
            case '"':
                oss << "\\\"";
                break;
            default:
                oss << c;
                break;
        }
    }
    return oss.str();
}

inline std::string event_to_json(const Event& event) {
    std::ostringstream oss;
    oss << "{\"event\":\"" << escape_json(event.name) << "\",\"args\":[";
    for (std::size_t i = 0; i < event.args.size(); ++i) {
        const auto& arg = event.args[i];
        std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    oss << "\"" << escape_json(value) << "\"";
                } else if constexpr (std::is_same_v<T, int>) {
                    oss << value;
                } else if constexpr (std::is_same_v<T, std::pair<int, int>>) {
                    oss << "[" << value.first << "," << value.second << "]";
                }
            },
            arg);
        if (i + 1 < event.args.size()) {
            oss << ",";
        }
    }
    oss << "]}";
    return oss.str();
}

}  // namespace pyrisk
