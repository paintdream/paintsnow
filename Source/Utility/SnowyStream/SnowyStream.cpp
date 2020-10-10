#define _CRT_SECURE_NO_WARNINGS
#include "SnowyStream.h"
#include "Resource/FontResource.h"
#include "Resource/MaterialResource.h"
#include "Resource/MeshResource.h"
#include "Resource/SkeletonResource.h"
#include "Resource/StreamResource.h"
#include "Resource/TextureResource.h"
#include "Resource/Passes/AntiAliasingPass.h"
#include "Resource/Passes/BloomPass.h"
#include "Resource/Passes/ConstMapPass.h"
#include "Resource/Passes/CustomMaterialPass.h"
#include "Resource/Passes/DeferredLightingPass.h"
#include "Resource/Passes/DepthResolvePass.h"
#include "Resource/Passes/DepthBoundingPass.h"
#include "Resource/Passes/DepthBoundingSetupPass.h"
#include "Resource/Passes/ForwardLightingPass.h"
#include "Resource/Passes/LightBufferPass.h"
#include "Resource/Passes/MultiHashSetupPass.h"
#include "Resource/Passes/MultiHashTracePass.h"
#include "Resource/Passes/ParticlePass.h"
#include "Resource/Passes/ScreenPass.h"
#include "Resource/Passes/ShadowMaskPass.h"
#include "Resource/Passes/StandardPass.h"
#include "Resource/Passes/TerrainPass.h"
#include "Resource/Passes/TextPass.h"
#include "Resource/Passes/VolumePass.h"
#include "Resource/Passes/WaterPass.h"
#include "Resource/Passes/WidgetPass.h"
#include "../../Core/System/MemoryStream.h"
#include "../../General/Driver/Filter/Json/Core/json.h"
#include <iterator>

using namespace PaintsNow;

String SnowyStream::reflectedExtension = "*.rds";

SnowyStream::SnowyStream(Interfaces& inters, BridgeSunset& bs, const TWrapper<IArchive*, IStreamBase&, size_t>& psubArchiveCreator, const TWrapper<void, const String&>& err) : interfaces(inters), bridgeSunset(bs), errorHandler(err), subArchiveCreator(psubArchiveCreator), resourceQueue(nullptr) {
}

void SnowyStream::Initialize() {
	assert(resourceManagers.empty());
	renderDevice = interfaces.render.CreateDevice("");
	resourceQueue = interfaces.render.CreateQueue(renderDevice, IRender::QUEUE_MULTITHREAD);
	RegisterReflectedSerializers();
	RegisterBuiltinPasses();

	CreateBuiltinResources();
}

void SnowyStream::TickDevice(IDevice& device) {
	if (&device == &interfaces.render) {
		if (resourceQueue != nullptr) {
			interfaces.render.PresentQueues(&resourceQueue, 1, IRender::PRESENT_EXECUTE_ALL);
		}
	}
}

SnowyStream::~SnowyStream() {}

Interfaces& SnowyStream::GetInterfaces() const {
	return interfaces;
}

void SnowyStream::Reset() {
}

