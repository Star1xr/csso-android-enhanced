"resource/hud/teamcounter_avatars_new.res"
{
	// "Avatar new" team counter style (hud_teamcounter_style 2), CS:GO competitive look:
	//   [ 5 CT avatars ] [ timer / CT score + T score ] [ 5 T avatars ]
	// The avatars (one row, colour bar under each) are placed by the code, see TC_NEW_* in
	// cs_hud_teamcounter.cpp. This file only places the centre block.
	// Everything is 1080p units. The centre block is 64 wide and as tall as an avatar + its
	// colour bar (36 + 4 = 40): timer on top (21), the two score boxes below (18).
	// Centre block starts at x = 5*36 + 4*4 + 4 = 200. If you change the avatar size, gap
	// or centre width in the code, change the xpos values here too.

	"RoundTimerLabel"
	{
		"fieldName"			"RoundTimerLabel"
		"xpos"				"200"
		"ypos"				"0"
		"wide"				"64"
		"tall"				"21"
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
		"xpos"				"221"
		"ypos"				"0"
		"wide"				"22"
		"tall"				"0" // computed procedurally
		"base_resolution_wide" "1920"
		"base_resolution_tall" "1080"

		"image"				"materials/vgui/hud/svg/bomb_c4.svg"
	}

	"CTWinCounterLabel"
	{
		"fieldName"			"CTWinCounterLabel"
		"xpos"				"200"
		"ypos"				"22"
		"wide"				"31"
		"tall"				"18"
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
		"xpos"				"233"
		"ypos"				"22"
		"wide"				"31"
		"tall"				"18"
		"base_resolution_wide" "1920"
		"base_resolution_tall" "1080"
		"textAlignment"		"center"
		"font"				"RoundTimerFont"
		"fgcolor_override"	"TeamT"

		"bgcolor_override"	"0 0 0 153"
		"PaintBackgroundType"	"0"
	}
}
