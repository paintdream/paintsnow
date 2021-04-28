local GUI = record
	spaceComponent : SpaceComponent
	rootEntity : Entity
	__index : any
end

GUI.__index = GUI

function GUI.New() : GUI
	local meta = {} as GUI
	local rootEntity = MythForest.NewEntity(-1)
	local spaceComponent = SpaceComponentModule.New(-1, true)
	rootEntity:AddEntityComponent(spaceComponent as Component)
	meta.spaceComponent = spaceComponent
	meta.rootEntity = rootEntity

	local renderPolicy = TextViewComponentModule.NewRenderPolicy("Widgets", 0, 1)

	local widgetEntity = MythForest.NewEntity(-1)
	local fontResource = SnowyStream.NewResource("/Packed/Font/Cantarell-Regular", "FontResource", false)
	local textViewComponent = TextViewComponentModule.New(fontResource as FontResource, nil as MeshResource, nil as MaterialResource, nil as BatchComponent)
	textViewComponent:AddRenderPolicy(renderPolicy)
	textViewComponent:SetText("Hello, world!")
	widgetEntity:AddEntityComponent(textViewComponent as Component)
	spaceComponent:InsertEntity(widgetEntity)

	return setmetatable(meta, GUI as METATABLE) as GUI
end

return GUI