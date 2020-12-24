local Terminal = require("Engine/Controller/Terminal")
local Storage = record
	ping : number
end

local HostRouter = record
	terminal : Terminal
	__index : any
end

HostRouter.__index = HostRouter

function HostRouter.New(dispatcher : WorkDispatcher, ip : string) : HostRouter
	return setmetatable({
		terminal = Terminal.New(dispatcher, ip, function () end,
		function (connection : Connection, state : boolean, src : string, dst : string) : any
			return { ping = 0 }
		end)
	}, HostRouter as METATABLE)
end

return HostRouter