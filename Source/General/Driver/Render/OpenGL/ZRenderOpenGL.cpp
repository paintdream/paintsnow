#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "../../../Interface/Interfaces.h"
#include "../../../Interface/IShader.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Template/TQueue.h"
#include "ZRenderOpenGL.h"
#include "GLSLShaderGenerator.h"
#include <cstdio>
#include <vector>
#include <iterator>
#include <sstream>

#if USE_STATIC_THIRDPARTY_LIBRARIES
#define GLEW_STATIC
#define GLAPI
#define GLFW_INCLUDE_VULKAN
#endif

// #define LOG_OPENGL

#include <GL/glew.h>

using namespace PaintsNow;

class GLErrorGuard {
public:
	GLErrorGuard() {
		Guard();
	}

	void Guard() {
#ifdef _DEBUG
		if (enableGuard) {
			int err = glGetError();
			// fprintf(stderr, "GL ERROR: %d\n", err);
			if (err != 0) {
				int test = 0;
			}
			assert(err == 0);
		}
#endif
	}

	~GLErrorGuard() {
		Guard();
	}

	static bool enableGuard;
};

bool GLErrorGuard::enableGuard = true;

#define GL_GUARD() GLErrorGuard guard;

class DeviceImplOpenGL final : public IRender::Device {
public:
	DeviceImplOpenGL(IRender& r) : lastProgramID(0), lastFrameBufferID(0), render(r) {}

	void CompleteStore() {
		if (!storeInvalidates.empty()) {
			glInvalidateFramebuffer(GL_FRAMEBUFFER, (GLsizei)storeInvalidates.size(), &storeInvalidates[0]);
			storeInvalidates.clear();
		}
	}

	IRender& render;
	Int2 resolution;
	IRender::Resource::RenderStateDescription lastRenderState;
	GLuint lastProgramID;
	GLuint lastFrameBufferID;
	std::vector<GLuint> storeInvalidates;
};

struct_aligned(8) ResourceAligned : public IRender::Resource {};
struct QueueImplOpenGL;
struct ResourceBaseImplOpenGL : public ResourceAligned {
	virtual ~ResourceBaseImplOpenGL() {}

	virtual IRender::Resource::Type GetType() const = 0;
	virtual void SetUploadDescription(IRender::Resource::Description* description) = 0;
	virtual void SetDownloadDescription(IRender::Resource::Description* description) = 0;

	virtual void Nop(QueueImplOpenGL& queue) {}
	virtual void Execute(QueueImplOpenGL& queue) = 0;
	virtual void Upload(QueueImplOpenGL& queue) = 0;
	virtual void Download(QueueImplOpenGL& queue) = 0;
	virtual void PreSwap(QueueImplOpenGL& queue) = 0;
	virtual void PostSwap(QueueImplOpenGL& queue) = 0;
	virtual void SyncDownload(QueueImplOpenGL& queue) = 0;
	virtual void Delete(QueueImplOpenGL& queue) = 0;

#ifdef _DEBUG
	String note;
#endif
};

struct ResourceCommandImplOpenGL {
	typedef void (ResourceBaseImplOpenGL::*Action)(QueueImplOpenGL& queue);
	enum Operation {
		OP_EXECUTE = 0,
		OP_UPLOAD = 1,
		OP_DOWNLOAD = 2,
		OP_DELETE = 3,
		OP_PRESWAP = 4,
		OP_POSTSWAP = 5,
		OP_MAX = 5,
		OP_MASK = 7
	};

	ResourceCommandImplOpenGL(Operation a = OP_EXECUTE, IRender::Resource* r = nullptr) {
		assert(((size_t)r & OP_MASK) == 0);
		resource = reinterpret_cast<IRender::Resource*>((size_t)r | (a & OP_MASK));
	}

	inline IRender::Resource* GetResource() const {
		return reinterpret_cast<IRender::Resource*>((size_t)resource & ~(size_t)OP_MASK);
	}

	Operation GetOperation() const {
		return (Operation)((size_t)resource & OP_MASK);
	}

	bool Invoke(QueueImplOpenGL& queue) {
		// decode mask	
		static Action actionTable[] = {
			&ResourceBaseImplOpenGL::Execute,
			&ResourceBaseImplOpenGL::Upload,
			&ResourceBaseImplOpenGL::Download,
			&ResourceBaseImplOpenGL::Delete,
			&ResourceBaseImplOpenGL::PreSwap,
			&ResourceBaseImplOpenGL::PostSwap
		};

		if (GetResource() == nullptr) {
			return false;
		}

		ResourceBaseImplOpenGL* impl = static_cast<ResourceBaseImplOpenGL*>(GetResource());
		uint8_t index = safe_cast<uint8_t>(GetOperation());
		assert(index < sizeof(actionTable) / sizeof(actionTable[0]));
		Action action = actionTable[index];
#ifdef _DEBUG
		const char* opnames[] = {
			"Execute", "Upload", "Download", "Delete", "PreSwap", "PostSwap"
		};

		const char* types[] = {
			"Unknown", "Texture", "Buffer", "Shader", "RenderState", "RenderTarget", "DrawCall"
		};

#ifdef LOG_OPENGL
		printf("[OpenGL Command] %s(%s) = %p\n", opnames[index], types[impl->GetType()], impl);
#endif
#endif
		(impl->*action)(queue);

		return true;
	}

private:
	IRender::Resource* resource;
};

struct QueueImplOpenGL final : public IRender::Queue {
	QueueImplOpenGL(DeviceImplOpenGL* d, bool shared) : device(d) {
		critical.store(shared ? 0u : ~(uint32_t)0u, std::memory_order_relaxed);
		flushCount.store(0u, std::memory_order_release);
	}

	inline void Flush() {
		QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_EXECUTE, nullptr));
		flushCount.fetch_add(1, std::memory_order_release);
	}

	inline void QueueCommand(const ResourceCommandImplOpenGL& command) {
		bool lock = critical.load(std::memory_order_relaxed) != ~(uint32_t)0;
		if (lock) {
			SpinLock(critical);
			queuedCommands.Push(command);
			SpinUnLock(critical);
		} else {
			queuedCommands.Push(command);
		}
	}

	static inline void PrefetchCommand(const ResourceCommandImplOpenGL& predict) {
		IMemory::PrefetchRead(&predict);
		IRender::Resource* resource = predict.GetResource();
		IMemory::PrefetchRead(resource);
		IMemory::PrefetchRead(reinterpret_cast<const char*>(resource) + 64); // prefetch more 64 bytes.
	}

	struct Scanner {
		Scanner(QueueImplOpenGL& q) : queue(q) {
#ifdef _DEBUG
			yielded = false;
			count = 0;
#endif
		}

		~Scanner() {
#ifdef _DEBUG
			assert(yielded == true);
			// printf("Render Command Count: %d\n", count);
#endif
		}

		bool operator () (ResourceCommandImplOpenGL& command, ResourceCommandImplOpenGL& predict) {
			PrefetchCommand(predict);
			assert(command.GetOperation() != ResourceCommandImplOpenGL::OP_UPLOAD && command.GetOperation() != ResourceCommandImplOpenGL::OP_DELETE);
			if (command.Invoke(queue)) {
#ifdef _DEBUG
				count++;
#endif
				return true;
			} else {
#ifdef _DEBUG
				yielded = true;
#endif
				return false;
			}
		}

		QueueImplOpenGL& queue;
#ifdef _DEBUG
		bool yielded;
		uint32_t count;
#endif
	};

	void Repeat() {
		while (flushCount.load(std::memory_order_acquire) > 1) {
			while (!queuedCommands.Empty()) {
				ResourceCommandImplOpenGL& command = queuedCommands.Top();
				queuedCommands.Pop();

				if (command.GetResource() == nullptr)
					break;
			}

			flushCount.fetch_sub(1, std::memory_order_relaxed);
		}

		if (flushCount.load(std::memory_order_acquire) > 0) {
			Scanner scanner(*this);
			queuedCommands.Iterate(scanner);
		}
	}

	void ExecuteAll() {
		while (!queuedCommands.Empty()) {
			ResourceCommandImplOpenGL command = queuedCommands.Top();
			queuedCommands.Pop();

			PrefetchCommand(queuedCommands.Predict());
			command.Invoke(*this);
		}
	}

	void ClearAll() {
		while (!queuedCommands.Empty()) {
			ResourceCommandImplOpenGL command = queuedCommands.Top();
			queuedCommands.Pop();

			if (command.GetOperation() == ResourceCommandImplOpenGL::OP_DELETE) {
				command.Invoke(*this);
			}
		}
	}

	void Execute() {
		while (!queuedCommands.Empty()) {
			ResourceCommandImplOpenGL command = queuedCommands.Top();
			queuedCommands.Pop();

			if (!command.Invoke(*this))
				return;
		}
	}

	DeviceImplOpenGL* device;
	std::atomic<int32_t> critical;
	std::atomic<uint32_t> flushCount;
	ResourceBaseImplOpenGL* swapResource;
	TQueueList<ResourceCommandImplOpenGL> queuedCommands;
};

