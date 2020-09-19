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

	local renderPolicy = RenderFlowComponentModule.NewRenderPolicy("Widgets", 0)

	local widgetEntity = MythForest.NewEntity(-1)
	local fontBatchComponent = BatchComponentModule.New()
	local fontResource = SnowyStream.NewResource("Packed/Font/Cantarell-Regular", "FontResource", false)
	local textViewComponent = TextViewComponentModule.New(fontResource as FontResource, fontBatchComponent)
	textViewComponent:SetRenderPolicy(renderPolicy)
	textViewComponent:SetText("Hello, world!")
	widgetEntity:AddEntityComponent(textViewComponent as Component)
	spaceComponent:InsertEntity(widgetEntity)

	return setmetatable(meta, GUI as METATABLE) as GUI
end

return GUI