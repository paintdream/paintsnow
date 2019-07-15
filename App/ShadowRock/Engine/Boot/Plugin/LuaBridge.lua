-- Optional lua libraries
local luaBridgeMod = RayForce.NewObject("Lua53.dll!NativeLua")
if luaBridgeMod then
	LuaBridge = RayForce.QueryInterface(luaBridgeMod)
end