#if defined(_MSC_VER) && _MSC_VER <= 1200
template <class T>
void MoveResource(T& target, T& source) {
	target = source;
}
#else
template <class T>
void MoveResource(T& target, T& source) {
	target = std::move(source);
}
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
template <>
void MoveResource<IRender::Resource::TextureDescription>(IRender::Resource::TextureDescription& target, IRender::Resource::TextureDescription& source) {
	Bytes data = std::move(source.data);
	target = source;
	target.data = std::move(data);
}

template <>
void MoveResource<IRender::Resource::BufferDescription>(IRender::Resource::BufferDescription& target, IRender::Resource::BufferDescription& source) {
	Bytes data = std::move(source.data);
	target = source;
	target.data = std::move(data);
}
#endif

template <class T>
struct ResourceBaseImplOpenGLDesc : public ResourceBaseImplOpenGL {
	typedef ResourceBaseImplOpenGLDesc<T> Base;
	ResourceBaseImplOpenGLDesc() : downloadDescription(nullptr) {
		critical.store(0, std::memory_order_relaxed);
	}

	virtual void SetUploadDescription(IRender::Resource::Description* d) override {
		SpinLock(critical);
		MoveResource(nextDescription, *static_cast<T*>(d));
		SpinUnLock(critical, 2u);
	}

	T& UpdateDescription() {
		// updated?
		if (critical.load(std::memory_order_relaxed) == 2u) {
			if (SpinLock(critical) == 2u) {
				currentDescription = std::move(nextDescription);
			}

			SpinUnLock(critical);
		}

		return currentDescription;
	}

	T& GetDescription() {
		return currentDescription;
	}

#ifdef _DEBUG
	T& GetNextDescription() {
		return nextDescription;
	}
#endif

	virtual void SetDownloadDescription(IRender::Resource::Description* d) override {
		downloadDescription = static_cast<T*>(d);
	}

	virtual void Download(QueueImplOpenGL& queue) override {
		assert(downloadDescription != nullptr);
		*downloadDescription = currentDescription;
	}
	
	virtual void SyncDownload(QueueImplOpenGL& queue) override {
		downloadDescription = nullptr;
	}

	virtual void PreSwap(QueueImplOpenGL& queue) override { assert(false); } // not implemented by default
	virtual void PostSwap(QueueImplOpenGL& queue) override { assert(false); }

protected:
	T* downloadDescription;

private:
	std::atomic<uint32_t> critical;
	T currentDescription;
#ifdef _DEBUG
public:
#endif
	T nextDescription;
};

template <class T>
struct ResourceImplOpenGL final : public ResourceBaseImplOpenGLDesc<T> {
	virtual IRender::Resource::Type GetType() const  override { return IRender::Resource::RESOURCE_UNKNOWN; }
	virtual void Upload(QueueImplOpenGL& queue) override { assert(false); }
	virtual void Download(QueueImplOpenGL& queue) override { assert(false); }
	virtual void Execute(QueueImplOpenGL& queue) override { assert(false); }
	virtual void Delete(QueueImplOpenGL& queue) override { assert(false); }
};

template <>
struct ResourceImplOpenGL<IRender::Resource::TextureDescription> final : public ResourceBaseImplOpenGLDesc<IRender::Resource::TextureDescription> {
	ResourceImplOpenGL() : textureID(0), textureType(GL_TEXTURE_2D), pixelBufferObjectID(0) {}
	virtual IRender::Resource::Type GetType() const override { return RESOURCE_TEXTURE; }

	inline void CreateMips(Resource::TextureDescription& d, uint32_t bitDepth, GLuint textureType, GLuint srcLayout, GLuint srcDataType, GLuint format, const void* buffer, size_t length) {
		GL_GUARD();

		GLuint t = textureType == GL_TEXTURE_2D ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
		switch (d.state.mip) {
			case Resource::TextureDescription::NOMIP:
			{
				glTexParameteri(t, GL_TEXTURE_MAX_LEVEL, 0);
				break;
			}
			case Resource::TextureDescription::AUTOMIP:
			{
				glTexParameteri(t, GL_TEXTURE_MAX_LEVEL, 1000);
				glGenerateMipmap(t);
				break;
			}
			case Resource::TextureDescription::SPECMIP:
			{
				uint16_t width = d.dimension.x(), height = d.dimension.y();
				const char* p = reinterpret_cast<const char*>(buffer) + bitDepth * width * height / 8;
				GLuint mip = 1;
				while (width > 1 && height > 1 && p < reinterpret_cast<const char*>(buffer) + length) {
					width >>= 1;
					height >>= 1;

					if (d.state.compress) {
						glCompressedTexImage2D(textureType, mip++, format, width, height, 0, bitDepth * width * height / 8, p);
					} else {
						if (d.state.immutable && glTexStorage2D != nullptr) {
							glTexSubImage2D(textureType, mip++, 0, 0, width, height, srcLayout, srcDataType, p);
						} else {
							glTexImage2D(textureType, mip++, format, width, height, 0, srcLayout, srcDataType, p);
						}
					}

					p += bitDepth * width * height / 8;
				}

				glTexParameteri(t, GL_TEXTURE_MAX_LEVEL, mip - 1);
				break;
			}
		}
	}

	inline void CreateMipsArray(Resource::TextureDescription& d, uint32_t bitDepth, GLuint textureType, GLuint srcLayout, GLuint srcDataType, GLuint format, const void* buffer, size_t length) {
		GL_GUARD();

		GLuint t = textureType == GL_TEXTURE_2D_ARRAY ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_CUBE_MAP_ARRAY ;
		switch (d.state.mip) {
			case Resource::TextureDescription::NOMIP:
			{
				glTexParameteri(t, GL_TEXTURE_MAX_LEVEL, 0);
				break;
			}
			case Resource::TextureDescription::AUTOMIP:
			{
				glTexParameteri(t, GL_TEXTURE_MAX_LEVEL, 1000);
				glGenerateMipmap(t);
				break;
			}
			case Resource::TextureDescription::SPECMIP:
			{
				uint16_t width = d.dimension.x(), height = d.dimension.y();
				const char* p = reinterpret_cast<const char*>(buffer) + bitDepth * width * height * d.dimension.z() / 8;
				GLuint mip = 1;
				while (width > 1 && height > 1 && p < reinterpret_cast<const char*>(buffer) + length) {
					width >>= 1;
					height >>= 1;
					if (d.state.compress) {
						glCompressedTexImage3D(textureType, mip++, format, width, height, d.dimension.z(), 0, bitDepth * width * height * d.dimension.z() / 8, p);
					} else {
						if (d.state.immutable && glTexStorage3D != nullptr) {
							glTexSubImage3D(textureType, mip++, 0, 0, 0, width, height, d.dimension.z(), srcLayout, srcDataType, p);
						} else {
							glTexImage3D(textureType, mip++, format, width, height, d.dimension.z(), 0, srcLayout, srcDataType, p);
						}
					}

					p += bitDepth * width * height * d.dimension.z() / 8;
				}

				glTexParameteri(t, GL_TEXTURE_MAX_LEVEL, mip - 1);
				break;
			}
		}
	}

	static inline uint32_t ParseFormatFromState(GLuint& srcLayout, GLuint& srcDataType, GLuint& format, const Resource::TextureDescription::State& state) {
		srcDataType = GL_UNSIGNED_BYTE;
		format = GL_RGBA8;
		uint32_t byteDepth = 4;
		switch (state.format) {
			case Resource::TextureDescription::UNSIGNED_BYTE:
			{
				static const GLuint presets[Resource::TextureDescription::Layout::END] = { GL_R8, GL_RG8, GL_RGB8, GL_RGBA8, GL_NONE, GL_STENCIL_INDEX8, GL_DEPTH24_STENCIL8, GL_NONE };
				static const GLuint byteDepths[Resource::TextureDescription::Layout::END] = { 1, 2, 3, 4, 0, 1, 4, 0 };
				format = presets[state.layout];
				byteDepth = byteDepths[state.layout];
				srcDataType = GL_UNSIGNED_BYTE;
				break;
			}
			case Resource::TextureDescription::UNSIGNED_SHORT:
			{
				static const GLuint presets[Resource::TextureDescription::Layout::END] = { GL_R16, GL_RG16, GL_RGB16, GL_RGBA16, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX16, GL_NONE, GL_RGB10_A2 };
				static const GLuint byteDepths[Resource::TextureDescription::Layout::END] = { 2, 4, 6, 8, 2, 2, 0, 4 };
				format = presets[state.layout];
				byteDepth = byteDepths[state.layout];
				srcDataType = GL_UNSIGNED_SHORT;
				break;
			}
			case Resource::TextureDescription::HALF:
			{
				static const GLuint presets[Resource::TextureDescription::Layout::END] = { GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX16, GL_NONE, GL_NONE };
				static const GLuint byteDepths[Resource::TextureDescription::Layout::END] = { 2, 4, 6, 8, 2, 2, 0, 0 };
				format = presets[state.layout];
				byteDepth = byteDepths[state.layout];
				srcDataType = GL_HALF_FLOAT;
				break;
			}
			case Resource::TextureDescription::FLOAT:
			{
				static const GLuint presets[Resource::TextureDescription::Layout::END] = { GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F, GL_DEPTH_COMPONENT32F, GL_NONE, GL_DEPTH24_STENCIL8, GL_R11F_G11F_B10F };
				static const GLuint byteDepths[Resource::TextureDescription::Layout::END] = { 4, 8, 12, 16, 4, 0, 4, 4 };
				format = presets[state.layout];
				byteDepth = byteDepths[state.layout];
				srcDataType = GL_FLOAT;
				break;
			}
			default:
			{
				assert(false);
			}
		}

		assert(format != GL_NONE);
		const GLint layouts[Resource::TextureDescription::Layout::END] = { GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT, GL_STENCIL_INDEX, GL_DEPTH_COMPONENT, srcDataType == GL_FLOAT ? GL_RGB : GL_RGBA };
		srcLayout = layouts[state.layout];

		uint32_t bitDepth = byteDepth * 8;
		if (state.compress) {
			assert(format == GL_RGBA8 && bitDepth == 32); // only support compression for GL_RGBA8
			assert(bitDepth % 4 == 0);
			bitDepth >>= 2;

			format = GL_COMPRESSED_RGBA_BPTC_UNORM;
		}

		return bitDepth;
	}
	
