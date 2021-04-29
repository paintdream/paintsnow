local args = { ... }
local sourceSolution = args[1] or "../../Build/PaintsNow.sln"
local targetSolution = args[2] or "./Purpleave.sln"

local blackList = {
	["ALL_BUILD"] = true,
	["INSTALL"] = true,
	["ZERO_CHECK"] = true,
	["PACKAGE"] = true,
	["LeavesWing"] = true,
	["LostDream"] = true,
	["glfw"] = true
}

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

local function GetFolder(path, ext)
	local folder = path:match("(.-)/[^/]+%." .. ext)
	if folder then
		return folder
	else
		return path:match("(.-)\\[^\\]+%." .. ext)
	end
end

local function ParseSolution(path)
	local file = io.open(path, "rb")
	if not file then
		error("Enable to open file: " .. path)
	end

	local content = file:read("*all")
	file:close()

	local folder = GetFolder(path, "sln")

	local projects = {}
	for guid, name, path, config in content:gmatch("Project%(\"{(.-)}\"%) = \"(.-)\", \"(.-)\", \"{(.-)}\"") do
		if not blackList[name] then
			print("Guid: " .. guid .. " | Name: " .. name .. " | Path: " .. path)

			-- try to parse project
			local cpps = {}
			local hpps = {}
			local includeFolder = ""
			if guid ~= "2150E333-8FDC-42A3-9474-1A3956D46DE8" then
				local proj = io.open(folder .. "/" .. path, "rb")
				if not proj then
					error("Unable to open " .. folder .. "/" .. path)
				end
				local xml = proj:read("*all")
				proj:close()

				for cpp in xml:gmatch("<ClCompile Include=\"(.-)\"") do
					table.insert(cpps, cpp)
				end

				for hpp in xml:gmatch("<ClInclude Include=\"(.-)\"") do
					table.insert(hpps, hpp)
				end
				
				includeFolder = xml:match("<AdditionalIncludeDirectories>(.-)</AdditionalIncludeDirectories>")
				assert(includeFolder)
			end

			table.insert(projects, {
				Guid = guid,
				Name = name,
				Path = path,
				ConfigurationGuid = config,
				Includes = hpps,
				Sources = cpps,
				IncludeFolder = includeFolder
			})
		end
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

local function WriteFile(path, content)
	local folder = GetFolder(path, "vcxproj")
	if folder then
		print("MKDIR " .. folder)
		os.execute("mkdir -p " .. folder)
	end

	local file = io.open(path, "wb")
	if not file then
		error("Unable to write file: " .. path)
		return
	end

	file:write(content)
	file:close()
end

local function GenerateProject(project)
	local vcxprojTemplate =
[[<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|ARM">
      <Configuration>Debug</Configuration>
      <Platform>ARM</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|ARM">
      <Configuration>Release</Configuration>
      <Platform>ARM</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|ARM64">
      <Configuration>Debug</Configuration>
      <Platform>ARM64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|ARM64">
      <Configuration>Release</Configuration>
      <Platform>ARM64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|x86">
      <Configuration>Debug</Configuration>
      <Platform>x86</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x86">
      <Configuration>Release</Configuration>
      <Platform>x86</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectGuid>{%s}</ProjectGuid>
    <Keyword>Android</Keyword>
    <RootNamespace>%s</RootNamespace>
    <MinimumVisualStudioVersion>14.0</MinimumVisualStudioVersion>
    <ApplicationType>Android</ApplicationType>
    <ApplicationTypeRevision>3.0</ApplicationTypeRevision>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x86'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x86'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM64'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM'" Label="Configuration">
    <ConfigurationType>StaticLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>Clang_5_0</PlatformToolset>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings" />
  <ImportGroup Label="Shared" />
  <ImportGroup Label="PropertySheets" />
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x86'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x86'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM64'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM64'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM'">
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM'">
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>Use</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x86'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>Use</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x86'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM64'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>Use</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM64'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM'">
    <ClCompile>
	  <AdditionalIncludeDirectories>%s</AdditionalIncludeDirectories>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <PrecompiledHeaderFile>pch.h</PrecompiledHeaderFile>
      <RuntimeTypeInfo>true</RuntimeTypeInfo>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemGroup>
%s
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets" />
</Project>
]]

	local xmlNode = {}
	print("Generating Project: " .. project.Name)
	for _, cpp in ipairs(project.Sources) do
		table.insert(xmlNode, "<ClCompile Include=\"" .. cpp .. "\" />")
	end

	for _, hpp in ipairs(project.Includes) do
		table.insert(xmlNode, "<ClInclude Include=\"" .. hpp .. "\" />")
	end

	local include = project.IncludeFolder
	return vcxprojTemplate:format(project.ConfigurationGuid, project.Name, 
		include, include, include, include, include, include, include, include,	
		table.concat(xmlNode, "\n"))
end

-- Main
local projects = ParseSolution(sourceSolution)
-- projects
for i = 1, #projects do
	local project = projects[i]
	if project.Guid ~= "2150E333-8FDC-42A3-9474-1A3956D46DE8" then
		local content = GenerateProject(project)	
		WriteFile(project.Path, content)
	end
end

InjectAndroidProjects(projects)
local content = GenerateSolution(projects)
WriteFile(targetSolution, content)
