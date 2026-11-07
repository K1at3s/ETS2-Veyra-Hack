#pragma once
#include <string_view>

namespace foundation {
constexpr bool is_game(std::wstring_view name) noexcept {
    constexpr std::wstring_view expected = L"eurotrucks2.exe";
    if (name.size() != expected.size()) return false;
    for (std::size_t i = 0; i < name.size(); ++i) {
        auto c = name[i];
        if (c >= L'A' && c <= L'Z') c += L'a' - L'A';
        if (c != expected[i]) return false;
    }
    return true;
}
}