	virtual void Upload(QueueImplOpenGL& queue) override {
		GL_GUARD();
		Resource::TextureDescription& d = UpdateDescription();
		// Convert texture format
		GLuint srcLayout, srcDataType, format;
		uint32_t bitDepth = ParseFormatFromState(srcLayout, srcDataType, format, d.state);
		bool newTexture = textureID == 0;
		if (newTexture) {
			glGenTextures(1, &textureID);
		}

		if (d.state.sample == IRender::Resource::TextureDescription::RENDERBUFFER) {
			assert(d.data.Empty());
			assert(renderbufferID == 0);
			glGenRenderbuffers(1, &renderbufferID);
			glBindRenderbuffer(GL_RENDERBUFFER, renderbufferID);
			glRenderbufferStorage(GL_RENDERBUFFER, format, d.dimension.x(), d.dimension.y());
			return;
		}

		const void* data = d.data.Empty() ? nullptr : d.data.GetData();
		textureType = GL_TEXTURE_2D;
		if (d.dimension.z() <= 1) {
			switch (d.state.type) {
				case Resource::TextureDescription::TEXTURE_1D:
				{
					GL_GUARD();
					glBindTexture(textureType = GL_TEXTURE_1D, textureID);
					if (d.state.compress) {
						assert(data != nullptr);
						glCompressedTexImage1D(textureType, 0, format, d.dimension.x(), 0, bitDepth * d.dimension.x() / 8, data);
					} else {
						if (d.state.immutable && glTexStorage1D != nullptr) {
							if (newTexture) {
								glTexStorage1D(textureType, 1, format, d.dimension.x());
							}

							if (data != nullptr) {
								glTexSubImage1D(textureType, 0, 0, d.dimension.x(), srcLayout, srcDataType, data);
							}
						} else {
							glTexImage1D(textureType, 0, format, d.dimension.x(), 0, srcLayout, srcDataType, data);
						}
					}
					break;
				}
				case Resource::TextureDescription::TEXTURE_2D:
				{
					GL_GUARD();
					glBindTexture(textureType = GL_TEXTURE_2D, textureID);
					if (d.state.compress) {
						assert(data != nullptr);
						glCompressedTexImage2D(textureType, 0, format, d.dimension.x(), d.dimension.y(), 0, bitDepth * d.dimension.x() * d.dimension.y() / 8, data);
					} else {
						if (d.state.immutable && glTexStorage2D != nullptr) {
							if (newTexture) {
								glTexStorage2D(textureType, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Math::Log2((uint32_t)Math::Min(d.dimension.x(), d.dimension.y())) + 1, format, d.dimension.x(), d.dimension.y());
							}

							if (data != nullptr) {
								glTexSubImage2D(textureType, 0, 0, 0, d.dimension.x(), d.dimension.y(), srcLayout, srcDataType, data);
							}
						} else {
							glTexImage2D(textureType, 0, format, d.dimension.x(), d.dimension.y(), 0, srcLayout, srcDataType, data);
						}
					}

					if (data != nullptr) {
						CreateMips(d, bitDepth, textureType, srcLayout, srcDataType, format, data, d.data.GetSize());
					}

					break;
				}
				case Resource::TextureDescription::TEXTURE_2D_CUBE:
				{
					GL_GUARD();
					glBindTexture(textureType = GL_TEXTURE_CUBE_MAP, textureID);
					assert(d.data.GetSize() % 6 == 0);
					size_t each = d.data.GetSize() / 6;

					static const GLuint index[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

					if (d.state.immutable && glTexStorage2D != nullptr) {
						if (newTexture) {
							glTexStorage2D(GL_TEXTURE_CUBE_MAP, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Math::Log2((uint32_t)Math::Min(d.dimension.x(), d.dimension.y())) + 1, format, d.dimension.x(), d.dimension.y());
						}
					}

					for (size_t k = 0; k < 6; k++) {
						const char* p = data != nullptr ? (const char*)data + k * each : nullptr;
						if (d.state.compress) {
							assert(p != nullptr);
							glCompressedTexImage2D(index[k], 0, format, d.dimension.x(), d.dimension.y(), 0, bitDepth * d.dimension.x() * d.dimension.y() / 8, p);
						} else {
							if (d.state.immutable && glTexStorage2D != nullptr) {
								if (p != nullptr) {
									glTexSubImage2D(index[k], 0, 0, 0, d.dimension.x(), d.dimension.y(), srcLayout, srcDataType, p);
								}
							} else {
								glTexImage2D(index[k], 0, format, d.dimension.x(), d.dimension.y(), 0, srcLayout, srcDataType, p);
							}
						}

						if (data != nullptr) {
							CreateMips(d, bitDepth, index[k], srcLayout, srcDataType, format, p, each);
						}
					}
					break;
				}
				case Resource::TextureDescription::TEXTURE_3D:
				{
					GL_GUARD();
					glBindTexture(textureType = GL_TEXTURE_3D, textureID);
					if (d.state.compress) {
						assert(data != nullptr);
						glCompressedTexImage3D(textureType, 0, format, d.dimension.x(), d.dimension.y(), d.dimension.z(), 0, bitDepth * d.dimension.x() * d.dimension.y() * d.dimension.z() / 8, data);
					} else {
						if (d.state.immutable && glTexStorage3D != nullptr) {
							if (newTexture) {
								glTexStorage3D(textureType, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Math::Log2((uint32_t)Math::Min(Math::Min(d.dimension.x(), d.dimension.y()), d.dimension.z())) + 1, format, d.dimension.x(), d.dimension.y(), d.dimension.z());
							}

							if (data != nullptr) {
								glTexSubImage3D(textureType, 0, 0, 0, 0, d.dimension.x(), d.dimension.y(), d.dimension.z(), srcLayout, srcDataType, data);
							}
						} else {
							glTexImage3D(textureType, 0, format, d.dimension.x(), d.dimension.y(), d.dimension.z(), 0, srcLayout, srcDataType, data);
						}
					}
					break;
				}
			}
		} else {
			switch (d.state.type) {
				case Resource::TextureDescription::TEXTURE_1D:
				{
					GL_GUARD();
					glBindTexture(textureType = GL_TEXTURE_1D_ARRAY, textureID);
					if (d.state.compress) {
						assert(data != nullptr);
						glCompressedTexImage2D(textureType, 0, format, d.dimension.x(), d.dimension.y(), 0, bitDepth * d.dimension.x() * d.dimension.y() / 8, data);
					} else {
						if (d.state.immutable && glTexStorage2D != nullptr) {
							if (newTexture) {
								glTexStorage2D(textureType, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Math::Log2((uint32_t)d.dimension.x()) + 1, format, d.dimension.x(), d.dimension.y());
							}

							if (data != nullptr) {
								glTexSubImage2D(textureType, 0, 0, 0, d.dimension.x(), d.dimension.y(), srcLayout, srcDataType, data);
							}
						} else {
							glTexImage2D(textureType, 0, format, d.dimension.x(), d.dimension.y(), 0, srcLayout, srcDataType, data);
						}
					}

					break;
				}
				case Resource::TextureDescription::TEXTURE_2D:
				{
					GL_GUARD();
					glBindTexture(textureType = GL_TEXTURE_2D_ARRAY, textureID);
					assert(d.data.GetSize() % d.dimension.z() == 0);
					if (d.state.compress) {
						assert(data != nullptr);
						glCompressedTexImage3D(textureType, 0, format, d.dimension.x(), d.dimension.y(), d.dimension.z(), 0, bitDepth * d.dimension.x() * d.dimension.y() * d.dimension.z() / 8, data);
					} else {
						if (d.state.immutable && glTexStorage3D != nullptr) {
							if (newTexture) {
								glTexStorage3D(textureType, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Math::Log2((uint32_t)Math::Min(d.dimension.x(), d.dimension.y())) + 1, format, d.dimension.x(), d.dimension.y(), d.dimension.z());
							}

							if (data != nullptr) {
								glTexSubImage3D(textureType, 0, 0, 0, 0, d.dimension.x(), d.dimension.y(), d.dimension.z(), srcLayout, srcDataType, data);
							}
						} else {
							glTexImage3D(textureType, 0, format, d.dimension.x(), d.dimension.y(), d.dimension.z(), 0, srcLayout, srcDataType, data);
						}
					}

					if (data != nullptr) {
						CreateMipsArray(d, bitDepth, textureType, srcLayout, srcDataType, format, d.data.GetData(), d.data.GetSize());
					}
					break;
				}
				case Resource::TextureDescription::TEXTURE_2D_CUBE:
				{
					GL_GUARD();
					glBindTexture(textureType = GL_TEXTURE_CUBE_MAP_ARRAY, textureID);
					assert(d.data.GetSize() % 6 == 0);
					size_t each = d.data.GetSize() / 6;

					static const GLuint index[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

					if (d.state.immutable && glTexStorage3D != nullptr) {
						if (newTexture) {
							glTexStorage3D(GL_TEXTURE_CUBE_MAP, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Math::Log2((uint32_t)Math::Min(d.dimension.x(), d.dimension.y())) + 1, format, d.dimension.x(), d.dimension.y(), d.dimension.z());
						}
					}

					for (uint32_t k = 0; k < 6; k++) {
						const char* p = data != nullptr ? (const char*)data + k * each : nullptr;
						if (d.state.compress) {
							assert(data != nullptr);
							glCompressedTexImage3D(index[k], 0, format, d.dimension.x(), d.dimension.y(), d.dimension.z(), 0, bitDepth * d.dimension.x() * d.dimension.y() * d.dimension.z() / 8, p);
						} else {
							if (d.state.immutable && glTexStorage3D != nullptr) {
								if (data != nullptr) {
									glTexSubImage3D(index[k], 0, 0, 0, 0, d.dimension.x(), d.dimension.y(), d.dimension.z(), srcLayout, srcDataType, data);
								}
							} else {
								glTexImage3D(index[k], 0, format, d.dimension.x(), d.dimension.y(), d.dimension.z(), 0, srcLayout, srcDataType, p);
							}
						}

						if (data != nullptr) {
							CreateMipsArray(d, bitDepth, textureType, srcLayout, srcDataType, format, p, each);
						}
					}
					break;
				}
				case Resource::TextureDescription::TEXTURE_3D:
				{
					assert(false); // not allowed
					break;
				}
			}
		}

		glTexParameteri(textureType, GL_TEXTURE_WRAP_S, d.state.wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE);
		if (textureType != GL_TEXTURE_1D) {
			glTexParameteri(textureType, GL_TEXTURE_WRAP_T, d.state.wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE);

			if (textureType == GL_TEXTURE_3D) {
				glTexParameteri(textureType, GL_TEXTURE_WRAP_R, d.state.wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE);
			}
		}

		glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER, d.state.sample == Resource::TextureDescription::POINT ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, d.state.sample == Resource::TextureDescription::POINT ? GL_NEAREST : GL_LINEAR);

		/*
#ifdef _DEBUG
		fprintf(stderr, "Device Texture Uploaded: %s\n", note.c_str());
#endif*/
			
		// free memory
		d.data.Clear();
	}

	virtual void Download(QueueImplOpenGL& queue) override {
		GL_GUARD();
		assert(downloadDescription != nullptr);
		GLuint srcLayout, srcDataType, format;
		uint32_t bitDepth = ParseFormatFromState(srcLayout, srcDataType, format, downloadDescription->state);

		assert(textureID != 0);
		glBindTexture(textureType, textureID);
		UShort3& dimension = downloadDescription->dimension = GetDescription().dimension;
		Bytes& data = downloadDescription->data;
		data.Resize(bitDepth * dimension.x() * dimension.y() / 8);

		if (pixelBufferObjectID == 0) {
			glGenBuffers(1, &pixelBufferObjectID);
		}

		glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBufferObjectID);
		glBufferData(GL_PIXEL_PACK_BUFFER, (GLsizei)data.GetSize(), nullptr, GL_STREAM_READ);

		// Only get mip 0
		glGetTexImage(textureType, 0, srcLayout, srcDataType, nullptr);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}

