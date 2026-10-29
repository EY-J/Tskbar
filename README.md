# Tskbar V1

Tskbar is an experimental native Windows 11 taskbar customizer built with C++/WinRT and Microsoft's XAML Diagnostics API. It allows users to customize the appearance of the Windows taskbar without requiring the .NET SDK.

## Features

- **Clear** — Makes the taskbar fully transparent.
- **Blur - Light** — Applies a subtle acrylic blur with minimal dark tint.
- **Blur - Medium** — Applies a more noticeable acrylic blur.
- **Blur - Strong** — Applies acrylic blur with a stronger tint while remaining translucent.
- **Windows Default** — Restores the original taskbar appearance while keeping Tskbar installed.

Additional features include:

- Change taskbar appearance.
- Repair or reinstall the application.
- Check installation status.
- Automatically start the controller with Windows.
- Save and reapply the selected appearance after Explorer restarts.
- Uninstall the application and restore the normal taskbar.

## Installation

1. Download the latest Tskbar ZIP from the [GitHub Releases](https://github.com/EY-J/Tskbar/releases) page.
2. Extract the downloaded ZIP file.
3. Close other taskbar customization tools.
4. Run `install.cmd`.
5. Choose your preferred taskbar appearance.
6. Follow the instructions displayed in the Command Prompt.

The installer can use the included native x64 binaries when Visual Studio C++ Build Tools are unavailable. When compatible build tools are detected, it attempts to rebuild the source code before installation.

If the automatic rebuild fails, the installer attempts to restore the bundled binaries and displays a warning.

## Changing Taskbar Appearance

Run `install.cmd` again to access the following options:

1. Change appearance
2. Repair / reinstall
3. Check status
4. Uninstall
5. Exit

Changing the appearance saves the selected setting and attempts to apply it without rebuilding the application. Repair and reinstall operations may restart Windows Explorer so it can load the updated components.

## Requirements

- Windows 11, build 22000 or newer.
- A compatible 64-bit Windows environment.
- Administrator permissions may be required for certain operations.
- Visual Studio C++ x64 Build Tools and the Windows SDK are needed to rebuild the native source code manually.

End users generally do not need Visual Studio when the bundled binaries are available.

## Tools and Troubleshooting

The project includes the following utility scripts:

- `install.cmd` — Install, change appearance, repair, or uninstall Tskbar.
- `build.cmd` — Manually compile the controller and taskbar customization DLL.
- `status.cmd` — Display the saved setting, controller process, startup entry, applied state, and recent log messages.
- `uninstall.cmd` — Remove the application and restart Explorer to unload the customization DLL and restore the normal taskbar.

### If Installation Fails

1. Run `install.cmd` again.
2. Select **Check status** to inspect the current installation.
3. If necessary, select **Repair / reinstall**.
4. Read any error messages displayed in the Command Prompt.
5. If a build fails, verify that the required Visual Studio C++ x64 Build Tools and Windows SDK components are installed.

If the installer cannot confirm that a taskbar change was applied, check the application logs.

**Technical note:** The current source may still use the original internal `ClearBar` folder name and build environment variable. Changing the README branding alone does not rename those internal paths or the compiled binaries.

## Windows Compatibility and Limitations

Tskbar modifies internal Windows taskbar XAML components. These are implementation details rather than a stable public taskbar customization interface, so compatibility may vary across Windows updates.

The acrylic effect is rendered by Windows. Its appearance can vary depending on system settings, transparency effects, power-saving conditions, and compositor behavior. A consistent translucent appearance cannot be guaranteed on every system.

Tskbar is an experimental project and should be tested before being used on a primary system.

## Version

**Current Version: V1**

Tskbar V1 is the initial release. Future versions may include bug fixes, compatibility improvements, and additional customization features.

## References

- [Microsoft AcrylicBrush API](https://learn.microsoft.com/en-us/uwp/api/windows.ui.xaml.media.acrylicbrush)
- [Microsoft XAML Diagnostics API](https://learn.microsoft.com/en-us/windows/win32/api/xamlom/)
