local args = { ... }
local sourceSolution = args[1] or "../../Build/PaintsNow.sln"
local targetSolution = args[2] or "./Purpleave.sln"

local function ParseSolution(path)
	local file = io.open(path, "rb")
	if not file then
		print("Enable to open file: " .. path)
		return
	end

	local content = file:read("*all")
	file:close()

	for guid, name, path in content:gmatch("Project%(\"(.-)\"%) = \"(.-)\", \"(.-)\"") do
		print("Guid: " .. guid .. " | Name: " .. name .. " | Path: " .. path)
	end
end

local sourceSolution = ParseSolution(sourceSolution)
local targetSolution = ParseSolution(targetSolution)