TObject<IReflect>& SnowyStream::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(interfaces)[Runtime];
		ReflectProperty(errorHandler)[Runtime];
		ReflectProperty(resourceSerializers)[Runtime];
		ReflectProperty(resourceManagers)[Runtime];
	}

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewResource)[ScriptMethod = "NewResource"];
		ReflectMethod(RequestLoadExternalResourceData)[ScriptMethod = "LoadExternalResourceData"];
		ReflectMethod(RequestPersistResource)[ScriptMethod = "PersistResource"];
		ReflectMethod(RequestCloneResource)[ScriptMethod = "CloneResource"];
		ReflectMethod(RequestMapResource)[ScriptMethod = "MapResource"];
		ReflectMethod(RequestUnmapResource)[ScriptMethod = "UnmapResource"];
		ReflectMethod(RequestInspectResource)[ScriptMethod = "InspectResource"];
		ReflectMethod(RequestCompressResourceAsync)[ScriptMethod = "CompressResourceAsync"];
		ReflectMethod(RequestNewResourcesAsync)[ScriptMethod = "NewResourcesAsync"];

		ReflectMethod(RequestParseJson)[ScriptMethod = "ParseJson"];
		ReflectMethod(RequestNewFile)[ScriptMethod = "NewFile"];
		ReflectMethod(RequestDeleteFile)[ScriptMethod = "DeleteFile"];
		ReflectMethod(RequestFlushFile)[ScriptMethod = "FlushFile"];
		ReflectMethod(RequestReadFile)[ScriptMethod = "ReadFile"];
		ReflectMethod(RequestGetFileSize)[ScriptMethod = "GetFileSize"];
		ReflectMethod(RequestCloseFile)[ScriptMethod = "CloseFile"];
		ReflectMethod(RequestGetFileLastModifiedTime)[ScriptMethod = "GetFileLastModifiedTime"];
		ReflectMethod(RequestWriteFile)[ScriptMethod = "WriteFile"];
		ReflectMethod(RequestSeekFile)[ScriptMethod = "SeekFile"];
		ReflectMethod(RequestQueryFiles)[ScriptMethod = "QueryFiles"];
		ReflectMethod(RequestFetchFileData)[ScriptMethod = "FetchFileData"];
		ReflectMethod(RequestFileExists)[ScriptMethod = "FileExists"];

		ReflectMethod(RequestNewZipper)[ScriptMethod = "NewZipper"];
		ReflectMethod(RequestPostZipperData)[ScriptMethod = "PostZipperData"];
		ReflectMethod(RequestWriteZipper)[ScriptMethod = "WriteZipper"];
	}

	return *this;
}

static void WriteValue(IScript::Request& request, const Json::Value& value) {
	using namespace Json;
	Value::const_iterator it;
	size_t i;

	switch (value.type()) {
	case nullValue:
		request << nil;
		break;
	case intValue:
		request << (int64_t)value.asInt64();
		break;
	case uintValue:
		request << (uint64_t)value.asUInt64();
		break;
	case realValue:
		request << value.asDouble();
		break;
	case stringValue:
		request << String(value.asString());
		break;
	case booleanValue:
		request << value.asBool();
		break;
	case arrayValue:
		request << beginarray;
		for (i = 0; i < value.size(); i++) {
			WriteValue(request, value[(Value::ArrayIndex)i]);
		}
		request << endarray;
		break;
	case objectValue:
		request << begintable;
		for (it = value.begin(); it != value.end(); ++it) {
			request << key(it.name());
			WriteValue(request, *it);
		}
		request << endtable;
	}
}

void SnowyStream::RequestParseJson(IScript::Request& request, const String& str) {
	using namespace Json;

	Reader reader;
	Value document;
	
	if (reader.parse(str, document)) {
		request.DoLock();
		WriteValue(request, document);
		request.UnLock();
	}
}

TShared<Zipper> SnowyStream::RequestNewZipper(IScript::Request& request, const String& path) {
	if (subArchiveCreator) {
		bridgeSunset.GetKernel().YieldCurrentWarp();
		size_t length;
		IStreamBase* stream = interfaces.archive.Open(path, false, length);
		if (stream != nullptr) {
			IArchive* a = subArchiveCreator(*stream, length);
			if (a != nullptr) {
				return TShared<Zipper>::From(new Zipper(a, stream));
			} else {
				delete a;
				request.Error("SnowyStream::CreateZipper(): Cannot open stream.");
			}
		} else {
			request.Error("SnowyStream::CreateZipper(): Cannot open archive.");
		}
	} else {
		request.Error("SnowyStream::CreateZipper(): No archive creator found.");
	}

	return nullptr;
}

void SnowyStream::RequestPostZipperData(IScript::Request& request, IScript::Delegate<Zipper> zipper, const String& path, const String& data) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(zipper);

	// TODO:
}

void SnowyStream::RequestWriteZipper(IScript::Request& request, IScript::Delegate<File> file, IScript::Delegate<Zipper> zipper) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	CHECK_DELEGATE(zipper);

	// TODO:
}

void SnowyStream::RequestFileExists(IScript::Request& request, const String& path) {
	bridgeSunset.GetKernel().YieldCurrentWarp();

	IArchive& archive = interfaces.archive;
	size_t length;
	IStreamBase* stream = archive.Open(path, false, length);
	bool ret = stream != nullptr;
	if (ret) {
		stream->ReleaseObject();
	} 

	request.DoLock();
	request << ret;
	request.UnLock();
}

