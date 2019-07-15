local TpsCameraController = {}
local StandardCamera = require("Script/Prefabs/Components/Camera/StandardCamera")
local StandardRenderFlow = require("Script/Prefabs/Components/RenderFlow/StandardRenderFlow")
local Vector = require("Engine/Math/Vector")
local anyone : any = {}
local entityPrototype : Entity = anyone
local transformPrototype : TransformComponent = anyone
local cameraPrototype : CameraComponent = anyone

TpsCameraController.cameraComponent = cameraPrototype
TpsCameraController.transformComponent = transformPrototype
TpsCameraController.__index = TpsCameraController
TpsCameraController.speed = 0.02
TpsCameraController.fov = math.pi / 2 -- 90
TpsCameraController.near = 0.05
TpsCameraController.far = 1000.0
TpsCameraController.aspect = 1.0
TpsCameraController.cameraController = entityPrototype

function TpsCameraController.New(eventListenerComponent : EventListenerComponent)
	local renderFlowComponent = StandardRenderFlow.New(3)
	local cameraComponent = StandardCamera.New(renderFlowComponent)
	local cameraController = MythForest.NewEntity(-1)
	local transformComponent = TransformComponentModule.New()

	cameraController:AddEntityComponent(transformComponent)
	cameraController:AddEntityComponent(renderFlowComponent)
	cameraController:AddEntityComponent(cameraComponent)
	cameraController:AddEntityComponent(eventListenerComponent)

	local meta = {}
	meta.cameraComponent = cameraComponent
	meta.transformComponent = transformComponent
	meta.cameraController = cameraController

	return setmetatable(meta, { __index = TpsCameraController, __chain = true })
end

function TpsCameraController:UpdatePerspective()
	self.cameraComponent:SetPerspective(self.fov, self.near, self.far, self.aspect)
end

function TpsCameraController:SetSpeed(speed : number)
	self.speed = speed
end

function TpsCameraController:Walk(distance : number)
	local position = Vector.New(self.transformComponent:GetTranslation())
	local mat = self.transformComponent:GetAxises()
	position = position + Vector.New(mat[3]) * (-self.speed * distance)
	self.transformComponent:SetTranslation(position)
end

function TpsCameraController:Translate(distance : float2)
	local position = Vector.New(self.transformComponent:GetTranslation())
	local mat = self.transformComponent:GetAxises()
	local delta = (Vector.New(mat[1]) * distance[1] + Vector.New(mat[2]) * distance[2]) * self.speed
	-- print("DELTA: " .. tostring(delta[1]) .. ", " .. tostring(delta[2]) .. ", " .. tostring(delta[3]))
	position = position + delta
	self.transformComponent:SetTranslation(position)
end

function TpsCameraController:Rotate(from : float2, to : float2)
	self.transformComponent:EditorRotate(from, to)
end

function TpsCameraController:Update()
	self.transformComponent:UpdateTransform()
end

return TpsCameraController