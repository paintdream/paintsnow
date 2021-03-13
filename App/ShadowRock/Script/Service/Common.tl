local Terminal = require("Engine/Controller/Terminal")

local Common = record
	TIME_OUT : number
	TIME_OUT_CLUSTER : number
	TIMER_INTERVAL : number
	Messages : { string : number }
	MessageMap : { number : string }
	MainThreadID : number
	Ticker : Entity
	Listener : Terminal
	Dispatcher : WorkDispatcher
	Port : number
	Script : ScriptComponent
	__index : any
end

Common.__index = Common
Common.TIME_OUT = 16 -- timed out
Common.TIME_OUT_CLUSTER = 16 -- timed out
Common.TIMER_INTERVAL = 3
Common.MainThreadID = ToolkitWin32.GetMainThreadID()

local Messages = {
	WM_QUIT				= 0x12,
	WM_USER				= 0x400,
	WM_ECHO				= 0x401,
	WM_ECHO_RESPONSE	= 0x402
}

local MessageMap : { number : string } = {}

for k, v in pairs(Messages) do
	MessageMap[v] = k
end

Common.Messages = Messages
Common.MessageMap = MessageMap

-- Helper functions
function Common:Tick(timestamp : number)
	assert(false, "Must overwrite this function!")
end

function Common:NetworkData(connection : Connection, data : string)
	assert(false, "Must overwrite this function!")
end

function Common:NetworkState(connection : Connection, state : boolean, src : string, dst : string) : any
	assert(false, "Must overwrite this function!")
end

function Common:InitializeListener(ip : string)
	if not self.Dispatcher then
		local dispatcher = EchoLegend.OpenDispatcher()
		dispatcher:ActivateDispatcher()

		local listener = Terminal.New(dispatcher, ip or "127.0.0.1:0", function (connection : Connection, data : string)
			self:NetworkData(connection, data)	
		end, function (connection : Connection, state : boolean, src : string, dst : string) : any
			return self:NetworkState(connection, state, src, dst)
		end)

		self.Listener = listener
		self.Dispatcher = dispatcher
		self.Port = tonumber(listener.Address:match(".+:(.+)"))
	end
end

function Common:InitializeTicker()
	if not self.Ticker then
		local entity = MythForest.NewEntity(-1)
		self.Ticker = entity

		local clock = HeartVioliner.NewClock(Common.TIMER_INTERVAL * 1000.0)
		local eventComponent : EventComponent = EventComponentModule.New()
		eventComponent:BindEventTick(clock)

		local scriptComponent = ScriptComponentModule.New("TickerComponent")
		scriptComponent:SetHandler("EVENT_TICK", function (eventData)
			self:Tick(clock:Now() / 1000.0)
		end)

		self.Script = scriptComponent
		entity:AddEntityComponent(eventComponent as Component)
		entity:AddEntityComponent(scriptComponent as Component)
	end
end

function Common:DispatchMessage(message : number, wparam : number, lparam : number)
	local lookup = self as { string : function (Common, number, number) }
	local name = MessageMap[message]
	if name then
		local handler = lookup[name]
		if handler then
			handler(self, wparam, lparam)
		else
			print("Error: message '" .. name .. "' not found!")
		end
	else
		print("Error: message " .. tostring(message) .. " not registerred!")
	end
end

return Common