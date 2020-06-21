local CrossRoutine = record end

function CrossRoutine.New(func : function (... : any) : any...) : function (...:any) : any...
	-- compile function 
	local nupvals = debug.getinfo(func, "u").nups as number
	local binaryCode = string.dump(func)
	local computeComponent = ComputeComponentModule.New(true) -- use transparent mode
	local remoteFunc = computeComponent:Load(binaryCode)
	assert(remoteFunc)
	-- set upvalues
	local upvalues : { any } = {}
	for i = 1, nupvals do
		local name, value = debug.getupvalue(func, i)
		if name ~= "_ENV" then
			table.insert(upvalues, i)
			table.insert(upvalues, value)
		end
	end

	local setupEnvironment = computeComponent:Load([[
		local func, upvalues = ...
		for i = 1, #upvalues / 2 do
			local index = upvalues[i * 2 - 1]
			local value = upvalues[i * 2]
			debug.setupvalue(func, i, value)
		end
	]])

	computeComponent:Call(setupEnvironment, remoteFunc, upvalues)

	return function (... : any) : any...
		local running = coroutine.running()

		computeComponent:CallAsync(function (... : any)
			coroutine.resume(running, ...)
		end, remoteFunc, ...)
		
		return coroutine.yield()
	end
end

return CrossRoutine