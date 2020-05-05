-- StandardRenderFlow.tl
-- 
local StandardRenderFlow = {}

function StandardRenderFlow.New(bloomLayerCount : integer)
	local renderFlowComponent = RenderFlowComponentModule.New()
	bloomLayerCount = math.max(bloomLayerCount, 1) -- at least one bloom layer
	local strBloomLayer = tostring(bloomLayerCount)

	-- Step 1: Setup Geometry stage
	local stageGeometryBuffer : RenderStage = renderFlowComponent:NewRenderStage("GeometryBufferRenderStage", "")
	local stageDeferredLighting : RenderStage = renderFlowComponent:NewRenderStage("DeferredLightingRenderStage", "")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "BaseColorOcclusion", stageDeferredLighting, "BaseColorOcclusion")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "NormalRoughnessMetallic", stageDeferredLighting, "NormalRoughnessMetallic")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageDeferredLighting, "Depth")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageDeferredLighting, "LoadDepth")

	-- Step 2: Set Light
	local tileLevel = 3
	local stageDepthBoundingSetup : RenderStage = renderFlowComponent:NewRenderStage("DepthBoundingSetupRenderStage", "1")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageDepthBoundingSetup, "InputDepth")

	local lastDepthStage = stageDepthBoundingSetup
	for i = 2, tileLevel do
		local stageDepthBounding : RenderStage = renderFlowComponent:NewRenderStage("DepthBoundingRenderStage", tostring(i))
		renderFlowComponent:LinkRenderStagePort(lastDepthStage, "OutputDepth", stageDepthBounding, "InputDepth")
		lastDepthStage = stageDepthBounding
	end

	local stageLightBuffer : RenderStage = renderFlowComponent:NewRenderStage("LightBufferRenderStage", tostring(tileLevel))
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "CameraView", stageLightBuffer, "CameraView")
	renderFlowComponent:LinkRenderStagePort(lastDepthStage, "OutputDepth", stageLightBuffer, "InputDepth")
	renderFlowComponent:LinkRenderStagePort(stageLightBuffer, "LightTexture", stageDeferredLighting, "LightTexture")
	renderFlowComponent:LinkRenderStagePort(stageLightBuffer, "LightSource", stageDeferredLighting, "LightSource")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "CameraView", stageDeferredLighting, "CameraView")

	local stageShadowMask : RenderStage = renderFlowComponent:NewRenderStage("ShadowMaskRenderStage", "1")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "CameraView", stageShadowMask, "CameraView")
	renderFlowComponent:LinkRenderStagePort(stageLightBuffer, "LightSource", stageShadowMask, "LightSource")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageShadowMask, "InputDepth")
	renderFlowComponent:LinkRenderStagePort(stageShadowMask, "OutputMask", stageDeferredLighting, "ShadowTexture")

	-- Step 3: Anti-Aliasing
	local stageColorBarrier : RenderStage = renderFlowComponent:NewRenderStage("FrameBarrierRenderStage", "")

	local stageAntiAliasing : RenderStage = renderFlowComponent:NewRenderStage("AntiAliasingRenderStage", "")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "CameraView", stageAntiAliasing, "CameraView")
	renderFlowComponent:LinkRenderStagePort(stageDeferredLighting, "OutputColor", stageAntiAliasing, "InputColor")
	renderFlowComponent:LinkRenderStagePort(stageAntiAliasing, "OutputColor", stageColorBarrier, "Front") -- chain
	renderFlowComponent:LinkRenderStagePort(stageColorBarrier, "Next", stageAntiAliasing, "LastInputColor")
	renderFlowComponent:LinkRenderStagePort(stageGeometryBuffer, "Depth", stageAntiAliasing, "Depth")

	-- Step 4: Apply HDR effects
	local stageScreen : RenderStage = renderFlowComponent:NewRenderStage("ScreenRenderStage", strBloomLayer)
	renderFlowComponent:LinkRenderStagePort(stageAntiAliasing, "OutputColor", stageScreen, "InputColor")

	-- Step 5: Prepare Bloom
	local lastStage = stageDeferredLighting
	for i = 0, bloomLayerCount - 1 do
		local stageBloom : RenderStage = renderFlowComponent:NewRenderStage("BloomRenderStage", tostring(i + 1))
		renderFlowComponent:LinkRenderStagePort(lastStage, "OutputColor", stageBloom, "InputColor")

		if i == 0 then
			renderFlowComponent:ExportRenderStagePort(stageBloom, "OutputColor", "PrevSceneColor")
		end

		renderFlowComponent:LinkRenderStagePort(stageBloom, "OutputColor", stageScreen, "BloomLayers[" .. tostring(i) .. "]")
		lastStage = stageBloom
	end

	-- Step 6: Write to output device
	local stageDevice : RenderStage = renderFlowComponent:NewRenderStage("DeviceRenderStage", "")
	renderFlowComponent:LinkRenderStagePort(stageScreen, "OutputColor", stageDevice, "InputColor")

	-- Step 7: Export symbols, notice that we only care abount scenes
	renderFlowComponent:ExportRenderStagePort(stageLightBuffer, "LightSource", "LightSource")
	renderFlowComponent:ExportRenderStagePort(stageGeometryBuffer, "Primitives", "Primitives")
	renderFlowComponent:ExportRenderStagePort(stageGeometryBuffer, "CameraView", "CameraView")
	renderFlowComponent:ExportRenderStagePort(stageScreen, "OutputColor", "SceneColor")
	return renderFlowComponent
end

return StandardRenderFlow