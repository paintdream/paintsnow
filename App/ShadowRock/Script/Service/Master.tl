local Terminal = require("Engine/Controller/Terminal")
local WorkerItem = record
	terminal : Terminal
	handle : number
	mainThread : number
end

local Master = record
	Workers : { number : WorkerItem }
	WorkerCount : number
	__index : any
end

Master.__index = Master

local WM_USER = 0x400
local Message : { string : number } = {
	MSG_ECHO = WM_USER + 1
}

local MessageMap : { number : string } = {}

for k, v in pairs(Message) do
	MessageMap[v] = k
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

function Master:SpawnWorker() : WorkerItem
	ToolkitWin32.CreateProcess("notepad.exe", "", "Show")
end

function Master:Start()
	for i = 1, self.WorkerCount do
		table.insert(self.Workers, self:SpawnWorker())
	end
end

function Master.New(workerCount : number) : Master
	local master = {
		WorkerCount = workerCount,
		Workers = {}
	}

	return setmetatable(master, Master as METATABLE)
end

function Master:HandleMessage(message : number, wparam : number, lparam : number)
	local lookup = self as { string : function (Master, number, number) }
	local name = MessageMap[message]
	if name then
		local handler = lookup[name]
		if handler then
			handler(self, wparam, lparam)
		end
	end
end

function Master.Main(workerCount : number)
	global MasterInstance = Master.New(workerCount or 1)
	ToolkitWin32.ListenMessage(function (message : number, wparam : number, lparam : number)
		MasterInstance:HandleMessage(message, wparam, lparam)
	end)

	MasterInstance:Start()
end

return Master