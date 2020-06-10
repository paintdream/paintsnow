local RuntimeVisibilityProbe = record end

function RuntimeVisibilityProbe.New() : VisibilityComponent
	local visibilityComponent = VisibilityComponentModule.New()
	visibilityComponent:Setup(500, { {-10, -10, -10 }, { 10, 10, 10 } }, { 128, 128, 128 }, 5, 8, { 128, 128 })
	return visibilityComponent
end

return RuntimeVisibilityProbe