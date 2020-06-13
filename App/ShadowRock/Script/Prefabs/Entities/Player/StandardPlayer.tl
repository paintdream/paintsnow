local TpsCameraController = require("Script/Prefabs/Entities/CameraController/TpsCameraController")

local StandardPlayer = record
	cameraController : TpsCameraController
	player : Entity
	__index : any
end

StandardPlayer.__index = StandardPlayer

function StandardPlayer.New(eventListenerComponent : EventListenerComponent) : StandardPlayer
	print("\tStardard Player selected.")
	local player = MythForest.NewEntity(-1)
	local spaceComponent = SpaceComponentModule.New(-1, false)
	player:AddEntityComponent(spaceComponent as Component)

	print("\tAttach camera to player ...")
	local tpsCameraController = TpsCameraController.New(eventListenerComponent)
	spaceComponent:InsertEntity(tpsCameraController.entity)
	
	local meta = {} as StandardPlayer
	meta.cameraController = tpsCameraController
	meta.player = player

	return setmetatable(meta, StandardPlayer as METATABLE) as StandardPlayer
end

return StandardPlayer