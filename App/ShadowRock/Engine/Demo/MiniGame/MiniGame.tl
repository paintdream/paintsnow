-- MiniGame.lua
-- A console game using Core.lua Library
--

local Core = require("Engine/Core")

local function StdInput(instance)
	while true do
		Core.Post(instance, io.read())
	end
end

-- placeholder for typedlua
function NextRoom(s) end

-- server-end state machine
function FirstRoom(writerForServer)
	Core.Post(writerForServer, "First Room!")
	for message in Core.Listen() do
		if message then
			if message == "Move" then
				return NextRoom(writerForServer)
			else
				Core.Post(writerForServer, "First Room _>  " .. message)
			end 
		end
	end
end

function NextRoom(writerForServer)
	Core.Post(writerForServer, "Next Room!")
	for message in Core.Listen() do
		if message then
			if message == "Move" then
				return FirstRoom(writerForServer)
			else
				Core.Post(writerForServer, "Next Room _>  " .. message)
			end
		end
	end
end

local function PlayerRoutine(readerForClient, writerForClient, name)
	print("Hi, " .. name .. " here.")
	for command in Core.Listen(readerForClient) do
		if command then
			print(name .. " receive message " .. command)
			if command == name then
				Core.Post(writerForClient, name .. ": ARE YOU CALLING ME?")	
			end
		end
	end
end

local function NewLocalPipe()
	local reader = {}
	local virtualNetwork = nil
	local writer = Core.Invoke(function ()
		repeat
			local args = table.pack(Core.StepListen())
			Core.Post(virtualNetwork, Core.Encode(args))
		until virtualNetwork == nil
	end)
	
	virtualNetwork = Core.Invoke(function ()
		for command in Core.Listen(writer) do
			-- the 2nd return value 'unread' from Core.Decode omitted
			local data = Core.Decode(command)
			if data then
				Core.Post(reader, table.unpack(data))
			end
		end
	end)
	return reader, writer
end

local function InputEventLoop(readerForClient, writerForClient, readerForServer, writerForServer)
	for command in Core.Request(StdInput) do
		if command then
			if command == "Quit" then
				Core.Post(readerForServer, "Quit")
				break
			elseif command == "Tick" then
				Core.Post(readerForClient, "Intel tick")
			elseif command == "Tock" then
				Core.Post(writerForClient, "Nvidia tock")
			else
				Core.Post(writerForServer, command)
			end
		end
	end
end

local function Server(readerForServer, writerForServer)
	local room = Core.Invoke(FirstRoom, writerForServer)
	for command in Core.Listen(readerForServer) do
		print("[[ Server Begin Forwarding ]]")
		Core.Post(room, command)
		print("[[ Server End Forwarding ]]")
	end
end

local function Main()
	local readerForClient, writerForServer = NewLocalPipe()
	local readerForServer, writerForClient = NewLocalPipe()
	local playerIntel = Core.Invoke(PlayerRoutine, readerForClient, writerForClient, "Intel")
	local playerNvidia = Core.Invoke(PlayerRoutine, readerForClient, writerForClient, "Nvidia")
	local server = Core.Invoke(Server, readerForServer, writerForServer)

	return InputEventLoop(readerForClient, writerForClient, readerForServer, writerForServer)
end

Core.Invoke(Main)
