local Math = {}
local MetaVector : { any : any } = {}

function Math.Print(c)
	io.write("{")
	for k, v in ipairs(c) do
		io.write(v)
		if (k ~= #c) then
			io.write(", ")
		end
	end
	io.write("}\n")
end

function Math.Vector(first, ...)
	if (type(first) == "table") then
		setmetatable(first, MetaVector)
	else
		local object = table.pack(first, ...)
		setmetatable(object, MetaVector)
		first = object
	end

	return first
end

function Math.SquareLength(a)
	local sum = 0
	for k, v in ipairs(a) do
		sum = sum + v * v
	end

	return sum
end

function Math.Length(a)
	return math.sqrt(Math.SquareLength(a))
end

function Math.Normalize(a)
	return a / Math.Length(a)
end

function Math.Cross2D(a, b)
	return { a[1] * b[2] - a[2] * b[1] }
end

function MetaVector.__add(a, b)
	assert(#a == #b)
	local c : { integer : number } = Math.Vector(table.unpack(a))
	for k, v in ipairs(c) do
		c[k] = a[k] + b[k]
	end

	return c
end

function MetaVector.__sub(a, b)
	assert(#a == #b)
	local c : { integer : number } = Math.Vector(table.unpack(a))
	for k, v in ipairs(c) do
		c[k] = a[k] - b[k]
	end

	return c
end

function MetaVector.__mul(a, b)
	local c : { integer : number } = Math.Vector(table.unpack(a))
	if (type(b) == "table") then
		assert(#a == #b)
		for k, v in ipairs(a) do
			c[k] = v * b[k]
		end
	else
		for k, v in ipairs(a) do
			c[k] = v * b
		end
	end

	return c
end


function MetaVector.__div(a, b)
	local c : { integer : number } = Math.Vector(table.unpack(a))
	if (type(b) == "table") then
		assert(#a == #b)
		for k, v in ipairs(a) do
			c[k] = v / b[k]
		end
	else
		for k, v in ipairs(a) do
			c[k] = v / b
		end
	end

	return c
end

local mapKey : { any : integer } = {
	x = 1,
	y = 2,
	z = 3,
	w = 4,
	s = 1,
	t = 2,
	r = 1,
	g = 2,
	b = 3,
	a = 4,
}

function MetaVector.__index(object, key)
	return type(key) == "string" and rawget(object, mapKey[key]) or rawget(object, key)
end

function MetaVector.__newindex(object, key, value)
	return rawset(object, type(key) == "string" and mapKey[key] or key, value)
end


return Math