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

using namespace PaintsNow;

String SnowyStream::reflectedExtension = "*.rds";

SnowyStream::SnowyStream(Interfaces& inters, BridgeSunset& bs, const TWrapper<IArchive*, IStreamBase&, size_t>& psubArchiveCreator, const String& dm, const TWrapper<void, const String&>& err) : interfaces(inters), bridgeSunset(bs), errorHandler(err), subArchiveCreator(psubArchiveCreator), defMount(dm), resourceQueue(nullptr) {
}

void SnowyStream::Initialize() {
	// Mount default drive
	if (!defMount.empty()) {
		IArchive& archive = interfaces.archive;
		uint64_t length = 0;
		uint64_t modifiedTime = 0;
		IStreamBase* stream = archive.Open(defMount, false, length, &modifiedTime);
		if (stream != nullptr) {
			TShared<File> file = TShared<File>::From(new File(stream, safe_cast<size_t>(length), modifiedTime));
			IArchive* ar = subArchiveCreator(*file->GetStream(), file->GetLength());
			defMountInstance = TShared<Mount>::From(new Mount(archive, "", ar, file));
		} else {
			fprintf(stderr, "Unable to mount default archive: %s\n", defMount.c_str());
		}
	}

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

		ReflectMethod(RequestMount)[ScriptMethod = "Mount"];
		ReflectMethod(RequestUnmount)[ScriptMethod = "Unmount"];
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
		request << StdToUtf8(value.asString());
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
			request << key(StdToUtf8(it.name()));
			WriteValue(request, *it);
		}
		request << endtable;
	}
}

void SnowyStream::RequestParseJson(IScript::Request& request, const String& str) {
	using namespace Json;

	Reader reader;
	Value document;
	
	if (reader.parse(Utf8ToStd(str), document)) {
		request.DoLock();
		WriteValue(request, document);
		request.UnLock();
	}
}

TShared<Mount>  SnowyStream::RequestMount(IScript::Request& request, const String& path, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);

	IArchive* ar = subArchiveCreator(*file->GetStream(), file->GetLength());
	TShared<Mount> mount = TShared<Mount>::From(new Mount(interfaces.archive, path, ar, file.Get()));

	return mount;
}

void SnowyStream::RequestUnmount(IScript::Request& request, IScript::Delegate<Mount> mount) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(mount);

	mount->Unmount();
}

void SnowyStream::RequestFileExists(IScript::Request& request, const String& path) {
	bridgeSunset.GetKernel().YieldCurrentWarp();

	IArchive& archive = interfaces.archive;
	bool ret = archive.Exists(path);

	request.DoLock();
	request << ret;
	request.UnLock();
}