	virtual void SyncDownload(QueueImplOpenGL& queue) override {
		GL_GUARD();
		assert(pixelBufferObjectID != 0);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBufferObjectID);
		const void* ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		Bytes& data = downloadDescription->data;
		memcpy(data.GetData(), ptr, data.GetSize());
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		glDeleteBuffers(1, &pixelBufferObjectID);
		pixelBufferObjectID = 0;
		downloadDescription = nullptr;
	}

	virtual void Execute(QueueImplOpenGL& queue) override {} // Not an executable resource
	virtual void Delete(QueueImplOpenGL& queue) override {
		GL_GUARD();

		if (textureID != 0) {
			if (GetDescription().state.sample != IRender::Resource::TextureDescription::RENDERBUFFER) {
				glDeleteRenderbuffers(1, &renderbufferID);
			} else {
				glDeleteTextures(1, &textureID);
			}
		}

		if (pixelBufferObjectID != 0) {
			glDeleteBuffers(1, &pixelBufferObjectID);
		}

		delete this;
	}

	virtual void PreSwap(QueueImplOpenGL& queue) override {
		queue.swapResource = this;
	}

	virtual void PostSwap(QueueImplOpenGL& queue) override {
		ResourceImplOpenGL<TextureDescription>* t = static_cast<ResourceImplOpenGL<TextureDescription>*>(queue.swapResource);
		std::swap(textureID, t->textureID);
	}

	union {
		GLuint textureID;
		GLuint renderbufferID;
	};

	GLuint textureType;
	GLuint pixelBufferObjectID;
};

template <>
struct ResourceImplOpenGL<IRender::Resource::BufferDescription> final : public ResourceBaseImplOpenGLDesc<IRender::Resource::BufferDescription> {
	ResourceImplOpenGL() : bufferID(0) {}

	virtual IRender::Resource::Type GetType() const  override { return RESOURCE_BUFFER; }
	virtual void Upload(QueueImplOpenGL& queue) override {
		GL_GUARD();

		if (bufferID == 0) {
			glGenBuffers(1, &bufferID);
		}

		Resource::BufferDescription& d = UpdateDescription();
		if (d.data.Empty()) return; // Multi-updating in one frame takes effect only once
		usage = d.usage;
		format = d.format;
		component = d.component;

		GLuint bufferType = GL_ELEMENT_ARRAY_BUFFER;
		switch (d.usage) {
			case Resource::BufferDescription::INDEX:
				bufferType = GL_ELEMENT_ARRAY_BUFFER;
				break;
			case Resource::BufferDescription::VERTEX:
				assert(component != 0);
				bufferType = GL_ARRAY_BUFFER;
				break;
			case Resource::BufferDescription::INSTANCED:
				bufferType = GL_ARRAY_BUFFER;
				break;
			case Resource::BufferDescription::UNIFORM:
				bufferType = GL_UNIFORM_BUFFER;
				break;
			case Resource::BufferDescription::STORAGE:
				bufferType = GL_SHADER_STORAGE_BUFFER;
				break;
		}
	
		glBindBuffer(bufferType, bufferID);
		glBufferData(bufferType, d.data.GetSize(), d.data.GetData(), d.dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

		length = safe_cast<uint32_t>(d.data.GetSize());
		d.data.Clear();
	}

	virtual void Download(QueueImplOpenGL& queue) override {
		assert(false); // not implemented
	}

	virtual void Execute(QueueImplOpenGL& queue) override {} // Not an executable resource
	virtual void Delete(QueueImplOpenGL& queue) override {
		GL_GUARD();

		if (bufferID != 0) {
			glDeleteBuffers(1, &bufferID);
		}

		delete this;
	}

	uint8_t usage;
	uint8_t format;
	uint16_t component;
	uint32_t length;
	GLuint bufferID;
};

template <>
struct ResourceImplOpenGL<IRender::Resource::ShaderDescription> final : public ResourceBaseImplOpenGLDesc<IRender::Resource::ShaderDescription> {
	ResourceImplOpenGL() {}
	virtual IRender::Resource::Type GetType() const  override { return RESOURCE_SHADER; }

	struct Program {
		Program() : programID(0), isComputeShader(0) {}
		GLuint isComputeShader : 1;
		GLuint programID : 31;
		std::vector<GLuint> shaderIDs;
		std::vector<GLuint> textureLocations;
		std::vector<GLuint> uniformBufferLocations;
		std::vector<GLuint> sharedBufferLocations;
		String shaderName;
	};

	void Cleanup() {
		GL_GUARD();

		if (program.programID != 0) {
			glDeleteProgram(program.programID);
		}

		for (size_t i = 0; i < program.shaderIDs.size(); i++) {
			glDeleteShader(program.shaderIDs[i]);
		}
	}

