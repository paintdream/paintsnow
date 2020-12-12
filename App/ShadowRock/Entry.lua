-- Entry.lua
-- Util bootstrap, do not use require here

local SnowyStream = System.SnowyStream
function require(name)
	local mod = package.loaded[name]
	if mod then
		return mod
	end

	local path = name .. ".lua"
	local content = SnowyStream.FetchFileData(path)
	if content then
		local chunk, errMsg = load(content, name, "t", _ENV)
		if chunk then
			mod = chunk(name)
		else
			print("Load module " .. name .. " failed!")
			print("Error: " .. errMsg)
		end
	else
		print("Module not exist or empty: " .. name)
	end

	package.loaded[name] = mod
	return mod
end

-- TODO: Remove this line in release build
EnableTL = false

print("Loading Entry ...")
local Bootstrap = require("Engine/Boot/Bootstrap")
-- Assure latest script
-- local Builder = require("Build")
-- Enable Debug Console

MakeConsole()
print("Starting ...")
-- local App = require("Script/Test/RemoteComponent")
local App = require("Script/Main")
print("Starting Main ...")
local co = coroutine.create(App.Main)
coroutine.resume(co)

