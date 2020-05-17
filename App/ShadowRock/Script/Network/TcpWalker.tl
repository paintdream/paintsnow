local TcpWalker = { __index = _ENV }
setmetatable(TcpWalker, TcpWalker)

local Json = require("Engine/Network/Json")
local Core = require("Engine/Core")

interface User
	Id : string
	Host : string
	Target : string
end

local dispatcher : WorkDispatcher = Core.Any()

TcpWalker.Dispatcher = dispatcher
TcpWalker.LoginPath = ""
TcpWalker.Host = ""
TcpWalker.Path = ""
TcpWalker.Id = ""
TcpWalker.Password = ""
TcpWalker.Cookie = ""
TcpWalker.Loggedin = false

local initUsers : { User } = {}
TcpWalker.OnlineUsers = initUsers

function TcpWalker:Write(connection : Connection, options)
	table.insert(options, { "action", "tcpwalker_request" })
	local url = EchoLegend.MakeURL(self.Id, self.Host, self.Path, options, "")
	-- print(url)
	connection:WriteConnectionHttpRequest(url, "GET", { { "cookie", self.Cookie } }, "")
end

function TcpWalker:Read(connection : Connection, instance)
	connection:ReadConnectionHttpRequest(function (uri : string, method : string, header : string, data : string)
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

function TcpWalker:Request(options : { any })
	local instance = Core.Instance()
	local connection = self.Dispatcher:OpenConnection(self.Host, true, function (connection : Connection, status : any)
		-- print("Http code: " .. status)
		if status == "Connected" then
			self:Write(connection, options)
		elseif type(status) == "number" and status >= 200 and status <= 499 then
			self:Read(connection, instance)
		end
	end, false)

	connection:ActivateConnection()
	return Core.StepListen()
end

function TcpWalker:Enumerate()
	-- print("Enumerate Tcpwalker users ...")
	local object = self:Request({ { "Action", "Enumerate" } })
	-- print("Reading result...")
	if object and object.Status ~= "ERROR" and object.data and type(object.data) == "table" then
		-- scan for active connections
		local onlineUsers : { User } = {}
		-- print("Got data" .. #object.data)
		for k, v in pairs(object.data) do
			local user : User = {
				Id = v["0"],
				Host = v["1"] .. ":" .. v["2"],
				Target = v["3"],
			}

			table.insert(onlineUsers, user)
		end

		self.OnlineUsers = onlineUsers
	end
end

function TcpWalker:Connect(id : string)
	return self:Request({ { "Action", "Connect" }, { "Target", id } })
end

function TcpWalker:Disconnect(id : string)
 	return self:Request({ { "Action", "Disconnect" }, { "Target", id } })
end

function TcpWalker:LoginCallback(instance, connection : Connection, uri : string, method : string, header : { any }, data : string)
	for _, item in ipairs(header) do
		local key : string = item[1]
		local cookie : string = item[2]
		if key == "Set-Cookie" then
			local c = string.sub(cookie, 1, string.find(cookie, ";"))

			if not string.find(c, "WP+Cookie+check", 1, true) then
				self:Write(connection, { { "Action", "Login" } })
				self.Cookie = c
				self.Loggedin = true
			else
				Core.Post(instance)
				print("TcpWalker connection established")
			end
		end
	end
end

function TcpWalker:OnConnected(connection : Connection)
	connection:GetConnectionInfo(function (srcIP : string, srcPort : integer, dstIP : string, dstPort : integer)
		-- print(">>>> PORT " .. srcPort)
		print("LOGIN Path: " .. self.LoginPath)
		local data = "log=" .. self.Id .. "&pwd=" .. self.Password
		local headers : { number : any } = {
			{ "Content-Type", "application/x-www-form-urlencoded; charset=UTF-8" },
			{ "Connection", "keep-alive" },
			{ "Host", self.Host },
		}

		connection:WriteConnectionHttpRequest(self.LoginPath, "POST", headers, data)
	end)
end

function TcpWalker:Login()
	local instance = Core.Instance()

	-- only 'Content-Type' and 'Connection' are required for php server
	self.Loggedin = false
	local connection : Connection = self.Dispatcher:OpenConnection(self.Host, true, function (connection : Connection, status : any)
		if status == "Connected" then
			self:OnConnected(connection)
		elseif type(status) == "number" and status >= 200 and status <= 499 then
			if not self.Loggedin then
				connection:ReadConnectionHttpRequest(function (uri : string, method : string, header : { any }, data : string)
					local a : any = self
					a:LoginCallback(instance, connection, uri, method, header, data)
				end)
			else
				self:Read(connection, instance)
			end
		end
	end, false)

	connection:ActivateConnection()
	Core.StepListen()

	return self.Loggedin
end

function TcpWalker.New(dispatcher : WorkDispatcher, login : string, url : string, id, password)
	local part = EchoLegend.ParseURL(url)
	local lg = EchoLegend.ParseURL(login)
	return setmetatable({
		WorkDispatcher = dispatcher,
		LoginPath = lg.Path,
		Host = part.Host,
		Path = part.Path,
		Id = id,
		Password = password,
		OnlineUsers = {},
		Loggedin = false
	}, { __index = TcpWalker })
end

return TcpWalker