	virtual void Upload(QueueImplOpenGL& queue) override {
		GL_GUARD();
		Cleanup();

		Resource::ShaderDescription& pass = UpdateDescription();
		GLuint programID = glCreateProgram();
		program.programID = programID;
		program.shaderName = pass.name;

		std::vector<IShader*> shaders[Resource::ShaderDescription::END];
		String common;
		for (size_t i = 0; i < pass.entries.size(); i++) {
			const std::pair<Resource::ShaderDescription::Stage, IShader*>& component = pass.entries[i];

			if (component.first == Resource::ShaderDescription::GLOBAL) {
				common += component.second->GetShaderText();
			} else {
				shaders[component.first].emplace_back(component.second);
			}
		}

		std::vector<String> textureNames;
		std::vector<String> uniformBufferNames;
		std::vector<String> sharedBufferNames;

		for (size_t k = 0; k < Resource::ShaderDescription::END; k++) {
			std::vector<IShader*>& pieces = shaders[k];
			if (pieces.empty()) continue;

			GLuint shaderType = GL_VERTEX_SHADER;
			switch (k) {
			case Resource::ShaderDescription::VERTEX:
				shaderType = GL_VERTEX_SHADER;
				break;
			case Resource::ShaderDescription::TESSELLATION_CONTROL:
				shaderType = GL_TESS_CONTROL_SHADER;
				break;
			case Resource::ShaderDescription::TESSELLATION_EVALUATION:
				shaderType = GL_TESS_EVALUATION_SHADER;
				break;
			case Resource::ShaderDescription::GEOMETRY:
				shaderType = GL_GEOMETRY_SHADER;
				break;
			case Resource::ShaderDescription::FRAGMENT:
				shaderType = GL_FRAGMENT_SHADER;
				break;
			case Resource::ShaderDescription::COMPUTE:
				shaderType = GL_COMPUTE_SHADER;
				program.isComputeShader = 1;
				break;
			}

			GLuint shaderID = glCreateShader(shaderType);
			String body = "void main(void) {\n";
			String head = "";
			uint32_t inputIndex = 0, outputIndex = 0, textureIndex = 0;
			for (size_t n = 0; n < pieces.size(); n++) {
				IShader* shader = pieces[n];
				// Generate declaration
				GLSLShaderGenerator declaration((Resource::ShaderDescription::Stage)k, inputIndex, outputIndex, textureIndex);
				(*shader)(declaration);
				declaration.Complete();

				body += declaration.initialization + shader->GetShaderText() + declaration.finalization + "\n";
				head += declaration.declaration;

				for (size_t k = 0; k < declaration.bufferBindings.size(); k++) {
					std::pair<const IShader::BindBuffer*, String>& item = declaration.bufferBindings[k];
					if (item.first->description.usage == IRender::Resource::BufferDescription::UNIFORM) {
						uniformBufferNames.emplace_back(item.second);
					} else if (item.first->description.usage == IRender::Resource::BufferDescription::STORAGE) {
						sharedBufferNames.emplace_back(item.second);
					}
				}

				for (size_t m = 0; m < declaration.textureBindings.size(); m++) {
					textureNames.emplace_back(declaration.textureBindings[m].second);
				}
			}

			body += "\n}\n"; // make a call to our function

			String fullShader = GLSLShaderGenerator::GetFrameCode() + common + head + body;
			const char* source[] = { fullShader.c_str() };
			glShaderSource(shaderID, 1, source, nullptr);
			glCompileShader(shaderID);

			// printf("Shader code: %s\n", fullShader.c_str());

			int success;
			glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

			if (success == 0) {
				const int MAX_INFO_LOG_SIZE = 4096;
				char info[MAX_INFO_LOG_SIZE] = { 0 };
				glGetShaderInfoLog(shaderID, MAX_INFO_LOG_SIZE - 1, nullptr, info);
				fprintf(stderr, "ZRenderOpenGL::CompileShader(): %s\n", info);
				fprintf(stderr, "ZRenderOpenGL::CompileShader(): %s\n", fullShader.c_str());
				// assert(false);
				if (pass.compileCallback) {
					pass.compileCallback(pass, (IRender::Resource::ShaderDescription::Stage)k, info, fullShader);
				}
				Cleanup();
				return;
			} else if (pass.compileCallback) {
				pass.compileCallback(pass, (IRender::Resource::ShaderDescription::Stage)k, "", fullShader);
			}

			glAttachShader(programID, shaderID);
			program.shaderIDs.emplace_back(shaderID);
		}

		glLinkProgram(programID);
		int success;
		glGetProgramiv(programID, GL_LINK_STATUS, &success);
		if (success == 0) {
			const int MAX_INFO_LOG_SIZE = 4096;
			char info[MAX_INFO_LOG_SIZE] = { 0 };
			glGetProgramInfoLog(programID, MAX_INFO_LOG_SIZE - 1, nullptr, info);
			fprintf(stderr, "ZRenderOpenGL::LinkProgram(): %s\n", info);
			if (pass.compileCallback) {
				pass.compileCallback(pass, IRender::Resource::ShaderDescription::END, info, "<Link>");
			}
			// assert(false);
			Cleanup();
			return;
		} else if (pass.compileCallback) {
			pass.compileCallback(pass, IRender::Resource::ShaderDescription::END, "", "<Link>");
		}

		// Query texture locations.
		std::vector<GLuint>& textureLocations = program.textureLocations;
		textureLocations.reserve(textureNames.size());
		for (size_t n = 0; n < textureNames.size(); n++) {
			textureLocations.emplace_back(glGetUniformLocation(program.programID, textureNames[n].c_str()));
		}

		// Query uniform blocks locations
		std::vector<GLuint>& uniformBufferLocations = program.uniformBufferLocations;
		uniformBufferLocations.reserve(uniformBufferNames.size());
		for (size_t m = 0; m < uniformBufferNames.size(); m++) {
			uniformBufferLocations.emplace_back(glGetUniformBlockIndex(program.programID, uniformBufferNames[m].c_str()));
		}

		// Query shared buffer locations (SSBO)
		std::vector<GLuint>& sharedBufferLocations = program.sharedBufferLocations;
		sharedBufferLocations.reserve(sharedBufferNames.size());

		for (size_t t = 0; t < sharedBufferNames.size(); t++) {
			sharedBufferLocations.emplace_back(glGetProgramResourceIndex(program.programID, GL_SHADER_STORAGE_BLOCK, sharedBufferNames[t].c_str()));
		}
	}

	virtual void Download(QueueImplOpenGL& queue) override {
		assert(false);
	}

	virtual void Execute(QueueImplOpenGL& queue) override {} // Not an executable resource
	virtual void Delete(QueueImplOpenGL& queue) override {
		Cleanup();
		delete this;
	}

	Program program;
};

template <>
struct ResourceImplOpenGL<IRender::Resource::RenderStateDescription> final : public ResourceBaseImplOpenGLDesc<IRender::Resource::RenderStateDescription> {
	virtual IRender::Resource::Type GetType() const  override { return RESOURCE_RENDERSTATE; }
	virtual void Upload(QueueImplOpenGL& queue) override {
		UpdateDescription();
	}
	virtual void Download(QueueImplOpenGL& queue) override {}

	static GLuint GetTestEnum(uint8_t type) {
		switch (type) {
			case Resource::RenderStateDescription::DISABLED:
				return GL_NONE;
			case Resource::RenderStateDescription::ALWAYS:
				return GL_ALWAYS;
			case Resource::RenderStateDescription::LESS:
				return GL_LESS;
			case Resource::RenderStateDescription::LESS_EQUAL:
				return GL_LEQUAL;
			case Resource::RenderStateDescription::GREATER:
				return GL_GREATER;
			case Resource::RenderStateDescription::GREATER_EQUAL:
				return GL_GEQUAL;
			case Resource::RenderStateDescription::EQUAL:
				return GL_EQUAL;
		}

		return GL_NONE;
	}

	virtual void Execute(QueueImplOpenGL& queue) override {
		GL_GUARD();

		IRender::Resource::RenderStateDescription& d = GetDescription();
		if (memcmp(&queue.device->lastRenderState, &d, sizeof(d)) == 0)
			return;

		if (d.cull) {
			glEnable(GL_CULL_FACE);
			glFrontFace(GL_CCW);
			glCullFace(d.cullFrontFace ? GL_FRONT : GL_BACK);
		} else {
			glDisable(GL_CULL_FACE);
		}

		glPolygonMode(GL_FRONT_AND_BACK, d.fill ? GL_FILL : GL_LINE);

		// depth
		GLuint depthTest = GetTestEnum(d.depthTest);
		if (depthTest == GL_NONE) {
			glDisable(GL_DEPTH_TEST);
		} else {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(depthTest);
		}
		
		glDepthMask(d.depthWrite ? GL_TRUE : GL_FALSE);

		// stencil
		GLuint stencilTest = GetTestEnum(d.stencilTest);
		if (stencilTest == GL_NONE) {
			glDisable(GL_STENCIL_TEST);
		} else {
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(stencilTest, d.stencilValue, d.stencilMask);
			glStencilOp(d.stencilReplaceFail ? GL_REPLACE : GL_KEEP, d.stencilReplaceZFail ? GL_REPLACE : GL_KEEP, d.stencilReplacePass ? GL_REPLACE : GL_KEEP);
		}
		
		glStencilMask(d.stencilWrite ? 0xFFFFFFFF : 0);

		// alpha
		// Alpha test is not always available
		// please use discard operation manualy
		/*
		GLuint alphaTest = GetTestEnum(d.alphaTest);
		if (alphaTest == GL_NONE) {
			glDisable(GL_ALPHA_TEST);
		} else {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(alphaTest, 0.5f);
		}*/

		if (d.colorWrite) {
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		} else {
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		}

		if (d.blend) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // premultiplied
		} else {
			glDisable(GL_BLEND);
		}

