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
	print("Receive network data!!!!!!!!!!!!!!!")
	local t = Core.Decode(data) as { string : string }
	if t then
		local s = ""
		for k, v in pairs(t) do
			s = s .. "[" .. k .. "] = " .. v .. "\n"
		end

		print("Receive Data: \n" .. s)
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
	local oldprint = print
	_G.print = function (...)
		oldprint("[Worker " .. Common.MainThreadID .. "] " .. tostring(table.concat({ ... } as { string }, " ")))
	end

	global WorkerInstance = Worker.New(tonumber(masterThreadID), tonumber(masterPort))
	ToolkitWin32.ListenMessage(function (message : number, wparam : number, lparam : number)
		Common.DispatchMessage(WorkerInstance as Common, message, wparam, lparam)
	end)

	Common.InitializeListener(WorkerInstance as Common, "127.0.0.1:0")
	Common.InitializeTicker(WorkerInstance as Common)

	-- Try to load system info from master
	global connection = WorkerInstance.Listener:Connect("127.0.0.1:" .. masterPort)
	if connection then
		local data = Core.Encode({ Action = "Info" } as { any : any })
		print("Send request to " .. masterPort .. " length: " .. tostring(#data))
		connection:WriteConnection(data)
	end
end

return Worker