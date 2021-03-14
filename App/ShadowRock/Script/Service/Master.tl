local Common = require("Script/Service/Common")
local Terminal = require("Engine/Controller/Terminal")
local Core = require("Engine/Core")

local WorkerItem = record
	Handle : number
	Port : number
	EchoTimeStamp : number
end

local ClusterItem = record
	WorkerCount : number
	Name : string
	Processor : string
	CoreCount : string
	EchoTimeStamp : number
end

local Master = record
	Workers : { number : WorkerItem } -- main thread id -> process handle 
	Clusters : { string : ClusterItem }
	UpstreamClusterAddress : string
	UpstreamConnection : Connection
	WorkerCount : number
	Listener : Terminal
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
	local id = ToolkitWin32.CreateProcess(hostPath .. " /service \"--IScript=#!Script/Service/Worker " .. tostring(mainThreadID) .. " " .. tostring(self.Port) .. "\"", "Show", "") as { number }
	local processHandle = id[1]
	local threadID = id[2]
	if processHandle ~= 0 then
		print("Spawn a worker with id: " .. threadID)
		self.Workers[threadID] = { Handle = processHandle, EchoTimeStamp = self.Now, Port = 0 } as WorkerItem
	else
		return nil as WorkerItem
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
			self.Workers[threadID] = nil as WorkerItem -- it's safe in lua
		else
			currentWorkerCount = currentWorkerCount + 1
		end
	end

	for i = currentWorkerCount, self.WorkerCount - 1 do
		self:SpawnWorker()
	end

	-- Process cluster
	for address, clusterItem in pairs(self.Clusters) do
		if clusterItem.EchoTimeStamp + Common.TIME_OUT_CLUSTER < now then
			print("Cluster \"" .. clusterItem.Name .. "\" (" .. address .. ") was not able to be connected for " .. Common.TIME_OUT_CLUSTER .. " seconds. Kill it at once!")
			self.Clusters[address] = nil as ClusterItem -- kick off
		end
	end

	-- Process cluster upstream
	if self.UpstreamClusterAddress then
		if not self.UpstreamConnection then
			self.UpstreamConnection = self.Listener:Connect(self.UpstreamClusterAddress)
		end

		if self.UpstreamConnection then
			local live = {
				Action = "ClusterTick",
				ClusterItem = {
					WorkerCount = self.WorkerCount,
					Name = systemInfo.ComputerName,
					Processor = systemInfo.ProcessorName,
					CoreCount = systemInfo.NumberOfProcessors,
				} as ClusterItem
			}

			self.UpstreamConnection:WriteConnection(Core.Encode(live as { any : any }))
		end
	end
end

function Master:NetworkData(connection : Connection, data : string)
	print("Master receive request length " .. tostring(#data))
	-- parse data
	local pack = Core.Decode(data) as { string : any}
	-- write result

	-- dispatch request
	local action = pack.Action as string
	if action == "Query" then
		connection:WriteConnection(Core.Encode(self.Workers as { any : any }))
	elseif action == "ClusterTick" then
		local clusterItem = pack.ClusterItem as ClusterItem
		local address = (connection:GetConnectionAddresses() as { string : string}).Destination
		if address then
			clusterItem.EchoTimeStamp = self.Now
			self.Clusters[address] = clusterItem
		end
	elseif action == "ClusterQuery" then
		connection:WriteConnection(Core.Encode(self.Clusters as { any : any }))
	elseif action == "Info" then
		connection:WriteConnection(Core.Encode(systemInfo as { any : any }))
	end
end

function Master:NetworkState(connection : Connection, state : boolean, src : string, dst : string) : any
	-- stateless, just print state out
	if state then
		print("Activate connection from " .. src)
	else
		if connection == self.UpstreamConnection then
			self.UpstreamConnection = nil as Connection
		end

		print("Deactivate connection from " .. src)
	end

	return true
end

function Master.New(workerCount : number, upstreamClusterAddress : string) : Master
	local master = {
		WorkerCount = workerCount,
		UpstreamClusterAddress = upstreamClusterAddress,
		Workers = {},
		Clusters = {}
	}

	return setmetatable(master, Master as METATABLE)
end

function Master.Main(workerCount : number, listenAddress : string, upstreamClusterAddress : string)
	print("LeavesWind Master")
	print("System information:")
	for k, v in pairs(systemInfo) do
		print(k .. ": " .. tostring(v))
	end

	local oldprint = print
	_G.print = function (...)
		oldprint("[Master " .. Common.MainThreadID .. "] " .. tostring(table.concat({ ... } as { string }, " ")))
	end

	global MasterInstance = Master.New(workerCount or 1, upstreamClusterAddress)
	ToolkitWin32.ListenMessage(function (message : number, wparam : number, lparam : number)
		Common.DispatchMessage(MasterInstance as Common, message, wparam, lparam)
	end)

	Common.InitializeListener(MasterInstance as Common, listenAddress or "127.0.0.1:29716")
	Common.InitializeTicker(MasterInstance as Common)
end

return Master