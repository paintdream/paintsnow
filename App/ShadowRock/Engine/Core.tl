--[====================================================================[
								<Core.lua>
					A pure Lua utility tool for Lua 5.3
							By PaintDream (paintdream@paintdream.com)
							Version: 2016.12.21
--]====================================================================]

--[====================================================================[
								<Document>
Usage:
	DEBUG = true -- or false if debug is disabled.
	local Core = require("Engine/Core")

<Table Operations>

	[function Core(t)]
		Create and return a core object for table t.
		You can use:
		1. s + Core(t) to MERGE s with t. Notice that this operation may
			modify the content of s.
		2. s % Core(t) to ASSERT that s provides all attributes of t and
			getmetatable(s) == nil or getmetatable(s) == getmetatable(t).
			This function returns s and takes no effects if Debug is not
			enabled.
		3. Core(t)[] is the same as t[].
		4. pairs(Core(t)) is the same as pairs(t)

		Typical usage:
		1. return { somedata ... } + Core({ AdditionalData })
		2. s = s % Core(TheAssertedTypeOfS).

	[function Core.Copy(t)]
		Make a deep copy of t.

	[function Core.Inspect(x, selector, iterator)]
		Returns an iterator to all nodes and subnodes of x.
			Parameter selector is to filter key-value pairs. You can
			either provide a string for key matching or a user-defined/
			function.
			Parameter iterator is the iteration method. The default value
			is pairs().
		Exsample:
			for key, value in Core.Inspect({ aaa = 1, bbb = { aaa = 2 },
				"aaa", 1234 }, "aaa") do
				print(value)
			end

<Queue Functions>
	[function Core.Heap(comparator)]
		Create a maxinum heap.
	[function Core.Queue()]
		Create a normal queue.
	[function Core.Push(q, v)]
		Push an element to queue
	[function Core.Pop(q, v)]
		Pop an element from queue
	Exsample:
		local q = Core.Heap()
		for i, v in ipairs({ 3, 89, 2, 4, 6, 7, 3, 100, -4 }) do
			Core.Push(q, v)
		end

		while #q ~= 0 do
			print("V " .. Core.Pop(q))
		end
	Example:
		local buffer = Queue()
		for i = 1, 1000 do
			Push(buffer, i)
		end

		local list = buffer.list
		local i = 1
		repeat
			print("I = " .. i .. " FROM " .. list.from .. " TO " .. list.length)
			i = i + 1
			list = list.next
		until list == buffer.list

		for i = 1, 1000 do
			local v = Pop(buffer)
			print("V = " .. v .. " i = " ..i)
			assert(i == v)
		end


<Schedule Functions>
	[function Core.Invoke(proc, ...)]
		Invokes a task with proc as entry function.
		A table of tasks is called port. You can create port by table:
		port = { task1, task2, task3, aaa = task4 }.
	
	[function Core.Post(task or port, ...)]
		Post messages to a task or task list.

	[function Core.Listen(task or port, callback, ...)]
		Captures all messages from specified task or tasks and returns an
		iterator for them if callback is nil. Otherwise, callback will be
		called with:
			(callback, message)
		The ... provides response parameters to sender.
		Exsample:
			1.	Core.Listen(port, function (callback, message, ...)
					if message ~= "EXIT" then
						return Core.Listen(port, callback)
					end
				end)("Hello")

			2.	for message in Core.Listen(port, nil, response) do
					print(message)
				end

	[function StepListen(task or port, ...)]
		Step Listen once. Equivalent to Listen(port, nil, ...)

	[function Request(proc(port, ...), ...)]
		Make a direct call to proc and receives all messages from port.
		Returns an iterator fro messages.
		
<State Management>
	[function WrapRoutine(key, v, packageName)]
		Wrap a routine in specified package.

	[function WrapPackage(package, name)]
		Wrap all global function in specified package.

	[function WrapState(package, routine)]
		Generate a state.

	[function LocalData(state)]
		Retrivies the local data table from current state.

	[function Run(state)]
		Run current state.

	Exsample:
		-- Module A --
		local ModuleA = {}
		function RoomA(state)
			local data = Core.LocalData(state)
			if data.Visited then return Error() end
			data.Visited. = true
			return RoomB(state)
		end
		function RoomB(state) ... return RoomA(state) end
		function Main(state) return RoomB(state) end
		...
		return WrapPackage(ModuleA, "ModuleA")

		-- Main --
		local state = Core.WrapState(ModuleA)
		Core.Run(state)


<Debug Functions>

	[function Core.DebugTrace()]
		Print current call stack. Additional features will be added in
		next version.

	[function Core.Hook(object, readHandler(object, key, value),
		writeHandler(object, key, oldvalue, newvalue))]
		Create a level of hook on object. Overrides all read or write
			operations.
	[function Core.UnHook(object)]
		Remove a level of hook on object.

	Exsample:
		local a = { test = { 'haha', b = 'str' }, c = 23 }
		Hook(a, function (object, key, value)
			print("TRYING TO READ " .. key .. " = " .. value)
			return value + 1
		end, function (object, key, oldValue, newValue)
			print("TRYING TO WRITE " .. key .. " = " .. newValue)
			return oldValue -- readonly
		end)

		print("PRINT VALUE" .. a.c)
		a.c = 2
		print("PRINT VALUE" .. a.c)
		UnHook(a)
		a.c = 2
		print("PRINT VALUE" .. a.c)


<Encode/Decode Functions>

	[function Dump(t)]
		Returns a lua string representation of t.

	[function Load(t)]
		Loads a lua string generated by Dump() and rebuild the table.
		Remarks: Dump/Load supports circled reference in table.

	[function Encode(t, dictionary)]
		Encode a lua table and returns a binary representation of it.

	[function Decode(t, dictionary)]
		Decode a lua table from a binary representation by Encode.
		Remarks: The data will be compressed if there are duplicated
			strings/tables in t.
		Remarks: The parameter dictionary can be omitted or reused for
			frequency data exchange to reduce data length.
		Exsample:
			-- sender --
			dict = {}
			a = Encode(aaa, dict)
			send(a)
			b = Encode(bbb, dict)

			-- receiver --
			dict = {}
			aaa = Decode(a, dict)
			bbb = Decode(b, dict)

			Notice that you must keep the same order of Encode(s)
				and Decode(s) between sender and receiver if parameter
				dict is enabled. Otherwise you must set dict to nil.
				For example:

			-- sender --
			a = Encode(aaa)
			b = Encode(bbb)

			-- receiver --
			-- misorder
			bbb = Decode(b)
			aaa = Decode(a)
		Long exsample:
			local inner = { 'def' }
			local ms = { 'abc' }
			local mc = { 'abc' }
			local md = { 'abc' }
			local test = { 'a', ['bbb'] = 23, 'fff', 89, ['def'] = 'bbb', ['ccc'] = inner, ['ms'] = ms, ['md'] = md, ['mm'] = mc }
			inner.rec = test
			local data = Encode(test)
			local org = Decode(data)

			local mark = { }
			local function Output(org)
				mark[org] = true
				for k, v in pairs(org) do
					print("[" .. k .. "] = ")
					if type(v) == "table" then
						print("{ ")
						if mark[v] == nil then
							mark[v] = true
							Output(v)
						else
							print("DUP")
						end
						print("} ")
					else
						print("" .. v)
					end
				end
			end

			Output(org)

			local large = {}
			for i = 0, 1000 do
				table.insert(large, { text = 'hello, world', 123478, 9234, 'ase' })
			end

			print("ENCODE LENGTH: " .. string.len(Encode(large)))


--]====================================================================]

