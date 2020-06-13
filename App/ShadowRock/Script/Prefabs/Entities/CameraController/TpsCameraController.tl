local TpsCameraController = record
	cameraComponent : CameraComponent
	transformComponent : TransformComponent
	renderFlowComponent : RenderFlowComponent
	phaseComponent : PhaseComponent
	entity : Entity
	__index : any
	speed : number
	fov : number
	near : number
	far : number
	aspect : number
end

TpsCameraController.__index = TpsCameraController

local StandardCamera = require("Script/Prefabs/Components/Camera/StandardCamera")
local StandardRenderFlow = require("Script/Prefabs/Components/RenderFlow/StandardRenderFlow")
local Phase = require("Script/Prefabs/Components/Phase/StandardPhase")
local Vector = require("Engine/Data/Math/Vector")

function TpsCameraController.New(eventListenerComponent : EventListenerComponent) : TpsCameraController
	local entity = MythForest.NewEntity(-1)
	local renderFlowComponent = StandardRenderFlow.New(3)
	local cameraComponent = StandardCamera.New(renderFlowComponent)
	local transformComponent = TransformComponentModule.New()
	local phaseComponent = Phase.New(renderFlowComponent)

	entity:AddEntityComponent(transformComponent as Component)
	entity:AddEntityComponent(renderFlowComponent as Component)
	entity:AddEntityComponent(cameraComponent as Component)
	entity:AddEntityComponent(phaseComponent as Component)
	entity:AddEntityComponent(eventListenerComponent as Component)

	-- setup phase
	phaseComponent:Setup(16, 8, { 10, 10, 10 }, { 256, 256 })

	local meta = {} as TpsCameraController
	meta.cameraComponent = cameraComponent
	meta.transformComponent = transformComponent
	meta.renderFlowComponent = renderFlowComponent
	meta.phaseComponent = phaseComponent
	meta.entity = entity
	meta.speed = 1
	meta.fov = math.pi / 4
	meta.far = 2000
	meta.near = 0.1
	meta.aspect = 1

	return setmetatable(meta, TpsCameraController as METATABLE) as TpsCameraController
end

function TpsCameraController:UpdatePerspective()
	self.cameraComponent:SetPerspective(self.fov, self.near, self.far, self.aspect)
end

function TpsCameraController:SetSpeed(speed : number)
	self.speed = speed
end

function TpsCameraController:Walk(distance : number)
	local position = Vector.Attach(self.transformComponent:GetTranslation())
	local mat = self.transformComponent:GetAxises() as {{ number }}
	position = (position as number + (Vector.Attach(mat[3]) as number * (-self.speed * distance)) as number) as Vector

	self.transformComponent:SetTranslation(position as float3)
end

function TpsCameraController:Translate(distance : float2)
	local position = Vector.Attach(self.transformComponent:GetTranslation())
	local mat = self.transformComponent:GetAxises() as {{ number }}
	local delta = (Vector.Attach(mat[1]) as number * (distance as { number }[1]) + Vector.Attach(mat[2]) as number * (distance as { number }[2])) * (self.speed)
	-- print("DELTA: " .. tostring(delta[1]) .. ", " .. tostring(delta[2]) .. ", " .. tostring(delta[3]))
	position = (position as number + delta) as Vector

	self.transformComponent:SetTranslation(position as float3)
end

function TpsCameraController:Rotate(from : float2, to : float2)
	self.transformComponent:EditorRotate(from, to)
end

function TpsCameraController:Update()
	self.transformComponent:UpdateTransform()
end

return TpsCameraController