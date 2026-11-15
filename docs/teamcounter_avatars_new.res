"resource/hud/teamcounter_avatars_new.res"
{
	// "Avatar new" team counter style (hud_teamcounter_style 2), CS:GO competitive look:
	//   [ 5 CT avatars ] [ timer / CT score + T score ] [ 5 T avatars ]
	// The avatars (one row, team-coloured bar under each) are placed by the code, see TC_NEW_* in
	// cs_hud_teamcounter.cpp. This file only places the centre block.
	// NOTE: the positions / sizes of the timer, score boxes and bomb icon below are re-applied by
	// LayoutNew() in the code (pixel-exact and symmetric at any resolution); this file mainly
	// provides fonts and colours. The values are the 1080p result, for reference.
	// Everything is 1080p units. The centre block has the same size as in the other styles: 84 wide,
	// as tall as an avatar + its colour bar (67 + 4 = 71): timer 84x37 on top, gap 3, the two score
	// boxes 41x31 below.
	// Centre block starts at x = 5*67 + 4*4 + 4 = 355. If you change the avatar size, gap
	// or centre width in the code, change the xpos values here too.

	"RoundTimerLabel"
	{
		"fieldName"			"RoundTimerLabel"
		"xpos"				"355"
		"ypos"				"0"
		"wide"				"84"
		"tall"				"37"
		"base_resolution_wide" "1920"
		"base_resolution_tall" "1080"
		"textAlignment"		"center"
		"font"				"RoundTimerFont"
		"fgcolor_override"	"White"

		"bgcolor_override"	"0 0 0 153"
		"PaintBackgroundType"	"0"
	}

	"BombIcon"
	{
		"fieldName"			"BombIcon"
		"xpos"				"376"
		"ypos"				"0"
		"wide"				"42"
		"tall"				"0" // computed procedurally
		"base_resolution_wide" "1920"
		"base_resolution_tall" "1080"

		"image"				"materials/vgui/hud/svg/bomb_c4.svg"
	}

	"CTWinCounterLabel"
	{
		"fieldName"			"CTWinCounterLabel"
		"xpos"				"355"
		"ypos"				"40"
		"wide"				"41"
		"tall"				"31"
		"base_resolution_wide" "1920"
		"base_resolution_tall" "1080"
		"textAlignment"		"center"
		"font"				"RoundTimerFont"
		"fgcolor_override"	"TeamCT"

		"bgcolor_override"	"0 0 0 153"
		"PaintBackgroundType"	"0"
	}

	"TWinCounterLabel"
	{
		"fieldName"			"TWinCounterLabel"
		"xpos"				"398"
		"ypos"				"40"
		"wide"				"41"
		"tall"				"31"
		"base_resolution_wide" "1920"
		"base_resolution_tall" "1080"
		"textAlignment"		"center"
		"font"				"RoundTimerFont"
		"fgcolor_override"	"TeamT"

		"bgcolor_override"	"0 0 0 153"
		"PaintBackgroundType"	"0"
	}
}
