local StandardCamera = record end

function StandardCamera.New(renderFlowComponent : RenderFlowComponent) : CameraComponent
	local camera = CameraComponentModule.New(renderFlowComponent, "CameraView")
	camera:SetProjectionJitter(true)
	camera:SetSmoothTrack(true)
	return camera
end

return StandardCamera