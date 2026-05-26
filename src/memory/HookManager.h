#pragma once
#include <string>
#include <format>

#include "Memory.h"

class HookManager {
public:
    struct RegisteredHook
    {
        int32_t id;
        Memory mem;
        explicit RegisteredHook(const int32_t id, const Memory mem) : id(id), mem(mem) {  }
        
        bool operator==(const RegisteredHook &other) const {
            return id == other.id;
        }
    };

    static inline uint32_t lastId = 0;
    
    template <class T, std::size_t Size>
    static RegisteredHook RegisterHook(const char(& pattern)[Size], T* fn, T** orig) {
        static Memory::Pattern pat(pattern); // todo constexpr
        static auto mem = pat.Search();
        return RegisterHook(mem, fn, orig);
    }
    
    template <class T>
    static RegisteredHook RegisterHook(Memory mem, T* fn, T** orig) {
        const auto id = lastId++;
        mem.Detour(fn, orig, id);
        return RegisteredHook(id, mem);
    }
    
    template <class T, std::size_t Size>
    static RegisteredHook RegisterHookExclusive(const char(& pattern)[Size], T* fn, T** orig) {
        static Memory::Pattern pat(pattern); // todo constexpr
        static auto mem = pat.Search();
        return RegisterHookExclusive(mem, fn, orig);
    }
    
    template <class T>
    static RegisteredHook RegisterHookExclusive(Memory mem, T* fn, T** orig) {
        mem.ExclusiveDetour(fn, orig);
        return RegisteredHook(-1, mem);
    }

    static void UnregisterHook(RegisteredHook hook);
};
    
template <>
struct std::hash<HookManager::RegisteredHook> {
    auto operator()(const HookManager::RegisteredHook &mem) const noexcept -> size_t {
        return mem.id;
    }
};