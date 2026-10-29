# ClearBar v2.3.0

Experimental native Windows 11 taskbar customizer. ClearBar uses C++/WinRT and Microsoft's XAML Diagnostics API to load a small TAP DLL into Explorer. It does not require the .NET SDK.

## Install and choose an appearance

1. Extract the project and close other taskbar styling tools.
<<<<<<< HEAD
2. Double-click `install.cmd` and choose an appearance.
3. The installer uses the included native x64 binaries when build tools are unavailable, so end users do not need Visual Studio. If Visual Studio C++ Build Tools are detected, it attempts to rebuild the updated source. A failed automatic rebuild restores the original binaries and displays a warning rather than silently leaving a half-built binary pair.
4. The installer copies the selected binaries to `%LOCALAPPDATA%\ClearBar`, enables auto-start, and waits for the taskbar change to be confirmed. If installation fails, the error remains visible in the Command Prompt.

To rebuild manually, install Visual Studio C++ x64 Build Tools and the Windows SDK, then run `build.cmd`. To require a rebuild during installation, open Command Prompt in the project folder and run `set CLEARBAR_REBUILD=1 && install.cmd`. A source-level fix is active in the installed app only after a successful rebuild; the bundled prebuilt binaries are retained as a fallback for machines without build tools.
=======
2. Run `install.cmd`.
3. Choose an appearance. The installer builds the native x64 binaries with Visual Studio Build Tools and the Windows SDK, installs them in `%LOCALAPPDATA%\ClearBar`, enables auto-start, and waits for the taskbar to be updated.
>>>>>>> origin/main

| Mode | Appearance |
| --- | --- |
| Clear | Fully transparent, matching the v2.2.2 opacity behavior. |
| Blur - Light | Noticeable but restrained acrylic softening with very little dark tint. |
| Blur - Medium | Clearly visible, balanced acrylic blur; close to the previous Light preset. |
| Blur - Strong | Acrylic blur with a stronger tint while remaining translucent. |
| Windows Default | Restores the original taskbar XAML fill, stroke, and opacity. ClearBar remains installed. |

Run `install.cmd` again to change appearance, repair the installation, check status, or uninstall. Changing appearance saves the setting and applies it without rebuilding or restarting Explorer. Repair and upgrades restart Explorer once so it loads the new TAP DLL.

The selection is stored in `%LOCALAPPDATA%\ClearBar\clearbar.ini` as `Mode=Clear`, `BlurLight`, `BlurMedium`, `BlurStrong`, or `Default`. ClearBar reads it at startup and the TAP checks for changes while Explorer is running. If Explorer restarts, the controller reconnects and reapplies the saved mode. A missing file falls back to Clear for compatibility with older installations; an invalid value falls back to Windows Default.

## Tools and troubleshooting

- `build.cmd` compiles the controller and TAP DLL manually.
- `status.cmd` displays the saved setting, controller process, Run entry, applied state, and recent log messages.
- `uninstall.cmd` removes ClearBar and restarts Explorer to unload the TAP DLL and restore the normal taskbar.

<<<<<<< HEAD
If installation reports that it cannot confirm a taskbar change, run `status.cmd` and inspect `%LOCALAPPDATA%\ClearBar\clearbar.log`. If an automatic source rebuild fails, check that Visual Studio C++ x64 Build Tools and Windows SDK components are installed. The installer restores the bundled binaries after an automatic build failure; run with `CLEARBAR_REBUILD=1` if you want a failed rebuild to stop installation instead of falling back.
=======
If installation reports that it cannot confirm a taskbar change, run `status.cmd` and inspect `%LOCALAPPDATA%\ClearBar\clearbar.log`. Build failures usually mean that Visual Studio C++ x64 Build Tools or Windows SDK components are missing.
>>>>>>> origin/main

## Windows limitations

ClearBar requires Windows 11 build 22000 or newer. It changes only `BackgroundFill` and `BackgroundStroke` inside `TaskbarFrame`; these taskbar XAML names are internal Windows details and can change after a Windows update. Acrylic is rendered by Windows with a fixed blur radius in this API. The presets vary the acrylic brush's opacity, tint, and luminosity instead of claiming to set a blur radius. Light blends less acrylic than Medium while keeping its dark tint low, so the blur remains visible without turning the taskbar into a dark strip. Windows may render the acrylic fallback when Transparency effects are disabled, power saving is active, or the compositor cannot provide the effect. In those conditions, the visual result cannot be guaranteed to stay translucent.

The implementation follows the native Windows XAML [AcrylicBrush API](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.media.acrylicbrush) and the taskbar's existing transparent XAML surface. The [XAML Diagnostics API](https://learn.microsoft.com/en-us/windows/win32/api/xamlom/) is documented, while Explorer's taskbar tree is not a stable public extension point.
