local RuntimeVisibilityProbe = record end

function RuntimeVisibilityProbe.New() : VisibilityComponent
	local streamComponent = StreamComponentModule.New({ 12, 12, 8 }, 64 )
	local visibilityComponent = VisibilityComponentModule.New(streamComponent)
	visibilityComponent:Setup(500, { 0.2, 0.2, 0.2 }, 10, { 128, 128 })

	return visibilityComponent
end

return RuntimeVisibilityProbe