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
- [~] 1.1 RmlUi 5.1 (Core only) in `rmlui/` + waf static lib. Waiting for Actions result (`--rocketui` is passed on the `rocketui-port` branch via `WAF_EXTRA`).
  RmlUi 6.x needs C++17, Android build is C++11 + NDK r10e libstdc++ 4.9, so 5.1 (C++14) is used. `thirdparty/` is a submodule, so it is not used.
- [ ] 1.2 `rocketui` module: system interface, file interface (via IFileSystem), GLES3 renderer, input (touch + SDL)
- [ ] 1.3 Client hooks under `USE_CSSOROCKETUI`: load `RocketUI001` interface, frame/render calls, input routing, device-lost handling
- [ ] 1.4 Main menu document + options panel, loadout entry in main menu (backed by existing `cs_loadout` / `loadout_*` cvars)
- [ ] 1.5 Adapt Kisak-Strike-Files `.rml/.rcss` to CS:SO element names, ship under `csso/rocketui/`
- [ ] 1.6 HUD documents (chat, killfeed, scoreboard, radar, roundtimer, buymenu, teammenu, loading screen, pause menu)

Later: Game Instructor + screen guides, CS:GO style net_graph, Fake CSM, Valve intro video.
