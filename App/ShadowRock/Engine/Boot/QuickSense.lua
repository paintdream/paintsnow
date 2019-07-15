-- Import necessary utilities first
local QuickSense = {}
EnableTypedLua = true
Util = load(SnowyStream.FetchFileData("Engine/Boot/Bootstrap.lua"))()

-- Prepare quick scene environment

function SyntaxCheck(code, chunkname, color)
	local ast, err = tlparser.parse(code, chunkname, false, true)
	if not ast then
		return err, ""
	end

	local messages = tlchecker.typecheck(ast, code, chunkname, true, true, color)
	local code = tlcode.generate(ast)
	-- Check if there were errors
	local has_errors = tlchecker.error_msgs(messages, false)

	-- omitted any & unused errors
	local filterMessages = {}
	for k, v in ipairs(messages) do
		if v.tag ~= "any" and v.tag ~= "unused" then
			table.insert(filterMessages, v)
		end
	end

	return filterMessages, has_errors and "" or code
end

local function Compile(path, content)
	local messages, code = SyntaxCheck(content, path, false)
	if type(messages) == "table" then
		print("Syntax checking " .. path .. " with " .. #messages .. " errors")
	elseif type(message) == "string" then
		print("Compiler error at" .. path .. ":")
		print(messages)
		messages = {}
	else
		print("Unexpected: " .. type(message))
	end

	return { Messages = messages, Code = code }
end

local function OnQuickSenseControl(command, ...)
	if command == "Compile" then
		return Compile(...)
	end
end

-- Register Quick Sense
SetQuickSenseController(OnQuickSenseControl)