void SnowyStream::RequestFetchFileData(IScript::Request& request, const String& path) {
	bridgeSunset.GetKernel().YieldCurrentWarp();
	uint64_t length;
	IArchive& archive = interfaces.archive;
	IStreamBase* stream = archive.Open(path, false, length);
	bool success = false;
	if (stream != nullptr) {
		String buf;
		size_t len = safe_cast<size_t>(length);
		buf.resize(len);

		if (stream->Read(const_cast<char*>(buf.data()), len)) {
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
	uint64_t length = 0;
	uint64_t modifiedTime = 0;
	IStreamBase* stream = path == ":memory:" ? new MemoryStream(0x1000, true) : archive.Open(path, write, length, &modifiedTime);
	if (stream != nullptr) {
		return TShared<File>::From(new File(stream, safe_cast<size_t>(length), modifiedTime));
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
	void Accept(const String& path) {
		request << path;
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
			resourceList[index] = snowyStream.CreateResource(pathList[index], resType, true);
			assert(resourceList[index]);
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
	TShared<ResourceBase> exist = resourceManager.LoadExistSafe(path);
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

	bool result = SaveResource(resource.Get(), extension);
	request.DoLock();
	request << result;
	request.UnLock();
}

struct CompressTask : public TaskOnce {
	CompressTask(SnowyStream& s) : snowyStream(s) {}
	void Execute(void* context) override {
		resource->Map();
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

		resource->Unmap();
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
	defMountInstance = nullptr;

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
void RegisterPass(ResourceManager& resourceManager, UniqueType<T> type, const String& matName = "", const IRender::Resource::RenderStateDescription& renderState = IRender::Resource::RenderStateDescription()) {
	ShaderResourceImpl<T>* shaderResource = new ShaderResourceImpl<T>(resourceManager, "", ResourceBase::RESOURCE_ETERNAL | ResourceBase::RESOURCE_VIRTUAL);
	PassBase& pass = shaderResource->GetPass();
	Unique unique = pass.GetUnique();
	assert(unique != UniqueType<PassBase>().Get());
	String name = pass.GetUnique()->GetName();
	auto pos = name.find_last_of(':');
	if (pos != String::npos) {
		name = name.substr(pos + 1);
	}

	resourceManager.DoLock();
	shaderResource->SetLocation(ShaderResource::GetShaderPathPrefix() + name);
	resourceManager.Insert(shaderResource);
	resourceManager.UnLock();

	if (!matName.empty()) {
		TShared<MaterialResource> materialResource = TShared<MaterialResource>::From(new MaterialResource(resourceManager, String("[Runtime]/MaterialResource/") + matName));
		materialResource->originalShaderResource = shaderResource;
		materialResource->materialParams.state = renderState;
		materialResource->Flag().fetch_or(ResourceBase::RESOURCE_ETERNAL | ResourceBase::RESOURCE_VIRTUAL, std::memory_order_release);

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

	IRender::Resource::RenderStateDescription state;
	state.depthTest = 0;
	RegisterPass(*resourceManager(), UniqueType<TextPass>(), "Text", state);
	RegisterPass(*resourceManager(), UniqueType<WidgetPass>(), "Widget", state);

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

bool SnowyStream::RegisterResourceSerializer(Unique unique, const String& extension, ResourceCreator* serializer) {
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
	std::unordered_map<String, std::pair<Unique, TShared<ResourceCreator> > >::iterator p = resourceSerializers.find(extension);
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.assetFilterBase;

	TShared<ResourceBase> resource;
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());
		ResourceManager& resourceManager = *(*t).second();

		resourceManager.DoLock();
		TShared<ResourceBase> existed = resourceManager.LoadExist(location);
		if (existed) {
			// Create failed, already exists
			resourceManager.UnLock();
			if (!openExisting) {
				return nullptr;
			} else {
				return existed;
			}
		}

		assert((path[0] != '/') == !!(flag & ResourceBase::RESOURCE_VIRTUAL));
		resource = (*p).second.second->Create(resourceManager, location);
		resource->Flag().fetch_or(flag, std::memory_order_relaxed);
		resourceManager.Insert(resource);
		resourceManager.UnLock();

		if (!(resource->Flag().load(std::memory_order_relaxed) & ResourceBase::RESOURCE_VIRTUAL)) {
			if (resource->Map()) {
				resourceManager.InvokeUpload(resource());
			}

			resource->Unmap();
		}
	}

	return resource;
}

String SnowyStream::GetReflectedExtension(Unique unique) {
	return unique->GetBriefName();
}

bool SnowyStream::LoadResource(const TShared<ResourceBase>& resource, const String& extension) {
	// Find resource serializer
	assert(resource);
	assert(resource->IsMapped());
	String typeExtension = extension.empty() ? GetReflectedExtension(resource->GetUnique()) : extension;
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.assetFilterBase;

	std::unordered_map<String, std::pair<Unique, TShared<ResourceCreator> > >::iterator p = resourceSerializers.find(typeExtension);
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());

		uint64_t length;
		IStreamBase* stream = archive.Open(resource->GetLocation() + "." + (*p).second.second->GetExtension() + (*t).second->GetLocationPostfix(), false, length);

		bool result = false;
		if (stream != nullptr) {
			IStreamBase* filter = protocol.CreateFilter(*stream);
			assert(filter != nullptr);
			SpinLock(resource->critical);
			result = *filter >> *resource();
			resource->Flag().fetch_or(Tiny::TINY_MODIFIED);
			SpinUnLock(resource->critical);
			filter->ReleaseObject();
			stream->ReleaseObject();

			ResourceManager& resourceManager = *(*t).second();
			resourceManager.InvokeRefresh(resource());
		}

		return result;
	} else {
		return false;
	}
}

bool SnowyStream::SaveResource(const TShared<ResourceBase>& resource, const String& extension) {
	// Find resource serializer
	assert(resource);
	assert(resource->IsMapped());
	String typeExtension = extension.empty() ? GetReflectedExtension(resource->GetUnique()) : extension;
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.assetFilterBase;

	std::unordered_map<String, std::pair<Unique, TShared<ResourceCreator> > >::iterator p = resourceSerializers.find(typeExtension);
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());

		uint64_t length;
		IStreamBase* stream = archive.Open(resource->GetLocation() + "." + (*p).second.second->GetExtension() + (*t).second->GetLocationPostfix(), true, length);

		bool result = false;
		if (stream != nullptr) {
			IStreamBase* filter = protocol.CreateFilter(*stream);
			assert(filter != nullptr);
			SpinLock(resource->critical);
			result = *filter << *resource();
			SpinUnLock(resource->critical);
			filter->ReleaseObject();
			stream->ReleaseObject();
		}

		return result;
	} else {
		return false;
	}
}

void SnowyStream::CreateBuiltinSolidTexture(const String& path, const UChar4& color) {
	IRender& render = interfaces.render;
	// Error Texture for missing textures ...
	TShared<TextureResource> errorTexture = CreateReflectedResource(UniqueType<TextureResource>(), path, false, ResourceBase::RESOURCE_ETERNAL | ResourceBase::RESOURCE_VIRTUAL);
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

void SnowyStream::CreateBuiltinMesh(const String& path, const Float3* vertices, size_t vertexCount, const UInt3* indices, size_t indexCount) {
	TShared<MeshResource> meshResource = CreateReflectedResource(UniqueType<MeshResource>(), path, false, ResourceBase::RESOURCE_ETERNAL | ResourceBase::RESOURCE_VIRTUAL);
	IRender& render = interfaces.render;

	if (meshResource) {
		meshResource->meshCollection.vertices.assign(vertices, vertices + vertexCount);
		meshResource->meshCollection.indices.assign(indices, indices + indexCount);

		// add mesh group
		IAsset::MeshGroup group;
		group.primitiveOffset = 0;
		group.primitiveCount = 2;
		meshResource->meshCollection.groups.emplace_back(group);
		meshResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		meshResource->GetResourceManager().InvokeUpload(meshResource());
	} else {
		errorHandler(String("Unable to create builtin mesh: ") + path);
	}
}

void SnowyStream::CreateBuiltinResources() {
	// MeshResource for widget rendering and deferred rendering ...
	static const Float3 quadVertices[] = {
		Float3(-1.0f, -1.0f, 0.0f),
		Float3(1.0f, -1.0f, 0.0f),
		Float3(1.0f, 1.0f, 0.0f),
		Float3(-1.0f, 1.0f, 0.0f),
	};

	static const UInt3 quadIndices[] = { UInt3(0, 1, 2), UInt3(2, 3, 0) };

	CreateBuiltinMesh("[Runtime]/MeshResource/StandardQuad", quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]));

	static const Float3 cubeVertices[] = {
		Float3(-1.0f, -1.0f, -1.0f),
		Float3(1.0f, -1.0f, -1.0f),
		Float3(1.0f, 1.0f, -1.0f),
		Float3(-1.0f, 1.0f, -1.0f),
		Float3(-1.0f, -1.0f, 1.0f),
		Float3(1.0f, -1.0f, 1.0f),
		Float3(1.0f, 1.0f, 1.0f),
		Float3(-1.0f, 1.0f, 1.0f),
	};

	static const UInt3 cubeIndices[] = {
		UInt3(0, 2, 1), UInt3(2, 0, 3), // bottom
		UInt3(0, 4, 7), UInt3(0, 7, 3), // left
		UInt3(0, 5, 4), UInt3(0, 1, 5), // back
		UInt3(4, 5, 6), UInt3(6, 7, 4), // top
		UInt3(1, 6, 5), UInt3(1, 2, 6), // right
		UInt3(3, 6, 2), UInt3(3, 7, 6), // front
	};

	CreateBuiltinMesh("[Runtime]/MeshResource/StandardCube", cubeVertices, sizeof(cubeVertices) / sizeof(cubeVertices[0]), cubeIndices, sizeof(cubeIndices) / sizeof(cubeIndices[0]));

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

String MetaResourceExternalPersist::GetUniqueName() const {
	return uniqueName;
}

MetaResourceExternalPersist::MetaResourceExternalPersist() {}