		queue.device->lastRenderState = d;
		glFlush();
	}

	virtual void Delete(QueueImplOpenGL& queue) override {
		delete this;
	}

};

template <>
struct ResourceImplOpenGL<IRender::Resource::RenderTargetDescription> final : public ResourceBaseImplOpenGLDesc<IRender::Resource::RenderTargetDescription> {
	ResourceImplOpenGL() : vertexArrayID(0), frameBufferID(0) {}
	virtual IRender::Resource::Type GetType() const override { return RESOURCE_RENDERTARGET; }

#ifdef _DEBUG
	virtual void SetUploadDescription(IRender::Resource::Description* d) override {
		IRender::Resource::RenderTargetDescription* t = static_cast<IRender::Resource::RenderTargetDescription*>(d);
		for (size_t i = 0; i < t->colorStorages.size(); i++) {
			ResourceImplOpenGL<IRender::Resource::TextureDescription>* x = static_cast<ResourceImplOpenGL<IRender::Resource::TextureDescription>*>(t->colorStorages[i].resource);
			if (x != nullptr) {
				assert(x->GetNextDescription().dimension.x() != 0 && x->GetNextDescription().dimension.y() != 0);
				assert(x->GetNextDescription().state.attachment);
			}
		}
		ResourceBaseImplOpenGLDesc<IRender::Resource::RenderTargetDescription>::SetUploadDescription(d);
	}
#endif //

	void Cleanup() {
		GL_GUARD();

		if (vertexArrayID != 0) {
			glDeleteVertexArrays(1, &vertexArrayID);
		}

		if (frameBufferID != 0) {
			glDeleteFramebuffers(1, &frameBufferID);
			frameBufferID = 0;
		}
	}

	virtual void Upload(QueueImplOpenGL& queue) override {
		GL_GUARD();
		Resource::RenderTargetDescription& d = UpdateDescription();

		// Not back buffer
		if (vertexArrayID == 0) {
			glGenVertexArrays(1, &vertexArrayID);
		}

		if (!d.colorStorages.empty()) {
			// currently formats are not configurable for depth & stencil buffer
			GL_GUARD();
			if (frameBufferID == 0) {
				glGenFramebuffers(1, &frameBufferID);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
			queue.device->lastFrameBufferID = frameBufferID;

			if (d.depthStorage.resource == nullptr && d.stencilStorage.resource == nullptr) {
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			} else {
				ResourceImplOpenGL<IRender::Resource::TextureDescription>* t = static_cast<ResourceImplOpenGL<IRender::Resource::TextureDescription>*>(d.depthStorage.resource);
				ResourceImplOpenGL<IRender::Resource::TextureDescription>* s = static_cast<ResourceImplOpenGL<IRender::Resource::TextureDescription>*>(d.stencilStorage.resource);
				assert(t->textureID != 0);
				// do not support other types :D
				assert(t->GetDescription().state.layout == IRender::Resource::TextureDescription::DEPTH_STENCIL || t->GetDescription().state.layout == IRender::Resource::TextureDescription::DEPTH);
				assert(s == nullptr || s->GetDescription().state.layout == IRender::Resource::TextureDescription::DEPTH_STENCIL || s->GetDescription().state.layout == IRender::Resource::TextureDescription::STENCIL);
				IRender::Resource::TextureDescription& desc = t->GetDescription();

				if (desc.state.layout == IRender::Resource::TextureDescription::DEPTH_STENCIL) { // DEPTH_STENCIL
					assert(s == nullptr || t == s);
					if (desc.state.sample == IRender::Resource::TextureDescription::RENDERBUFFER) {
						glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, t->renderbufferID);
					} else {
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, t->textureID, d.depthStorage.mipLevel);
					}
				} else { // may not be supported on all platforms
					if (desc.state.sample == IRender::Resource::TextureDescription::RENDERBUFFER) {
						glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t->renderbufferID);
					} else {
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, t->textureID, d.depthStorage.mipLevel);
					}

					if (s != nullptr) {
						IRender::Resource::TextureDescription& descs = s->GetDescription();
						if (descs.state.sample == IRender::Resource::TextureDescription::RENDERBUFFER) {
							glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, s->renderbufferID);
						} else {
							glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, t->textureID, d.depthStorage.mipLevel);
						}
					} else {
						glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
					}
				}
			}

			// create texture slots first
			std::vector<uint32_t> renderBufferSlots;
			for (size_t i = 0; i < d.colorStorages.size(); i++) {
				GL_GUARD();
				IRender::Resource::RenderTargetDescription::Storage& storage = d.colorStorages[i];
				ResourceImplOpenGL<IRender::Resource::TextureDescription>* t = static_cast<ResourceImplOpenGL<IRender::Resource::TextureDescription>*>(storage.resource);
				assert(t != nullptr);
				assert(t->textureID != 0);
				if (t->GetDescription().state.sample == IRender::Resource::TextureDescription::RENDERBUFFER) {
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLsizei)i, GL_RENDERBUFFER, t->renderbufferID);
				} else {
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLsizei)i, GL_TEXTURE_2D, t->textureID, storage.mipLevel);
				}
#ifdef LOG_OPENGL
				printf("[OpenGL ColorAttach] %d = %p from %p\n", i, t, this);
#endif
			}

#ifdef _DEBUG
			unsigned int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			assert(status == GL_FRAMEBUFFER_COMPLETE);
