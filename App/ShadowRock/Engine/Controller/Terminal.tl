--[[
	Terminal.tl: Simple Network Terminal Wrapper for EchoLegend APIs.
	Author: PaintDream (paintdream@paintdream.com)
]]

local Core = require("Engine/Core")

local ConnectionRecord = record
	Source : string
	Destination : string
end

local HttpRequestRecord = record
	Uri : string
	Method : string
	Header : { { string } }
	Data : string
end

local Terminal = record
	Peers : { Connection : ConnectionRecord }
	__index : any
end

Terminal.__index = Terminal

-- uniform global dispatcher
local dispatcher : WorkDispatcher = EchoLegend.OpenDispatcher()
-- currently we do not deactivate it manually, just let gc do it.
dispatcher.ActivateDispatcher()

function Terminal.StateHandler(connection : Connection, isConnect : boolean, src : string, dst : string) : boolean
	return true
end

function Terminal.DataHandler(connection : Connection, data : string, method : string, header : { { string } }, extraData : string) : (number, string, string, { { string } }) 
	return 200, "", "", {}
end

function Terminal:Write(connection : Connection, data : string)
	connection:WriteConnection(data)
end

function Terminal:Read(connection : Connection) : string
	return connection:ReadConnection()
end

-- Broadcast data to all connected terminals.
function Terminal:Broadcast(data : string)
	for k in pairs(self.Peers) do
		self:Write(k, data)
	end
end

-- Add a connection terminal to current terminal
function Terminal:AddConnection(connection : Connection)
	self.Peers[connection] = connection:GetConnectionAddresses() as ConnectionRecord
end

-- Remove a connection terminal from current terminal
function Terminal:RemoveConnection(connection : Connection)
	if connection then
		self.Peers[connection] = nil
	end
end

-- Handle and dispatch http events, it's an internal function.
function Terminal:EventHandlerHttp(connection : Connection, status : string) : boolean
	if status == "Connected" then
		local info = connection:GetConnectionAddresses() as ConnectionRecord
		if self.StateHandler(connection, true, info.Source, info.Destination) then
			self:AddConnection(connection)
		end
	end

	if status == "Read" or status == "Connected" then
		local request = connection:ReadConnectionHttpRequest() as HttpRequestRecord
		local code, data, reason, header = self.DataHandler(connection, request.Uri, request.Method, request.Header, request.Data)
		connection:WriteConnectionHttpResponse(code, reason, header as {any}, data)
		return true
	elseif status == "Close" or status == "Error" then
		self.StateHandler(connection, false, "", "")
		self:RemoveConnection(connection)
		return false
	else
		return false
	end
end

function Terminal:EventHandler(connection : Connection, status : string) : boolean
	if status == "Connected" then
		local info = connection:GetConnectionAddresses() as ConnectionRecord
		if self.StateHandler(connection, true, info.Source, info.Destination) then
			self:AddConnection(connection)
		end

		return true
	elseif status == "Read" then
		self.DataHandler(connection, connection:ReadConnection(), nil, nil, nil)
		return true
	elseif status == "Close" or status == "Error" then
		self.StateHandler(connection, false, "", "")
		self:RemoveConnection(connection)
		return false
	end
end

function Terminal:HttpRequest(host : string, path : string, method : string,
	options : { string : string }, header : { string : string }, callback : function (string) : (any) ) : Connection
	local connection : Connection = dispatcher:OpenConnection(host, true, function (connection : Connection, status : any)
		local statusCode = tonumber(status) or 0
		if status == "Connected" then
			local opts : { any } = {}
			for k, v in pairs(options) do
				table.insert(opts, { k, v })
			end

			local url = EchoLegend.MakeUri("", "", path, opts, "")
			
			local list : { number : any } = {}
			for k, v in pairs(header) do
				table.insert(list, { k, v })
			end

			connection:WriteConnectionHttpRequest(url, method, list, "")
		elseif type(status) == "number" and statusCode >= 200 and statusCode <= 499 then
			local request = connection:ReadConnectionHttpRequest() as HttpRequestRecord
			callback(request.Data)
		end
	end, false)

	if connection then
		connection:ActivateConnection()
	end

	return connection
end

function Terminal:Connect(ip : string) : Connection
	local instance = coroutine.running()
	local connection : Connection = dispatcher:OpenConnection(ip, false, function (connection : Connection, status : string)
		local success = self:EventHandler(connection, status)
		if status == "Connected" then
			coroutine.resume(instance, success)
		end
	end, true)

	-- establishing
	connection:ActivateConnection()

	if coroutine.yield() then
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
			http : boolean) : Terminal
	local instance = coroutine.running()
	local eventHandler = http and Terminal.EventHandlerHttp or Terminal.EventHandler
	local terminal = setmetatable({ IP = ip, Peers = {}, DataHandler = dataHandler, StateHandler = stateHandler }, Terminal as METATABLE) as Terminal

	local listener : Listener = dispatcher:OpenListener(ip, http, function (listener : Listener, event : string)
		if event == "Connected" then
			print("Listener created on IP: " .. listener:GetListenerAddress())
			coroutine.resume(instance)
		end
	end, function (listener : Listener, connection : Connection)
		-- do not add connection here
		-- wait for actual event
		-- EchoLegend.WriteConnectionRecord(connection, "Hello")
		-- print("NEW CONNECTION")
		eventHandler(terminal, connection, "Connected")
	end, function (connection : Connection, status : string)
		--print("SERVER GOT" .. status)
		eventHandler(terminal, connection, status)
	end, true)

	if listener then
		listener:ActivateListener()
		print("Listener activated")

		coroutine.yield()
		return terminal
	else
		print("Create listener failed!")
		return nil
	end
end

return Terminal