void SnowyStream::RequestFetchFileData(IScript::Request& request, const String& path) {
	bridgeSunset.GetKernel().YieldCurrentWarp();
	size_t length;
	IArchive& archive = interfaces.archive;
	IStreamBase* stream = archive.Open(path, false, length);
	bool success = false;
	if (stream != nullptr) {
		String buf;
		buf.resize(length);
		if (stream->Read(const_cast<char*>(buf.data()), length)) {
			stream->ReleaseObject();
			request.DoLock();
			request << buf;
			request.UnLock();
		}
	}
}

void SnowyStream::RequestCloseFile(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	bridgeSunset.GetKernel().YieldCurrentWarp();
	file->Close();
}

void SnowyStream::RequestDeleteFile(IScript::Request& request, const String& path) {
	CHECK_REFERENCES_NONE();

	interfaces.archive.Delete(path);
}

TShared<File> SnowyStream::RequestNewFile(IScript::Request& request, const String& path, bool write) {
	CHECK_REFERENCES_NONE();
	bridgeSunset.GetKernel().YieldCurrentWarp();
	IArchive& archive = interfaces.archive;
	size_t length = 0;
	uint64_t modifiedTime = 0;
	IStreamBase* stream = path == ":memory:" ? new MemoryStream(0x1000, true) : archive.Open(path, write, length, &modifiedTime);
	if (stream != nullptr) {
		return TShared<File>::From(new File(stream, length, modifiedTime));
	} else {
		return nullptr;
	}
}

uint64_t SnowyStream::RequestGetFileLastModifiedTime(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	return file->GetLastModifiedTime();
}

uint64_t SnowyStream::RequestGetFileSize(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	return file->GetLength();
}

void SnowyStream::RequestReadFile(IScript::Request& request, IScript::Delegate<File> file, int64_t length, IScript::Request::Ref callback) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	if (callback) {
		CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	}
	bridgeSunset.GetKernel().YieldCurrentWarp();
	IStreamBase* stream = file->GetStream();
	if (stream != nullptr) {
		String content;
		size_t len = (size_t)length;
		content.resize(len);
		if (stream->Read(const_cast<char*>(content.data()), len)) {
			request.DoLock();
			if (callback) {
				request.Call(deferred, callback, content);
			} else {
				request << content;
			}
			request.UnLock();
		}
	} else {
		request.Error("SnowyStream::ReadFile() : File already closed.");
	}
}

void SnowyStream::RequestWriteFile(IScript::Request& request, IScript::Delegate<File> file, const String& content, IScript::Request::Ref callback) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	if (callback) {
		CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	}
	bridgeSunset.GetKernel().YieldCurrentWarp();

	IStreamBase* stream = file->GetStream();
	if (stream != nullptr) {
		size_t len = content.size();
		if (stream->Write(content.data(), len)) {
			request.DoLock();
			if (callback) {
				request.Call(deferred, callback, len);
			} else {
				request << len;
			}
			request.UnLock();
		}
	} else {
		request.Error("SnowyStream::WriteFile() : File already closed.");
	}
}

void SnowyStream::RequestFlushFile(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	IStreamBase* stream = file->GetStream();
	if (stream != nullptr) {
		stream->Flush();
	} else {
		request.Error("SnowyStream::FlushFile() : File already closed.");
	}
}

void SnowyStream::RequestSeekFile(IScript::Request& request, IScript::Delegate<File> file, const String& type, int64_t offset) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	bridgeSunset.GetKernel().YieldCurrentWarp();
	IStreamBase* stream = file->GetStream();
	if (stream != nullptr) {
		IStreamBase::SEEK_OPTION option = IStreamBase::CUR;
		if (type == "Current") {
			option = IStreamBase::CUR;
		} else if (type == "Begin") {
			option = IStreamBase::BEGIN;
		} else if (type == "End") {
			option = IStreamBase::END;
		}

		if (stream->Seek(option, (long)offset)) {
			request.DoLock();
			request << true;
			request.UnLock();
		}
	} else {
		request.Error("SnowyStream::SeekFile() : File already closed.");
	}
}

struct QueryHandler {
	QueryHandler(const String& p, IScript::Request& r) : prefix(p), request(r) {}
	void Accept(bool isDir, const String& path) {
		String t = path;
		if (isDir) t += "/";
		request << t;
	}

	const String prefix;
	IScript::Request& request;
};

bool SnowyStream::FilterPath(const String& path) {
	return path.find("..") == String::npos;
}

