local Test = record end

function Test.Main()
	local component = RemoteComponentModule.New(false)
	local func = component:Load("print(...) return 2345")
	component:Call(func, "1234")
	local running = coroutine.running()
	component:CallAsync(function (value)
		print("RET: " .. tostring(value))
		coroutine.resume(running)
	end, func, "0123")

	coroutine.yield()
	print("NEXT")

	local backward = component:Load([[
		local function test(callback)
		   SysCall(callback, "It' me!")
		end
		test(...)
	]])

	component:Call(backward, function (msg : string)
	   print("GET MSG: " .. msg)
	end)

	local transparent = RemoteComponentModule.New(true)
	local foo = transparent:Load([[
		local function test(callback)
		   callback("It' me~~~!")
		end
		test(...)
	]])
	transparent:Call(foo, function (msg : string)
	   print("GET MSG: " .. msg)
	end)
	
	local CrossRoutine = require("Engine/Controller/CrossRoutine")

	-- CrossRoutine is running on another LuaVM
	local upval = "~~~"
	local rpc = CrossRoutine.New(function (value : string) : string
		return "Hello " .. value .. upval
	end)

	print("MSG " .. rpc("World!") as string)
	print("MSG " .. rpc("NewWorld!") as string)
end

return Test