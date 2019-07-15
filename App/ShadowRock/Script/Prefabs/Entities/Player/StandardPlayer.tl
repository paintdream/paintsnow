local StandardPlayer = {}
local TpsCameraController = require("Script/Prefabs/Entities/CameraController/TpsCameraController")
local anyone : any = {}
local entityPrototype : Entity = anyone
StandardPlayer.cameraController = TpsCameraController
StandardPlayer.player = entityPrototype
StandardPlayer.__index = StandardPlayer

function StandardPlayer.New(eventListenerComponent : EventListenerComponent)
	print("\tStardard Player selected.")
	local player = MythForest.NewEntity(-1)
	local spaceComponent = SpaceComponentModule.New(-1, false)
	player:AddEntityComponent(spaceComponent)

	print("\tAttach camera to player ...")
	local tpsCameraController = TpsCameraController.New(eventListenerComponent)
	spaceComponent:InsertEntity(tpsCameraController.cameraController)
	local meta = {}
	meta.cameraController = tpsCameraController
	meta.player = player

	return setmetatable(meta, { __index = StandardPlayer, __chain = true })
end

return StandardPlayer