void SnowyStream::RequestQueryFiles(IScript::Request& request, const String& p) {
	if (!FilterPath(p)) return;

	String path = p;
	char ch = path[path.length() - 1];
	if (ch != '/' && ch != '\\') {
		path += "/";
	}

	IArchive& archive = interfaces.archive;
	QueryHandler handler(path, request);
	bridgeSunset.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << beginarray;
	archive.Query(path, Wrap(&handler, &QueryHandler::Accept));
	request << endarray;
	request.UnLock();
}

TShared<ResourceBase> SnowyStream::RequestNewResource(IScript::Request& request, const String& path, const String& expectedResType, bool createAlways) {
	TShared<ResourceBase> resource = CreateResource(path, expectedResType, !createAlways);
	bridgeSunset.GetKernel().YieldCurrentWarp();

	if (resource) {
		if (!(resource->Flag() & ResourceBase::RESOURCE_UPLOADED) && !(resource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release) & Tiny::TINY_MODIFIED)) {
			assert(resource->Flag() & Tiny::TINY_MODIFIED);
			resource->GetResourceManager().InvokeUpload(resource());
		}

		return resource;
	} else {
		request.Error(String("Unable to create resource ") + path);
		return nullptr;
	}
}

class TaskResourceCreator final : public TReflected<TaskResourceCreator, SharedTiny> {
public:
	TaskResourceCreator(BridgeSunset& bs, SnowyStream& ss, rvalue<std::vector<String> > pl, rvalue<String> type, IScript::Request::Ref step, IScript::Request::Ref complete) : bridgeSunset(bs), snowyStream(ss), callbackStep(step), callbackComplete(complete), resType(std::move(type)) {
		std::swap(pathList, (std::vector<String>&)pl);
		resourceList.resize(pathList.size());
		completed.store(0, std::memory_order_release);
	}
	
	void Start(IScript::Request& request) {
		ThreadPool& threadPool = bridgeSunset.GetKernel().threadPool;
		ReferenceObject();
		for (uint32_t i = 0; i < safe_cast<uint32_t>(pathList.size()); i++) {
			// Create async task and use low-level thread pool dispatching it
			threadPool.Push(CreateTask(Wrap(this, &TaskResourceCreator::RoutineCreateResource), i));
		}
	}

private:
	inline void Finalize(IScript::Request& request) {
		request.DoLock();
		if (callbackComplete) {
			request.Push();
			request.Call(sync, callbackComplete, resourceList);
			request.Pop();
			request.Dereference(callbackComplete);
		}
		request.Dereference(callbackStep);
		request.UnLock();
	}

	inline void RoutineCreateResource(void* context, bool run, uint32_t index) {
		if (run) {
			TShared<ResourceBase> resource = resourceList[index] = snowyStream.CreateResource(pathList[index], resType, true);
			if (resource && !(resource->Flag() & ResourceBase::RESOURCE_UPLOADED) && !(resource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release) & Tiny::TINY_MODIFIED)) {
				assert(resource->Flag() & Tiny::TINY_MODIFIED);
				resource->GetResourceManager().InvokeUpload(resource());
			}
		}

		if (callbackStep) {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& request = *bridgeSunset.AcquireSafe();
			request.DoLock();
			request.Push();
			request.Call(sync, callbackStep, pathList[index], resourceList[index]);
			request.Pop();
			request.UnLock();
			bridgeSunset.ReleaseSafe(&request);
		}

		// is abount to finish
		if (completed.fetch_add(1, std::memory_order_release) + 1 == pathList.size()) {
#ifdef _DEBUG
			for (size_t k = 0; k < resourceList.size(); k++) {
				if (resourceList[k]) {
					assert(resourceList[k]->Flag() & (Tiny::TINY_MODIFIED | ResourceBase::RESOURCE_UPLOADED));
				}
			}
#endif
			assert(context != nullptr);
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& request = *bridgeSunset.AcquireSafe();
			Finalize(request);
			bridgeSunset.ReleaseSafe(&request);

			ReleaseObject();
		}
	}

	std::vector<String> pathList;
	std::vector<TShared<ResourceBase> > resourceList;
	std::atomic<uint32_t> completed;

	String resType;
	BridgeSunset& bridgeSunset;
	SnowyStream& snowyStream;
	IScript::Request::Ref callbackStep;
	IScript::Request::Ref callbackComplete;
};

