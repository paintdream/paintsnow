// ZRenderOpenGL.h -- OpenGL Render API provider
// By PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once
#include "../../../../Core/PaintsNow.h"
#include "../../../Interface/IRender.h"
#include "../../../../Core/Interface/IThread.h"
#include "../../../../Core/Interface/IReflect.h"
#include "../../../../Core/Template/TQueue.h"

namespace PaintsNow {
	class ZRenderOpenGL final : public IRender {
	public:
		ZRenderOpenGL();
		virtual ~ZRenderOpenGL();

		virtual std::vector<String> EnumerateDevices() override;
		virtual Device* CreateDevice(const String& description) override;
		virtual Int2 GetDeviceResolution(Device* device) override;
		virtual void SetDeviceResolution(Device* device, const Int2& resolution) override;
		virtual void NextDeviceFrame(Device* device) override;
		virtual void DeleteDevice(Device* device) override;

		virtual void PresentQueues(Queue** queues, uint32_t count, PresentOption option) override;
		virtual bool SupportParallelPresent(Device* device) override;

		// Queue
		virtual Queue* CreateQueue(Device* device, uint32_t flag) override;
		virtual Device* GetQueueDevice(Queue* queue) override;
		virtual void DeleteQueue(Queue* queue) override;
		virtual void FlushQueue(Queue* queue) override;
		virtual bool IsQueueModified(Queue* queue) override;

		// Resource
		virtual Resource* CreateResource(Device* device, Resource::Type resourceType) override;
		virtual void UploadResource(Queue* queue, Resource* resource, Resource::Description* description) override;
		virtual void AcquireResource(Queue* queue, Resource* resource) override;
		virtual void ReleaseResource(Queue* queue, Resource* resource) override;
		virtual void RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) override;
		virtual void CompleteDownloadResource(Queue* queue, Resource* resource) override;
		virtual void ExecuteResource(Queue* queue, Resource* resource) override;
		virtual void SwapResource(Queue* queue, Resource* lhs, Resource* rhs) override;
		virtual void DeleteResource(Queue* queue, Resource* resource) override;
		virtual void SetResourceNotation(Resource* lhs, const String& note) override;

	protected:
		void ClearDeletedQueues();

	protected:
		TQueueList<Queue*, 4> deletedQueues;
	};
}