#endif
		} else {
			assert(frameBufferID == 0);
		}
	}

	virtual void Download(QueueImplOpenGL& queue) override {
		// only supports downloading from textures.
		assert(false);
	}

	static void SetupAttachment(IRender::Resource::RenderTargetDescription::Storage& d, GLuint id) {
		if (d.loadOp == IRender::Resource::RenderTargetDescription::CLEAR) {
			glDrawBuffer(id);
			glClearColor(d.clearColor.r(), d.clearColor.g(), d.clearColor.b(), d.clearColor.a());
			glClear(GL_COLOR_BUFFER_BIT);
		}
		
		if (d.storeOp == IRender::Resource::RenderTargetDescription::DISCARD) {
			glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &id);
		}
	}

	virtual void Execute(QueueImplOpenGL& queue) override {
		Resource::RenderTargetDescription& d = GetDescription();
		GL_GUARD();

		queue.device->CompleteStore();
		queue.device->lastFrameBufferID = frameBufferID;

		glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
		glBindVertexArray(vertexArrayID);

		if (frameBufferID != 0) {
			GL_GUARD();

			GLuint clearMask = 0;
			std::vector<GLuint> loadInvalidates;
			std::vector<GLuint> storeInvalidates;
			loadInvalidates.reserve(8);
			storeInvalidates.reserve(8);

			if (d.depthStorage.loadOp == IRender::Resource::RenderTargetDescription::CLEAR) {
				if (!queue.device->lastRenderState.depthWrite) {
					glDepthMask(GL_TRUE);
				}

				clearMask |= GL_DEPTH_BUFFER_BIT;
			} else if (d.depthStorage.loadOp == IRender::Resource::RenderTargetDescription::DISCARD) {
				loadInvalidates.emplace_back(GL_DEPTH_ATTACHMENT);
			}

			if (d.depthStorage.storeOp == IRender::Resource::RenderTargetDescription::DISCARD) {
				storeInvalidates.emplace_back(GL_DEPTH_ATTACHMENT);
			}

			if (d.stencilStorage.loadOp == IRender::Resource::RenderTargetDescription::CLEAR) {
				if (!queue.device->lastRenderState.stencilWrite) {
					glStencilMask(0xFFFFFFFF);
				}

				clearMask |= GL_STENCIL_BUFFER_BIT;
			} else if (d.stencilStorage.loadOp == IRender::Resource::RenderTargetDescription::DISCARD) {
				if (loadInvalidates.empty()) {
					loadInvalidates.emplace_back(GL_STENCIL_ATTACHMENT);
				} else {
					loadInvalidates[0] = GL_DEPTH_STENCIL_ATTACHMENT;
				}
			}

			if (d.stencilStorage.storeOp == IRender::Resource::RenderTargetDescription::DISCARD) {
				if (storeInvalidates.empty()) {
					storeInvalidates.emplace_back(GL_STENCIL_ATTACHMENT);
				} else {
					storeInvalidates[0] = GL_DEPTH_STENCIL_ATTACHMENT;
				}
			}

			if (clearMask != 0) {
				glClearDepth(0.0f);
				glClearStencil(0);
				glClear(clearMask);
			}

			for (size_t i = 0; i < d.colorStorages.size(); i++) {
				IRender::Resource::RenderTargetDescription::Storage& t = d.colorStorages[i];
				if (t.loadOp == IRender::Resource::RenderTargetDescription::CLEAR) {
					glDrawBuffer((GLenum)(GL_COLOR_ATTACHMENT0 + i));
					glClearColor(t.clearColor.r(), t.clearColor.g(), t.clearColor.b(), t.clearColor.a());
					glClear(GL_COLOR_BUFFER_BIT);
				} else if (t.loadOp == IRender::Resource::RenderTargetDescription::DISCARD) {
					loadInvalidates.emplace_back((GLenum)(GL_COLOR_ATTACHMENT0 + i));
				}

				if (t.storeOp == IRender::Resource::RenderTargetDescription::DISCARD) {
					storeInvalidates.emplace_back((GLenum)(GL_COLOR_ATTACHMENT0 + i));
				}
			}

			if (!loadInvalidates.empty()) {
				glInvalidateFramebuffer(GL_FRAMEBUFFER, (GLsizei)loadInvalidates.size(), &loadInvalidates[0]);
			}

			std::swap(queue.device->storeInvalidates, storeInvalidates);

			// recover state
			if (d.depthStorage.loadOp == IRender::Resource::RenderTargetDescription::CLEAR) {
				if (!queue.device->lastRenderState.depthWrite) {
					glDepthMask(GL_FALSE);
				}
			}

			if (d.stencilStorage.loadOp == IRender::Resource::RenderTargetDescription::CLEAR) {
				if (!queue.device->lastRenderState.stencilWrite) {
					glStencilMask(0);
				}
			}

			const size_t MAX_ID = 8;
			static GLuint idlist[MAX_ID] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1,
			GL_COLOR_ATTACHMENT0 + 2, GL_COLOR_ATTACHMENT0 + 3, GL_COLOR_ATTACHMENT0 + 4,
			GL_COLOR_ATTACHMENT0 + 5, GL_COLOR_ATTACHMENT0 + 6, GL_COLOR_ATTACHMENT0 + 7 };

			if (d.colorStorages.empty()) {
				glDrawBuffer(GL_NONE);
			} else {
				glDrawBuffers((GLsizei)Math::Min(MAX_ID, d.colorStorages.size()), idlist);
			}
		}

		UShort2Pair range = d.range;
		if (range.second.x() == 0) {
			if (d.colorStorages.empty()) {
				range.second.x() = queue.device->resolution.x();
			} else {
				ResourceImplOpenGL<TextureDescription>* texture = static_cast<ResourceImplOpenGL<TextureDescription>*>(d.colorStorages[0].resource);
				range.second.x() = texture->GetDescription().dimension.x();
			}
		}

		if (range.second.y() == 0) {
			if (d.colorStorages.empty()) {
				range.second.y() = queue.device->resolution.y();
			} else {
				ResourceImplOpenGL<TextureDescription>* texture = static_cast<ResourceImplOpenGL<TextureDescription>*>(d.colorStorages[0].resource);
				range.second.y() = texture->GetDescription().dimension.y();
			}
		}

		glViewport(range.first.x(), range.first.y(), range.second.x() - range.first.x(), range.second.y() - range.first.y());
	}

	virtual void Delete(QueueImplOpenGL& queue) override {
		Cleanup();
		delete this;
	}

	GLuint vertexArrayID;
	GLuint frameBufferID;
	GLuint depthStencilBufferID;
	GLuint clearMask;
};

template <>
struct ResourceImplOpenGL<IRender::Resource::DrawCallDescription> final : public ResourceBaseImplOpenGLDesc<IRender::Resource::DrawCallDescription> {
	virtual IRender::Resource::Type GetType() const override { return RESOURCE_DRAWCALL; }
	virtual void Upload(QueueImplOpenGL& queue) override {
#ifdef _DEBUG
		for (size_t i = 0; i < nextDescription.textureResources.size(); i++) {
			assert(nextDescription.textureResources[i] != nullptr);
		}

		for (size_t k = 0; k < nextDescription.bufferResources.size(); k++) {
			assert(nextDescription.bufferResources[k].buffer != nullptr);
		}
#endif
		UpdateDescription();
	}

	virtual void Download(QueueImplOpenGL& queue) override {}
	virtual void Execute(QueueImplOpenGL& queue) override {
		GL_GUARD();

		typedef ResourceImplOpenGL<ShaderDescription> Shader;
		typedef ResourceImplOpenGL<BufferDescription> Buffer;
		typedef DrawCallDescription::BufferRange BufferRange;
		typedef ResourceImplOpenGL<TextureDescription> Texture;
		Resource::DrawCallDescription& d = GetDescription();

		Shader* shader = static_cast<Shader*>(d.shaderResource);
		assert(shader != nullptr);

		const Shader::Program& program = shader->program;
		GLuint programID = program.programID;
		if (queue.device->lastProgramID != programID) {
			glUseProgram(queue.device->lastProgramID = programID);
		}

		GLuint vertexBufferBindingCount = 0;
		GLuint uniformBufferBindingCount = 0;
		GLuint sharedBufferBindingCount = 0;
		for (size_t i = 0; i < d.bufferResources.size(); i++) {
			const BufferRange& bufferRange = d.bufferResources[i];
			const Buffer* buffer = static_cast<const Buffer*>(bufferRange.buffer);
			assert(buffer != nullptr);
			uint32_t k;
			GLuint bufferElementType = GL_FLOAT;
			GLuint bufferElementSize = sizeof(float);
			if (buffer->usage != IRender::Resource::BufferDescription::UNIFORM) {
				switch (buffer->format) {
				case IRender::Resource::Description::UNSIGNED_BYTE:
					bufferElementType = GL_UNSIGNED_BYTE;
					bufferElementSize = sizeof(unsigned char);
					break;
				case IRender::Resource::Description::UNSIGNED_SHORT:
					bufferElementType = GL_UNSIGNED_SHORT;
					bufferElementSize = sizeof(unsigned short);
					break;
				case IRender::Resource::Description::FLOAT:
					bufferElementType = GL_FLOAT;
					bufferElementSize = sizeof(float);
					break;
				case IRender::Resource::Description::UNSIGNED_INT:
					bufferElementType = GL_UNSIGNED_INT;
					bufferElementSize = sizeof(unsigned int);
					break;
				}
			}

			uint32_t bufferComponent = bufferRange.component == 0 ? buffer->component : bufferRange.component;
			assert(bufferComponent != 0);

			switch (buffer->usage) {
				case Resource::BufferDescription::VERTEX:
				{
					GL_GUARD();
					glEnableVertexAttribArray(vertexBufferBindingCount);
					glBindBuffer(GL_ARRAY_BUFFER, buffer->bufferID);
					glVertexAttribPointer(vertexBufferBindingCount, bufferComponent, bufferElementType, GL_FALSE, buffer->component * bufferElementSize, reinterpret_cast<void*>((size_t)bufferRange.offset));
					vertexBufferBindingCount++;
					break;
				}
				case Resource::BufferDescription::INSTANCED:
				{
					GL_GUARD();
					assert(bufferComponent % 4 == 0);
					glBindBuffer(GL_ARRAY_BUFFER, buffer->bufferID);
					for (k = 0; k < bufferComponent; k += 4) {
						glEnableVertexAttribArray(vertexBufferBindingCount);
						glVertexAttribPointer(vertexBufferBindingCount, Math::Min(4u, bufferComponent - k), bufferElementType, GL_FALSE, bufferComponent * sizeof(float), reinterpret_cast<void*>((size_t)bufferRange.offset + sizeof(float) * k));
						glVertexAttribDivisor(vertexBufferBindingCount, 1);
						vertexBufferBindingCount++;
					}
					break;
				}
				case Resource::BufferDescription::UNIFORM:
				{
					GL_GUARD();
					GLuint blockIndex = program.uniformBufferLocations[uniformBufferBindingCount];
					assert(blockIndex != (GLuint)-1);
					assert(bufferRange.offset + bufferRange.length <= buffer->length);
					glBindBufferRange(GL_UNIFORM_BUFFER, uniformBufferBindingCount, (GLuint)buffer->bufferID, bufferRange.offset, bufferRange.length == 0 ? buffer->length : bufferRange.length);
					glUniformBlockBinding(programID, blockIndex, uniformBufferBindingCount);
					uniformBufferBindingCount++;
					break;
				}
				case Resource::BufferDescription::STORAGE:
				{
					GL_GUARD();
					GLuint blockIndex = program.sharedBufferLocations[sharedBufferBindingCount];
					assert(blockIndex != (GLuint)-1);
					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, sharedBufferBindingCount, (GLuint)buffer->bufferID, bufferRange.offset, bufferRange.length == 0 ? buffer->length : bufferRange.length);
					glShaderStorageBlockBinding(programID, blockIndex, sharedBufferBindingCount);
					sharedBufferBindingCount++;
					break;
				}
			}
		}