void SnowyStream::RequestNewResourcesAsync(IScript::Request& request, std::vector<String>& pathList, String& expectedResType, IScript::Request::Ref callbackStep, IScript::Request::Ref callbackComplete) {
	Kernel& kernel = bridgeSunset.GetKernel();
	kernel.YieldCurrentWarp();

	TShared<TaskResourceCreator> taskResourceCreator = TShared<TaskResourceCreator>::From(new TaskResourceCreator(bridgeSunset, *this, std::move(pathList), std::move(expectedResType), callbackStep, callbackComplete));
	taskResourceCreator->Start(request);
}

void SnowyStream::RequestLoadExternalResourceData(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& externalData) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);
	bridgeSunset.GetKernel().YieldCurrentWarp();

	MemoryStream ms(externalData.size());
	size_t len = externalData.size();
	ms.Write(externalData.c_str(), len);
	ms.Seek(IStreamBase::BEGIN, 0);
	bool result = resource->LoadExternalResource(interfaces, ms, len);

	if (result) {
		resource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		resource->GetResourceManager().InvokeUpload(resource.Get(), resource->GetResourceManager().GetContext());
	}

	request.DoLock();
	request << result;
	request.UnLock();
}

void SnowyStream::RequestInspectResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);
	bridgeSunset.GetKernel().YieldCurrentWarp();

	Tiny::FLAG flag = resource->Flag().load(std::memory_order_relaxed);
	std::vector<ResourceBase::Dependency> dependencies;
	resource->GetDependencies(dependencies);

	request.DoLock();
	request << begintable
		<< key("Flag") << flag
		<< key("Path") << resource->GetLocation()
		<< key("Depends") << begintable;

	for (size_t k = 0; k < dependencies.size(); k++) {
		ResourceBase::Dependency& d = dependencies[k];
		request << key("Key") << d.value;
	}

	request << endtable << endtable;
	request.UnLock();
}

TShared<ResourceBase> SnowyStream::RequestCloneResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& path) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);
	
	ResourceManager& resourceManager = resource->GetResourceManager();
	TShared<ResourceBase> exist = resourceManager.SafeLoadExist(path);
	if (!exist) {
		TShared<ResourceBase> p = TShared<ResourceBase>::From(static_cast<ResourceBase*>(resource->Clone()));
		if (p) {
			p->SetLocation(path);
			resourceManager.Insert(p);
			return p;
		}
	} else {
		request.Error(String("SnowyStream::CloneResource() : The path ") + path + " already exists");
	}

	return nullptr;
}

void SnowyStream::RequestMapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);
	bridgeSunset.GetKernel().YieldCurrentWarp();

	bool result = resource->Map();

	request.DoLock();
	request << result;
	request.UnLock();
}

void SnowyStream::RequestUnmapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);

	resource->Unmap();
}

void SnowyStream::RequestPersistResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& extension) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);
	bridgeSunset.GetKernel().YieldCurrentWarp();

	bool result = PersistResource(resource.Get(), extension);
	request.DoLock();
	request << result;
	request.UnLock();
}

struct CompressTask : public TaskOnce {
	CompressTask(SnowyStream& s) : snowyStream(s) {}
	void Execute(void* context) override {
		snowyStream.MapResource(resource);
		bool success = resource->Compress(compressType);
		if (callback) {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& request = *bridgeSunset.AcquireSafe();
			request.DoLock();
			request.Push();
			request.Call(sync, callback, success);
			request.Pop();
			request.Dereference(callback);
			request.UnLock();
			bridgeSunset.ReleaseSafe(&request);
		} else {
			Finalize(context);
		}

		snowyStream.UnmapResource(resource);
		delete this;
	}

	void Abort(void* context) override {
		Finalize(context);
		TaskOnce::Abort(context);
	}

	void Finalize(void* context) {
		if (callback) {
			BridgeSunset& bridgeSunset = *reinterpret_cast<BridgeSunset*>(context);
			IScript::Request& request = *bridgeSunset.AcquireSafe();
			request.DoLock();
			request.Dereference(callback);
			request.UnLock();
			bridgeSunset.ReleaseSafe(&request);
		}
	}

	SnowyStream& snowyStream;
	TShared<ResourceBase> resource;
	String compressType;
	IScript::Request::Ref callback;
};

