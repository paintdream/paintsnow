-- Main.lua
-- Application Entry Module
local Main = {}
Debug = require("Script/Util/Debug")
local Core = require("Engine/Core")
local Database = require("Engine/Data/Database")
local Vector = require("Engine/Math/Vector")

local fpsBuffer : { integer } = {}
local tickBuffer : { integer } = {}
local bufferIndex = 1

local function SetFrameMonitorOnTitle(deltaTime : number, cameraComponent : CameraComponent, spaceComponent : SpaceComponent)
	-- show fps
	fpsBuffer[bufferIndex] = MythForest.GetFrameTickTime()
	tickBuffer[bufferIndex] = deltaTime

	bufferIndex = (bufferIndex + 1) % 8
	local sumFrame = 1
	local sumTick = 1

	for _, v in ipairs(fpsBuffer) do
		sumFrame = sumFrame + v
	end

	for _, v in ipairs(tickBuffer) do
		sumTick = sumTick + v
	end

	if bufferIndex % 4 == 0 then
		System.SetAppTitle("ShadowRock DEMO"
			.. " | FPS: " .. string.format("%3.2f", math.min(1000 * #fpsBuffer / sumFrame, 999.99))
			.. " | TPS: " .. string.format("%3.2f", math.min(1000 * #tickBuffer / sumTick, 999.99))
			.. " | Render (" .. tostring(cameraComponent:GetCollectedVisibleEntityCount()) .. "/" .. tostring(cameraComponent:GetCollectedEntityCount()) .. "/" .. tostring(spaceComponent:GetEntityCount()) .. ")")
	end
end

local function InitializeWorld(databasePath : string)
	local clock : Clock = HeartVioliner.NewClock(1000.0 / 80, 0) -- 80 FPS logic by default
	print("InitializeWorld: Creating ticker ...")
	local eventListenerComponent : EventListenerComponent = EventListenerComponentModule.New()
	eventListenerComponent:BindEventTick(clock)
	eventListenerComponent:BindEventFrame(true)
	eventListenerComponent:BindEventUserInput(true)

	print("InitializeWorld: Creating GUI ...")
	local GUI = require("Script/Root/GUI")
	local guiEntity = GUI.New()

	print("InitializeWorld: Loading repository " .. databasePath .. " ...")
	local database = Database.New(databasePath)

	print("InitializeWorld: Creating scene ...")
	local Scene = require("Script/Root/Scene")
	local scene = Scene.New(database, clock)

	print("\tCreating Player ...")
	local Player = require("Script/Prefabs/Entities/Player/StandardPlayer")
	local playerEntity = Player.New(eventListenerComponent)
	print("InitializeWorld: Mounting entities...")

	local spaceComponent = scene.spaceComponent
	spaceComponent:InsertEntity(playerEntity.player)
	-- spaceComponent:InsertEntity(guiEntity) -- use scene renderer to render gui at this moment

	local cameraController = playerEntity.cameraController
	local cameraComponent = cameraController.cameraComponent
	cameraComponent:BindRootEntity(scene.rootEntity)

	-- set camera transform
	local cameraTranslation : float3 = { -5, 5, 3 }
	local cameraRotation : float3 = { 0.25 * math.pi, 0, math.pi }
	local cameraController = playerEntity.cameraController
	local cameraTransform = cameraController.transformComponent
	cameraTransform:SetTranslation(cameraTranslation)
	cameraTransform:SetRotation(cameraRotation)
	cameraTransform:UpdateTransform()
	
	local screenSize : float2 = System.GetScreenSize()
	cameraController.aspect = screenSize[1] / screenSize[2]
	cameraController:UpdatePerspective()

	local lastMousePos = { 0, 0 }
	local moveSpeed = 1.0
	local inOperation = false
	scene.rootEntity:UpdateEntity()
	eventListenerComponent:SetEventHandler(function (eventType : integer, eventData)
		if eventType == "EVENT_TICK" then
			local deltaTime : number = eventData
			SetFrameMonitorOnTitle(deltaTime, cameraComponent, spaceComponent)
		elseif eventType == "EVENT_INPUT" then
			if inOperation then return end

			local evSize = eventData.Size
			if evSize then
				cameraController.aspect = evSize[1] / evSize[2]
				cameraController:UpdatePerspective()
			end

			inOperation = true
			local key = eventData.Keyboard
			if key then
				-- print("KEY PRESSED: " .. key.Name)
				if key.Down then
					local name : string = string.upper(key.Name)
					if name == "W" then
						cameraController:Walk(moveSpeed)
					elseif name == "S" then
						cameraController:Walk(-moveSpeed)
					elseif name == "A" then
						cameraController:Translate({ -moveSpeed, 0 })
					elseif name == "D" then
						cameraController:Translate({ moveSpeed, 0})
					end

					moveSpeed = math.min(moveSpeed + 0.1, 10.0)
				else
					moveSpeed = 1.0
				end
			end

			local mouse = eventData.Mouse
			if mouse then
				local size : float2 = System.GetScreenSize()
				local to = Vector.New(mouse.Position) / size * 2 - { 1, 1 }
				if mouse.Wheel then
					if mouse.Down then
						cameraController:Walk(moveSpeed)
					else
						cameraController:Walk(-moveSpeed)
					end
				elseif mouse.Down and mouse.Move then
					local from = Vector.New(lastMousePos) / size * 2 - { 1, 1 }

					if mouse.Button then
						-- Conflicts with LeavesWine.
						cameraController:Translate((from - to) * moveSpeed * 100)
					else
						cameraController:Rotate(from, to)
					end
				elseif mouse.Down then
					local position = cameraTransform:GetTranslation()
					local mat = cameraTransform:GetAxises()
					local target = (Vector.New(mat[3]) * (-1) + Vector.New(mat[1]) * to[1] * cameraController.aspect + Vector.New(mat[2]) * to[2]) * 100.0
					local results : { any }? = scene.rootEntity:Raycast(position, target, 1)
					if results then
						for i, result in ipairs(results) do
							local formComponent : FormComponent = result.Parent:GetUniqueEntityComponent("FormComponent")
							print("[" .. tostring(i) .. "] Mouse clicked at: " .. formComponent:GetName())
						end
					end
				end

				lastMousePos = mouse.Position
			end

			cameraController:Update()
			inOperation = false
		end
	end)

	print("InitializeWorld: World initialization finished. Waiting for system command")

	local instance = Core.Instance()
	System.ListenConsole(function (command)
		if command:sub(1, 1) == "@" then
			Core.Post(instance, command)
		else
			local func, message = load(command)
			if func then
				func()
			else
				print("Invalid command: " .. message)
			end
		end
	end)

	for command in Core.Listen() do
		if command == "@Quit" then
			print("Battle control terminated.")
			local n : any = nil
			eventListenerComponent:BindEventTick(n)
			eventListenerComponent:BindEventUserInput(false)
			eventListenerComponent:BindEventFrame(false)
			return System.Exit()
		end
	end
end

function Main.Main()
	print("Main initializing ...")
	print("Loading Repository Daemon ...")
	local databasePath = "Weaver/Repository.db"

	Debug.CompressResource = function ()
		local database : Database? = Database.New(databasePath)
		if database then
			local compressor = require("Script/Util/RepositoryCompressor")
			compressor.Compress(database)
		end
	end

	local RepositoryDaemon = require("Script/Util/RepositoryDaemon")
	local weaver = RepositoryDaemon:New("127.0.0.1:16384", "Database/Repository.sql", databasePath)
	Debug.Weaver = weaver
	local s, msg = pcall(function ()
		InitializeWorld(databasePath)
	end)

	if not s then
		print("Error: " .. msg)
	end
end

return Main