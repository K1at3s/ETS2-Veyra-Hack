#include "../src/core.hpp"
#include <iostream>

int main() {
    using namespace foundation;
    int failures = 0;
    auto check = [&](bool ok, const char* name) {
        if (!ok) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
    };
    check(is_game(L"eurotrucks2.exe"), "exact executable");
    check(is_game(L"EUROTRUCKS2.EXE"), "case insensitive executable");
    check(!is_game(L"eurotrucks2.exe.bak"), "reject suffix");
    check(!is_game(L""), "reject empty");
    check(!is_game(L"eurotrucks2"), "reject partial executable");
    return failures ? 1 : 0;
}
