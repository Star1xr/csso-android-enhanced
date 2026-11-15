# Game instructor (port from Kisak-Strike)

Code: `game/client/c_gameinstructor.*`, `c_baselesson.*`, `hud_locator_target.*`, `c_keyvalue_saver.*`,
`cstrike/c_cs_lesson.cpp`, `gameinstructor_compat.h` (CSSO has no splitscreen/console storage).

Game files (copy into the game folder, same relative paths):

- `scripts/instructor_lessons.txt`, `scripts/mod_lessons.txt` (CS:GO lessons without Arsenal/Survival), `scripts/instructor_textures.txt`, `scripts/instructor_texturemanifest.txt`
- `materials/vgui/hud/gameinstructor_iconsheet1/2.vtf + .vmt`
- `resource/UI/Locator.res` (minimal; the CS:GO one works too)
- `resource/modevents.res` is the CSSO `modevents.res` with the instructor events already merged in (replace the game's file with it; `modevents_instructor_additions.res` is just the added part).
- English caption tokens are registered from code (`GameInstructor_RegisterEnglishTokens` in `c_gameinstructor.cpp`), no language file edit needed. A token that a language file already defines wins.
- Needs the scheme fonts `InstructorTitle`, `InstructorButtons`, `InstructorKeyBindings`, `InstructorKeyBindingsSmall` (and `_ss` variants) in `ClientScheme.res`, and the arrow materials `vgui/hud/icon_arrow_left/right/up/down/plain` (all from CS:GO; the arrow `.vmt`/`.vtf` files are included under `materials/vgui/hud/`).

Lesson counts are saved in `save/game_instructor_counts.txt`. Console: `gameinstructor_enable`, `gameinstructor_reset_counts`, `gameinstructor_verbose 1`.
