# Launcher / engine file logging

The DXVK APK is built from `ndke01/csso-android-launcher`. That repository is
patched by `.github/workflows/apk-dxvk.yml` with the files and patch in this
directory.

Logs are written to the first writable directory of:

1. `<gamepath>/csso_logs/` (for example `/sdcard/srceng/csso_logs/`)
2. `/sdcard/Android/data/me.dmk95.csso/files/logs/`
3. app-internal `files/logs/`

Files:

- `launcher.log` - Java launcher and SDLActivity lifecycle, library loading,
  intent/argv/gamepath, asset extraction, uncaught Java exceptions
- `engine.log` - engine `Msg()`/`Warning()`/`Error()` output from the first
  native call, including startup stage logging
- `stdout.log` - raw stdout/stderr (SDL, Vulkan loader, DXVK, native prints)
- `dxvk_*.log` - DXVK's own logs (`DXVK_LOG_LEVEL=debug`)
- `crash.log` - native signal / `std::terminate` reports with backtraces

The launcher clone is pinned with `LAUNCHER_COMMIT` in the workflow. When
updating the launcher, bump that commit and regenerate
`launcher-logging.patch` against the new tree.
