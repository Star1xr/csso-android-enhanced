# RocketUI (RmlUi) port - status & plan

Sources:
- Code: https://github.com/SwagSoftware/Kisak-Strike (`rocketui/`, `game/client/cstrike15/RocketUI/`, `thirdparty/RmlUi`)
- Assets: https://github.com/SwagSoftware/Kisak-Strike-Files (`csgo/rocketui/*.rml|*.rcss`)

## UI backend switch

| Macro | Meaning | waf |
|---|---|---|
| `USE_CSSOUI` | original CS:SO vgui UI (default) | `./waf configure ...` |
| `USE_CSSOROCKETUI` | RmlUi based UI | `./waf configure ... --rocketui` |

Exactly one is always defined. `public/csso_ui_config.h` enforces this at compile time.
Kisak's own `INCLUDE_ROCKETUI` is replaced by `USE_CSSOROCKETUI` while porting.

## Differences between Kisak-Strike and this repo (why this is not a copy-paste)

- Kisak targets `game/client/cstrike15` (CS:GO client); this repo uses `game/client/cstrike` + `client_csso.vpc`
  with the old vgui `game/client/gameui`. Hook points (BasePanel, loading screen, pause menu, HUD) are different.
- Kisak builds with CMake; this repo uses waf (`wscript` per module). RmlUi + `rocketui` need waf wscripts.
- Kisak's `rocketrenderGL.cpp` uses fixed-function desktop OpenGL (`glOrtho`, `glMatrixMode`, client arrays).
  Android builds use ToGLES (GLES3), so the renderer must be rewritten (shader based, VAO/VBO).
- Kisak vendors RmlUi 4.0 WIP (pre-4.0 API, no bundled GL3 backend). Android needs FreeType (already checked in wscript).
- RML files in Kisak-Strike-Files use CS:GO element/panel names; they must be renamed to match CS:SO.

## Steps

- [x] 1.0 `USE_CSSOUI` / `USE_CSSOROCKETUI` switch (`--rocketui`), no behaviour change by default
- [x] 1.1 RmlUi 5.1 (Core only) in `rmlui/` + waf static lib. Built OK in Actions (32-bit).
  RmlUi 6.x needs C++17, Android build is C++11 + NDK r10e libstdc++ 4.9, so 5.1 (C++14) is used. `thirdparty/` is a submodule, so it is not used.
- [~] 1.2 `rocketui/` module -> `librocketui_client.so` (waf target `rocketui_client`, only with `--rocketui`). Written, host syntax-checked; waiting for Actions.
  - `RocketFileSystem`: reads `rocketui/<path>` from the `GAME` search path (= `csso/rocketui/`), no `csgo/` anywhere
  - `RocketSystem`: log -> Warning/DevMsg, clipboard via vgui ISystem
  - `RocketRender`: GLES3 shader renderer (own VAO/VBO, saves + restores all GL state it touches, TGA textures). No call into ToGLES internals.
  - `RocketUIImpl` (`IRocketUI`, `public/rocketui/rocketui.h`): menu + hud contexts, fonts, input (buttons, mouse X/Y, touch fingers -> pointer), `rocket_enable` / `rocket_reload`
  - Dropped vs Kisak: device-dependent-object callbacks (this engine has none; screen size is polled via `IVEngineClient::GetScreenSize`), SDL cursors, RmlUi debugger (Core only)
  - Custom elements (`KisakConvarSetting*`, `KisakURL`): done in 1.4a
- [~] 1.3 Hooks under `USE_CSSOROCKETUI` (written, waiting for Actions):
  - engine owns the module: `CEngineVGui::Init` loads `librocketui_client.so` (`Sys_LoadModule`), `Connect(g_AppSystemFactory)` + `Init()`; missing/failed module -> old vgui menu stays (just a warning). `Shutdown` tears it down.
  - engine `Simulate` -> `RunFrame`; `Paint(PAINT_UIPANELS)` -> flush matsys, `RenderMenuFrame()` while GameUI is visible
  - engine `CGame::DispatchInputEvent` -> `HandleInputEvent` first, consumed events never reach binds/vgui
  - client `VGui_PostRender` -> `RenderHUDFrame()`; client only borrows the interface (`g_pRocketUI`), never Connect/Init
  - Open: text input (`HandleTextInput` is not fed yet, needed for text fields), ESC -> pause menu registration (1.4), real-device check of ToGLES state interplay
- [~] 1.4 UI documents. Split in two, the engine owns the module and loads it before the client DLL exists, so menu documents live in the module and HUD documents in the client:
  - [~] 1.4a module-owned documents (written, host syntax-checked, waiting for Actions): `rocketui/data/` = CS:SO `.rml/.rcss` (own, not Kisak's invader theme), packed into `csso/rocketui/` by the `game-files` workflow together with Kisak's fonts + images.
    - `customelements.cpp`: `KisakConvarSettingEnum`, `KisakConvarSettingSlider`, `KisakURL` on the RmlUi 5.1 API (unknown convar -> "(unavailable)", no crash)
    - `rkmenu.cpp`: `menu.rml` (map select + Start, Options, Console, Quit), visible only while `!IsInGame()` and owns input then; `panel_options.rml` (video/game/audio convars, Save = `mat_savechanges` + `host_writeconfig`), `panel_popup.rml`
    - `IRocketUI::ShowOptionsPanel` / `ShowPopup` so the client pause menu can open them; console: `rocket_options`, `rocket_test_popup`
    - Open: text fields (`HandleTextInput` still not fed), touch scrolling in the options panel, real-device look of select/slider styling
  - [~] 1.4b HUD. RmlUi headers cannot be included in the client DLL (NDK r10e / gnustl 4.9 has no C++14 library: `std::make_unique`, `std::exchange`, `underlying_type_t`; the first 1.4b try failed exactly on this). So the split is: documents + RmlUi code in the module (`rocketui/rkhud.cpp`, `rocketui/data/hud_*.rml`), the client only reads CS:SO data and passes plain structs (`RocketHud*_t` in `public/rocketui/rocketui.h`) through `IRocketUI::Hud*` (`game/client/cstrike/RocketUI/rkhud_bridge.cpp`). Old vgui panels hide themselves with `ROCKET_HUD_HIDE_VGUI()` while RocketUI is active.
    - [~] A: infobar (hp/armor/ammo/money), round timer + team scores (also replaces CHudTeamCounter), killfeed (waiting for Actions). Console: `rocket_test_kill`
    - [~] B: chat (module `hud_chat.rml`, `ChatPrintf` + `StartMessageMode` hooks, typed characters `IE_FirstVguiEvent+3` now reach `HandleTextInput`; console `rocket_test_chat`; waiting for Actions)
    - [ ] C: scoreboard  - [ ] D: radar  - [ ] E: buy menu + team menu  - [ ] F: loading screen + pause menu (ESC; engine `ActivateGameUI` must not show the vgui menu in game)

- [ ] 1.5 Console + net_graph: port the CS:GO look from Kisak-Strike to CS:SO (restyle the vgui console, CS:GO style `net_graph`); needs a look at Kisak's console/netgraph sources first

Later: Game Instructor + screen guides, Fake CSM, Valve intro video.