		assert(d.textureResources.size() == program.textureLocations.size());
		for (uint32_t k = 0; k < d.textureResources.size(); k++) {
			GL_GUARD();
			const Texture* texture = static_cast<const Texture*>(d.textureResources[k]);
			assert(texture != nullptr);
			assert(texture->textureID != 0);
			glActiveTexture((GLsizei)(GL_TEXTURE0 + k));
			glBindTexture(texture->textureType, texture->textureID);
			glUniform1i(program.textureLocations[k], (GLuint)k);
		}

		if (program.isComputeShader) {
			glDispatchCompute(d.instanceCounts.x(), d.instanceCounts.y(), d.instanceCounts.z());
		} else {
			const Buffer* indexBuffer = static_cast<const Buffer*>(d.indexBufferResource.buffer);
			if (indexBuffer != nullptr) {
				uint32_t indexBufferLength = d.indexBufferResource.length == 0 ? indexBuffer->length : d.indexBufferResource.length;
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->bufferID);
				if (d.instanceCounts.x() == 0) {
					glDrawElements(GL_TRIANGLES, (GLsizei)indexBufferLength / sizeof(GLuint), GL_UNSIGNED_INT, (const void*)((size_t)d.indexBufferResource.offset));
				} else {
					glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)indexBufferLength / sizeof(GLuint), GL_UNSIGNED_INT, (const void*)((size_t)d.indexBufferResource.offset), (GLsizei)d.instanceCounts.x());
				}
			} else {
				if (d.instanceCounts.x() == 0) {
					glDrawArrays(GL_TRIANGLES, d.indexBufferResource.offset, d.indexBufferResource.length);
				} else {
					glDrawArraysInstanced(GL_TRIANGLES, d.indexBufferResource.offset, d.indexBufferResource.length, (GLsizei)d.instanceCounts.x());
				}
			}
		}
	}

	virtual void Delete(QueueImplOpenGL& queue) override {
		delete this;
	}
};

IRender::Device* ZRenderOpenGL::CreateDevice(const String& description) {
	if (description.empty()) {
		return new DeviceImplOpenGL(*this); // by now we only supports one device
	} else {
		return nullptr;
	}
}

void ZRenderOpenGL::SetDeviceResolution(IRender::Device* device, const Int2& resolution) {
	DeviceImplOpenGL* impl = static_cast<DeviceImplOpenGL*>(device);
	impl->resolution = resolution;
}

Int2 ZRenderOpenGL::GetDeviceResolution(IRender::Device* device) {
	DeviceImplOpenGL* impl = static_cast<DeviceImplOpenGL*>(device);
	return impl->resolution;
}

void ZRenderOpenGL::DeleteDevice(IRender::Device* device) {
	DeviceImplOpenGL* impl = static_cast<DeviceImplOpenGL*>(device);
	delete impl;
}

void ZRenderOpenGL::NextDeviceFrame(IRender::Device* device) {
	ClearDeletedQueues();
}

// Queue
IRender::Queue* ZRenderOpenGL::CreateQueue(Device* device, uint32_t flag) {
	assert(device != nullptr);
	return new QueueImplOpenGL(static_cast<DeviceImplOpenGL*>(device), !!(flag & IRender::QUEUE_MULTITHREAD));
}

IRender::Device* ZRenderOpenGL::GetQueueDevice(Queue* queue) {
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	return q->device;
}

std::vector<String> ZRenderOpenGL::EnumerateDevices() {
	return std::vector<String>();
}

void ZRenderOpenGL::PresentQueues(Queue** queues, uint32_t count, PresentOption option) {
	GL_GUARD();
	void (QueueImplOpenGL::*op)() = &QueueImplOpenGL::ExecuteAll;
	switch (option) {
	case PresentOption::PRESENT_EXECUTE_ALL:
		op = &QueueImplOpenGL::ExecuteAll;
		break;
	case PresentOption::PRESENT_REPEAT:
		op = &QueueImplOpenGL::Repeat;
		break;
	case PresentOption::PRESENT_EXECUTE:
		op = &QueueImplOpenGL::Execute;
		break;
	case PresentOption::PRESENT_CLEAR_ALL:
		op = &QueueImplOpenGL::ClearAll;
		break;
	}

	for (uint32_t k = 0; k < count; k++) {
		QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queues[k]);
		(q->*op)();
	}
}

bool ZRenderOpenGL::SupportParallelPresent(Device* device) {
	return false;
}

void ZRenderOpenGL::FlushQueue(Queue* queue) {
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	assert(queue != nullptr);
	q->Flush();
}

void ZRenderOpenGL::DeleteQueue(Queue* queue) {
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	assert(queue != nullptr);
	SpinLock(critical);
	deletedQueues.Push(q);
	SpinUnLock(critical);
}

// Resource
IRender::Resource* ZRenderOpenGL::CreateResource(Device* device, Resource::Type resourceType) {
	switch (resourceType) {
	case Resource::RESOURCE_TEXTURE:
		return new ResourceImplOpenGL<Resource::TextureDescription>();
	case Resource::RESOURCE_BUFFER:
		return new ResourceImplOpenGL<Resource::BufferDescription>();
	case Resource::RESOURCE_SHADER:
		return new ResourceImplOpenGL<Resource::ShaderDescription>();
	case Resource::RESOURCE_RENDERSTATE:
		return new ResourceImplOpenGL<Resource::RenderStateDescription>();
	case Resource::RESOURCE_RENDERTARGET:
		return new ResourceImplOpenGL<Resource::RenderTargetDescription>();
	case Resource::RESOURCE_DRAWCALL:
		return new ResourceImplOpenGL<Resource::DrawCallDescription>();
	}

	assert(false);
	return nullptr;
}

void ZRenderOpenGL::UploadResource(Queue* queue, Resource* resource, Resource::Description* description) {
	assert(queue != nullptr);
	assert(resource != nullptr);
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	ResourceBaseImplOpenGL* impl = static_cast<ResourceBaseImplOpenGL*>(resource);
	impl->SetUploadDescription(description);
	q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_UPLOAD, resource));
}

void ZRenderOpenGL::RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) {
	assert(queue != nullptr);
	assert(resource != nullptr);
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	ResourceBaseImplOpenGL* impl = static_cast<ResourceBaseImplOpenGL*>(resource);
	impl->SetDownloadDescription(description);
	q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_DOWNLOAD, resource));
}

void ZRenderOpenGL::CompleteDownloadResource(Queue* queue, Resource* resource) {
	assert(queue != nullptr);
	assert(resource != nullptr);
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	ResourceBaseImplOpenGL* impl = static_cast<ResourceBaseImplOpenGL*>(resource);
	impl->SyncDownload(*q);
	// q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_SYNC_DOWNLOAD, resource));
}

void ZRenderOpenGL::ExecuteResource(Queue* queue, Resource* resource) {
	assert(queue != nullptr);
	assert(resource != nullptr);
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_EXECUTE, resource));
}

void ZRenderOpenGL::AcquireResource(Queue* queue, Resource* resource) {
	// OpenGL does not support multi-thread committing and reordering
}

void ZRenderOpenGL::ReleaseResource(Queue* queue, Resource* resource) {
	// OpenGL does not support multi-thread committing and reordering
}

void ZRenderOpenGL::SwapResource(Queue* queue, Resource* lhs, Resource* rhs) {
	assert(queue != nullptr);
	assert(lhs != nullptr);
	assert(rhs != nullptr);
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_PRESWAP, lhs));
	q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_POSTSWAP, rhs));
}

void ZRenderOpenGL::SetResourceNotation(Resource* resource, const String& note) {
#ifdef _DEBUG
	ResourceBaseImplOpenGL* base = static_cast<ResourceBaseImplOpenGL*>(resource);
	base->note = note;
#endif
}

void ZRenderOpenGL::DeleteResource(Queue* queue, Resource* resource) {
	assert(resource != nullptr);
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_DELETE, resource));
}

class GlewInit {
public:
	GlewInit() {
		glewExperimental = true;
		glewInit();
		printf("%s\n%s\n", glGetString(GL_VERSION), glGetString(GL_VENDOR));
		glDisable(GL_MULTISAMPLE);

		/*
		GLint extCount = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extCount);
		for (GLint i = 0; i < extCount; i++) {
			printf("Extension: %s supported.\n", glGetStringi(GL_EXTENSIONS, i));
		}*/
	}
};

ZRenderOpenGL::ZRenderOpenGL() {
	static GlewInit init;
	critical.store(0, std::memory_order_release);
}

void ZRenderOpenGL::ClearDeletedQueues() {
	// free all deleted resources
	while (!deletedQueues.Empty()) {
		Queue* q = deletedQueues.Top();
		deletedQueues.Pop();
		QueueImplOpenGL* queue = static_cast<QueueImplOpenGL*>(q);
		queue->ClearAll();
		delete queue;
	}
}

ZRenderOpenGL::~ZRenderOpenGL() {
	GLErrorGuard::enableGuard = false;
	ClearDeletedQueues();
}
