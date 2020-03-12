local StandardPhase = {}

function StandardPhase.New(renderFlowComponent : RenderFlowComponent)
	local phaseComponent = PhaseComponentModule.New(renderFlowComponent, "PhaseView")
	return phaseComponent
end

return StandardPhase