local globalFont = "Packed/Font/Cantarell-Regular.rds"

return {
	Image = "Packed/Texture/DarkUI.rds",
	Size = { 512, 512 },
	Presettings =
	{
		Title =
		{
			Font = { globalFont, 24 },
			TextAlignCenter = true
		},
		Plain =
		{
			Font = { globalFont, 18 },
			TextAlignCenter = false
		},
		Border =
		{
			LineWidth = 2,
			Margin = 3,
		},
		Button =
		{
			Font = { globalFont, 18 },
			TextAlignCenter = true
		},
		Tag = { Font = { globalFont, 18 } },
		Frame = { },
		Progress = { },
		Slider = { },
		Switch = { },
		Loading = { },
		TextBox = { },
		SliderVertical = { }
	},
	Patches =
	{
		add_hover =
		{
			frame = { x = 452, y = 145, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		add_normal =
		{
			frame = { x = 452, y = 177, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		add_pressed =
		{
			frame = { x = 452, y = 209, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_left_hover =
		{
			frame = { x = 452, y = 241, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_left_normal =
		{
			frame = { x = 452, y = 273, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_left_pressed =
		{
			frame = { x = 452, y = 305, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_right_hover =
		{
			frame = { x = 440, y = 337, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_right_normal =
		{
			frame = { x = 472, y = 337, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		arrow_right_pressed =
		{
			frame = { x = 440, y = 369, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		button_hover =
		{
			frame = { x = 168, y = 479, w = 105, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 105, h = 30 },
			sourceSize = { w = 105, h = 30 }
		},
		button_normal =
		{
			frame = { x = 218, y = 76, w = 105, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 105, h = 30 },
			sourceSize = { w = 105, h = 30 }
		},
		button_pressed =
		{
			frame = { x = 325, y = 86, w = 105, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 105, h = 30 },
			sourceSize = { w = 105, h = 30 }
		},
		checkbox_checked =
		{
			frame = { x = 484, y = 154, w = 24, h = 24 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 24, h = 24 },
			sourceSize = { w = 24, h = 24 }
		},
		checkbox_unchecked =
		{
			frame = { x = 484, y = 180, w = 24, h = 24 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 24, h = 24 },
			sourceSize = { w = 24, h = 24 }
		},
		field =
		{
			frame = { x = 2, y = 134, w = 211, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		field_hover =
		{
			frame = { x = 215, y = 139, w = 211, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		field_round =
		{
			frame = { x = 2, y = 172, w = 211, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		field_round_hover =
		{
			frame = { x = 215, y = 177, w = 211, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		icon_confirm =
		{
			frame = { x = 452, y = 79, w = 36, h = 31 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 36, h = 31 },
			sourceSize = { w = 36, h = 31 }
		},
		icon_question =
		{
			frame = { x = 452, y = 41, w = 36, h = 36 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 36, h = 36 },
			sourceSize = { w = 36, h = 36 }
		},
		icon_warning =
		{
			frame = { x = 452, y = 2, w = 36, h = 37 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 36, h = 37 },
			sourceSize = { w = 36, h = 37 }
		},
		loading_fill =
		{
			frame = { x = 356, y = 2, w = 82, h = 82 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 82, h = 82 },
			sourceSize = { w = 82, h = 82 }
		},
		loading_frame =
		{
			frame = { x = 356, y = 335, w = 82, h = 82 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 82, h = 82 },
			sourceSize = { w = 82, h = 82 }
		},
		message_background =
		{
			frame = { x = 2, y = 2, w = 352, h = 72 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 352, h = 72 },
			sourceSize = { w = 352, h = 72 }
		},
		message_frame =
		{
			frame = { x = 2, y = 335, w = 352, h = 72 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 352, h = 72 },
			sourceSize = { w = 352, h = 72 }
		},
		minus_hover =
		{
			frame = { x = 472, y = 369, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		minus_normal =
		{
			frame = { x = 440, y = 401, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		minus_pressed =
		{
			frame = { x = 472, y = 401, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		mult_hover =
		{
			frame = { x = 435, y = 473, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		mult_normal =
		{
			frame = { x = 467, y = 473, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		mult_pressed =
		{
			frame = { x = 275, y = 479, w = 30, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 30 },
			sourceSize = { w = 30, h = 30 }
		},
		playcontrol =
		{
			frame = { x = 2, y = 76, w = 214, h = 56 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 214, h = 56 },
			sourceSize = { w = 214, h = 56 }
		},
		progress_control =
		{
			frame = { x = 490, y = 32, w = 20, h = 20 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 20, h = 20 },
			sourceSize = { w = 20, h = 20 }
		},
		progress_fill =
		{
			frame = { x = 2, y = 253, w = 267, h = 44 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 267, h = 44 },
			sourceSize = { w = 267, h = 44 }
		},
		progress_frame =
		{
			edge = { x = 15, y = 5 },
			frame = { x = 2, y = 433, w = 356, h = 44 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 356, h = 44 },
			sourceSize = { w = 356, h = 44 }
		},
		progress_hint =
		{
			frame = { x = 360, y = 419, w = 73, h = 40 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 73, h = 40 },
			sourceSize = { w = 73, h = 40 }
		},
		progress_slot =
		{
			frame = { x = 2, y = 419, w = 331, h = 10 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 331, h = 10 },
			sourceSize = { w = 331, h = 10 }
		},
		radio_checked =
		{
			frame = { x = 484, y = 206, w = 24, h = 24 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 24, h = 24 },
			sourceSize = { w = 24, h = 24 }
		},
		radio_unchecked =
		{
			frame = { x = 484, y = 232, w = 24, h = 24 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 24, h = 24 },
			sourceSize = { w = 24, h = 24 }
		},
		search =
		{
			frame = { x = 2, y = 210, w = 211, h = 36 },
			edge = { x = 40 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		search_hover =
		{
			frame = { x = 215, y = 215, w = 211, h = 36 },
			edge = { x = 60 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 211, h = 36 },
			sourceSize = { w = 211, h = 36 }
		},
		selector =
		{
			frame = { x = 490, y = 2, w = 20, h = 28 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 20, h = 28 },
			sourceSize = { w = 20, h = 28 }
		},
		slider_control =
		{
			frame = { x = 307, y = 479, w = 40, h = 22 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 40, h = 22 },
			sourceSize = { w = 40, h = 22 }
		},
		slider_control_vertical =
		{
			frame = { x = 486, y = 112, w = 22, h = 40 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 22, h = 40 },
			sourceSize = { w = 22, h = 40 }
		},
		slider_line =
		{
			frame = { x = 2, y = 419, w = 331, h = 10 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 331, h = 10 },
			sourceSize = { w = 331, h = 10 }
		},
		slider_line_vertical =
		{
			frame = { x = 440, y = 2, w = 10, h = 331 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 10, h = 331 },
			sourceSize = { w = 10, h = 331 }
		},
		star_empty =
		{
			frame = { x = 452, y = 112, w = 32, h = 31 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 32, h = 31 },
			sourceSize = { w = 32, h = 31 }
		},
		star_full =
		{
			frame = { x = 218, y = 108, w = 30, h = 29 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 30, h = 29 },
			sourceSize = { w = 30, h = 29 }
		},
		switch_off =
		{
			frame = { x = 435, y = 433, w = 73, h = 38 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 73, h = 38 },
			sourceSize = { w = 73, h = 38 }
		},
		switch_on =
		{
			frame = { x = 360, y = 461, w = 73, h = 38 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 73, h = 38 },
			sourceSize = { w = 73, h = 38 }
		},
		tag =
		{
			frame = { x = 2, y = 479, w = 164, h = 30 },
			rotated = false,
			trimmed = false,
			spriteSourceSize = { x = 0, y = 0, w = 164, h = 30 },
			sourceSize = { w = 164, h = 30 }
		}
	}
}