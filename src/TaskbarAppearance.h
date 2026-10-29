#pragma once

#include <windows.h>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

enum class TaskbarMode { Default, Clear, BlurLight, BlurMedium, BlurStrong };

struct AppearancePreset {
    TaskbarMode mode;
    std::wstring_view name;
    double brushOpacity;
    double tintOpacity;
    double luminosityOpacity;
    std::uint8_t fallbackAlpha;
};

inline constexpr std::uint8_t AcrylicTintChannel = 32;

inline constexpr std::array<AppearancePreset, 5> AppearancePresets{{
    {TaskbarMode::Default, L"Default", 0.0, 0.0, 0.0, 0},
    {TaskbarMode::Clear, L"Clear", 0.0, 0.0, 0.0, 0},
    // Acrylic has a fixed blur radius here. Brush opacity blends its result
    // with the clear backdrop; tint and luminosity control dark frosting.
    {TaskbarMode::BlurLight, L"BlurLight", 0.62, 0.025, 0.06, 24},
    {TaskbarMode::BlurMedium, L"BlurMedium", 1.0, 0.12, 0.18, 100},
    {TaskbarMode::BlurStrong, L"BlurStrong", 1.0, 0.52, 0.50, 100},
}};

inline const AppearancePreset& Preset(TaskbarMode mode) {
    for (const auto& preset : AppearancePresets)
        if (preset.mode == mode) return preset;
    return AppearancePresets[0];
}

inline bool ParseMode(std::wstring_view text, TaskbarMode& mode) {
    for (const auto& preset : AppearancePresets) {
        if (text == preset.name) {
            mode = preset.mode;
            return true;
        }
    }
    return false;
}

inline std::wstring ModuleDirectory(HMODULE module) {
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        if (!length) return {};
        if (length < path.size()) {
            path.resize(length);
            const auto slash = path.find_last_of(L"\\/");
            return slash == std::wstring::npos ? std::wstring{} : path.substr(0, slash);
        }
        if (path.size() >= 32768) return {};
        path.resize(path.size() * 2);
    }
}

inline std::wstring ConfigPath(HMODULE module) {
    return ModuleDirectory(module) + L"\\clearbar.ini";
}

struct LoadedMode {
    TaskbarMode mode;
    bool valid;
};

inline LoadedMode LoadMode(HMODULE module) {
    wchar_t value[64]{};
    const auto path = ConfigPath(module);
    GetPrivateProfileStringW(L"ClearBar", L"Mode", L"", value, 64, path.c_str());
    TaskbarMode mode{};
    if (ParseMode(value, mode)) return {mode, true};
    // A missing setting preserves the behavior of an existing v2 installation.
    if (!*value) return {TaskbarMode::Clear, false};
    return {TaskbarMode::Default, false};
}

inline bool SaveMode(HMODULE module, TaskbarMode mode) {
    const auto path = ConfigPath(module);
    const auto name = Preset(mode).name;
    return WritePrivateProfileStringW(L"ClearBar", L"Mode", std::wstring(name).c_str(), path.c_str()) != FALSE;
}
