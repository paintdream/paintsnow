-- Build script
-- PaintDream (paintdream@paintdream.com)
--

local commandLine = { ... }

local function Main()
	local QuickCompile = require("Engine/Boot/QuickCompile")
	local compiler = QuickCompile.New()
	print("CommandLine: " .. table.concat(commandLine, " "))
	local forceRecompile = commandLine[1] == "true"
	compiler:CompileRecursive("Engine/", forceRecompile)
	compiler:CompileRecursive("Script/", forceRecompile)

	-- Then pack resources
	local Packer = require("Engine/Model/Packer.lua")
	Packer:Import("Assets/", "/Packed/")
	print("Build finished.")
end

local status, msg = xpcall(Main, function (msg) 
	print("Error: " .. tostring(msg) .. "\nCallstack: \n" .. debug.traceback())
end,...)