local Core = {}

local function RecursiveMerge(g, t)
	for k, v in pairs(t) do
		if type(v) == "table" then
			local w = g[k] or {}
			assert(type(w) == "table")
			g[k] = RecursiveMerge(w, v)
		else
			g[k] = v
		end
	end

	return g
end

Core.Merge = RecursiveMerge

local function RecursiveDiff(g, t)
	for k, v in pairs(t) do
		if type(t[k]) == "table" and type(v) == "table" then
			local w = g[k] or {}
			assert(type(w) == "table")
			g[k] = RecursiveDiff(w, v)
		else
			g[k] = nil
		end
	end

	return g
end

Core.Diff = RecursiveDiff

function Core.Patch(g, t)
	-- clear metatable first
	setmetatable(g, nil)
	-- clear old values ...
	for k, v in pairs(g) do
		g[k] = nil
	end

	-- copy new values ...
	for k, v in pairs(t) do
		g[k] = v
	end

	setmetatable(g, getmetatable(t))

	return g
end

local function RecursiveCheck(g, t, depth : integer, record : { string })
	assert(t)
	local record : { integer : any } = {}

	local prefix : string = tostring(#record + 1) .. string.rep("\t", depth + 1) 
	if g == nil then
		table.insert(record, prefix .. "Invalid input variable : should not be nil")
	elseif type(t) == "string" then
		if type(g) ~= t then
			table.insert(record, prefix .. "Invalid input variable of type '" .. type(g) .. "', should be '" .. t .. "'.\n")
		end
	elseif type(t) == "function" then
		t(g, depth, record)
	else
		local gmeta = getmetatable(g)
		local tmeta = getmetatable(t)

		if tmeta and tmeta ~= gmeta then
			table.insert(record, prefix .. "Unmatched metatable\n")
		end
		for k, v in pairs(t) do
			local w = g[k]
			if type(w) == "table" and type(v) == "table" then
				RecursiveCheck(w, v, depth + 1, record)
			elseif type(w) ~= type(v) then
				table.insert(record, prefix .. "Invalid key '" .. k .. "' of type '" .. type(w) .. "', should be '" .. type(v) .. ".'\n")
			end
		end
	end

	local failed = #record ~= 0
	assert(not failed, failed and ("\n================================================================\nCore TypeCheck Assertion Failed!\n\n" .. table.concat(record) .. "\n================================================================\n" .. debug.traceback()))

	return g
end

Core.Check = RecursiveCheck

local function Extract(func)
	return function (a, b)
		assert(b.value)
		return func(a, b.value)
	end
end

local MetaStyle = {
	__add = Extract(Core.Merge),
	__sub = Extract(Core.Diff),
	__mod = Extract(Core.Check),
	__index = function (host, key : string)
		return host.value[key]
	end,
	__newindex = function (host, key : string, value : any)
		host.value[key] = value
	end,
	__pairs = function (host)
		return pairs(host.value)
	end,
}

function MetaStyle:__call(s)
	assert(s, debug.traceback())
	local wrap = { value = s }
	setmetatable(wrap, MetaStyle)
	return wrap
end


function Core.UnHook(object)
	local mt = getmetatable(object)
	assert(mt)

	local org = getmetatable(mt)
	setmetatable(object, org)

	-- Export vars
	for k, v in pairs(mt.__Holder) do
		object[k] = v
	end

	return object
end

function Core.Hook(object, readHandler, writeHandler)
	local org = getmetatable(object)
	local holder : { any : any } = {}
	local mt : { string : any } = {}
	mt.__Holder = holder
	if readHandler then
		mt.__index = function (o, k)
			assert(o == object)
			return readHandler(object, k, mt.__Holder[k])
		end
	end

	if writeHandler then
		mt.__newindex = function (o, k, v)
			assert(o == object)
			if writeHandler then
				mt.__Holder[k] = writeHandler(object, k, mt.__Holder[k], v)
			else
				mt.__Holder[k] = v
			end
		end
	end

	-- Import vars
	for k, v in pairs(object) do
		mt.__Holder[k] = v
		object[k] = nil
	end

	setmetatable(mt, org)
	setmetatable(object, mt)

	return object
end
local function Execute(defer, ...)
	return ...
end

local function Return(status, ...)
	if not status then
		print(...)
	end

	return ...
end

local anyone : any = {}

function Core.Any() : any
	return anyone
end

function Core.Instance()
	return coroutine.running()
end

local LoopPosts : { any : any } = {}
setmetatable(LoopPosts, { __mode = "k" })

function Core.Invoke(func, ...)
	local co = coroutine.create(function (...)
		local status, msg = xpcall(func, function (msg)
			print("Error: " .. tostring(msg) .. "\nCallstack: \n" .. debug.traceback())
		end, ...)
	end)
	-- typedlua do not support thread as key
	LoopPosts[co] = {}

	-- Go
	assert(coroutine.status(co) == "suspended")
	coroutine.resume(co, ...)
	return co
end

local function PostOne(instance : thread, ...)
	if coroutine.running() ~= instance then
		-- Listen

		-- loop?
		if coroutine.status(instance) == "normal" then
			-- queued
			LoopPosts[instance] = LoopPosts[instance] or {}
			table.insert(LoopPosts[instance], table.pack(...))
		else
			assert(coroutine.status(instance) == "suspended", "Incorrect coroutine state: " .. coroutine.status(instance))
			return Return(coroutine.resume(instance, ...))
		end
	else
		-- Request
		return coroutine.yield(...)
	end
end

local function RecursivePost(port, ...)
	if type(port) == "thread" then
		PostOne(port, ...)
	else
		for k, v in pairs(port) do
			RecursivePost(v, ...)	
		end
	end
end

Core.Post = RecursivePost

-- Listen: receive posts in passive way

local function Finalize(port, instance : thread, ...)
	port[instance] = nil
	return ...	
end
-- Listen([port], callback)

local function YieldWithProcessLoopPosts(...)
	local instance = coroutine.running()
	local posts = LoopPosts[instance]
	if posts and #posts ~= 0 then
		return coroutine.yield(table.unpack(table.remove(posts)))
	else
		return coroutine.yield(...)
	end
end

function Core.WrapListenWithArgs(port, callback)
	local c = port
	if type(c) == "table" then
		local instance = coroutine.running()
		if callback then
			port[instance] = instance
			return function (...)
				return callback(callback, Finalize(port, instance, YieldWithProcessLoopPosts(...)))
			end
		else
			return function (...)
				return function (...)
					port[instance] = instance
					return Finalize(port, instance, YieldWithProcessLoopPosts(...))
				end, ...
			end
		end
	else
		if callback then
			return function (...)
				return callback(callback, YieldWithProcessLoopPosts(...))
			end
		else
			return function (...)
				return YieldWithProcessLoopPosts, ...
			end
		end
	end
end

function Core.Listen(port, callback, ...)
	local handler : any = Core.WrapListenWithArgs(port, callback)
	return handler(...)
end

function Core.StepListen(port, ...)
	return Core.Listen(port, ...)()
end

-- Request: receive posts in active way
function Core.Request(proc, ...)
	local inst = coroutine.running()
	local instance = coroutine.create(function (...)
		return proc(inst, ...)
	end)

	inst = instance

	return function (...)
		if coroutine.status(instance) == "suspended" then
			return Return(coroutine.resume(instance, ...))
		end
	end, ...
end

local function RouteImpl(port, map, command, ...)
	if command == nil then return end

	local handler = map[command]
	assert(handler)

	if type(handler) == "thread" then
		Core.Post(handler, ...)
	elseif type(handler) == "function" then
		handler(...)
	end

	return RouteImpl(port, map, Core.StepListen())
end

function Core.Route(port, map)
	return RouteImpl(port, map, Core.StepListen())
end

function Core.SortedPairs(input, comp)
	local keys : { integer : any } = {}
	for k in pairs(input) do
		table.insert(keys, k)
	end

	table.sort(keys, comp)
	local i = 0

	return function ()
		i = i + 1
		local v = keys[i]
		return v, input[v]
	end
end

--[[
local test = { 'a', 'b', ['c'] = 'd', 1234 }

local function YieldReturnProc(instance, object)
	for k, v in pairs(object) do
		Core.Post(instance, v)
	end
end

for v in Core.Request(YieldReturnProc, test) do
	print(v)
end
]]


local function RecursiveCopy(source)
	if type(source) ~= "table" then
		return source
	end

	local ret : { any : any } = {}
	for k, v in pairs(source) do
		ret[k] = RecursiveCopy(v)
	end

	local meta = getmetatable(source)
	if meta then
		setmetatable(ret, meta)
	end

	return ret
end

function Core.Copy(s)
	local ret : any = RecursiveCopy(s)
	return ret
end

-- Iterators
function Core.Inspect(x, selector, iterator)
	iterator = iterator or pairs
	selector = type(selector) == "string" and function (t) return t[selector] end or selector
	local iter = iterator(x)
	local subroute : any = nil
	local lastKey = nil

	return function ()
		local k : any = nil
		local v : any = nil

		while not v do	
			while not subroute do
				local key, value = iter(x, lastKey)
				if value ~= nil then
					lastKey = key
					subroute = subroute or (type(value) == "table" and Inspect(value, selector, iterator))

					if not selector or selector(value) then
						return key, value
					end
				else
					return
				end
			end
		
			k, v = subroute()
			subroute = v and subroute
		end

		return k, v
	end
end

-- Instance save/load

local function WrapRoutine(package, key, v, packageName)
	return function (state, ...)
		assert(state)
		local current : any = require(packageName)

		if current ~= package then -- switch module or hot patch?
			local leaveRoutine = package.Leave
			if leaveRoutine then
				leaveRoutine(state)
			end

			state.CurrentRoutine = key
			state.CurrentPackage = packageName

			local enterRoutine = current.Enter
			if enterRoutine then
				enterRoutine(state)
			end

			v = current[key] -- switch to new routine
		else
			state.CurrentRoutine = key
			state.CurrentPackage = packageName
		end

		return v(state, ...)
	end
end

function Core.Package(package, name)
	assert(name) -- You should use return Instance.WrapPackage([[ Package ]], ...) to pass argument 'name'.

	for k, v in pairs(package) do
		if type(v) == "function" then	
			package[k] = WrapRoutine(package, k, v, name)
		end
	end

	return package
end

function Core.State(package, routine)
	return { CurrentPackage = package, CurrentRoutine = routine or "Main", Data = {} }
end

function Core.LocalData(state)
	local data = state.Data[state.CurrentRoutine]
	if data == nil then
		data = {}
		state.Data[state.CurrentRoutine] = data
	end

	return data
end

function Core.Run(state)
	local package : any = require(state.CurrentPackage)
	local routine = package[state.CurrentRoutine]
	if not routine or type(routine) ~= "function" then
		error("Instance.Execute() : Invalid package function " .. state.CurrentRoutine .. " in package " .. state.CurrentPackage)	
	end

	return routine(state)
end

function Core.DebugTrace()
	print(debug.traceback())
end

local function Safepairs(tab)
	local compare : { integer : any } = { }
	for k in pairs(tab) do
		table.insert(compare, k)
	end

	table.sort(compare, function(a, b)
		local ta = type(a)
		local tb = type(b)
		return ta == tb and a < b or ta < tb
	end)
	local i = 0

	return function()
		i = i + 1
		local k = compare[i]
		local v = tab[k]
		return k, v
	end
end

function Core.Load(str, binary, name)
	local f = load(str, name or "chunk", binary and "b" or "t", { })
	if f then
		local record : { integer : any } = { }
		local function LoadEx(node : { any: any }?)
			if type(node) == "table" then
				local r = node.__ref
				if r then
					return record[r]
				end

				table.insert(record, node)
				local index = #record

				for k, v in Safepairs(node) do
					node[k] = LoadEx(v)
				end
			end

			return node
		end

		local ret = LoadEx(f())
		return ret
	else
		print("Load failed")
	end
end


local function SafeStr(v)
	return string.gsub(v, "'", "\\'")
end

local function ToStr(v)
	local t = type(v)
	if t == "number" then
		return "" .. v
	elseif t == "boolean" then
		return v and "true" or "false"
	elseif t == "string" then
		return "'" .. SafeStr(v) .. "'"
	end

	error("Error type " .. t)
	return "Invalid"
end

function Core.Dump(obj, binary)
	local record : { any : any } = { }
	local str : { string } = {}

	table.insert(str, "return \n")

	local current = 1
	local function DumpEx(node)
		local t = type(node)
		if t == "table" then
			table.insert(str, "{\n")

			if record[node] then
				table.insert(str, "__ref = " .. record[node])
			else
				record[node] = current
				current = current + 1
				for k, v in Safepairs(node) do
					-- table as key is not allowed
					assert(type(k) == "boolean" or type(k) == "number" or type(k) == "string")
					local s : any = ToStr(k)
					table.insert(str, "[" .. s .. "] = ")
					DumpEx(v)
					table.insert(str, ", \n")
				end

				-- node.__base = nil -- clear
			end

			table.insert(str, "}")
		else
			local t = ToStr(node)
			if t then
				table.insert(str, t)
			end
		end
	end

	DumpEx(obj)

	if binary then
		local func : any = load(table.concat(str))
		return string.dump(func, true)
	else
		return str[1]
	end
end


local maskValueTable = 1 -- 1 << 0
local maskValueRef = 2 -- 1 << 1
local maskKeyTable = 4 -- 1 << 2
local maskKeyRef = 8 -- 1 << 3
local maskBoolAsNumber = 16 -- 1 << 4

local function EncodeEx(object, formatDict, refDict)
	refDict = refDict or {}
	local ret : { string } = {}
	local format : { string } = {}
	table.insert(format, "!1<")

	local function AddDictEntry(object)
		refDict.__counter = (refDict.__counter or 0) + 1
		refDict[object] = refDict.__counter
	end

	AddDictEntry(object)

	local function WriteMask(typeMask, value, ref, tab) : (integer, string, any)
		local typeValue = type(value)
		local cache = refDict[value]
		local retTypeMask : integer = typeMask
		local retType : string = "I4"
		local retValue : any = ""

		if cache then
			retTypeMask = typeMask | ref
			retType = "I4"
			retValue = cache
		elseif typeValue == "table" then
			local s, c = EncodeEx(value, formatDict, refDict)
			cache = refDict[s]
			if cache then
				retTypeMask = typeMask | tab | ref
				retType = "I4"
				retValue = cache
			else
				AddDictEntry(s)
				retTypeMask = typeMask | tab
				retType = "s"
				retValue = s
			end
		elseif typeValue == "string" then
			AddDictEntry(value)
			retTypeMask = typeMask
			retType = "s"
			retValue = value
		elseif typeValue == "boolean" then
			retTypeMask = typeMask | maskBoolAsNumber
			retType = "I4"
			retValue = (value and 1 or 0)
		else
			assert(typeValue == "number", typeValue)
			retTypeMask = typeMask
			retType = "n"
			retValue = value
		end

		return retTypeMask, retType, retValue
	end

	for k, v in pairs(object) do
		table.insert(format, "b")
		local typeMask : integer = 0
		local kt : any, kc : any, vt : any, vc : any

		typeMask, kt, kc = WriteMask(typeMask, k, maskKeyRef, maskKeyTable)
		typeMask, vt, vc = WriteMask(typeMask, v, maskValueRef, maskValueTable)

		table.insert(ret, typeMask)
		table.insert(format, kt)
		table.insert(ret, kc)
		table.insert(format, vt)
		table.insert(ret, vc)
	end

	local s = table.concat(format)
	local f = formatDict[s]
	if not f then
		formatDict.__counter = (formatDict.__counter or 0) + 1
		formatDict[s] = formatDict.__counter
		f = formatDict.__counter
	end

	local content = string.pack(s, table.unpack(ret))
	return string.pack("!1<I4s", f, content)
end

function Core.Encode(object, refDict)
	local formatDict : { string : any } = {}
	local data = EncodeEx(object, formatDict, refDict)
	local title = "!1<" .. string.rep("s", formatDict.__counter or 0)
	local formatTable : { any : any } = {}

	for i = 1, formatDict.__counter or 0 do
		formatTable[i] = false
	end

	for k, v in pairs(formatDict) do
		if k ~= "__counter" then
			formatTable[v] = k
		end
	end
	
	local format = string.pack(title, table.unpack(formatTable))
	return string.pack("!1<sss", title, format, data)
end

local function DecodeEx(data, formatDict, refDict)
	refDict = refDict or {}
	local s, content, unread = string.unpack("!1<I4s", data)
	local expand = table.pack(string.unpack(formatDict[s], content))
	local object : { any : any } = {}

	table.insert(refDict, object)

	local function WriteContent(object, typeMask, ref, tab)
		if (typeMask & ref) == ref then
			assert(type(object) == "number")
			local ret = refDict[object]
			if (typeMask & tab) == tab then
				ret = DecodeEx(ret, formatDict, refDict)				
			end
			return ret
		elseif (typeMask & tab) == tab then
			assert(type(object) == "string")
			local ret = DecodeEx(object, formatDict, refDict)
			table.insert(refDict, object)
			return ret
		else
			if type(object) == "string" then
				table.insert(refDict, object)
			end

			return object
		end
	end

	for i = 1, #expand - 3, 3 do
		local typeMask = expand[i]
		local key = WriteContent(expand[i + 1], typeMask, maskKeyRef, maskKeyTable)
		local value : any = WriteContent(expand[i + 2], typeMask, maskValueRef, maskValueTable)
		
		if (typeMask & maskBoolAsNumber) == maskBoolAsNumber then
			value = (value ~= 0)
		end

		-- key of type number must be integer
		if type(key) == "number" then
			object[math.tointeger(key)] = value
		else
			object[key] = value
		end
	end

	return object, unread
end

function Core.Decode(data, refDict)
	local title, format, text = string.unpack("!1<sss", data)
	local formatTable = table.pack(string.unpack(title, format))

	return DecodeEx(text, formatTable, refDict)
end

function Core.Verify(value, message, state, ...)
	state = state == nil and value or state
	if state then
		return value
	else
		error(message(value) .. "\nTrace back: " + debug.traceback(), 1, ...)
	end
end

function Core.Cast(value, expectedType : string)
	return Core.Verify(value, function (value)
		return "Unexpected type " .. type(value) .. ", expect " .. expectedType
	end, type(value) == expectedType)
end

function Core.Try(func, ...)
	return pcall(func, ...)
end

local function less(a, b)
	return a < b
end

function Core.Queue(batchSize : number)
	local list = { from = 1, length = 0 }
	list.front = list
	list.next = list
	local queue = {}
	queue.list = list
	queue.batchSize = batchSize or 64

	return queue
end

function Core.Heap(comparator)
	local queue = {}
	queue.__comparator = comparator or less

	return queue
end

function Core.Push(queue, object)
	local comparator = queue.__comparator
	if comparator == nil then
		-- deque
		local list = queue.list.front
		if list.length > queue.batchSize then
			local new = { from = 1, length = 0}
			new.front = list
			new.next = list.next
			list.next.front = new
			list.next = new

			list = new
		end
		
		list.length = list.length + 1
		list[list.length] = object
	else
		table.insert(queue, object)
	
		local v = #queue
		local p = v >> 1
		while p ~= 0 and comparator(queue[p], queue[v]) do
			queue[v], queue[p] = queue[p], queue[v]
			v = p
			p = p >> 1
		end
	end
end

function Core.Pop(queue, object)
	local comparator = queue.__comparator
	if comparator == nil then
		-- deque
		local list = queue.list
		if list.from > list.length then
			-- remove
			assert(list.next ~= list)
			list.front.next = list.next
			list.next.front = list.front
			list = list.next
			queue.list = list
		end

		assert(list.from < list.length + 1, "FROM = " .. list.from ..  " TO = " .. list.length + 1)

		local value = list[list.from]
		list[list.from] = nil
		list.from = list.from + 1

		return value
	else
		assert(#queue ~= 0)
		if #queue == 1 then return table.remove(queue) end
	
		local ret = queue[1]
		queue[1] = table.remove(queue)

		local p = 1
		local v = p * 2
		local size = #queue
	
	
		while v <= size do
			if v < size and comparator(queue[v], queue[v + 1]) then
				v = v + 1
			end

			if comparator(queue[p], queue[v]) then
				queue[v], queue[p] = queue[p], queue[v]
				p = v
				v = v * 2
			else
				break
			end
		end	
	
		return ret
	end
end

function Core.RandomLucky(t : number, alpha : number, eps : number, random, initthres)
	assert(t < 0.5)
	assert(alpha >= 0 and alpha <= 1)

	local s = 2.0 * t / (1.0 - 2.0 * t)
	alpha = 1.0 - (1.0 - ((1 - t) ^ s) * (t ^ (s * t / (1 - t)))) * t * alpha

	eps = eps or 1e-12
	random = random or math.random

	local e = 1.0 / t
	local accurate = eps * e

	-- numeric method
	local min = 0.0
	local max = 1.0
	local p = t

	-- print("BEGIN")
	while max - min > eps do
		local q = 1.0
		local a1 = 1.0
		local a2 = 1.0
		local sum = 0.0
		local item = 0.0
		local qq = 1.0 - p

		for i = 1, 16 * e do
			assert(a1 <= 1)
			item = q * i * a2 * (1.0 - qq * a1)
			a2 = a2 * a1
			a1 = a1 * alpha
			sum = sum + item
			q = q * (1.0 - p)

			if sum > e + accurate or item < accurate then
				break
			end
		end

		-- print("SUM = ", sum)

		local diff = sum - e
		if math.abs(diff) < accurate then
			break
		elseif diff > 0 then
			min = p
		else
			max = p
		end
		
		p = (min + max) / 2
	end

	-- print("FINAL TEST p = ", p)
	-- then we get p

	local threshold = initthres or p
	return function ()
		if math.random() <= threshold then
			threshold = p 
			return true, threshold
		else
			-- print("THRES = ", threshold)
			threshold = 1.0 - (1.0 - threshold) * alpha
			return false, threshold
		end
	end, p
end

--[[
math.randomseed(os.time())

local args = { ... }

local p = tonumber(args[1]) or 0.05
local last = tonumber(args[2]) or 16
for k = 1, last do
	local sum = 0
	local count = 100000
	local generator, p0 = RandomLucky(p, 0.5 ^ (k - 1))
	for i = 1, count do
		for j = 1, 100000000 do
			if generator( then
				sum = sum + j
				break
			end
		end
	end

	print(string.format("K = %d E = %6.3f E0 = %6.3f |E - E0| = %6.3f p0 = %6.12f p = %6.3f", k, 1.0 / p, sum / count, 1.0 / p - sum / count, p0, p))
end

]]
return Core
