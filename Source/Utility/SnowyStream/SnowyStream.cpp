#define _CRT_SECURE_NO_WARNINGS
#include "SnowyStream.h"
#include "Resource/AudioResource.h"
#include "Resource/FontResource.h"
#include "Resource/MaterialResource.h"
#include "Resource/MeshResource.h"
#include "Resource/SkeletonResource.h"
#include "Resource/TapeResource.h"
#include "Resource/TextResource.h"
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
#include "Resource/Passes/VolumePass.h"
#include "Resource/Passes/WaterPass.h"
#include "Resource/Passes/WidgetPass.h"
#include "../../General/Misc/ZMemoryStream.h"
#include <iterator>

using namespace PaintsNow;
using namespace NsSnowyStream;
using namespace NsBridgeSunset;

String SnowyStream::reflectedExtension = "*.rds";

SnowyStream::SnowyStream(Interfaces& inters, BridgeSunset& bs, const TWrapper<IArchive*, IStreamBase&, size_t>& psubArchiveCreator, const TWrapper<void, const String&>& err) : interfaces(inters), bridgeSunset(bs), errorHandler(err), subArchiveCreator(psubArchiveCreator), resourceQueue(nullptr) {
}

void SnowyStream::Initialize() {
	assert(resourceManagers.empty());
	renderDevice = interfaces.render.CreateDevice("");
	resourceQueue = interfaces.render.CreateQueue(renderDevice, true);
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
		// TODO: Generate correct inspection ...
		ReflectMethod(RequestNewResourcesAsync)[ScriptMethod = "NewResourcesAsync"];
		ReflectMethod(RequestImportResourceConfig)[ScriptMethod = "ImportResourceConfig"];
		ReflectMethod(RequestExportResourceConfig)[ScriptMethod = "ExportResourceConfig"];

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

		ReflectMethod(RequestSetShaderResourceCode)[ScriptMethod = "SetShaderResourceCode"];
		ReflectMethod(RequestSetShaderResourceInput)[ScriptMethod = "SetShaderResourceInput"];
		ReflectMethod(RequestSetShaderResourceComplete)[ScriptMethod = "SetShaderResourceComplete"];
	}

	return *this;
}

void SnowyStream::RequestImportResourceConfig(IScript::Request& request, std::vector<std::pair<String, String> >& config) {
	interfaces.filterBase.ImportConfig(config);
}

void SnowyStream::RequestExportResourceConfig(IScript::Request& request) {
	std::vector<std::pair<String, String> > config;
	interfaces.filterBase.ExportConfig(config);

	bridgeSunset.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << config;
	request.UnLock();
}

