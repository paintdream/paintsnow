-- DataLive.lua
-- Create data live server
-- TODO: transport to typedlua.

local DataLive = { __index = _ENV }
local Core = require("Engine/Core")
local Terminal = require("Engine/Network/Terminal")

local function DirectContent(mime)
	return function (file, request)
		request.OutputHeader["Content-Type"] = mime
		request.OutputData = file
	end
end

local function ExecuteScript(defContent, request, filePath : string)
	-- trying to read script file
	local path = filePath:match(".*%.")
	if path then
		path = path .. ".lua"
		local file = SnowyStream.NewFile(path, false)
		if file then
			local size = SnowyStream.GetFileSize(file)
			local content = SnowyStream.ReadFile(file, size)
			SnowyStream.CloseFile(file)

			if content then
				local result = ""
				local inst = Core.Invoke(function ()
					local code : any = (load(content, "lua", "t", _ENV))
					local status, msg = pcall(code, defContent)
					if not status then
						print("Error: " .. msg)
					else
						result = msg
					end
				end)

				return result
			end
		end
	end

	return defContent

--[[
	local env : { any : any } = {}
	env.__index = _ENV
	env.Request = request,

	setmetatable(env, env)
	local buffer : { string } = {}
	env.print = function (x)
		table.insert(buffer, x)
	end

	local output = string.gsub(lua, "%<%?lua([^%?]-[^%>]-)%?%>", function (s, ...) : string
		local inst = Core.Invoke(function ()
			local code : any = (load(s, "lua", "t", env))
			local status, err = pcall(code)
			if not status then
				print("Error: " .. err)
			end
		end)
		return table.concat(buffer)
	end)

	if output then
		request.OutputData = request.OutputData .. output
	end
	]]
end

local translators : { string : any } = {
	["json"] = ExecuteScript,
	["html"] = DirectContent("text/html"),
	["js"] = DirectContent("text/javascript"),
	["css"] = DirectContent("text/css"),
	["ico"] = DirectContent("image/x-icon"),
	["png"] = DirectContent("image/png"),
	["jpg"] = DirectContent("image/jpeg"),
	["gif"] = DirectContent("image/gif"),
	["bmp"] = DirectContent("image/bmp"),
	["mp3"] = DirectContent("audio/mpeg"),
	["ogg"] = DirectContent("audio/ogg"),
	["mid"] = DirectContent("audio/mid"),
	["tif"] = DirectContent("image/tiff"),
	["tiff"] = DirectContent("image/tiff"),
	["eot"] = DirectContent("application/vnd.ms-fontobject"),
	["svg"] = DirectContent("image/svg+xml"),
	["ttf"] = DirectContent("application/x-font-truetype"),
	["woff"] = DirectContent("application/x-font-woff"),
	["woff2"] = DirectContent("application/x-font-woff2"),
}

function DataLive.New(ip, directory)
	return Terminal.New(ip, function (terminal : any, uri : string, method : string, header : string, data : string)
		if string.sub(uri, -1) == "/" then
			uri = uri .. "index.html"
		end

		-- find ext
		local ext = string.find
		local start : number = 1
		local last = start
		while start do
			last = start	
			start = (string.find(uri, ".", start, true)) or 1
			if start then
				start = start + 1
			end
		end

		local handler = translators[string.sub(uri, last)]
		
		-- parse selected lua
		print("New request [" .. method .. "] = " .. uri)
		if handler then
			local filePath = directory .. uri
			local file = SnowyStream.NewFile(filePath, false)
			if file then
				local length = SnowyStream.GetFileSize(file)
				local content = SnowyStream.ReadFile(file, length)
				if content then
					local request = {
						Code = 200,
						Method = method,
						InputHeader = header,
						InputData = data,
						OutputHeader = {},
						OutputData = "",
						Reason = "OK"
					}

					handler(content, request, filePath)
					return request.Code, request.OutputData, request.Reason, request.OutputHeader
				end
			else
				print("File not found: " .. filePath)
			end
		end

		return 404, "Page not found"
	end, function (terminal : any, isConnect : boolean, src : string, dst : string)
		return true
	end, true)
end

return DataLive
