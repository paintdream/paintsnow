-- Widget.tl
-- PaintDream (paintdream@paintdream.com)
--
local Theme = require("Engine/Widget/Theme")
local Style = require("Engine/Widget/Style")

local Widget = record
	name : string
	entity : Entity
	textViewComponent : TextViewComponent
	spaceComponent : SpaceComponent
	layoutComponent : LayoutComponent
	style : string
	theme : Theme
	childs : { any : Widget }
	__index : any
end

Widget.__index = Widget

local fontCache : { string : FontResource } = {}
local loader = require

local function LoadFont(fontPath : string) : FontResource
	local exist = fontCache[fontPath]
	if exist then return exist end

	exist = SnowyStream.NewResource(fontPath, "") as FontResource
	fontCache[fontPath] = exist
	return exist
end

local function LoadTheme(themePath : string) : Theme
	local theme = loader(themePath) as Theme
	-- load all fonts into cache
	for _, v in pairs(theme.styles) do
		LoadFont(v.font)
	end

	return theme
end

Widget.theme = LoadTheme("Engine/Widget/Theme/PurpleLove")

function Widget:GetStyle() : Style
	return self.theme.styles[self.style]
end

function Widget:SetText(text : string)
	if not self.textViewComponent then
		-- get font from theme
		local style = self:GetStyle()
		local textViewComponent = TextViewComponentModule.New(LoadFont(style.font))
		self.entity:AddEntityComponent(textViewComponent as Component)
		self.textViewComponent = textViewComponent
	end

	self.textViewComponent:SetText()
end

function Widget:AddChild(child : Widget)
	table.insert(self.childs as { Widget }, child)

	if child.name ~= "" then
		assert(not self.childs[child.name])
		self.childs[child.name] = child
	end

	if not self.spaceComponent then
		self.spaceComponent = SpaceComponentModule.New(-1, false)
	end

	self.spaceComponent:InsertEntity(child.entity)
end

function Widget:RemoveChild(child : Widget)
	local childs = self.childs as { Widget }
	for k, v in ipairs(childs) do
		if v == child then
			table.remove(childs, k)
			break
		end
	end

	if child.name ~= "" then
		self.childs[child.name] = nil
	end

	assert(self.spaceComponent)
	self.spaceComponent:RemoveEntity(child.entity)
end

function Widget:GetChild(...: any) : Widget
	local lists = { ... } as { any }
	local p : Widget = self
	for _, v in ipairs(lists) do
		p = p.childs[v]
		if p == nil then return nil end	
	end

	return p
end

function Widget:Update()
	local style = self:GetStyle()
	local layoutComponent = self.layoutComponent

	layoutComponent:SetPadding(style.padding)
	layoutComponent:SetMargin(style.margin)
	layoutComponent:SetSize(style.size)
end

function Widget.New(style : string) : Widget
	local widget = {
		entity = MythForest.NewEntity(-1)
		style = style,
		childs = {},
		layoutComponent = LayoutComponentModule.New()
	}

	local ret = setmetatable(widget, Widget as METATABLE) as Widget
	ret:Update()
	return ret
end

function Widget.NewFromDocument(document : { any : any }) : Widget
	return {} as Widget
end

return Widget