void SnowyStream::RequestCompressResourceAsync(IScript::Request& request, IScript::Delegate<ResourceBase> resource, String& compressType, IScript::Request::Ref callback) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);
	bridgeSunset.GetKernel().YieldCurrentWarp();
	CompressTask* task = new CompressTask(*this);
	task->resource = resource.Get();
	task->compressType = std::move(compressType);
	task->callback = callback;

	bridgeSunset.GetKernel().threadPool.Push(task);
}

void SnowyStream::Uninitialize() {
	for (std::map<Unique, TShared<ResourceManager> >::const_iterator p = resourceManagers.begin(); p != resourceManagers.end(); ++p) {
		(*p).second->DoLock();
		(*p).second->RemoveAll();
		(*p).second->UnLock();
	}

	interfaces.render.DeleteQueue(resourceQueue);
	resourceQueue = nullptr;
	interfaces.render.DeleteDevice(renderDevice);
	renderDevice = nullptr;
}

template <class T>
void RegisterPass(ResourceManager& resourceManager, UniqueType<T> type, const String& matName = "") {
	ShaderResourceImpl<T>* shaderResource = new ShaderResourceImpl<T>(resourceManager, "", ResourceBase::RESOURCE_ETERNAL);
	PassBase& pass = shaderResource->GetPass();
	Unique unique = pass.GetUnique();
	assert(unique != UniqueType<PassBase>().Get());
	String name = pass.GetUnique()->GetName();
	auto pos = name.find_last_of(':');
	if (pos != std::string::npos) {
		name = name.substr(pos + 1);
	}

	resourceManager.DoLock();
	shaderResource->SetLocation(ShaderResource::GetShaderPathPrefix() + name);
	resourceManager.Insert(shaderResource);
	resourceManager.UnLock();

	if (!matName.empty()) {
		TShared<MaterialResource> materialResource = TShared<MaterialResource>::From(new MaterialResource(resourceManager, String("[Runtime]/MaterialResource/") + matName));
		materialResource->originalShaderResource = shaderResource;
		materialResource->Flag().fetch_or(ResourceBase::RESOURCE_ETERNAL, std::memory_order_acquire);
		resourceManager.DoLock();
		resourceManager.Insert(materialResource());
		resourceManager.UnLock();
	}
	shaderResource->ReleaseObject();
}

void SnowyStream::RegisterBuiltinPasses() {
	TShared<ResourceManager> resourceManager = resourceManagers[UniqueType<IRender>::Get()];
	assert(resourceManager);

	RegisterPass(*resourceManager(), UniqueType<AntiAliasingPass>());
	RegisterPass(*resourceManager(), UniqueType<BloomPass>());
	RegisterPass(*resourceManager(), UniqueType<ConstMapPass>());
	RegisterPass(*resourceManager(), UniqueType<CustomMaterialPass>());
	RegisterPass(*resourceManager(), UniqueType<DeferredLightingPass>());
	RegisterPass(*resourceManager(), UniqueType<DepthResolvePass>());
	RegisterPass(*resourceManager(), UniqueType<DepthBoundingPass>());
	RegisterPass(*resourceManager(), UniqueType<DepthBoundingSetupPass>());
	RegisterPass(*resourceManager(), UniqueType<LightBufferPass>());
	RegisterPass(*resourceManager(), UniqueType<MultiHashSetupPass>());
	RegisterPass(*resourceManager(), UniqueType<MultiHashTracePass>());
	RegisterPass(*resourceManager(), UniqueType<ScreenPass>());
	RegisterPass(*resourceManager(), UniqueType<ShadowMaskPass>());
	RegisterPass(*resourceManager(), UniqueType<StandardPass>());
	RegisterPass(*resourceManager(), UniqueType<TextPass>(), "Text");
	RegisterPass(*resourceManager(), UniqueType<WidgetPass>(), "Widget");

	/*
	RegisterPass(*resourceManager(), UniqueType<ForwardLightingPass>());
	RegisterPass(*resourceManager(), UniqueType<ParticlePass>());
	RegisterPass(*resourceManager(), UniqueType<TerrainPass>());
	RegisterPass(*resourceManager(), UniqueType<VolumePass>());
	RegisterPass(*resourceManager(), UniqueType<WaterPass>());*/
}

IRender::Queue* SnowyStream::GetResourceQueue() {
	return resourceQueue;
}

