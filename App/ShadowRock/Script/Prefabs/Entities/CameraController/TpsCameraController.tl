local TpsCameraController = record
	cameraComponent : CameraComponent
	transformComponent : TransformComponent
	renderFlowComponent : RenderFlowComponent
	phaseComponent : PhaseComponent
	spaceComponent : SpaceComponent
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
local Vector = require("Engine/Model/Math/Vector")

function TpsCameraController.New(eventComponent : EventComponent) : TpsCameraController
	local entity = MythForest.NewEntity(-1)
	local renderFlowComponent = StandardRenderFlow.New(3)
	local cameraComponent = StandardCamera.New(renderFlowComponent)
	local transformComponent = TransformComponentModule.New()
	local phaseComponent = Phase.New(renderFlowComponent)
	local spaceComponent = SpaceComponentModule.New(0, false)

	entity:AddEntityComponent(transformComponent as Component)
	entity:AddEntityComponent(renderFlowComponent as Component)
	entity:AddEntityComponent(cameraComponent as Component)
	entity:AddEntityComponent(phaseComponent as Component)
	entity:AddEntityComponent(eventComponent as Component)
	entity:AddEntityComponent(spaceComponent as Component)

	-- setup phase
	phaseComponent:Setup(16, 8, { 4, 4, 4 }, { 256, 256 })

	local meta = {} as TpsCameraController
	meta.cameraComponent = cameraComponent
	meta.transformComponent = transformComponent
	meta.renderFlowComponent = renderFlowComponent
	meta.phaseComponent = phaseComponent
	meta.spaceComponent = spaceComponent
	meta.entity = entity
	meta.speed = 0.02
	meta.fov = math.pi / 2
	meta.far = 1000
	meta.near = 0.05
	meta.aspect = 1.0

	debug.setuservalue(entity, meta)
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