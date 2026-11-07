#include "../src/local_pass_patch.hpp"
#include <iostream>
#include <cstring>

namespace foundation {
struct LocalPassPatchFixture {
    static bool run() {
        auto memory = static_cast<unsigned char*>(VirtualAlloc(nullptr, 0x200000,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (!memory) return false;
        bool ok = false;
        {
            LocalPassPatch patch;
            patch.process_ = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE |
                PROCESS_VM_OPERATION | SYNCHRONIZE, FALSE, GetCurrentProcessId());
            patch.base_ = reinterpret_cast<std::uintptr_t>(memory);
            std::memcpy(memory + patch.context_rva, patch.expected.data(), patch.expected.size());
            memory[0x1b40b1] = 0x75;
            DWORD old = 0;
            if (!patch.process_ || !VirtualProtect(memory, 0x200000, PAGE_EXECUTE_READ, &old)) {
                VirtualFree(memory, 0, MEM_RELEASE); return false;
            }
            std::wstring message;
            MEMORY_BASIC_INFORMATION info{};
            ok = patch.context_matches(false) && patch.write_branch(0x00, message)
                && patch.active() && patch.context_matches(true)
                && memory[0x1b40b1] == 0x75
                && VirtualQuery(memory + patch.branch_rva, &info, sizeof(info))
                && info.Protect == PAGE_EXECUTE_READ;
            // An unexpected third-party byte must prevent overwrite on restoration.
            DWORD previous = 0;
            if (ok && VirtualProtect(memory + patch.context_rva, 12, PAGE_EXECUTE_READWRITE, &previous)) {
                memory[patch.context_rva] ^= 1;
                DWORD ignored = 0;
                ok = VirtualProtect(memory + patch.context_rva, 12, previous, &ignored)
                    && !patch.restore(message) && patch.needs_restore();
                if (VirtualProtect(memory + patch.context_rva, 12, PAGE_EXECUTE_READWRITE, &previous)) {
                    memory[patch.context_rva] = patch.expected[0];
                    ok = VirtualProtect(memory + patch.context_rva, 12, previous, &ignored) && ok;
                } else ok = false;
            } else ok = false;
            ok = patch.restore(message) && ok;
            ok = ok && !patch.needs_restore() && memory[0x191884] == 0x01
                && memory[0x1b40b1] == 0x75;
        }
        VirtualFree(memory, 0, MEM_RELEASE);
        return ok;
    }
};
}
int main() {
    const bool ok = foundation::LocalPassPatchFixture::run();
    std::cout << (ok ? "PASS: self-process write/readback, page protection, mismatch refusal, restore.\n"
                     : "FAIL: local-pass fixture.\n");
    return ok ? 0 : 1;
}
