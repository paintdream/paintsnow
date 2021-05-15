// RayTraceComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "RayTraceComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class RayTraceComponent;
	class RayTraceComponentModule : public TReflected<RayTraceComponentModule, ModuleImpl<RayTraceComponent> > {
	public:
		RayTraceComponentModule(Engine& engine);
		~RayTraceComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create RayTraceComponent
		/// </summary>
		/// <returns> RayTraceComponent object </returns>
		TShared<RayTraceComponent> RequestNew(IScript::Request& request);

		/// <summary>
		/// Get completed pixel count of raytracing
		/// </summary>
		/// <param name="rayTraceComponent"> the RayTraceComponent</param>
		/// <returns> completed pixel count </returns>
		size_t RequestGetCompletedPixelCount(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent);

		/// <summary>
		/// Get capture size of RayTraceComponent
		/// </summary>
		/// <param name="rayTraceComponent"> the RayTraceComponent</param>
		/// <param name="size"> size in { x, y } </param>
		void RequestSetCaptureSize(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent, const UShort2& size);

		/// <summary>
		/// Get capture size of RayTraceComponent
		/// </summary>
		/// <param name="rayTraceComponent"> the RayTraceComponent</param>
		/// <returns> size in { x, y } </returns>
		const UShort2& RequestGetCaptureSize(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent);

		/// <summary>
		/// Start capture
		/// </summary>
		/// <param name="rayTraceComponent"> the RayTraceComponent</param>
		/// <param name="cameraComponent"> the reference camera </param>
		void RequestCapture(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent, IScript::Delegate<CameraComponent> cameraComponent);

		/// <summary>
		/// Get capture result texture
		/// </summary>
		/// <param name="rayTraceComponent"> the RayTraceComponent</param>
		/// <returns> result texture </returns>
		TShared<TextureResource> RequestGetCapturedTexture(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent);
	};
}

