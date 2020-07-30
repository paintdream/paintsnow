local Style = require("Engine/View/Widget/Style")
local Slice = require("Engine/View/Widget/Slice")

local Theme = record
	image : string
	width : number
	height : number
	styles : { string : Style }
	slices : { string : Slice }
end

return Theme