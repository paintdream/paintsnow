local Terminal = require("Engine/Controller/Terminal")
local Core = require("Engine/Core")
local Storage = record
	ping : number
end

local HostRouter = record
	terminal : Terminal
	connection : Connection
	__index : any
end

HostRouter.__index = HostRouter

function HostRouter:Query()
	-- send query command
	assert(self.connection)

	self.terminal:Write(self.connection, Core.Encode({ Action = "Query" } as { any : any }))
	-- TODO: response
end

function HostRouter:Connect(ip : string) : Connection
	local connection = self.terminal:Connect(ip)
	if connection then
		if self.connection then
			-- disconnect previous one
			self.terminal:Disconnect(self.connection)
			self.connection = connection
		end
	end

	return connection
end

function HostRouter.New(dispatcher : WorkDispatcher, ip : string) : HostRouter
	return setmetatable({
		terminal = Terminal.New(dispatcher, ip, function () end,
		function (connection : Connection, state : boolean, src : string, dst : string) : any
			return { ping = 0 }
		end)
	}, HostRouter as METATABLE)
end

return HostRouter