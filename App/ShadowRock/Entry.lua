-- Entry.lua
-- Util bootstrap, do not use require here

-- TODO: Remove this line in release build
EnableTL = false

print("Loading Entry ...")
local Bootstrap = require("Engine/Boot/Bootstrap")
-- Assure latest script
-- local Builder = require("Build")
-- Enable Debug Console

MakeConsole()
print("Starting ...")
local App = require("Script/Main")
print("Starting Main ...")
local co = coroutine.create(App.Main)
coroutine.resume(co)

