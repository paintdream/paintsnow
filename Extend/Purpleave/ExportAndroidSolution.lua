local args = { ... }
local sourceSolution = args[1] or "../../BuildARM/PaintsNow.sln"
local targetSolution = args[2] or "./Purpleave.sln"

if not table.unpack then
	table.unpack = unpack
end

local function GenerateGuid()
	local numbers = {}
	for i = 1, 16 do
		table.insert(numbers, math.random(0, 255))
	end

	return string.format("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		table.unpack(numbers)
	)
end

local function ParseSolution(path)
	local file = io.open(path, "rb")
	if not file then
		print("Enable to open file: " .. path)
		return
	end

	local content = file:read("*all")
	file:close()

	local projects = {}
	for guid, name, path, config in content:gmatch("Project%(\"{(.-)}\"%) = \"(.-)\", \"(.-)\", \"{(.-)}\"") do
		print("Guid: " .. guid .. " | Name: " .. name .. " | Path: " .. path)

		table.insert(projects, {
			Guid = guid,
			Name = name,
			Path = path,
			ConfigurationGuid = config,
		})
	end

	local nested = content:find("GlobalSection%(NestedProjects%) = preSolution")
	if nested then
		for sub, parent in content:gmatch("{([^%s]+)} = {([^%s]+)}", nested) do
			print("Sub: " .. sub .. " | Parent: " .. parent)
			for i = 1, #projects do
				local project = projects[i]
				if project.ConfigurationGuid == sub then
					project.NestedGuid = parent
					break
				end
			end
		end
	end

	return projects
end

local function GenerateDeclaration(projects)
	local projectDeclare =
[[Project("{%s}") = "%s", "%s", "{%s}"
EndProject]]
	local target = {}
	for i = 1, #projects do
		local project = projects[i]
		table.insert(target, projectDeclare:format(
			project.Guid, project.Name, project.Path, project.ConfigurationGuid
		))
	end

	return table.concat(target, "\n")
end

local function GenerateConfiguration(projects)
	local projectConfiguration =
[[	{%s}.Debug|ARM.ActiveCfg = Debug|ARM
	{%s}.Debug|ARM.Build.0 = Debug|ARM
	{%s}.Debug|ARM64.ActiveCfg = Debug|ARM64
	{%s}.Debug|ARM64.Build.0 = Debug|ARM64
	{%s}.Debug|x64.ActiveCfg = Debug|x64
	{%s}.Debug|x64.Build.0 = Debug|x64
	{%s}.Debug|x86.ActiveCfg = Debug|x86
	{%s}.Debug|x86.Build.0 = Debug|x86
	{%s}.Release|ARM.ActiveCfg = Release|ARM
	{%s}.Release|ARM.Build.0 = Release|ARM
	{%s}.Release|ARM64.ActiveCfg = Release|ARM64
	{%s}.Release|ARM64.Build.0 = Release|ARM64
	{%s}.Release|x64.ActiveCfg = Release|x64
	{%s}.Release|x64.Build.0 = Release|x64
	{%s}.Release|x86.ActiveCfg = Release|x86
	{%s}.Release|x86.Build.0 = Release|x86]]

	local target = {}
	for i = 1, #projects do
		local project = projects[i]
		if project.Guid ~= "2150E333-8FDC-42A3-9474-1A3956D46DE8" then
			local guids = {}
			for j = 1, 16 do
				table.insert(guids, project.ConfigurationGuid)
			end

			table.insert(target, projectConfiguration:format(table.unpack(guids)))
		end
	end

	return table.concat(target, "\n")
end

local function GenerateNested(projects)
	local projectNested = 
[[	{%s} = {%s}]]
	local target = {}
	for i = 1, #projects do
		local project = projects[i]
		if project.NestedGuid then
			table.insert(target, projectNested:format(project.ConfigurationGuid, project.NestedGuid))
		end
	end

	return table.concat(target, "\n")
end

local function InjectAndroidProjects(projects)
	local hostGuid = "F896F62F-0D10-4111-9D67-519610420117"

	table.insert(projects, {
		Guid = "2150E333-8FDC-42A3-9474-1A3956D46DE8",
		ConfigurationGuid = hostGuid,
		Name = "Purpleave",
		Path = "Purpleave",
	})

	table.insert(projects, {
		Guid = "39E2626F-3545-4960-A6E8-258AD8476CE5",
		ConfigurationGuid = "5950AEA1-2639-4826-9C79-AFA61E9D6468",
		Name = "Purpleave.Packaging",
		Path = "Purpleave.Packaging\\Purpleave.Packaging.androidproj",
		NestedGuid = hostGuid
	})

	table.insert(projects, {
		Guid = "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942",
		ConfigurationGuid = "4E02E07E-1853-47BF-A692-89A7B1C98D5A",
		Name = "Purpleave.NativeActivity",
		Path = "Purpleave.NativeActivity\\Purpleave.NativeActivity.vcxproj",
		NestedGuid = hostGuid
	})
end

local function GenerateSolution(projects)
	local slnTemplate = [[
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 16
VisualStudioVersion = 16.0.31205.134
MinimumVisualStudioVersion = 10.0.40219.1
%s
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|ARM = Debug|ARM
		Debug|ARM64 = Debug|ARM64
		Debug|x64 = Debug|x64
		Debug|x86 = Debug|x86
		Release|ARM = Release|ARM
		Release|ARM64 = Release|ARM64
		Release|x64 = Release|x64
		Release|x86 = Release|x86
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
%s
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
	GlobalSection(NestedProjects) = preSolution
%s
	EndGlobalSection
	GlobalSection(ExtensibilityGlobals) = postSolution
		SolutionGuid = {%s}
	EndGlobalSection
EndGlobal
]]

	return slnTemplate:format(
		GenerateDeclaration(projects),
		GenerateConfiguration(projects),
		GenerateNested(projects),
		"25DC1629-CA39-4D5F-968D-A5BC32ADF9E6"
	)
end

local function WriteSolution(path, content)
	local file = io.open(path, "wb")
	if not file then
		print("Unable to write solution file!")
		return
	end

	file:write(content)
	file:close()
end

-- Main
local projects = ParseSolution(sourceSolution)
InjectAndroidProjects(projects)
local content = GenerateSolution(projects)
WriteSolution(targetSolution, content)