void SnowyStream::RegisterReflectedSerializers() {
	assert(resourceSerializers.empty()); // can only be registerred once

	// Commented resources may have dependencies, so we do not serialize/deserialize them at once.
	// PaintsNow recommends database-managed resource dependencies ...

	RegisterReflectedSerializer(UniqueType<FontResource>(), interfaces.fontBase, this);
	RegisterReflectedSerializer(UniqueType<MaterialResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<ShaderResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<MeshResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<SkeletonResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<TextureResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<StreamResource>(), interfaces.archive, nullptr);
}

bool SnowyStream::RegisterResourceManager(Unique unique, ResourceManager* resourceManager) {
	if (resourceManagers.count(unique) == 0) {
		resourceManagers[unique] = resourceManager;
		return true;
	} else {
		return false;
	}
}

bool SnowyStream::RegisterResourceSerializer(Unique unique, const String& extension, ResourceSerializerBase* serializer) {
	if (resourceSerializers.count(extension) == 0) {
		resourceSerializers[extension] = std::make_pair(unique, serializer);
		return true;
	} else {
		return false;
	}
}

TShared<ResourceBase> SnowyStream::CreateResource(const String& path, const String& ext, bool openExisting, Tiny::FLAG flag, IStreamBase* sourceStream) {
	assert(!path.empty());
	// try to parse extension from location if no ext provided
	String location;
	String extension;
	if (ext.empty()) {
		String::size_type pos = path.rfind('.');
		if (pos == String::npos) return nullptr; // unknown resource type

		location = path.substr(0, pos);
		extension = path.substr(pos + 1);
	} else {
		location = path;
		extension = ext;
	}

	// Find resource serializer
	std::unordered_map<String, std::pair<Unique, TShared<ResourceSerializerBase> > >::iterator p = resourceSerializers.find(extension);
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.assetFilterBase;

	TShared<ResourceBase> resource;
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());
		if (sourceStream == nullptr) {
			resource = (*p).second.second->DeserializeFromArchive(*(*t).second(), archive, location, protocol, openExisting, flag);
		} else {
			assert(!openExisting);
			resource = (*p).second.second->Deserialize(*(*t).second(), location, protocol, flag, sourceStream);
		}
	} else {
		return nullptr; // unknown resource type
	}

	return resource;
}

String SnowyStream::GetReflectedExtension(Unique unique) {
	return unique->GetBriefName();
}

void SnowyStream::UnmapResource(const TShared<ResourceBase>& resource) {
	// Find resource serializer
	assert(resource);
	resource->Unmap();
}

bool SnowyStream::MapResource(const TShared<ResourceBase>& resource, const String& extension) {
	// Find resource serializer
	assert(resource);
	String typeExtension = extension.empty() ? GetReflectedExtension(resource->GetUnique()) : extension;
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.assetFilterBase;

	std::unordered_map<String, std::pair<Unique, TShared<ResourceSerializerBase> > >::iterator p = resourceSerializers.find(typeExtension);
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());
		return (*p).second.second->MapFromArchive(resource(), archive, protocol, resource->GetLocation());
	} else {
		return false;
	}
}

bool SnowyStream::PersistResource(const TShared<ResourceBase>& resource, const String& extension) {
	// Find resource serializer
	assert(resource);
	String typeExtension = extension.empty() ? GetReflectedExtension(resource->GetUnique()) : extension;
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.assetFilterBase;

	std::unordered_map<String, std::pair<Unique, TShared<ResourceSerializerBase> > >::iterator p = resourceSerializers.find(typeExtension);
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());
		return (*p).second.second->SerializeToArchive(resource(), archive, protocol, resource->GetLocation());
	} else {
		return false;
	}
}

void SnowyStream::CreateBuiltinSolidTexture(const String& path, const UChar4& color) {
	IRender& render = interfaces.render;
	// Error Texture for missing textures ...
	TShared<TextureResource> errorTexture = CreateReflectedResource(UniqueType<TextureResource>(), path, false, ResourceBase::RESOURCE_ETERNAL);
	if (errorTexture) {
		errorTexture->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		errorTexture->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		// 2x2 pixel
		const int width = 2, height = 2;
		errorTexture->description.dimension.x() = width;
		errorTexture->description.dimension.y() = height;
		errorTexture->description.data.Resize(width * height * sizeof(UChar4));

		UChar4* buffer = reinterpret_cast<UChar4*>(errorTexture->description.data.GetData());
		std::fill(buffer, buffer + width * height, color);

		errorTexture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		errorTexture->GetResourceManager().InvokeUpload(errorTexture());
	} else {
		errorHandler("Unable to create error texture ...");
	}
}

