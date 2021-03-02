local Master = record
	Workers : { number : number }
	StartPort : number
	WorkerCount : number
end

function Master:OnTimer()
	-- send heart beat packet
end

function Master:HandlerEcho() : { number }
	local ports : { number } = {}
	for k in pairs(self.Workers) do
		table.insert(ports, k)
	end

	return ports
end

function Master:SpawnWorker(port : number)

end

function Master:Start()
	for i = 1, self.WorkerCount do
		self:SpawnWorker(self.StartPort + i)
	end
end

function Master.New(workerCount : number, port : number) : Master
	local master = {
		WorkerCount = workerCount,
		StartPort = port,
		Workers = {}
	}

	return setmetatable(master, Master as METATABLE)
end

function Master.Main(workerCount : number, port : number)
	workerCount = workerCount or 2
	port = port or 29716	

	global MasterInstance = Master.New(workerCount, port)
	MasterInstance:Start()
end

return Master