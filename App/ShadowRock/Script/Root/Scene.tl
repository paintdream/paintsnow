-- derive from entity

local Scene = record
	spaceComponent : SpaceComponent
	rootEntity : Entity
	__index : any
end

Scene.__index = Scene

local Database = require("Engine/Data/Database")

function Scene.New(database : Database, clock : Clock) : Scene
	local meta = {} as Scene
	local rootEntity = MythForest.NewEntity(-1)
	local spaceComponent = SpaceComponentModule.New(-1, true)
	rootEntity:AddEntityComponent(spaceComponent as Component)
	meta.spaceComponent = spaceComponent
	meta.rootEntity = rootEntity

	local RuntimeVisibilityProbe = require("Script/Prefabs/Components/Visibility/RuntimeVisibilityProbe")
	local eventListenerComponent = EventListenerComponentModule.New()
	eventListenerComponent:BindEventTick(clock)
	eventListenerComponent:BindEventFrame(true)
	rootEntity:AddEntityComponent(eventListenerComponent as Component)

	local renderPolicy = RenderFlowComponentModule.NewRenderPolicy("Primitives", 0)
	local lightPolicy = RenderFlowComponentModule.NewRenderPolicy("LightSource", 0)

	-- add scene entities
	if database then
		local dupCount = 6
		print("\tCreating entities.")

		for i = 1, dupCount do
			print("\tCreating at warp " .. i)
			local offset = i * 7.5
			local interSpaceComponent = SpaceComponentModule.New(i)
			rootEntity:AddEntityComponent(interSpaceComponent as Component)

			BridgeSunset.QueueRoutine(interSpaceComponent, function ()
				local subRootEntity = MythForest.NewEntity(-1)
				local subSpaceComponent = SpaceComponentModule.New(-1, true)
				local subSpaceComponentHuge = SpaceComponentModule.New(-1, false)
				subRootEntity:AddEntityComponent(subSpaceComponent as Component)
				subRootEntity:AddEntityComponent(subSpaceComponentHuge as Component)
				local batchComponent = BatchComponentModule.New()

				local entities : { Entity } = {}
				local hugeEntity : { Entity : boolean } = {}
				local hugeComponent : { Component : boolean } = {}

				for _, line in database:Lines("select ID from `Entities`;", 32) do
					local entityID = table.unpack(line as { number }) as number
					entities[entityID] = MythForest.NewEntity(-1)
				end

				print("\tCreating model components.")
				local components : { Component } = {}
				local shapeComponents : { ShapeComponent } = {}
				local formComponents : { FormComponent } = {}
				for _, line in database:Lines("select ModelComponents.ID, Path, ViewDistance from `ModelComponents`, `Resources` where `ResourceID` == Resources.ID;", 32) do
					local componentID, path, viewDistance = table.unpack(line as { any })
					if not path as string:find("Sky") then
						local meshResource = SnowyStream.NewResource(path as string, "MeshResource", false) as MeshResource
						if not meshResource then
							print("\tUnable to load mesh resource: " .. path as string)
						else
							components[componentID as number] = components[componentID as number] or ModelComponentModule.New(meshResource, batchComponent) as Component
							shapeComponents[componentID as number] = shapeComponents[componentID as number] or ShapeComponentModule.New(meshResource) 
							formComponents[componentID as number] = formComponents[componentID as number] or FormComponentModule.New(path as string)
						end
					end
				end

				local count = 0
				for _, line in database:Lines("select ModelComponentMaterials.ID, MeshGroupID, Path from `ModelComponentMaterials`, `Resources` where `ResourceID` == Resources.ID;", 32) do
					local componentID, meshGroupID, path = table.unpack(line as { any })
					local materialResource = SnowyStream.NewResource(path as string, "MaterialResource", false) as MaterialResource
					if not materialResource then
						print("\tUnable to load material resource: " .. path as string)
					else
						local modelComponent = components[componentID as number] as ModelComponent
						if modelComponent then
							modelComponent:SetRenderPolicy(renderPolicy)
							modelComponent:AddMaterial(meshGroupID as number, materialResource)
						end
					end

					count = count + 1
				end

				print("\tCreating transform components.")
				for _, line in database:Lines("select ID, X, Y, Z, Scale, Yaw, Pitch, Roll from `TransformComponents`;", 32) do
					local componentID, x, y, z, s, yaw, pitch, roll = table.unpack(line as { number })
					local transformComponent = TransformComponentModule.New()
					transformComponent:SetTranslation({ x as number + offset, y as number, z as number })
					transformComponent:SetScale({ s as number, s as number, s as number })
					transformComponent:SetRotation({ yaw as number, pitch as number, roll as number })
					transformComponent:UpdateTransform()

					components[componentID as number] = transformComponent as Component
				end

				print("\tCreating envcube components")
				for _, line in database:Lines("select EnvCubeComponents.ID, Path from `EnvCubeComponents`, `Resources` where `ResourceID` == Resources.ID;", 32) do
					local componentID, path = table.unpack(line as { any })
					components[componentID as number] = components[componentID as number] or EnvCubeComponentModule.New() as Component
					local textureResource = SnowyStream.NewResource(path as string, "TextureResource", false)
					if textureResource then
						local component = components[componentID as number] as EnvCubeComponent
						hugeComponent[component as Component] = true
						component:SetTexture(textureResource as TextureResource)
						component:SetRenderPolicy(lightPolicy)
						component:SetRange({1000, 1000, 1000})
					else
						print("\tUnable to load texture resource: " .. path as string)
					end
				end

				print("\tAttaching components.")
				-- create model components
				for _, line in database:Lines("select ID, ComponentID from `EntityComponents`;", 32) do
					local entityID, componentID = table.unpack(line as { any })
					local entity = entities[entityID as number]
					local component = components[componentID as number]
					if entity and component then
						if hugeComponent[component] then
							hugeEntity[entity] = true
						end
						entity:AddEntityComponent(component)
						local shapeComponent = shapeComponents[componentID as number]
						if shapeComponent then
							entity:AddEntityComponent(shapeComponent as Component)
						end

						local formComponent = formComponents[componentID as number]
						if formComponent then
							entity:AddEntityComponent(formComponent as Component)
						end
					end
				end

				for _, entity in ipairs(entities) do
					entity:UpdateEntity()
					if hugeEntity[entity] then
						subSpaceComponentHuge:InsertEntity(entity)
					else
						subSpaceComponent:InsertEntity(entity)
					end
				end

				local subEventListenerComponent = EventListenerComponentModule.New()
				subEventListenerComponent:BindEventFrame(true)
				local visibilityComponent = RuntimeVisibilityProbe.New()
				subRootEntity:AddEntityComponent(subEventListenerComponent as Component)
				subRootEntity:AddEntityComponent(visibilityComponent as Component)
				subRootEntity:AddEntityComponent(batchComponent as Component)
				interSpaceComponent:InsertEntity(subRootEntity)
			end)
		end

		print("\tAdding lights")
		
		local sun = MythForest.NewEntity(-(1))
		local sunTransformComponent : TransformComponent = TransformComponentModule.New()
		sunTransformComponent:SetScale({ 8, 8, 8 })
		sunTransformComponent:SetRotation({0.25 * math.pi, 0.75 * math.pi, 0})
		sunTransformComponent:SetTranslation({-3, 2, 4})
		sunTransformComponent:UpdateTransform()
		sun:AddEntityComponent(sunTransformComponent as Component)
		local lightComponent = LightComponentModule.New()
		lightComponent:SetLightColor({2, 2, 2})
		lightComponent:SetLightRange({1000, 1000, 1000})
		lightComponent:SetLightAttenuation(0.1)
		lightComponent:SetLightDirectional(true)
		lightComponent:SetRenderPolicy(lightPolicy)
		local streamComponent = StreamComponentModule.New({32, 32, 1}, 16)
		lightComponent:BindLightShadowStream(0, streamComponent, { 1024, 1024 }, 0.25, 1)
		lightComponent:BindLightShadowStream(1, streamComponent, { 1024, 1024 }, 0.25, 4)
		lightComponent:BindLightShadowStream(2, streamComponent, { 1024, 1024 }, 0.25, 16)

		sun:AddEntityComponent(lightComponent as Component)
		spaceComponent:InsertEntity(sun)

		--[[
		print("\tAdding point lights")
		math.randomseed(20)
		local colors : { number : any } = {
			{1, 0, 0},{0, 1, 0}, {0, 0, 1}, {1, 1, 0}, {0, 1, 1}, {1, 0, 1}, {1, 1, 1}, {2, 2, 2}
		}
		for k = 1, 8 do 
			local c = colors[k]
			local r = math.random(20, 40)
			for j = 1, 3 do
				c[j] = c[j] * r
			end
		end

		for i = 1, 32 do 
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

	return setmetatable(meta, Scene as METATABLE) as Scene
end

return Scene