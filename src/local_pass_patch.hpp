#pragma once
#if !defined(_WIN64)
#error The HaulMP experimental trainer requires Windows x64.
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace foundation {
// Experimental LOCAL filter gate for HaulMP 868. Does not write published NCZ.
// Inverts the filter's NCZ comparison: outside -> candidate suppression path.
// Freshness/class/identity tests remain. Real zones retain other original logic.
// This is not a force/impulse generator. Peer effects and stability unverified.
class LocalPassPatch {
#ifdef FOUNDATION_PATCH_TESTS
    friend struct LocalPassPatchFixture;
#endif
    HANDLE process_ = nullptr;
    std::uintptr_t base_ = 0;
    bool active_ = false;
    DWORD pending_protection_ = 0;
    static constexpr std::uintptr_t branch_rva = 0x191884;
    static constexpr std::uintptr_t context_rva = 0x19187b;
    static constexpr std::array<unsigned char, 12> expected{
        0x0f,0xb6,0x0d,0x2e,0x90,0x79,0x00,0x80,0xf9,0x01,0x75,0x37};
    static constexpr std::array<unsigned char, 32> digest{
        0xb0,0xfa,0xcd,0x6f,0xe0,0xb1,0xee,0xe0,0xf4,0x4a,0xdd,0xd7,0xbd,0xa8,0xae,0xa3,
        0x3f,0x17,0xd1,0xbb,0x85,0xce,0x0d,0x86,0xc0,0x1f,0x21,0xcf,0x05,0xc1,0x6c,0xc5};

    static bool known_file(const wchar_t* path) {
        std::ifstream input(std::filesystem::path(path), std::ios::binary | std::ios::ate);
        if (!input || input.tellg() != std::streampos(9697264)) return false;
        input.seekg(0);
        std::vector<unsigned char> data(9697264);
        if (!input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()))) return false;
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return false;
        std::array<unsigned char,32> actual{};
        const auto result = BCryptHash(algorithm, nullptr, 0, data.data(),
            static_cast<ULONG>(data.size()), actual.data(), static_cast<ULONG>(actual.size()));
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return result >= 0 && actual == digest;
    }
    bool read(std::uintptr_t offset, void* buffer, SIZE_T size) const {
        SIZE_T done = 0;
        return process_ && ReadProcessMemory(process_, reinterpret_cast<const void*>(base_ + offset),
            buffer, size, &done) && done == size;
    }
    bool context_matches(bool enabled) const {
        std::array<unsigned char,12> bytes{};
        auto wanted = expected;
        if (enabled) wanted[9] = 0x00;
        return read(context_rva, bytes.data(), bytes.size()) && bytes == wanted;
    }
    void release() {
        if (process_) CloseHandle(process_);
        process_ = nullptr; base_ = 0; pending_protection_ = 0;
    }
    bool connect(DWORD pid, std::wstring& message) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot == INVALID_HANDLE_VALUE) { message = L"Module scan failed: " + std::to_wstring(GetLastError()); return false; }
        MODULEENTRY32W module{}; module.dwSize = sizeof(module);
        bool known = false;
        for (BOOL more = Module32FirstW(snapshot, &module); more; more = Module32NextW(snapshot, &module)) {
            if (_wcsicmp(module.szModule, L"haulmp_stub.dll") == 0) {
                known = module.modBaseSize > branch_rva && known_file(module.szExePath);
                if (known) base_ = reinterpret_cast<std::uintptr_t>(module.modBaseAddr);
                break;
            }
        }
        CloseHandle(snapshot);
        if (!known) { message = L"Unsupported or missing HaulMP module. Only the inspected build 868 hash is accepted."; return false; }
        process_ = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
                               SYNCHRONIZE, FALSE, pid);
        if (!process_) { message = L"Game access failed: " + std::to_wstring(GetLastError()); return false; }
        return true;
    }
    bool write_branch(unsigned char value, std::wstring& message) {
        auto location = reinterpret_cast<void*>(base_ + branch_rva);
        DWORD old_protection = 0;
        if (!VirtualProtectEx(process_, location, 1, PAGE_EXECUTE_READWRITE, &old_protection)) {
            message = L"Cannot change code-page protection: " + std::to_wstring(GetLastError()); return false;
        }
        SIZE_T done = 0;
        if (!pending_protection_) pending_protection_ = old_protection;
        const bool wrote = WriteProcessMemory(process_, location, &value, 1, &done) && done == 1;
        const DWORD write_error = wrote ? 0 : GetLastError();
        const bool flushed = FlushInstructionCache(process_, location, 1) != FALSE;
        DWORD ignored = 0;
        const bool protected_again = VirtualProtectEx(process_, location, 1, pending_protection_, &ignored) != FALSE;
        if (protected_again) pending_protection_ = 0;
        unsigned char actual = 0;
        const bool verified = read(branch_rva, &actual, 1) && actual == value;
        // Track the actual changed byte even if a later flush/protection step fails.
        if (verified || wrote) active_ = value == 0x00;
        if (!wrote || !flushed || !protected_again || !verified) {
            message = L"Patch operation incomplete; write error=" + std::to_wstring(write_error)
                + L", cache=" + std::to_wstring(flushed) + L", protection=" + std::to_wstring(protected_again)
                + L", readback=" + std::to_wstring(verified) + L". Retry restoration before exiting.";
            return false;
        }
        return true;
    }
public:
    LocalPassPatch() = default;
    LocalPassPatch(const LocalPassPatch&) = delete;
    LocalPassPatch& operator=(const LocalPassPatch&) = delete;
    ~LocalPassPatch() { std::wstring ignored; restore(ignored); release(); }
    bool active() const { return active_; }
    bool needs_restore() const { return active_ || pending_protection_ != 0; }
    bool alive() const { return process_ && WaitForSingleObject(process_, 0) == WAIT_TIMEOUT; }
    std::optional<unsigned char> zone_status() const {
        unsigned char value = 0;
        if (!alive() || !read(0x92a8b0, &value, 1)) return std::nullopt;
        return value;
    }
    bool enable(DWORD pid, std::wstring& message) {
        if (needs_restore()) { message = L"Restore the previous operation before enabling again."; return false; }
        release();
        if (!connect(pid, message)) { release(); return false; }
        unsigned char legacy = 0;
        if (!read(0x1b40b1, &legacy, 1) || legacy != 0x75) {
            message = L"Legacy NCZ override detected or unreadable. Restore it first; no write performed.";
            release(); return false;
        }
        if (!context_matches(false)) {
            message = L"Code mismatch or another override is installed; no write performed."; release(); return false;
        }
        if (!write_branch(0x00, message)) return false;
        message = L"Local filter experiment installed. Published NCZ is not written. Pass-through and peer effects unverified.";
        return true;
    }
    bool restore(std::wstring& message) {
        if (!active_ && !pending_protection_) { release(); return true; }
        if (!alive()) { active_ = false; release(); message = L"Game exited; override no longer exists."; return true; }
        if (context_matches(false)) {
            if (pending_protection_) {
                DWORD ignored = 0;
                if (!VirtualProtectEx(process_, reinterpret_cast<void*>(base_ + branch_rva), 1, pending_protection_, &ignored)) {
                    message = L"Original branch is present but page protection restoration failed."; return false;
                }
            }
            active_ = false; release(); message = L"Original branch and page protection verified."; return true;
        }
        if (!context_matches(true)) {
            message = L"Restoration refused: game code changed unexpectedly. Override status unknown."; return false;
        }
        if (!write_branch(0x01, message)) return false;
        release();
        message = L"Original local filter comparison restored and verified.";
        return true;
    }
};
}
