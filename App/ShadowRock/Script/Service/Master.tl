local Common = require("Script/Service/Common")
local Core = require("Engine/Core")

local WorkerItem = record
	Handle : number
	Port : number
	EchoTimeStamp : number
end

local Master = record
	Workers : { number : WorkerItem } -- main thread id -> process handle 
	WorkerCount : number
	Now : number
	Port : number
	__index : any
end

Master.__index = Master

local MessageMap = Common.MessageMap
local Messages = Common.Messages
local mainThreadID = Common.MainThreadID
local hostPath = ToolkitWin32.GetHostPath()
local systemInfo = ToolkitWin32.GetSystemInfo() as { string : string } 

function Master:__gc()
	for threadID, item in pairs(self.Workers) do
		ToolkitWin32.PostThreadMessage(threadID, Messages.WM_QUIT, 0, 0)
		ToolkitWin32.CloseHandle(item.Handle)
	end
end

function Master:WM_ECHO(wparam : number, lparam: number)
	-- extract main thread id
	print("ECHO RECEIVED: " .. tostring(self.Now) .. " Listen at " .. tostring(lparam))
	local workerThreadID = wparam
	if workerThreadID ~= 0 then
		local worker = self.Workers[workerThreadID]
		if worker then
			worker.EchoTimeStamp = self.Now
			worker.Port = lparam
			ToolkitWin32.PostThreadMessage(workerThreadID, Messages.WM_ECHO_RESPONSE, Common.MainThreadID, self.Port)
		else
			-- not exist, let it die
			ToolkitWin32.PostThreadMessage(workerThreadID, Messages.WM_QUIT, 0, 0)
		end
	end
end

function Master:SpawnWorker() : WorkerItem
	-- empty string for self
	local id = ToolkitWin32.CreateProcess(hostPath .. " /service \"--IScript=#!Script/Service/Worker " .. tostring(mainThreadID) .. "\"", "Show", "") as { number }
	local processHandle = id[1]
	local threadID = id[2]
	if processHandle ~= 0 then
		print("Spawn a worker with id: " .. threadID)
		self.Workers[threadID] = { Handle = processHandle, EchoTimeStamp = self.Now } as WorkerItem
	else
		return nil
	end
end

function Master:Tick(now : number)
	-- Spawn workers
	-- Check timed out workers
	print("Time: " .. tostring(now))
	self.Now = now

	local currentWorkerCount = 0
	for threadID, item in pairs(self.Workers) do
		if item.EchoTimeStamp + Common.TIME_OUT < now then
			-- kick off
			print("Worker " .. threadID .. " was not able to be connected for " .. Common.TIME_OUT .. " seconds. Kill it at once!")
			ToolkitWin32.PostThreadMessage(threadID, Messages.WM_QUIT, 0, 0)
			ToolkitWin32.CloseHandle(item.Handle)
			self.Workers[threadID] = nil -- it's safe in lua
		else
			currentWorkerCount = currentWorkerCount + 1
		end
	end

	for i = currentWorkerCount, self.WorkerCount - 1 do
		self:SpawnWorker()
	end
end

function Master:NetworkData(connection : Connection, data : string)
	-- parse data
	local pack = Core.Decode(data) as { string : any}

	-- write result

	-- dispatch request
	if pack.Action == "Query" then
		connection:WriteConnection(Core.Encode(self.Workers as { any : any }))
	end
end

function Master:NetworkState(connection : Connection, state : boolean, src : string, dst : string) : any
	-- stateless, just print state out
	if state then
		print("New connection from " .. src)
	else
		print("Lost connection from " .. src)
	end
end

function Master.New(workerCount : number) : Master
	local master = {
		WorkerCount = workerCount,
		Workers = {}
	}

	return setmetatable(master, Master as METATABLE)
end

function Master.Main(workerCount : number)
	print("LeavesWind Master")
	print("System information:")
	for k, v in pairs(systemInfo) do
		print(k .. ": " .. tostring(v))
	end

	local oldprint = print
	_G.print = function (...)
		oldprint("[Master " .. Common.MainThreadID .. "] " .. tostring(table.concat({ ... } as { string }, " ")))
	end

	global MasterInstance = Master.New(workerCount or 2)
	ToolkitWin32.ListenMessage(function (message : number, wparam : number, lparam : number)
		Common.DispatchMessage(MasterInstance as Common, message, wparam, lparam)
	end)

	Common.InitializeListener(MasterInstance as Common)
	Common.InitializeTicker(MasterInstance as Common)
end

return Master