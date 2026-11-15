// Add these to resource/modevents.res (inside the top level "modevents" block).
// Only the ones that don't exist in your modevents.res yet.

	"buymenu_open"			// fired by the client when the buy menu opens (game instructor)
	{
		"userid"	"short"
	}

	"buymenu_close"			// fired by the client when the buy menu closes (game instructor)
	{
		"userid"	"short"
	}

	"gameinstructor_draw"
	{
	}

	"gameinstructor_nodraw"
	{
	}

	"set_instructor_group_enabled"
	{
		"group"		"string"
		"enabled"	"short"
	}

	"instructor_server_hint_create"
	{
		"userid"				"short"
		"hint_name"				"string"
		"hint_replace_key"		"string"
		"hint_target"			"long"
		"hint_activator_userid"	"short"
		"hint_timeout"			"short"
		"hint_icon_onscreen"	"string"
		"hint_icon_offscreen"	"string"
		"hint_caption"			"string"
		"hint_activator_caption"	"string"
		"hint_color"			"string"
		"hint_icon_offset"		"float"
		"hint_range"			"float"
		"hint_flags"			"long"
		"hint_binding"			"string"
		"hint_gamepad_binding"	"string"
		"hint_allow_nodraw_target"	"bool"
		"hint_nooffscreen"		"bool"
		"hint_forcecaption"		"bool"
		"hint_local_player_only"	"bool"
	}

	"instructor_server_hint_stop"
	{
		"hint_name"		"string"
	}
