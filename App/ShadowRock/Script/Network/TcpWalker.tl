local TcpWalker = { __index = _ENV }
setmetatable(TcpWalker, TcpWalker)

--[[
local function Write(walker, connection, options)
	options.action = "tcpwalker_request"
	local url = EchoLegend.MakeURL("", "", walker.Port, walker.Path, options)
	-- print(url)
	EchoLegend.WriteConnectionHttpRequest(connection, url, "GET", { cookie = walker.cookie })
end

local function Read(walker, connection, instance)
	EchoLegend.ReadConnectionHttpRequest(connection, function (uri, method, header, data)
		local object
		if string.len(data) < 10 then -- wp auth failed!
			object = { Status = "ERROR", Info = "Expired" }
		else
			object = Json.Parse(data)
		end

		-- print("OBJECT " .. object)
		Core.Post(instance, object)
	end)
end

local function Request(walker, options)
	local instance = Core.Instance()
	local connection = EchoLegend.OpenConnection(walker.Host, walker.Port, true, function (connection, status)
		-- print("Http code: " .. status)
		if status == "Connected" then
			Write(walker, connection, options)
		elseif type(status) == "number" and status >= 200 and status <= 499 then
			Read(walker, connection, instance)
		end
	end)

	EchoLegend.ActivateConnection(connection)
	return Core.StepListen()
end

function Create(login, url, id, password)
	local part = EchoLegend.ParseURL(url)
	local lg = EchoLegend.ParseURL(login)
	return { LoginPath = lg.Path, Host = part.Host, Port = part.Port, path = part.Path, Id = id, Password = password, OnlineUsers = {} }
end

function Enumerate(walker)
	-- print("Enumerate Tcpwalker users ...")
	local object = Request(walker, { Action = "Enumerate" })
	-- print("Reading result...")
	if object and object.Status ~= "ERROR" and object.data and type(object.data) == "table" then
		-- scan for active connections
		local onlineUsers = {}
		-- print("Got data" .. #object.data)
		for k, v in pairs(object.data) do
			local user = {
				Id = v["0"],
				IP = v["1"],
				Port = v["2"],
				Target = v["3"],
			}

			table.insert(onlineUsers, user)
		end

		walker.OnlineUsers = onlineUsers
	end
end

function Connect(walker, id)
	-- Core.Invoke(function ()
		return Request(walker, { Action = "Connect", Target = id })
	-- end)
end

function Disconnect(walker, id)
	-- Must wrap request call in a new coroutine
	-- Otherwise normal tcp sockets will not be able to send any packet from now on Windows System.
	-- I don't know if it is a bug in libevent2 or lua....
	-- So just wrap it.
	Core.Invoke(function ()
 		Request(walker, { Action = "Disconnect" })
	end)
end

function Login(walker)
	local instance = Core.Instance()

	-- only 'Content-Type' and 'Connection' are required for php server
	local headers = {
		["Content-Type"] = "application/x-www-form-urlencoded; charset=UTF-8",
		["Connection"] = "keep-alive",
		-- ["User-Agent"] = "curl/7.35.0",
		["Host"] = walker.host,
		-- ["Accept"] = "*/*",
		-- ["Content-Length"] = 24,
	}
	
	local login = false

	local connection = EchoLegend.OpenConnection(walker.Host, walker.Port, true, function (connection, status)
		if status == "Connected" then
			EchoLegend.GetConnectionInfo(connection, function (srcIP, srcPort, dstIP, dstPort)
				-- print(">>>> PORT " .. srcPort)
				print("LOGIN Path: " .. walker.LoginPath)
				local data = "log=" .. walker.Id .. "&pwd=" .. walker.Password
				EchoLegend.WriteConnectionHttpRequest(connection, walker.LoginPath, "POST", headers, data)
			end)
		elseif type(status) == "number" and status >= 200 and status <= 499 then
			if not login then
				EchoLegend.ReadConnectionHttpRequest(connection, function (uri, method, header, data)
					print("RESDATA " .. data)
					local cookie = header["Set-Cookie"]
					if cookie then
						local c
						if type(cookie) == "table" then
							for k, v in ipairs(cookie) do
								if string.find(v, '/wp-admin', 1, true) then
									c = string.sub(v, 1, string.find(v, ";"))
									break
								end
							end
						else
							c = string.sub(cookie, 1, string.find(cookie, ";"))
						end
							
							
						local options = {}
						if not string.find(c, "WP+Cookie+check", 1, true) then
							walker.cookie = c
							Write(walker, connection, { Action = "Login" })
							login = true
						else
							Core.Post(instance)
							print("TcpWalker connection established")
						end
					end
				end)
			else
				EchoLegend.GetConnectionInfo(connection, function (srcIP, srcPort, dstIP, dstPort)
					walker.bindHost = srcIP
					walker.bindPort = srcPort
				end)

				print("Login finished")
				Read(walker, connection, instance)
			end
		end
	end)

	EchoLegend.ActivateConnection(connection)
	Core.StepListen()

	print("Connection result")
	return login
end
]]

return TcpWalker
