--[[
	Terminal.tl: Simple Network Terminal Wrapper for EchoLegend APIs.
	Author: PaintDream (paintdream@paintdream.com)
]]
local Core = require("Engine/Core")

local ConnectionRecord = record
	Handle : any
	Src : string
	Dst : string
end

local Terminal = record end
local Peers : { any : ConnectionRecord } = {}

-- uniform global dispatcher
local dispatcher : WorkDispatcher = EchoLegend.OpenDispatcher()
-- currently we do not deactivate it manually, just let gc do it.
dispatcher.ActivateDispatcher()

Terminal.Peers = Peers

function Terminal.StateHandler(connection : Connection, isConnect : boolean, src : string, dst : string)
	return true
end

function Terminal.DataHandler(connection : Connection, data : string, method : string, header : string, extraData : string)
	return true
end

function Terminal:Write(connection : Connection, data : string)
	return connection:WriteConnection(data)
end

function Terminal:Read(connection : Connection, data : string)
	return connection:ReadConnection(data)
end

-- Broadcast data to all connected terminals.
function Terminal:Broadcast(data : string)
	for k in pairs(self.Peers) do
		self:Write(k, data)
	end
end

-- Add a connection terminal to current terminal
function Terminal:AddConnection(connection : Connection)
	connection:GetConnectionInfo(function (src : string, dst : string)
		self.Peers[connection] = {
			Handle = connection,
			Src = src,
			Dst = dst,
		}
	end)
end

-- Remove a connection terminal from current terminal
function Terminal:RemoveConnection(connection : Connection)
	if connection then
		self.Peers[connection] = nil
	end
end

function Terminal:Query()
	return self.Peers
end

-- Handle and dispatch http events, it's an internal function.
function Terminal:EventHandlerHttp(connection : Connection, status : string)
	if status == "Connected" then
		connection:GetConnectionInfo(function (src : string, dst : string)
			if self.StateHandler(connection, true, src, dst) then
				self:AddConnection(connection)
			end
		end)

		connection:ReadConnectionHttpRequest(function (uri : string, method : string, header : string, data : string)
			local code, data, reason, header = self.DataHandler(connection, uri, method, header, data)
			if code and data and reason and header then
				connection:WriteConnectionHttpResponse(code, data, reason, header)
			end
		end)

		return true
	elseif status == "Read" then
		connection:ReadConnectionHttpRequest(function (uri : string, method : string, header : string, data : string)
			local code, data, reason = self.DataHandler(connection, uri, method, header, data)
			if code and data and reason then
				connection:WriteConnectionHttpResponse(code, data, reason)
			end
		end)

		return true
	elseif status == "Close" or status == "Error" then
		self.StateHandler(connection, false, "", "")
		self:RemoveConnection(connection)
		return false
	end
end

function Terminal:EventHandler(connection : Connection, status : string)
	if status == "Connected" then
		connection:GetConnectionInfo(function (src : string, dst : string)
			if self.StateHandler(connection, true, src, dst) then
				self:AddConnection(connection)
			end
		end)

		return true
	elseif status == "Read" then
		connection:ReadConnection(function (data : string)
			self.DataHandler(connection, data)
		end)

		return true
	elseif status == "Close" or status == "Error" then
		self.StateHandler(connection, false, "", "")
		self:RemoveConnection(connection)
		return false
	end
end

function Terminal:HttpRequest(host : string, path : string, method : string, options : { string : string }, header : { string : string }, callback : function (string) : any )
	local connection : Connection = dispatcher:OpenConnection(host, true, function (connection : Connection, status : any)
		local statusCode = tonumber(status) or 0
		if status == "Connected" then
			local opts : { number : any } = {}
			for k, v in pairs(options) do
				table.insert(opts, { k, v })
			end

			local url = EchoLegend.MakeURL("", "", path, opts, "")
			
			local list : { number : any } = {}
			for k, v in pairs(header) do
				table.insert(list, { k, v })
			end
			connection:WriteConnectionHttpRequest(url, method, list, "")
		elseif type(status) == "number" and statusCode >= 200 and statusCode <= 499 then
			connection:ReadConnectionHttpRequest(function (uri : string, method : string, header : string, data : string)
				connection:DeactivateConnection()
				print("Data received")
				callback(data)
			end)
		end
	end, false)

	if connection then
		connection:ActivateConnection()
	end

	return connection
end

function Terminal:Connect(ip : string)
	local instance = Core.Instance()
	local connection : Connection = dispatcher:OpenConnection(ip, false, function (connection : Connection, status : string)
		local success = self:EventHandler(connection, status)
		if status == "Connected" then
			Core.Post(instance, success)
		end
	end, true)

	-- establishing
	connection:ActivateConnection()

	local success = Core.StepListen()
	if success then
		return connection
	else
		connection:DeactivateConnection()
		return nil
	end
end

function Terminal:Disconnect(connection : Connection)
	self.Peers[connection] = nil
	connection:DeactivateConnection()
end

function Terminal.New(ip : string, 
			dataHandler : any,
			stateHandler : function (any, boolean, string, string) : (boolean),
			http : boolean)
	local instance = Core.Instance()
	local eventHandler = http and Terminal.EventHandlerHttp or Terminal.EventHandler
	local terminal = setmetatable({ IP = ip, Peers = {}, DataHandler = dataHandler, StateHandler = stateHandler }, { __index = Terminal })

	local listener : Listener = dispatcher:OpenListener(ip, http, function (listener, event)
		if event == "Connected" then
			listener:GetListenerInfo(function (ip)
				print("Listener created on IP: " .. ip)
			end)
			Core.Post(instance)
		end
	end, function (listener : Listener, connection : Connection)
		-- do not add connection here
		-- wait for actual event
		-- EchoLegend.WriteConnectionRecord(connection, "Hello")
		-- print("NEW CONNECTION")
		eventHandler(terminal, connection, "Connected")
	end, function (connection, status)
		--print("SERVER GOT" .. status)
		eventHandler(terminal, connection, status)
	end, true)

	if listener then
		listener:ActivateListener()
		print("Listener activated")
		return terminal
	else
		print("Create listener failed!")
		return nil
	end
end

return Terminal