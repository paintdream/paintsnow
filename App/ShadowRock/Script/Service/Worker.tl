local Common = require("Script/Service/Common")
local Terminal = require("Engine/Controller/Terminal")
local Core = require("Engine/Core")
local Messages = Common.Messages

local Worker = record
	MasterThreadID : number
	Now : number
	ResponseTimeStamp : number
	Listener : Terminal
	Port : number
	MasterPort : number
	__index : any
end

Worker.__index = Worker
local oldprint = print

function Worker:WM_ECHO_RESPONSE(wparam, lparam)
	print("ECHO CONFIRMED: " .. tostring(self.Now))
	self.ResponseTimeStamp = self.Now
end

function Worker:Tick(now : number)
	-- Send heart beat
	print("Time: " .. tostring(self.ResponseTimeStamp))
	self.Now = now
	if self.ResponseTimeStamp + Common.TIME_OUT < now then
		print("Cannot connect to master, exit now.")
		ToolkitWin32.PostThreadMessage(Common.MainThreadID, Messages.WM_QUIT, 0, 0)
	else
		ToolkitWin32.PostThreadMessage(self.MasterThreadID, Messages.WM_ECHO, Common.MainThreadID, self.Port)
	end
end

function Worker:NetworkData(connection : Connection, data : string)
	print("Worker receive request length " .. tostring(#data))
	-- parse data
	local pack = Core.Decode(data, nil as { any }) as { string : any }
	-- write result

	-- dispatch request
	local action = pack.Action as string
	if action == "Execute" then
		local code = pack.Code as string
		local func, err = load(code, pack.Name as string)
		local ret : { string : string } = {}
		if not func then
			ret.Message = "Compile error: " .. err
		else
			local s, msg = pcall(function () : string
				return func(connection) as string
			end)

			if not s then
				ret.Message = "Run error: " .. msg
			else
				ret.Message = ""
			end
		end

		connection:WriteConnection(Core.Encode(ret as { any : any }, nil as { any : any }))
	elseif action == "Upload" then
		local ret : { string : string } = {}
		local path = pack.Path as string
		if path:sub(1, 5) == "Temp/" then -- can only create file in this folder
			local file = SnowyStream.NewFile(path, true)
			SnowyStream.WriteFile(file, pack.Content as string, nil)
			SnowyStream.CloseFile(file)
			ret.Message = ""
		else
			ret.Message = "Cannot create file " .. path
		end

		connection:WriteConnection(Core.Encode(ret as { any : any }, nil as { any : any }))
	elseif action == "Download" then
		local content = SnowyStream.FetchFileData(pack.Path as string)
		local ret : { string : string } = {}
		ret.Message = ""
		ret.Content = content as string
		connection:WriteConnection(Core.Encode(ret as { any : any }, nil as { any : any }))
	elseif action == "Delete" then
		SnowyStream.DeleteFile(pack.Path as string)
	elseif action == "Query" then
		-- query file/folder information
		local path = pack.Path as string
		local ret : { string : any } = {}
		ret.Message = ""

		if path:sub(-1) == "/" then -- folder
			local files = SnowyStream.QueryFiles(path)
			ret.Files = files
		else
			local source = SnowyStream.NewFile(path, false)
			if source then
				local size = SnowyStream.GetFileSize(source)
				local time = SnowyStream.GetFileLastModifiedTime(source)
				ret.Size = size
				ret.ModifiedTime = time
			else
				ret.Message = "File not exist"
			end
		end

		SnowyStream.DeleteFile(pack.Path as string)
	end
end

function Worker:NetworkState(connection : Connection, state : boolean, src : string, dst : string) : any
	return true
end

function Worker.New(masterThreadID : number, masterPort : number) : Worker
	local worker = {
		MasterThreadID = masterThreadID,
		Now = 0,
		ResponseTimeStamp = 0,
		MasterPort = masterPort
	}

	return setmetatable(worker, Worker as METATABLE)
end

function Worker.Main(masterThreadID : string, masterPort : string)
	print("LeavesWind Worker at " .. Common.MainThreadID)
	_G.print = function (...)
		oldprint("[Worker " .. Common.MainThreadID .. "] " .. tostring(table.concat({ ... } as { string }, " ")))
	end

	global WorkerInstance = Worker.New(tonumber(masterThreadID), tonumber(masterPort))
	ToolkitWin32.ListenMessage(function (message : number, wparam : number, lparam : number)
		Common.DispatchMessage(WorkerInstance as Common, message, wparam, lparam)
	end)

	Common.InitializeListener(WorkerInstance as Common, "127.0.0.1:0")
	Common.InitializeTicker(WorkerInstance as Common)
end

return Worker