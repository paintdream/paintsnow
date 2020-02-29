local StandardPhase = {}

function StandardPhase.New(renderFlowComponent : RenderFlowComponent)
	local phaseComponent = PhaseComponentModule.New(renderFlowComponent, "PhaseView")
	phaseComponent:Setup(16, 8, { 10, 10, 10 }, { 256, 256 })
	return phaseComponent
end

return StandardPhase