void SnowyStream::RequestNewZipper(IScript::Request& request, const String& path) {
	if (subArchiveCreator) {
		bridgeSunset.GetKernel().YieldCurrentWarp();
		size_t length;
		IStreamBase* stream = interfaces.archive.Open(path, false, length);
		if (stream != nullptr) {
			IArchive* a = subArchiveCreator(*stream, length);
			if (a != nullptr) {
				Zipper* zipper = new Zipper(a, stream);
				request.DoLock();
				request << zipper;
				request.UnLock();
				zipper->ReleaseObject();
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
	} else {
		size_t length;
		ret = request.GetScript()->QueryUniformResource(path, length) != nullptr;
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
			request.DoLock();
			request << buf;
			request.UnLock();
		}

		stream->ReleaseObject();
	} else {
		const char* builtin = request.GetScript()->QueryUniformResource(path, length);
		if (builtin != nullptr) {
			request.DoLock();
			request << String(builtin, length);
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

void SnowyStream::RequestNewFile(IScript::Request& request, const String& path, bool write) {
	CHECK_REFERENCES_NONE();
	bridgeSunset.GetKernel().YieldCurrentWarp();
	IArchive& archive = interfaces.archive;
	size_t length = 0;
	uint64_t modifiedTime = 0;
	IStreamBase* stream = path == ":memory:" ? new ZMemoryStream(0x1000, true) : archive.Open(path, write, length, &modifiedTime);
	if (stream != nullptr) {
		TShared<File> file = TShared<File>::From(new File(stream, length, modifiedTime));
		request.DoLock();
		request << file;
		request.UnLock();
	}
}

void SnowyStream::RequestGetFileLastModifiedTime(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	bridgeSunset.GetKernel().YieldCurrentWarp();
	uint64_t lastModifiedTime = file->GetLastModifiedTime();

	request.DoLock();
	request << lastModifiedTime;
	request.UnLock();
}

void SnowyStream::RequestGetFileSize(IScript::Request& request, IScript::Delegate<File> file) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(file);
	size_t length = file->GetLength();
	bridgeSunset.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << length;
	request.UnLock();
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
	request << begintable;
	archive.Query(path, Wrap(&handler, &QueryHandler::Accept));
	request << endtable;
	request.UnLock();
}

void SnowyStream::RequestNewResource(IScript::Request& request, const String& path, const String& expectedResType, bool createAlways) {
	TShared<ResourceBase> resource = CreateResource(path, expectedResType, !createAlways);
	bridgeSunset.GetKernel().YieldCurrentWarp();

	if (resource) {
		resource->GetResourceManager().InvokeUpload(resource());
		request.DoLock();
		request << resource;
		request.UnLock();
	} else {
		request.Error(String("Unable to create resource ") + path);
	}
}

class TaskResourceCreator final : public TReflected<TaskResourceCreator, SharedTiny> {
public:
	TaskResourceCreator(BridgeSunset& bs, SnowyStream& ss, const std::vector<String>& pl, const String& type, IScript::Request::Ref cb) : bridgeSunset(bs), snowyStream(ss), callback(cb) {
		pathList = pl; resType = type;
		resourceList.resize(pathList.size());
	}
	
	void Start(IScript::Request& request) {
		ThreadPool& threadPool = bridgeSunset.GetKernel().threadPool;
		for (uint32_t i = 0; i < safe_cast<uint32_t>(pathList.size()); i++) {
			// Create async task and use low-level thread pool dispatching it
			threadPool.Push(CreateTask(Wrap(this, &TaskResourceCreator::RoutineCreateResource), i));
			ReferenceObject();
		}

		Finalize(request);
	}

private:
	inline void Finalize(IScript::Request& request) {
		if (GetExtReferCount() == 0 && callback) {
			request.DoLock();
			request.Push();
			request << begintable;
			for (size_t i = 0; i < resourceList.size(); i++) {
				request << resourceList[i];
			}
			request << endtable;
			request.Call(sync, callback);
			request.Pop();
			request.UnLock();
		}
	}

	inline void RoutineCreateResource(void* context, bool run, uint32_t index) {
		if (run) {
			resourceList[index] = snowyStream.CreateResource(pathList[index], resType, true);
		}

		assert(context != nullptr);
		NsBridgeSunset::BridgeSunset& bridgeSunset = *reinterpret_cast<NsBridgeSunset::BridgeSunset*>(context);
		IScript::Request& request = bridgeSunset.AllocateRequest();
		Finalize(request);
		bridgeSunset.FreeRequest(request);
		ReleaseObject();
	}

	std::vector<String> pathList;
	std::vector<TShared<ResourceBase> > resourceList;
	String resType;
	BridgeSunset& bridgeSunset;
	SnowyStream& snowyStream;
	IScript::Request::Ref callback;
};

void SnowyStream::RequestNewResourcesAsync(IScript::Request& request, std::vector<String>& pathList, String& expectedResType, IScript::Request::Ref callback) {
	CHECK_REFERENCES(callback);
	Kernel& kernel = bridgeSunset.GetKernel();
	kernel.YieldCurrentWarp();

	TShared<TaskResourceCreator> taskResourceCreator = TShared<TaskResourceCreator>::From(new TaskResourceCreator(bridgeSunset, *this, std::move(pathList), std::move(expectedResType), callback));
	taskResourceCreator->Start(request);
}

void SnowyStream::RequestLoadExternalResourceData(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& externalData) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);
	bridgeSunset.GetKernel().YieldCurrentWarp();

	ZMemoryStream ms(externalData.size());
	size_t len = externalData.size();
	ms.Write(externalData.c_str(), len);
	ms.Seek(IStreamBase::BEGIN, 0);
	bool result = resource->LoadExternalResource(ms, len);
	if (result) {
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

	Tiny::FLAG flag = resource->Flag().load();
	std::vector<ResourceBase::Dependency> dependencies;
	resource->GetDependencies(dependencies);

	request.DoLock();
	request << begintable
		<< key("Flag") << flag
		<< key("Depends") << begintable;

	for (size_t k = 0; k < dependencies.size(); k++) {
		ResourceBase::Dependency& d = dependencies[k];
		request << key("Key") << d.value;
	}

	request << endtable << endtable;
	request.UnLock();
}

void SnowyStream::RequestCloneResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& path) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(resource);
	
	ResourceManager& resourceManager = resource->GetResourceManager();
	TShared<ResourceBase> exist = resourceManager.LoadExist(path);
	if (!exist) {
		ResourceBase* p = static_cast<ResourceBase*>(resource->Clone());
		if (p != nullptr) {
			resourceManager.Insert(path, p);
			request << p;
			p->ReleaseObject();
		}
	} else {
		request.Error(String("SnowyStream::CloneResource() : The path ") + path + " already exists");
	}
}

