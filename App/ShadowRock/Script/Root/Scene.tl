-- derive from entity

local Scene = {}
Scene.__index = Scene

local Database = require("Engine/Data/Database")
local anyone : any = {}
local spaceComponentPrototype : SpaceComponent = anyone
Scene.spaceComponent = spaceComponentPrototype
local entityPrototype : Entity = anyone
Scene.rootEntity = entityPrototype

function Scene.New(database : Database?, clock : Clock)
	local meta = {}
	local rootEntity = MythForest.NewEntity(-1)
	local spaceComponent = SpaceComponentModule.New(-1, true)
	rootEntity:AddEntityComponent(spaceComponent)
	meta.spaceComponent = spaceComponent
	meta.rootEntity = rootEntity

	local eventListenerComponent = EventListenerComponentModule.New()
	eventListenerComponent:BindEventTick(clock)
	eventListenerComponent:BindEventFrame(true)
	rootEntity:AddEntityComponent(eventListenerComponent)
	local batchComponent = BatchComponentModule.New()
	local renderPolicy : RenderPolicy = RenderFlowComponentModule.NewRenderPolicy("Primitives", 0)
	local lightPolicy : RenderPolicy = RenderFlowComponentModule.NewRenderPolicy("LightSource", 0)

	-- add scene entities
	if database then
		print("\tCreating entities.")
		local entities : { Entity } = {}
		for _, line in database:Lines("select ID, GroupID from `Entities`;", 32) do
			local entityID, entityGroup = table.unpack(line)
			entities[entityID] = MythForest.NewEntity(-1)
		end

		print("\tCreating model components.")
		local components : { any } = {}
		local shapeComponents : { any } = {}
		local formComponents : { any } = {}
		for _, line in database:Lines("select ModelComponents.ID, Path, ViewDistance from `ModelComponents`, `Resources` where `ResourceID` == Resources.ID;", 32) do
			local componentID, path, viewDistance = table.unpack(line)
			local meshResource = SnowyStream.NewResource(path, "MeshResource", false)
			if not meshResource then
				print("\tUnable to load mesh resource: " .. path)
			else
				components[componentID] = components[componentID] or ModelComponentModule.New(meshResource, batchComponent)
				shapeComponents[componentID] = shapeComponents[componentID] or ShapeComponentModule.New(meshResource)
				formComponents[componentID] = formComponents[componentID] or FormComponentModule.New(path)
			end
		end

		local count = 0
		for _, line in database:Lines("select ModelComponentMaterials.ID, MeshGroupID, Path from `ModelComponentMaterials`, `Resources` where `ResourceID` == Resources.ID;", 32) do
			local componentID, meshGroupID, path = table.unpack(line)
			local materialResource = SnowyStream.NewResource(path, "MaterialResource", false)
			if not materialResource then
				print("\tUnable to load material resource: " .. path)
			else
				local modelComponent : ModelComponent = components[componentID]
				if modelComponent then
					modelComponent:SetRenderPolicy(renderPolicy)
					modelComponent:AddMaterial(meshGroupID, materialResource)
				end
			end

			count = count + 1
		end

		print("\tCreating transform components.")
		for _, line in database:Lines("select ID, X, Y, Z, Scale, Yaw, Pitch, Roll from `TransformComponents`;", 32) do
			local componentID, x, y, z, s, yaw, pitch, roll = table.unpack(line)
			local transformComponent = TransformComponentModule.New()
			transformComponent:SetTranslation({ x, y, z })
			transformComponent:SetScale({ s, s, s })
			transformComponent:SetRotation({ yaw, pitch, roll })
			transformComponent:UpdateTransform()
			components[componentID] = transformComponent
		end

		print("\tCreating envcube components")
		for _, line in database:Lines("select EnvCubeComponents.ID, Path from `EnvCubeComponents`, `Resources` where `ResourceID` == Resources.ID;", 32) do
			local componentID, path = table.unpack(line)
			components[componentID] = components[componentID] or EnvCubeComponentModule.New()
			local textureResource = SnowyStream.NewResource(path, "TextureResource", false)
			if textureResource then
				local component = components[componentID]
				component:SetTexture(textureResource)
				component:SetRenderPolicy(lightPolicy)
				component:SetRange({1000, 1000, 1000})
			else
				print("\tUnable to load texture resource: " .. path)
			end
		end
		
		print("\tAttaching components.")
		-- create model components
		for _, line in database:Lines("select ID, ComponentID from `EntityComponents`;", 32) do
			local entityID, componentID = table.unpack(line)
			local entity = entities[entityID]
			local component = components[componentID]
			if entity and component then
				entity:AddEntityComponent(component)
				local shapeComponent = shapeComponents[componentID]
				if shapeComponent then
					entity:AddEntityComponent(shapeComponent)
				end

				local formComponent = formComponents[componentID]
				if formComponent then
					entity:AddEntityComponent(formComponent)
				end
			end
		end

		for _, entity in pairs(entities) do
			entity:UpdateEntity()
			spaceComponent:InsertEntity(entity)
		end

		print("\tAdding lights")
		
		local sun = MythForest.NewEntity(-(1))
		local sunTransformComponent = TransformComponentModule.New()
		sunTransformComponent:SetRotation({0.25 * math.pi, 0.75 * math.pi, 0})
		sunTransformComponent:SetTranslation({-3, 2, 4})
		sunTransformComponent:UpdateTransform()
		sun:AddEntityComponent(sunTransformComponent)
		local lightComponent = LightComponentModule.New()
		lightComponent:SetLightColor({5, 5, 5})
		lightComponent:SetLightRange({1000, 1000, 1000})
		lightComponent:SetLightAttenuation(0.1)
		lightComponent:SetLightDirectional(true)
		lightComponent:SetRenderPolicy(lightPolicy)
		local streamComponent = StreamComponentModule.New({32, 32, 1}, 16)
		lightComponent:BindLightShadowStream(0, streamComponent)

		sun:AddEntityComponent(lightComponent)
		spaceComponent:InsertEntity(sun)

		--[[
		print("\tAdding point lights")
		math.randomseed(20)
		local colors : { integer : any } = {
			{1, 0, 0},{0, 1, 0}, {0, 0, 1}, {1, 1, 0}, {0, 1, 1}, {1, 0, 1}, {1, 1, 1}, {2, 2, 2}
		}
		for k = 1, 8 do 
			local c = colors[k]
			local r = math.random(20,40)
			for j = 1, 3 do
				c[j] = c[j] * r
			end
		end

		for i = 1, 64 do 
			local light = MythForest.NewEntity(-(((1))))
			local lightTransformComponent = TransformComponentModule.New()
			lightTransformComponent:SetRotation({0.25 * math.pi, 0.75 * math.pi, 0})
			lightTransformComponent:SetTranslation({math.random(-((5)),0), math.random(0,3), math.random(2,5)})
			lightTransformComponent:UpdateTransform()
			light:AddEntityComponent(lightTransformComponent)
			local lightComponent = LightComponentModule.New()
			lightComponent:SetLightColor(colors[((i + 1) % 8) - 1])
			lightComponent:SetLightRange({1000, 1000, 1000})
			lightComponent:SetLightAttenuation(math.random(100,400) * 0.2)
			lightComponent:SetLightDirectional(false)
			lightComponent:SetRenderPolicy(lightPolicy)
			light:AddEntityComponent(lightComponent)
			spaceComponent:InsertEntity(light)
		end
		]]
	end

	local RuntimeVisibilityProbe = require("Script/Prefabs/Components/Visibility/RuntimeVisibilityProbe")
	local visibilityComponent = RuntimeVisibilityProbe.New()
	rootEntity:AddEntityComponent(visibilityComponent)
	rootEntity:AddEntityComponent(batchComponent)
 
	return setmetatable(meta, { __index = Scene, __chain = true })
end

return Scene