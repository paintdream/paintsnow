local Common = require("Script/Service/Common")
local Terminal = require("Engine/Controller/Terminal")
local Core = require("Engine/Core")
local Messages = Common.Messages

local Worker = record
	MasterThreadID : number
	Now : number
	ResponseTimeStamp : number
	Listener : Terminal
	Port : number
	MasterPort : number
	__index : any
end

Worker.__index = Worker
local oldprint = print

function Worker:WM_ECHO_RESPONSE(wparam, lparam)
	print("ECHO CONFIRMED: " .. tostring(self.Now))
	self.ResponseTimeStamp = self.Now
end

function Worker:Tick(now : number)
	-- Send heart beat
	print("Time: " .. tostring(self.ResponseTimeStamp))
	self.Now = now
	if self.ResponseTimeStamp + Common.TIME_OUT < now then
		print("Cannot connect to master, exit now.")
		ToolkitWin32.PostThreadMessage(Common.MainThreadID, Messages.WM_QUIT, 0, 0)
	else
		ToolkitWin32.PostThreadMessage(self.MasterThreadID, Messages.WM_ECHO, Common.MainThreadID, self.Port)
	end
end

function Worker:NetworkData(connection : Connection, data : string)
	print("Worker receive request length " .. tostring(#data))
	-- parse data
	local pack = Core.Decode(data) as { string : any }
	-- write result

	-- dispatch request
	local action = pack.Action as string
	if action == "Execute" then
		local code = pack.Code as string
		local func, err = load(code, pack.Name as string)
		local ret : { string : string } = {}
		if not func then
			ret.Message = "Compile error: " .. err
		else
			local s, msg = pcall(function () : string
				return func(connection) as string
			end)

			if not s then
				ret.Message = "Run error: " .. msg
			else
				ret.Message = ""
			end
		end

		connection:WriteConnection(Core.Encode(ret as { any : any }))
	end
end

function Worker:NetworkState(connection : Connection, state : boolean, src : string, dst : string) : any
	return true
end

function Worker.New(masterThreadID : number, masterPort : number) : Worker
	local worker = {
		MasterThreadID = masterThreadID,
		Now = 0,
		ResponseTimeStamp = 0,
		MasterPort = masterPort
	}

	return setmetatable(worker, Worker as METATABLE)
end

function Worker.Main(masterThreadID : string, masterPort : string)
	print("LeavesWind Worker at " .. Common.MainThreadID)
	_G.print = function (...)
		oldprint("[Worker " .. Common.MainThreadID .. "] " .. tostring(table.concat({ ... } as { string }, " ")))
	end

	global WorkerInstance = Worker.New(tonumber(masterThreadID), tonumber(masterPort))
	ToolkitWin32.ListenMessage(function (message : number, wparam : number, lparam : number)
		Common.DispatchMessage(WorkerInstance as Common, message, wparam, lparam)
	end)

	Common.InitializeListener(WorkerInstance as Common, "127.0.0.1:0")
	Common.InitializeTicker(WorkerInstance as Common)
end

return Worker