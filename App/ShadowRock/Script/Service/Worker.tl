local Common = require("Script/Service/Common")
local Messages = Common.Messages

local Worker = record
	MasterThreadID : number
	Now : number
	ResponseTimeStamp : number
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
		ToolkitWin32.PostThreadMessage(self.MasterThreadID, Messages.WM_ECHO, Common.MainThreadID, 0)
	end
end

function Worker.New(masterThreadID : number) : Worker
	local worker = {
		MasterThreadID = masterThreadID,
		Now = 0,
		ResponseTimeStamp = 0,
	}

	return setmetatable(worker, Worker as METATABLE)
end

function Worker.Main(masterThreadID : string)
	print("LeavesWind Worker at " .. Common.MainThreadID)
	local oldprint = print
	_G.print = function (...)
		oldprint("<Worker id=" .. Common.MainThreadID .. ">")
		oldprint(...)
		oldprint("<Worker/>")
	end

	global WorkerInstance = Worker.New(tonumber(masterThreadID))
	ToolkitWin32.ListenMessage(function (message : number, wparam : number, lparam : number)
		Common.DispatchMessage(WorkerInstance as Common, message, wparam, lparam)
	end)

	Common.InitializeListener(WorkerInstance as Common, "127.0.0.1:0")
	Common.InitializeTicker(WorkerInstance as Common)
end

return Worker