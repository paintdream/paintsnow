local Range = record
	x : number
	y : number
	w : number
	h : number
end

local Size = record
	w : number
	h : number
end

local Slice = record
	frame : Range
	spriteSourceSize : Range
	rotated : boolean
	trimmed : boolean
	sourceSize : Size
end

return Slice