void SnowyStream::RequestMapResource(IScript::Request& request, IScript::Delegate<ResourceBase> resource, const String& typeExtension) {
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
	virtual void Execute(void* context) override {
		snowyStream.MapResource(resource);
		bool success = resource->Compress(compressType);
		if (callback) {
			NsBridgeSunset::BridgeSunset& bridgeSunset = *reinterpret_cast<NsBridgeSunset::BridgeSunset*>(context);
			IScript::Request& request = bridgeSunset.AllocateRequest();
			request.DoLock();
			request.Push();
			request.Call(sync, callback, success);
			request.Pop();
			request.Dereference(callback);
			request.UnLock();
			bridgeSunset.FreeRequest(request);
		} else {
			Finalize(context);
		}

		snowyStream.UnmapResource(resource);
		delete this;
	}

	virtual void Abort(void* context) override {
		Finalize(context);
		TaskOnce::Abort(context);
	}

	void Finalize(void* context) {
		if (callback) {
			NsBridgeSunset::BridgeSunset& bridgeSunset = *reinterpret_cast<NsBridgeSunset::BridgeSunset*>(context);
			IScript::Request& request = bridgeSunset.AllocateRequest();
			request.DoLock();
			request.Dereference(callback);
			request.UnLock();
			bridgeSunset.FreeRequest(request);
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
	bridgeSunset.GetKernel().threadPool.Push(task->UpdateFlag(ITask::TASK_PRIORITY_BACKGROUND));
}

void SnowyStream::Uninitialize() {
	for (std::map<Unique, TShared<ResourceManager> >::const_iterator p = resourceManagers.begin(); p != resourceManagers.end(); ++p) {
		(*p).second->RemoveAll();
	}

	interfaces.render.DeleteQueue(resourceQueue);
	resourceQueue = nullptr;
	interfaces.render.DeleteDevice(renderDevice);
	renderDevice = nullptr;
}

template <class T>
void RegisterPass(ResourceManager& resourceManager, UniqueType<T> type) {
	ShaderResourceImpl<T>* shaderResource = new ShaderResourceImpl<T>(resourceManager, "", ResourceBase::RESOURCE_ETERNAL);
	ZPassBase& pass = shaderResource->GetPass();
	Unique unique = pass.GetUnique();
	assert(unique != UniqueType<ZPassBase>().Get());
	String name = pass.GetUnique()->typeName;
	auto pos = name.find_last_of(':');
	if (pos != std::string::npos) {
		name = name.substr(pos + 1);
	}

	shaderResource->SetLocation(ShaderResource::GetShaderPathPrefix() + name);
	resourceManager.Insert(shaderResource->GetLocation(), shaderResource);
	shaderResource->ReleaseObject();
}

void SnowyStream::RegisterBuiltinPasses() {
	TShared<ResourceManager> resourceManager = resourceManagers[UniqueType<IRender>::Get()];
	assert(resourceManager);

	RegisterPass(*resourceManager(), UniqueType<AntiAliasingPass>());
	RegisterPass(*resourceManager(), UniqueType<BloomPass>());
	RegisterPass(*resourceManager(), UniqueType<ConstMapPass>());
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
	RegisterPass(*resourceManager(), UniqueType<WidgetPass>());
	// RegisterPass(*resourceManager(), UniqueType<CustomMaterialPass>());
	/*
	RegisterPass(*resourceManager(), UniqueType<ForwardLightingPass>());
	RegisterPass(*resourceManager(), UniqueType<ParticlePass>());
	RegisterPass(*resourceManager(), UniqueType<TerrainPass>());
	RegisterPass(*resourceManager(), UniqueType<VolumePass>());
	RegisterPass(*resourceManager(), UniqueType<WaterPass>());*/
}

void SnowyStream::RegisterReflectedSerializers() {
	assert(resourceSerializers.empty()); // can only be registerred once

	// Commented resources may have dependencies, so we do not serialize/deserialize them at once.
	// PaintsNow recommends database-managed resource dependencies ...

	RegisterReflectedSerializer(UniqueType<AudioResource>(), interfaces.audio, nullptr);
	RegisterReflectedSerializer(UniqueType<FontResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<MaterialResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<ShaderResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<MeshResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<SkeletonResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<TextureResource>(), interfaces.render, resourceQueue);
	RegisterReflectedSerializer(UniqueType<TapeResource>(), interfaces.archive, nullptr);
	RegisterReflectedSerializer(UniqueType<TextResource>(), interfaces.archive, nullptr);
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
	unordered_map<String, std::pair<Unique, TShared<ResourceSerializerBase> > >::iterator p = resourceSerializers.find(extension);
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.filterBase;

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
	return unique->GetSubName();
}

void SnowyStream::UnmapResource(TShared<ResourceBase> resource) {
	// Find resource serializer
	assert(resource);
	resource->Unmap();
}

bool SnowyStream::MapResource(TShared<ResourceBase> resource, const String& extension) {
	// Find resource serializer
	assert(resource);
	String typeExtension = extension.empty() ? GetReflectedExtension(resource->GetUnique()) : extension;
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.filterBase;

	unordered_map<String, std::pair<Unique, TShared<ResourceSerializerBase> > >::iterator p = resourceSerializers.find(typeExtension);
	if (p != resourceSerializers.end()) {
		// query manager
		std::map<Unique, TShared<ResourceManager> >::iterator t = resourceManagers.find((*p).second.first);
		assert(t != resourceManagers.end());
		return (*p).second.second->MapFromArchive(resource(), archive, protocol, resource->GetLocation());
	} else {
		return false;
	}
}

bool SnowyStream::PersistResource(TShared<ResourceBase> resource, const String& extension) {
	// Find resource serializer
	assert(resource);
	String typeExtension = extension.empty() ? GetReflectedExtension(resource->GetUnique()) : extension;
	IArchive& archive = interfaces.archive;
	IFilterBase& protocol = interfaces.filterBase;

	unordered_map<String, std::pair<Unique, TShared<ResourceSerializerBase> > >::iterator p = resourceSerializers.find(typeExtension);
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
		meshResource->GetResourceManager().InvokeUpload(meshResource());
	} else {
		errorHandler("Unable to create builtin mesh ...");
	}

	CreateBuiltinSolidTexture("[Runtime]/TextureResource/Black", UChar4(0, 0, 0, 0));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/White", UChar4(0, 0, 0, 0));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/MissingBaseColor", UChar4(255, 0, 255, 255));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/MissingNormal", UChar4(127, 127, 255, 255));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/MissingMaterial", UChar4(255, 255, 0, 0));

	TShared<MaterialResource> widgetMaterial = CreateReflectedResource(UniqueType<MaterialResource>(), "[Runtime]/MaterialResource/Widget", false, ResourceBase::RESOURCE_ETERNAL);
	TShared<ShaderResource> shaderResource = CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + "WidgetPass");
	widgetMaterial->originalShaderResource = shaderResource;
	widgetMaterial->GetResourceManager().InvokeUpload(widgetMaterial());
}

void SnowyStream::RequestSetShaderResourceCode(IScript::Request& request, IScript::Delegate<ShaderResource> shaderResource, const String& stage, const String& text, const std::vector<std::pair<String, String> >& config) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderResource);

	shaderResource->SetCode(stage, text, config);
}

void SnowyStream::RequestSetShaderResourceInput(IScript::Request& request, IScript::Delegate<ShaderResource> shaderResource, const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderResource);

	shaderResource->SetInput(stage, type, name, config);
}

void SnowyStream::RequestSetShaderResourceComplete(IScript::Request& request, IScript::Delegate<ShaderResource> shaderResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shaderResource);

	shaderResource->SetComplete();
}

IRender::Device* SnowyStream::GetRenderDevice() const {
	return renderDevice;
}

IRender::Queue* SnowyStream::GetResourceQueue() const {
	return resourceQueue;
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
		resource->GetResourceManager().InvokeUpload(resource());
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
		path = resource->GetLocation() + "." + resource->GetUnique()->GetSubName();
	}

	return streamBase << path;
}

const String& MetaResourceExternalPersist::GetUniqueName() const {
	return uniqueName;
}

MetaResourceExternalPersist::MetaResourceExternalPersist() {}