local Vector = {}
Vector.__add = function (a, b : any)
	local c : any = {}
	if type(b) ~= "number" then
		for i, v in ipairs(a) do
			c[i] = v + b[i]
		end
	else
		for i, v in ipairs(a) do
			c[i] = v + b
		end
	end
	
	return setmetatable(c, Vector)
end

Vector.__sub = function (a, b : any)
	local c : any = {}
	if type(b) ~= "number" then
		for i, v in ipairs(a) do
			c[i] = v - b[i]
		end
	else
		for i, v in ipairs(a) do
			c[i] = v - b
		end
	end

	return setmetatable(c, Vector)
end

Vector.__mul = function (a, b : any)
	local c : any = {}
	if type(b) ~= "number" then
		for i, v in ipairs(a) do
			c[i] = v * b[i]
		end
	else
		for i, v in ipairs(a) do
			c[i] = v * b
		end
	end

	return setmetatable(c, Vector)
end

Vector.__div = function (a, b : any)
	local c : any = {}
	if type(b) ~= "number" then
		for i, v in ipairs(a) do
			c[i] = v / b[i]
		end
	else
		for i, v in ipairs(a) do
			c[i] = v / b
		end
	end

	return setmetatable(c, Vector)
end

function Vector.New(vec, vec2, ...) : any
	if vec2 then
		return setmetatable({vec, vec2, ...}, Vector)
	else
		return setmetatable(vec, Vector)
	end
end

return Vector