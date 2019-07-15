local t = {
	__index = function (t, w)
		return w
	end
}

setmetatable(t, t)

return t