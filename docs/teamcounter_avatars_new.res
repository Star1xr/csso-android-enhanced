"resource/hud/teamcounter_avatars_new.res"
{
	// "Avatar new" team counter style (hud_teamcounter_style 2).
	// The avatars (5 CT left, 5 T right, one row, colour bar under each) are placed by the code,
	// see TC_NEW_* in cs_hud_teamcounter.cpp. This file only places the centre block.
	// Centre block starts at x = 5*36 + 4*4 + 6 = 202 (1080p units). If you change the avatar size,
	// gap or centre width in the code, change the xpos values here too.

	"RoundTimerLabel"
	{
		"fieldName"			"RoundTimerLabel"
		"xpos"				"202"
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
		"xpos"				"226"
		"ypos"				"-2"
		"wide"				"42"
		"tall"				"0" // computed procedurally
		"base_resolution_wide" "1920"
		"base_resolution_tall" "1080"

		"image"				"materials/vgui/hud/svg/bomb_c4.svg"
	}

	"CTWinCounterLabel"
	{
		"fieldName"			"CTWinCounterLabel"
		"xpos"				"202"
		"ypos"				"40"
		"wide"				"40"
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
		"xpos"				"244"
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