void SnowyStream::CreateBuiltinResources() {
	// MeshResource for widget rendering and deferred rendering ...
	TShared<MeshResource> meshResource = CreateReflectedResource(UniqueType<MeshResource>(), "[Runtime]/MeshResource/StandardSquare", false, ResourceBase::RESOURCE_ETERNAL);
	IRender& render = interfaces.render;

	if (meshResource) {
		static const Float3 vertices[] = {
			Float3(-1.0f, -1.0f, 0.0f),
			Float3(1.0f, -1.0f, 0.0f),
			Float3(1.0f, 1.0f, 0.0f),
			Float3(-1.0f, 1.0f, 0.0f),
		};

		/*
		static const Float4 texCoords[] = {
			Float4(0.0f, 0.0f, 0.0f, 0.0f),
			Float4(1.0f, 0.0f, 1.0f, 0.0f),
			Float4(1.0f, 1.0f, 1.0f, 1.0f),
			Float4(0.0f, 1.0f, 0.0f, 1.0f)
		};*/

		static const UInt3 indices[] = { UInt3(0, 1, 2), UInt3(2, 3, 0) };
		// No UVs
		std::copy(vertices, vertices + sizeof(vertices) / sizeof(vertices[0]), std::back_inserter(meshResource->meshCollection.vertices));
		std::copy(indices, indices + sizeof(indices) / sizeof(indices[0]), std::back_inserter(meshResource->meshCollection.indices));

		// add mesh group
		IAsset::MeshGroup group;
		group.primitiveOffset = 0;
		group.primitiveCount = 2;
		meshResource->meshCollection.groups.emplace_back(group);
		meshResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		meshResource->GetResourceManager().InvokeUpload(meshResource());
	} else {
		errorHandler("Unable to create builtin mesh ...");
	}

	CreateBuiltinSolidTexture("[Runtime]/TextureResource/Black", UChar4(0, 0, 0, 0));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/White", UChar4(255, 255, 255, 255));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/MissingBaseColor", UChar4(255, 0, 255, 255));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/MissingNormal", UChar4(127, 127, 255, 255));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/MissingMaterial", UChar4(255, 255, 0, 0));
}

IRender::Device* SnowyStream::GetRenderDevice() const {
	return renderDevice;
}

IReflectObject* MetaResourceExternalPersist::Clone() const {
	return new MetaResourceExternalPersist(*this);
}

bool MetaResourceExternalPersist::Read(IStreamBase& streamBase, void* ptr) const {
	IReflectObject& env = streamBase.GetEnvironment();
	assert(!env.IsBasicObject());
	assert(env.QueryInterface(UniqueType<SnowyStream>()) != nullptr);

	SnowyStream& snowyStream = static_cast<SnowyStream&>(env);
	TShared<ResourceBase>& resource = *reinterpret_cast<TShared<ResourceBase>*>(ptr);
	String path;
	if (streamBase >> path) {
		resource = snowyStream.CreateResource(path);
		if (resource && !(resource->Flag() & ResourceBase::RESOURCE_UPLOADED) && !(resource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release) & Tiny::TINY_MODIFIED)) {
			assert(resource->Flag() & Tiny::TINY_MODIFIED);
			resource->GetResourceManager().InvokeUpload(resource());
		}
	}

	return resource;
}

bool MetaResourceExternalPersist::Write(IStreamBase& streamBase, const void* ptr) const {
	IReflectObject& env = streamBase.GetEnvironment();
	assert(!env.IsBasicObject());
	assert(env.QueryInterface(UniqueType<SnowyStream>()) != nullptr);

	const TShared<ResourceBase>& resource = *reinterpret_cast<const TShared<ResourceBase>*>(ptr);
	String path;
	if (resource) {
		path = resource->GetLocation() + "." + resource->GetUnique()->GetBriefName();
	}

	return streamBase << path;
}

const String& MetaResourceExternalPersist::GetUniqueName() const {
	return uniqueName;
}

MetaResourceExternalPersist::MetaResourceExternalPersist() {}