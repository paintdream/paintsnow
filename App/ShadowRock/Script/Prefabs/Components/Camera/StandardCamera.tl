local StandardCamera = {}
StandardCamera.__index = StandardCamera

function StandardCamera.New(renderFlowComponent : RenderFlowComponent)
	local camera = CameraComponentModule.New(renderFlowComponent, "CameraView")
	camera:SetProjectionJitter(true)
	camera:SetSmoothTrack(true)
	return camera
end

return StandardCamera