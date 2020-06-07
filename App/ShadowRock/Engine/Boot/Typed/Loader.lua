local exts = { "", ".lua" }
TypedDescriptions = {}

if EnableTL then
	print("Overriding io.open/close")
	exts = { "", ".tl", ".lua" }
	io = io or {}
	local orgOpen = io.open
	local orgClose = io.close
	local orgSearcher = package.searchpath

	local simfile = {}
	simfile.__index = simfile

	function simfile:read(s)
		local c = self.content
		self.content = nil
		return c
	end

	function simfile:close()
	end

	-- io operations only for tld usage
	io.open = function (f, m)
		-- print("Opening file " .. f)	
		local desc = TypedDescriptions[f]
		if type(desc) == "string" then
			local file = { content = desc }
			-- print("Extern Module " .. f)
			setmetatable(file, simfile)
			return file
		end

		-- override all tld requests
		-- print("Try Load resource: " .. f)
		local content = SnowyStream.FetchFileData(f)
		if content then
			local file = { content = content }
			-- print("Buildin Module " .. f)
			setmetatable(file, simfile)
			return file
		end

		return nil, "Enable to find " .. f
	end

	io.close = function (f)
		return type(f) ~= "table" and oldclose(f)
	end

	os = {}
	os.getenv = function() return end

	tl = nil
end

function package.searchpath(name, filter, ...)
	if TypedDescriptions[name] then
		if isTld then
			return name
		else
			return nil, "No tld file for Buildin module " .. name
		end
	end

	for i, v in ipairs(exts) do
		local c = name .. v
		if SnowyStream.FileExists(c) then
			return c
		end
	end

	return nil, "Failed to open file: " .. name	
end

function require(name, ...)
	-- print("Requiring " .. name)
	local mod = package.loaded[name]
	if mod then
		return mod
	end

	path, msg = package.searchpath(name, "")
	if not path then
		print("Module " .. name .. " not found.")
		return nil
	end

	local content = SnowyStream.FetchFileData(path)
	if content then
		local chunk, errMsg = load(content, name, "t", _ENV)
		if chunk then
			mod = chunk(name)
		else
			print("Load module " .. name .. " failed!")
			print("Compiler log: " .. errMsg)
		end
	end

	package.loaded[name] = mod
	return mod
end

if EnableTL then
	print("Initializing tl ...")
	tl = require("tl")

	-- register API prototypes
	local mapTypes = {
		["int"] = "integer",
		["short"] = "integer",
		["char"] = "integer",
		["float"] = "number",
		["double"] = "number",
		["__int64"] = "integer",
		["long"] = "integer",
		["string"] = "string",
		["String"] = "string",
		["bool"] = "boolean",
		["vector"] = "{integer:any}",
		["BaseDelegate"] = "any",
		["Ref"] = "any",
		["PlaceHolder"] = "any",
		["WarpTiny"] = "any"
	}

	function Bootstrap.GetTypeName(t, regTypes)
		if type(t) == "string" then
			local m = mapTypes[t]
			assert(m, "Unrecognized type: " .. t)
			return m
		end

		local m = mapTypes[t.Type]
		if m then return m end

		local r = regTypes[t.Type]
		if not r then
			local stmt = {}
			if not t.Pointer then
				table.insert(stmt, "global " .. t.Type .. " = record" .. "\n")
				regTypes[t.Type] = true
				local mid = {}
				for k, v in SortedPairs(t.Fields) do
					table.insert(mid, "\t" .. (type(k) == "string" and ("\"" .. k .. "\"") or k)  .. ": " .. Bootstrap.GetTypeName(v, regTypes))	
				end
				table.insert(stmt, table.concat(mid, "\n"))
				-- table.insert(stmt, "\t\"__rawpointer\" : integer")
				table.insert(stmt, "\nend\n")

				regTypes[t.Type] = table.concat(stmt)
			end
		end

		return (t.List and "{ integer : " or "") .. t.Type .. (t.List and " }" or "")
	end
end