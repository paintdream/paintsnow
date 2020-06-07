
-- Compile.lua --
-- A simple tl compiler executable
--

local QuickCompile = {}
EnableTL = true
local BootModule, errMsg = load(System.SnowyStream.FetchFileData("Engine/Boot/Bootstrap.lua"))
if errMsg then
	print("Util syntax error: " .. errMsg)
end

BootModule()
print("QuickCompile started.")
local tl = require("tl")


local function report_errors(category, errors)
   if not errors then
      return false
   end
   if #errors > 0 then
      local n = #errors
      print("========================================")
      print(n .. " " .. category .. (n ~= 1 and "s" or "") .. ":")
      for _, err in ipairs(errors) do
         print(err.filename .. ":" .. err.y .. ":" .. err.x .. ": " .. (err.msg or ""))
      end
      return true
   end
   return false
end

local exit = 0

local function report_all_errors(result)
   local has_errors = report_errors("Syntax error", result.syntax_errors)
   report_errors("unknown variable", result.unknowns)
   return not has_errors
end

function QuickCompile:CompileOne(filename, outputfile)
	local filenamelength = string.len(filename)
	-- extract file title
	local title = string.sub(filename, 1, filenamelength - 3)
	print("Compiling: " .. filename .. " ...")
	-- read file content
	local result, err = tl.process(filename)
	if result then
		local ok = report_all_errors(result)
		if not ok then
			return
		end

		assert(result.ast)
		local code = tl.pretty_print_ast(result.ast)
		local t = SnowyStream.NewFile(outputfile, true)
		if t then
			-- removes all continuous returns ...
			local length = string.len(code)
			local shrinkLength = length
			repeat
				length = shrinkLength
				code = string.gsub(code, "\n\n\n", "\n")
				shrinkLength = string.len(code)
			until length == shrinkLength

			-- make header
			local header = "-- " .. outputfile .. "\n"
			header = header .. "-- PaintDream (paintdream@paintdream.com)\n"
			header = header .. "-- Generated By TL Compiler (TLC)\n"
			header = header .. "-- Please do not modify it manually\n"
			header = header .. "-- \n\n"

			local text = header .. string.gsub(code, "  ", "\t")
			SnowyStream.WriteFile(t, text)
			SnowyStream.FlushFile(t)
			SnowyStream.CloseFile(t)
			-- print("Generated script " .. title .. ".lua")
		else
			print("Error on writing file " .. title .. ".lua")
		end
	else
		print("Compile error: \n" .. err)
	end
end

function QuickCompile:BuildUpdatedOne(filename, tree, forceRecompile)
	-- already checked?
	local state = tree[filename]
	if state then -- need compile
		return true
	elseif state == false then
		return false
	end

	tree[filename] = false
	-- check if it was already up to date
	local title = filename:match("(.-)%.[tl|tlm]")
	local outputfile = title .. ".lua"
	if not forceRecompile then
		local source = SnowyStream.NewFile(filename, false)
		if source then
			local checker = SnowyStream.NewFile(outputfile, false)
			if checker then
				local deps = false
				local content = SnowyStream.ReadFile(source, SnowyStream.GetFileSize(source))

				for k in string.gmatch(content, "require.-[%\'%\"](.-)[%\'%\"]") do
					deps = deps or self:BuildUpdatedOne(k .. ".tl", tree) or self:BuildUpdatedOne(k .. ".tlm", tree)
				end

				if not deps then
					local srcLastModifiedTime = SnowyStream.GetFileLastModifiedTime(source)
					local checkerLastModifiedTime = SnowyStream.GetFileLastModifiedTime(checker)
					-- print("TIME " .. srcLastModifiedTime .. " VS " .. checkerLastModifiedTime)
					if checkerLastModifiedTime > srcLastModifiedTime then
						-- already compiled?
						tree[filename] = false
						SnowyStream.CloseFile(source)
						SnowyStream.CloseFile(checker)
						return false
					end
				end
				SnowyStream.CloseFile(checker)
			end
			SnowyStream.CloseFile(source)
		else
			tree[filename] = false
			return false
		end
	end

	tree[filename] = outputfile
	return true
end

function QuickCompile:BuildUpdatedRecursive(path, tree, forceRecompile)
	-- Opens file and search for require statements
	assert(type(path) == "string", debug.traceback())
	if string.endswith(path, "/") then
		for index, file in ipairs(SnowyStream.QueryFiles(path)) do
			self:BuildUpdatedRecursive(path .. file, tree, forceRecompile)
		end
	elseif string.endswith(path, ".tl") or string.endswith(path, ".tlm") then
		self:BuildUpdatedOne(path, tree, forceRecompile)
	end
end

function QuickCompile:CompileRecursive(path, forceRecompile)
	local filelist = {}
	self:BuildUpdatedRecursive(path, filelist, forceRecompile)
	for k, v in SortedPairs(filelist) do
		if v then
			self:CompileOne(k, v)
		end
	end
end

function QuickCompile.New()
	local compiler = {}
	return setmetatable(compiler, { __index = QuickCompile })
end

local function Main(inputFileName)
	if inputFileName then
		local compiler = QuickCompile.New()
		compiler:CompileRecursive(inputFileName, true)
	end
end

Main(...)
return QuickCompile
