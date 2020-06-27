-- Main.lua
-- Application Entry Module
local Main = record end
local Debug = require("Script/Util/Debug")
local Core = require("Engine/Core")
local Database = require("Engine/Data/Database")
local Vector = require("Engine/Data/Math/Vector")

local fpsBuffer : { number } = {}
local tickBuffer : { number } = {}
local bufferIndex = 1

local function SetFrameMonitorOnTitle(deltaTime : number, cameraComponent : CameraComponent, transformComponent : TransformComponent)
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
		local position = transformComponent:GetTranslation()
		local fps = string.format("%3.2f", math.min(1000 * #fpsBuffer / sumFrame, 999.99))
		local tps = string.format("%3.2f", math.min(1000 * #tickBuffer / sumTick, 999.99))
		local eye = string.format("%3.2f, %3.2f, %3.2f", position[1], position[2], position[3])
		local render = tostring(cameraComponent:GetCollectedVisibleEntityCount()) .. "/" .. tostring(cameraComponent:GetCollectedEntityCount())
		local triangle = tostring(cameraComponent:GetCollectedTriangleCount())

		System.SetAppTitle("ShadowRock DEMO"
			.. " | FPS: " .. fps
			.. " | TPS: " .. tps
			.. " | Eye: " .. eye
			.. " | Render " .. render
			.. " | Triangles " .. triangle)
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
	-- local GUI = require("Script/Root/GUI")
	-- local guiEntity = GUI.New()

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
	local cameraTranslation : float3 = { 5, 5, 8 }
	local cameraRotation : float3 = { 0.25 * math.pi, 0, math.pi }
	local cameraController = playerEntity.cameraController
	local cameraTransform = cameraController.transformComponent
	cameraTransform:SetTranslation(cameraTranslation)
	cameraTransform:SetRotation(cameraRotation)
	cameraTransform:UpdateTransform()
	
	local screenSize = System.GetScreenSize() as { number }
	cameraController.aspect = screenSize[1] / screenSize[2]
	cameraController:UpdatePerspective()
	
	local phaseComponent = cameraController.phaseComponent
	phaseComponent:BindRootEntity(scene.rootEntity)
	--phaseComponent:Update({ 5, 1.5, 5})

	local lastMousePos = { 0, 0 }
	local moveSpeed = 1.0
	local inOperation = false
	scene.rootEntity:UpdateEntity()

	local stepPhase = false
	local stepBounceCount : number = 16

	local function ReportStepConfig()
		print("PhaseStep: " .. (stepPhase and "[ON] " or "[OFF] BOUNCE: ") .. tostring(stepBounceCount))
	end

	local EventKeyboard = record
		Name : string
		Down : boolean
	end

	local EventMouse = record
		Button : boolean
		Down : boolean
		Move : boolean
		Wheel : boolean
		Position : float2
	end

	local EventData = record
		Size : { number }
		Keyboard : EventKeyboard
		Mouse : EventMouse
	end

	eventListenerComponent:SetEventHandler(function (eventType : string, eventData : EventData)
		if eventType == "EVENT_TICK" then
			local deltaTime = eventData as number
			SetFrameMonitorOnTitle(deltaTime, cameraComponent, cameraTransform)
			if stepPhase then
				phaseComponent:Step(stepBounceCount)
			end
		elseif eventType == "EVENT_INPUT" then
			if inOperation then return end

			local evSize = eventData.Size
			if evSize then
				print("Resizing to (" .. tostring(evSize[1]) .. ", " .. tostring(evSize[2]) .. ")")
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
					elseif name == "G" then
						-- step phase
						stepPhase = not stepPhase
						ReportStepConfig()
					elseif name == "," then
						stepBounceCount = math.min(stepBounceCount - 1, 1)
						ReportStepConfig()
					elseif name == "." then
						stepBounceCount = stepBounceCount + 1
						ReportStepConfig()
					end

					moveSpeed = math.min(moveSpeed + 0.1, 10.0)
				else
					moveSpeed = 1.0
				end
			end

			local mouse = eventData.Mouse
			if mouse then
				local size = System.GetScreenSize() as { number }
				local to = { mouse.Position[1] / size[1] * 2 - 1, mouse.Position[2] / size[2] * 2 - 1 }
				if mouse.Wheel then
					if mouse.Down then
						cameraController:Walk(moveSpeed)
					else
						cameraController:Walk(-moveSpeed)
					end
				elseif mouse.Down and mouse.Move then
					local from = { lastMousePos[1] / size[1] * 2 - 1, lastMousePos[2] / size[2] * 2 - 1 }

					if mouse.Button then
						-- Conflicts with LeavesWine.
						local s = moveSpeed * 100
						cameraController:Translate({ (from[1] - to[1]) * s, (from[2] - to[2]) * s} as float2)
					else
						cameraController:Rotate(from, to)
					end
				elseif mouse.Down then
					local position = cameraTransform:GetTranslation() as { number }
					local mat = cameraTransform:GetAxises() as { { number }}
					local target = ((Vector.Attach(mat[3]) as number * (-1) + Vector.Attach(mat[1]) as number * to[1] * cameraController.aspect + Vector.Attach(mat[2]) as number * to[2]) * 100.0) as float3
					local RaycastResult = record
						Intersection : float3
						Normal : float3
						TexCoord : float2
						Distance : number
						Object : Component
						Parent : Entity
					end

					local results = scene.rootEntity:Raycast(function (results : { RaycastResult })
						for i, result in ipairs(results) do
							BridgeSunset.QueueRoutine(result.Parent, function ()
								local formComponent = result.Parent:GetUniqueEntityComponent("FormComponent") as FormComponent
								print("[" .. tostring(i) .. "] Mouse clicked at: " .. formComponent:GetName())
							end)
						end
					end, position, target, 1)
				end

				lastMousePos = mouse.Position
			end

			cameraController:Update()
			inOperation = false
		end
	end)

	print("InitializeWorld: World initialization finished. Waiting for system command")

	local instance = coroutine.running()
	System.ListenConsole(function (command : string)
		if command:sub(1, 1) == "@" then
			coroutine.resume(instance, command)
		else
			local func, message = load(command) as function, string
			if func then
				func()
			else
				print("Invalid command: " .. message as string)
			end
		end
	end)

	while true do
		local command = coroutine.yield()
		if command == "@Quit" then
			print("Battle control terminated.")
			eventListenerComponent:BindEventTick(nil)
			eventListenerComponent:BindEventUserInput(false)
			eventListenerComponent:BindEventFrame(false)
			System.Exit()
			break
		end
	end
end

function Main.Main()
	print("Main initializing ...")
	print("Loading Repository Daemon ...")
	local databasePath = "Weaver/Repository.db"

	Debug.CompressResource = function ()
		local database : Database = Database.New(databasePath)
		if database then
			local compressor = require("Script/Util/RepositoryCompressor")
			compressor.Compress(database)
		end
	end

	local RepositoryDaemon = require("Script/Util/RepositoryDaemon")
	local weaver = RepositoryDaemon:New("127.0.0.1:16384", "Database/Repository.sql", databasePath)
	Debug.Weaver = weaver
	
	local s, msg = pcall(function () : string
		InitializeWorld(databasePath)
		return "World Initialized"
	end)

	if not s then
		print("Error: " .. msg as string)
	end

end

return Main