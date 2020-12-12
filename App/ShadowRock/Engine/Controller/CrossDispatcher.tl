local CrossDispatcher = record
	Registered : { { string : any } }
	States : { boolean }
	Routines : { any }
	Coroutine : thread
	__index : any
end

CrossDispatcher.__index = CrossDispatcher

local CrossRoutine = require("Engine/Controller/CrossRoutine")

function CrossDispatcher.New(count : number) : CrossDispatcher
	local object = {
		Routines = {},
		States = {},
		Registered = {},
		Coroutine = nil,
	}

	local running = coroutine.running()
	for i = 1, count do
		table.insert(object.Registered, {})
		table.insert(object.States, true)
		table.insert(object.Routines, CrossRoutine.New(function ()
			object.States[i] = true
			if object.Coroutine then
				coroutine.resume(running)
			end
		end))
	end

	return setmetatable(object, CrossDispatcher as METATABLE)
end

function CrossDispatcher:Register(cacheName : string, func : function (... : any) : any...)
	for i, v in ipairs(self.Registered) do
		local routine = self.Routines[i] as function (any) : any
		v[cacheName] = routine(func) -- wraps function
	end
end

function CrossDispatcher:Dispatch(cacheName : string, ...:any)
	-- find routine
	while true do 
		for i, v in ipairs(self.Routines) do
			if self.States[i] then
				local func = self.Registered[i][cacheName] as function (...:any)
				assert(func, "must register first!")
				self.States[i] = false
				self.Coroutine = nil
				func(...)
				return
			end
		end

		self.Coroutine = coroutine.running()
		coroutine.yield()
	end
end

function CrossDispatcher:Wait()
	assert(not self.Coroutine)
	self.Coroutine = coroutine.running()
	for i, v in ipairs(self.Routines) do
		while not self.States[i] do
			coroutine.yield()
		end
	end

	self.Coroutine = nil
end

function CrossDispatcher:Clear()
	for i, _ in ipairs(self.Registered) do
		self.Registered[i] = {}
	end
end

return CrossDispatcher