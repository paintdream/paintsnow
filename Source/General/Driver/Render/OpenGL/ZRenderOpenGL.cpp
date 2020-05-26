#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "../../../Interface/Interfaces.h"
#include "../../../Interface/IShader.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Template/TQueue.h"
#include "ZRenderOpenGL.h"
#include <cstdio>
#include <vector>
#include <iterator>
#include <sstream>

#if USE_STATIC_THIRDPARTY_LIBRARIES
#define GLEW_STATIC
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

class DeviceImplOpenGL : public IRender::Device {
public:
	DeviceImplOpenGL(IRender& r) : lastProgramID(0), lastFrameBufferID(0), render(r) {}

	IRender& render;
	Int2 resolution;
	IRender::Resource::RenderStateDescription lastRenderState;
	IRender::Resource::ClearDescription lastClear;
	GLuint lastProgramID;
	GLuint lastFrameBufferID;
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
};

struct ResourceCommandImplOpenGL {
	typedef void (ResourceBaseImplOpenGL::*Action)(QueueImplOpenGL& queue);
	enum Operation {
		OP_NOP = 0,
		OP_EXECUTE = 1,
		OP_UPLOAD = 2,
		OP_DOWNLOAD = 3,
		OP_DELETE = 4,
		OP_PRESWAP = 5,
		OP_POSTSWAP = 6,
		OP_MAX = 6,
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
			&ResourceBaseImplOpenGL::Nop,
			&ResourceBaseImplOpenGL::Execute,
			&ResourceBaseImplOpenGL::Upload,
			&ResourceBaseImplOpenGL::Download,
			&ResourceBaseImplOpenGL::Delete,
			&ResourceBaseImplOpenGL::PreSwap,
			&ResourceBaseImplOpenGL::PostSwap
		};

		if (GetResource() == nullptr) {
			return GetOperation() == OP_NOP;
		}

		ResourceBaseImplOpenGL* impl = static_cast<ResourceBaseImplOpenGL*>(GetResource());
		uint8_t index = safe_cast<uint8_t>(GetOperation());
		assert(index < sizeof(actionTable) / sizeof(actionTable[0]));
		Action action = actionTable[index];
#ifdef _DEBUG
		const char* opnames[] = {
			"Nop", "Execute", "Upload", "Download", "Delete", "PreSwap"
		};

		const char* types[] = {
			"Nop", "Unknown", "Texture", "Buffer", "Shader", "RenderState", "RenderTarget", "Clear", "DrawCall"
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

struct QueueImplOpenGL : public IRender::Queue {
	QueueImplOpenGL(DeviceImplOpenGL* d, bool shared) : device(d) {
		critical.store(shared ? 0u : ~(uint32_t)0u, std::memory_order_relaxed);
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

	inline void Merge(QueueImplOpenGL* source) {
		while (!source->queuedCommands.Empty()) {
			ResourceCommandImplOpenGL command = source->queuedCommands.Top();
			source->queuedCommands.Pop();
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
			if (command.Invoke(queue)) {
#ifdef _DEBUG
				count++;
#endif
				if (command.GetOperation() != ResourceCommandImplOpenGL::OP_EXECUTE) {
					// replace it with nop
					command = ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_NOP, nullptr);
				}

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

	void RepeatToYield() {
		Scanner scanner(*this);
		queuedCommands.Iterate(scanner);
	}

	void ExecuteAll() {
		ResourceCommandImplOpenGL command;
		while (!queuedCommands.Empty()) {
			ResourceCommandImplOpenGL& command = queuedCommands.Top();
			PrefetchCommand(queuedCommands.Predict());
			command.Invoke(*this);
			queuedCommands.Pop();
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

	void ConsumeYield() {
		while (!queuedCommands.Empty()) {
			ResourceCommandImplOpenGL command = queuedCommands.Top();
			queuedCommands.Pop();
			if (command.GetOperation() != ResourceCommandImplOpenGL::OP_NOP) {
				if (command.GetResource() == nullptr) { // yield?
					return;
				} else if (command.GetOperation() != ResourceCommandImplOpenGL::OP_EXECUTE) {
					command.Invoke(*this);
				}
			}
		}

		assert(false); // No yield detected
	}

	void ExecuteToYield() {
		while (!queuedCommands.Empty()) {
			ResourceCommandImplOpenGL command = queuedCommands.Top();
			if (command.GetOperation() != ResourceCommandImplOpenGL::OP_NOP) {
				if (command.GetResource() == nullptr) { // yield?
					return;
				} else {
					command.Invoke(*this);
				}
			}

			queuedCommands.Pop();
		}

		assert(false); // No yield detected
	}

	DeviceImplOpenGL* device;
	TAtomic<int32_t> critical;
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
	TAtomic<uint32_t> critical;
	T currentDescription;
	T nextDescription;
};

template <class T>
struct ResourceImplOpenGL : public ResourceBaseImplOpenGLDesc<T> {
	virtual IRender::Resource::Type GetType() const  override { return IRender::Resource::RESOURCE_UNKNOWN; }
	virtual void Upload(QueueImplOpenGL& queue) override { assert(false); }
	virtual void Download(QueueImplOpenGL& queue) override { assert(false); }
	virtual void Execute(QueueImplOpenGL& queue) override { assert(false); }
	virtual void Delete(QueueImplOpenGL& queue) override { assert(false); }
};

template <>
struct ResourceImplOpenGL<IRender::Resource::TextureDescription> : public ResourceBaseImplOpenGLDesc<IRender::Resource::TextureDescription> {
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
				static const GLuint presets[] = { GL_R8, GL_RG8, GL_RGB8, GL_RGBA8, GL_NONE, GL_STENCIL_INDEX8, GL_DEPTH24_STENCIL8, GL_NONE, GL_NONE };
				static const GLuint byteDepths[] = { 1, 2, 3, 4, 0, 1, 4, 0, 0 };
				format = presets[state.layout];
				byteDepth = byteDepths[state.layout];
				srcDataType = GL_UNSIGNED_BYTE;
				break;
			}
			case Resource::TextureDescription::UNSIGNED_SHORT:
			{
				static const GLuint presets[] = { GL_R16, GL_RG16, GL_RGB16, GL_RGBA16, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX16, GL_NONE, GL_RGB10_A2, GL_NONE };
				static const GLuint byteDepths[] = { 2, 4, 6, 8, 2, 2, 0, 4, 0 };
				format = presets[state.layout];
				byteDepth = byteDepths[state.layout];
				srcDataType = GL_UNSIGNED_SHORT;
				break;
			}
			case Resource::TextureDescription::HALF_FLOAT:
			{
				static const GLuint presets[] = { GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX16, GL_NONE, GL_NONE, GL_NONE };
				static const GLuint byteDepths[] = { 2, 4, 6, 8, 2, 2, 0, 0, 0 };
				format = presets[state.layout];
				byteDepth = byteDepths[state.layout];
				srcDataType = GL_HALF_FLOAT;
				break;
			}
			case Resource::TextureDescription::FLOAT:
			{
				static const GLuint presets[] = { GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F, GL_DEPTH24_STENCIL8, GL_NONE, GL_DEPTH24_STENCIL8, GL_NONE, GL_R11F_G11F_B10F };
				static const GLuint byteDepths[] = { 4, 8, 12, 16, 4, 0, 4, 0, 4 };
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
		static const GLuint layouts[] = { GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_DEPTH_COMPONENT, GL_STENCIL_INDEX, GL_DEPTH_STENCIL, GL_RGBA, GL_RGB };
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
					glBindTexture(textureType = GL_TEXTURE_2D, textureID);
					if (d.state.compress) {
						glCompressedTexImage2D(textureType, 0, format, d.dimension.x(), d.dimension.y(), 0, bitDepth * d.dimension.x() * d.dimension.y() / 8, data);
					} else {
						if (d.state.immutable && glTexStorage2D != nullptr) {
							if (newTexture) {
								glTexStorage2D(textureType, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Log2((uint32_t)Min(d.dimension.x(), d.dimension.y())) + 1, format, d.dimension.x(), d.dimension.y());
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
					glBindTexture(textureType = GL_TEXTURE_CUBE_MAP, textureID);
					assert(d.data.GetSize() % 6 == 0);
					size_t each = d.data.GetSize() / 6;

					static const GLuint index[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

					if (d.state.immutable && glTexStorage2D != nullptr) {
						if (newTexture) {
							glTexStorage2D(GL_TEXTURE_CUBE_MAP, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Log2((uint32_t)Min(d.dimension.x(), d.dimension.y())) + 1, format, d.dimension.x(), d.dimension.y());
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
					glBindTexture(textureType = GL_TEXTURE_3D, textureID);
					if (d.state.compress) {
						assert(data != nullptr);
						glCompressedTexImage3D(textureType, 0, format, d.dimension.x(), d.dimension.y(), d.dimension.z(), 0, bitDepth * d.dimension.x() * d.dimension.y() * d.dimension.z() / 8, data);
					} else {
						if (d.state.immutable && glTexStorage3D != nullptr) {
							if (newTexture) {
								glTexStorage3D(textureType, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Log2((uint32_t)Min(Min(d.dimension.x(), d.dimension.y()), d.dimension.z())) + 1, format, d.dimension.x(), d.dimension.y(), d.dimension.z());
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
					glBindTexture(textureType = GL_TEXTURE_1D_ARRAY, textureID);
					if (d.state.compress) {
						assert(data != nullptr);
						glCompressedTexImage2D(textureType, 0, format, d.dimension.x(), d.dimension.y(), 0, bitDepth * d.dimension.x() * d.dimension.y() / 8, data);
					} else {
						if (d.state.immutable && glTexStorage2D != nullptr) {
							if (newTexture) {
								glTexStorage2D(textureType, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Log2((uint32_t)d.dimension.x()) + 1, format, d.dimension.x(), d.dimension.y());
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
					glBindTexture(textureType = GL_TEXTURE_2D_ARRAY, textureID);
					assert(d.data.GetSize() % d.dimension.z() == 0);
					if (d.state.compress) {
						assert(data != nullptr);
						glCompressedTexImage3D(textureType, 0, format, d.dimension.x(), d.dimension.y(), d.dimension.z(), 0, bitDepth * d.dimension.x() * d.dimension.y() * d.dimension.z() / 8, data);
					} else {
						if (d.state.immutable && glTexStorage3D != nullptr) {
							if (newTexture) {
								glTexStorage3D(textureType, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Log2((uint32_t)Min(d.dimension.x(), d.dimension.y())) + 1, format, d.dimension.x(), d.dimension.y(), d.dimension.z());
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
					glBindTexture(textureType = GL_TEXTURE_CUBE_MAP_ARRAY, textureID);
					assert(d.data.GetSize() % 6 == 0);
					size_t each = d.data.GetSize() / 6;

					static const GLuint index[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

					if (d.state.immutable && glTexStorage3D != nullptr) {
						if (newTexture) {
							glTexStorage3D(GL_TEXTURE_CUBE_MAP, d.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Log2((uint32_t)Min(d.dimension.x(), d.dimension.y())) + 1, format, d.dimension.x(), d.dimension.y(), d.dimension.z());
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
struct ResourceImplOpenGL<IRender::Resource::BufferDescription> : public ResourceBaseImplOpenGLDesc<IRender::Resource::BufferDescription> {
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
			case Resource::BufferDescription::SHARED:
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

// Shader Compilation
static const String frameCode = "#version 330\n\
#define PI 3.1415926 \n\
#define GAMMA 2.2 \n\
#define clip(f) if (f < 0) discard; \n\
#define float2 vec2 \n\
#define float3 vec3 \n\
#define float4 vec4 \n\
#define float3x3 mat3 \n\
#define float4x4 mat4 \n\
#define make_float3x3(a11, a12, a13, a21, a22, a23, a31, a32, a33) float3x3(a11, a12, a13, a21, a22, a23, a31, a32, a33) \n\
#define make_float4x4(a11, a12, a13, a14, a21, a22, a23, a24, a31, a32, a33, a34, a41, a42, a43, a44) float4x4(a11, a12, a13, a14, a21, a22, a23, a24, a31, a32, a33, a34, a41, a42, a43, a44) \n\
#define lerp(a, b, v) mix(a, b, v) \n\
#define ddx dFdx \n\
#define ddy dFdy \n\
float saturate(float x) { return clamp(x, 0.0, 1.0); } \n\
float2 saturate(float2 x) { return clamp(x, float2(0.0, 0.0), float2(1.0, 1.0)); } \n\
float3 saturate(float3 x) { return clamp(x, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0)); } \n\
float4 saturate(float4 x) { return clamp(x, float4(0.0, 0.0, 0.0, 0.0), float4(1.0, 1.0, 1.0, 1.0)); } \n\
#define mult_mat(a, b) (a * b) \n\
#define mult_vec(m, v) (m * v) \n";

// Compute shader frame code
static const String computeFrameCode = "\n\
#define WorkGroupSize gl_WorkGroupSize \n\
#define NumWorkGroups gl_NumWorkGroups \n\
#define LocalInvocationID gl_LocalInvocationID \n\
#define WorkGroupID gl_WorkGroupID \n\
#define GlobalInvocationID gl_GlobalInvocationID \n\
#define LocalInvocationIndex gl_LocalInvocationIndex \n\
\n";

class ShaderDeclarationOpenGL : public IReflect {
public:
	ShaderDeclarationOpenGL(IRender::Resource::ShaderDescription::Stage s, uint32_t& pinputIndex, uint32_t& poutputIndex, uint32_t& ptextureIndex) : IReflect(true, false), stage(s), inputIndex(pinputIndex), outputIndex(poutputIndex), textureIndex(ptextureIndex) {}
	void Complete() {
		for (size_t i = 0; i < orderedBuffers.size(); i++) {
			const IShader::BindBuffer* buffer = orderedBuffers[i];
			const std::pair<String, String>& info = mapBufferDeclaration[buffer];
			switch (buffer->description.usage) {
				case IRender::Resource::BufferDescription::VERTEX:
				case IRender::Resource::BufferDescription::INSTANCED:
					declaration += info.second;
					break;
				case IRender::Resource::BufferDescription::UNIFORM:
				case IRender::Resource::BufferDescription::SHARED:
					if (!info.second.empty()) {
						declaration += (buffer->description.usage == IRender::Resource::BufferDescription::SHARED ? String("buffer _") : stage == IRender::Resource::ShaderDescription::COMPUTE ? String("shared _") : String("uniform _")) + info.first + " {\n" + info.second + "} " + info.first + ";\n";
					}
					break;
			}
		}
	}

	IRender::Resource::ShaderDescription::Stage stage;
	String declaration;
	String initialization;
	String finalization;

	uint32_t& inputIndex;
	uint32_t& outputIndex;
	uint32_t& textureIndex;

	static inline String ToString(uint32_t value) {
		std::stringstream ss;
		ss << value;
		return ss.str();
	}

	struct DeclareMap {
		DeclareMap() {
			mapTypeNames[UniqueType<float>::Get()] = "float";
			mapTypeNames[UniqueType<Float2>::Get()] = "float2";
			mapTypeNames[UniqueType<Float3>::Get()] = "float3";
			mapTypeNames[UniqueType<Float4>::Get()] = "float4";
			mapTypeNames[UniqueType<float>::Get()] = "float";
			mapTypeNames[UniqueType<Float2>::Get()] = "float2";
			mapTypeNames[UniqueType<Float3>::Get()] = "float3";
			mapTypeNames[UniqueType<Float4>::Get()] = "float4";
			mapTypeNames[UniqueType<MatrixFloat3x3>::Get()] = "float3x3";
			mapTypeNames[UniqueType<MatrixFloat4x4>::Get()] = "float4x4";
		}

		String operator [] (Unique id) {
			std::map<Unique, String>::iterator it = mapTypeNames.find(id);
			if (it != mapTypeNames.end()) {
				return it->second;
			} else {
				assert(false); // unrecognized
				return "float4";
			}
		}

		std::map<Unique, String> mapTypeNames;
	};

	virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
		// static Unique uniqueBindOffset = UniqueType<IShader::BindOffset>::Get();
		static Unique uniqueBindInput = UniqueType<IShader::BindInput>::Get();
		static Unique uniqueBindOutput = UniqueType<IShader::BindOutput>::Get();
		static Unique uniqueBindTexture = UniqueType<IShader::BindTexture>::Get();
		static Unique uniqueBindBuffer = UniqueType<IShader::BindBuffer>::Get();
		static Unique uniqueBindConstBool = UniqueType<IShader::BindConst<bool> >::Get();
		static Unique uniqueBindConstInt = UniqueType<IShader::BindConst<int> >::Get();
		static Unique uniqueBindConstFloat = UniqueType<IShader::BindConst<float> >::Get();
		static DeclareMap declareMap;

		if (s.IsBasicObject() || s.IsIterator()) {
			String statement;
			const IShader::BindBuffer* bindBuffer = nullptr;
			String arr;
			String arrDef;
			static Unique uniqueBindOption = UniqueType<IShader::BindOption>::Get();

			for (const MetaChainBase* pre = meta; pre != nullptr; pre = pre->GetNext()) {
				const MetaNodeBase* node = pre->GetNode();
				Unique uniqueNode = node->GetUnique();
				if (uniqueNode == uniqueBindOption) {
					const IShader::BindOption* bindOption = static_cast<const IShader::BindOption*>(pre->GetNode());
					if (!*bindOption->description) {
						// defines as local
						if (s.IsIterator()) {
							IIterator& iterator = static_cast<IIterator&>(s);
							initialization += String("\t") + declareMap[iterator.GetPrototypeUnique()] + " " + name + "[1];\n";
						} else {
							initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
						}
						return;
					}
				}

				// Bind directly
				if (uniqueNode == uniqueBindBuffer) {
					bindBuffer = static_cast<const IShader::BindBuffer*>(pre->GetRawNode());
				}
			}

			if (s.IsIterator()) {
				IIterator& iterator = static_cast<IIterator&>(s);
				std::stringstream ss;
				ss << "[" << (int)iterator.GetTotalCount() << "]";
				arr = ss.str();
				arrDef = "[]";

				typeID = iterator.GetPrototypeUnique();
			}

			for (const MetaChainBase* chain = meta; chain != nullptr; chain = chain->GetNext()) {
				const MetaNodeBase* node = chain->GetNode();
				Unique uniqueNode = node->GetUnique();

				// Bind directly
				if (uniqueNode == uniqueBindInput) {
					const IShader::BindInput* bindInput = static_cast<const IShader::BindInput*>(node);
					if (bindInput->description == IShader::BindInput::COMPUTE_GROUP) {
						assert(bindBuffer == nullptr);
						// get dimension data
						assert(typeID == UniqueType<UInt3>::Get());
						if (typeID == UniqueType<UInt3>::Get()) {
							const UInt3& data = *reinterpret_cast<UInt3*>(ptr);
							std::stringstream ss;
							ss << "layout (local_size_x = " << data.x() << ", local_size_y = " << data.y() << ", local_size_z = " << data.z() << ") in;\n";
							initialization += ss.str();
						}
					} else if (bindInput->description == IShader::BindInput::LOCAL) {
						// Do not declare it here
						// initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
					} else {
						if (bindBuffer == nullptr || mapBufferEnabled[bindBuffer]) {
							if (stage == IRender::Resource::ShaderDescription::VERTEX) {
								assert(bindBuffer != nullptr);
								switch (bindBuffer->description.usage) {
								case IRender::Resource::BufferDescription::VERTEX:
									assert(typeID->GetSize() <= 4 * sizeof(float));
									statement += String("layout (location = ") + ToString(inputIndex++) + ") in " + declareMap[typeID] + " " + name + ";\n";
									break;
								case IRender::Resource::BufferDescription::INSTANCED:
									assert(typeID->GetSize() < 4 * sizeof(float) || typeID->GetSize() % (4 * sizeof(float)) == 0);
									statement += String("layout (location = ") + ToString(inputIndex) + ") in " + declareMap[typeID] + " " + name + ";\n";
									inputIndex += ((uint32_t)safe_cast<uint32_t>(typeID->GetSize()) + sizeof(float) * 3) / (sizeof(float) * 4u);
									break;
								}
							} else {
								statement += "in " + declareMap[typeID] + " " + name + ";\n";
								inputIndex++;
							}
						} else {
							// Not enabled, fallback to local
							assert(bindBuffer->description.usage != IRender::Resource::BufferDescription::UNIFORM);
							initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
						}
					}
				} else if (uniqueNode == uniqueBindOutput) {
					const IShader::BindOutput* bindOutput = static_cast<const IShader::BindOutput*>(node);
					if (bindOutput->description == IShader::BindOutput::LOCAL) {
						initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
					} else if (bindOutput->description == IShader::BindOutput::HPOSITION) {
						assert(arr.empty());
						initialization += String("\t") + declareMap[typeID] + " " + name + ";\n";
						finalization += String("\tgl_Position = ") + name + ";\n";
					} else {
						if (stage == IRender::Resource::ShaderDescription::FRAGMENT) {
							statement += String("layout (location = ") + ToString(outputIndex++) + ") out " + declareMap[typeID] + " " + name + ";\n";
						} else {
							outputIndex++;
							statement += "out " + declareMap[typeID] + " " + name + ";\n";
						}
					}
				} else if (uniqueNode == uniqueBindConstBool) {
					const IShader::BindConst<bool>* bindOption = static_cast<const IShader::BindConst<bool>*>(node);
					declaration += String("#define ") + name + " " + (bindOption->description ? "true" : "false") + "\n";
				} else if (uniqueNode == uniqueBindConstInt) {
					const IShader::BindConst<int>* bindOption = static_cast<const IShader::BindConst<int>*>(node);
					std::stringstream ss;
					ss << bindOption->description;
					declaration += String("#define ") + name + " " + ss.str() + "\n";
				} else if (uniqueNode == uniqueBindConstFloat) {
					const IShader::BindConst<float>* bindOption = static_cast<const IShader::BindConst<float>*>(node);
					std::stringstream ss;
					ss << bindOption->description;
					declaration += String("#define ") + name + " " + ss.str() + "\n";
				}
			}

			if (bindBuffer != nullptr && mapBufferEnabled[bindBuffer]) {
				std::map<const IShader::BindBuffer*, std::pair<String, String> >::iterator it = mapBufferDeclaration.find(bindBuffer);
				assert(it != mapBufferDeclaration.end());
				if (it != mapBufferDeclaration.end()) {
					switch (bindBuffer->description.usage) {
						case IRender::Resource::BufferDescription::VERTEX:
						case IRender::Resource::BufferDescription::INSTANCED:
							assert(!statement.empty());
							it->second.second += statement;
							break;
						case IRender::Resource::BufferDescription::UNIFORM:
							it->second.second += String("\t") + declareMap[typeID] + " _" + name + arr + ";\n";
							if (stage != IRender::Resource::ShaderDescription::COMPUTE) {
								// initialization += String("\t") + declareMap[typeID] + arrDef + " " + name + " = " + it->second.first + "." + name + ";\n";
								initialization += String("#define ") + name + " " + it->second.first + "." + "_" + name + "\n";
							}
							break;
					}
				} else {
					// these buffer usage do not support structure-mapped layout.
					assert(bindBuffer->description.usage != IRender::Resource::BufferDescription::VERTEX);
					assert(bindBuffer->description.usage != IRender::Resource::BufferDescription::INSTANCED);
					declaration += statement;
				}
			} else {
				declaration += statement;
			}
		} else {
			bool enabled = true;
			for (const MetaChainBase* check = meta; check != nullptr; check = check->GetNext()) {
				const MetaNodeBase* node = check->GetNode();
				if (node->GetUnique() == UniqueType<IShader::BindOption>::Get()) {
					const IShader::BindOption* bind = static_cast<const IShader::BindOption*>(node);
					if (!*bind->description) {
						enabled = false;
					}
				}
			}

			if (typeID == uniqueBindBuffer) {
				const IShader::BindBuffer* bindBuffer = static_cast<const IShader::BindBuffer*>(&s);
				mapBufferDeclaration[bindBuffer] = std::make_pair(String(name), String(""));
				mapBufferEnabled[bindBuffer] = enabled;
				if (bindBuffer->description.usage == IRender::Resource::BufferDescription::UNIFORM) {
					uniformBufferNames.emplace_back(String("_") + name);
				} else if (bindBuffer->description.usage == IRender::Resource::BufferDescription::SHARED) {
					sharedBufferNames.emplace_back(String("_") + name);
				}
				orderedBuffers.emplace_back(bindBuffer);
			} else if (typeID == uniqueBindTexture) {
				assert(enabled);
				const IShader::BindTexture* bindTexture = static_cast<const IShader::BindTexture*>(&s);
				static const char* samplerTypes[] = {
					"sampler1D", "sampler2D", "samplerCube", "sampler3D"
				};

				// declaration += String("layout(location = ") + ToString(textureIndex++) + ") uniform " + samplerTypes[bindTexture->description.state.type] + (bindTexture->description.dimension.z() != 0 && bindTexture->description.state.type != IRender::Resource::TextureDescription::TEXTURE_3D ? "Array " : " ") + name + ";\n";
				declaration += String("uniform ") + samplerTypes[bindTexture->description.state.type] + (bindTexture->description.dimension.z() != 0 && bindTexture->description.state.type != IRender::Resource::TextureDescription::TEXTURE_3D ? "Array " : " ") + name + ";\n";
				textureNames.push_back(name);
				textureIndex++;
			}
		}
	}

	virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

	std::vector<String> textureNames;
	std::vector<String> uniformBufferNames;
	std::vector<String> sharedBufferNames;
	std::vector<const IShader::BindBuffer*> orderedBuffers;
	std::map<const IShader::BindBuffer*, std::pair<String, String> > mapBufferDeclaration;
	std::map<const IShader::BindBuffer*, bool> mapBufferEnabled;
};

template <>
struct ResourceImplOpenGL<IRender::Resource::ShaderDescription> : public ResourceBaseImplOpenGLDesc<IRender::Resource::ShaderDescription> {
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
				ShaderDeclarationOpenGL declaration((Resource::ShaderDescription::Stage)k, inputIndex, outputIndex, textureIndex);
				(*shader)(declaration);
				declaration.Complete();

				body += declaration.initialization + shader->GetShaderText() + declaration.finalization + "\n";
				head += declaration.declaration;

				std::copy(declaration.textureNames.begin(), declaration.textureNames.end(), std::back_inserter(textureNames));
				std::copy(declaration.uniformBufferNames.begin(), declaration.uniformBufferNames.end(), std::back_inserter(uniformBufferNames));
				std::copy(declaration.sharedBufferNames.begin(), declaration.sharedBufferNames.end(), std::back_inserter(sharedBufferNames));
			}

			body += "\n}\n"; // make a call to our function

			String fullShader = frameCode + common + head + body;
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
struct ResourceImplOpenGL<IRender::Resource::RenderStateDescription> : public ResourceBaseImplOpenGLDesc<IRender::Resource::RenderStateDescription> {
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

		if (d.alphaBlend) {
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
struct ResourceImplOpenGL<IRender::Resource::ClearDescription> : public ResourceBaseImplOpenGLDesc<IRender::Resource::ClearDescription> {
	virtual IRender::Resource::Type GetType() const override { return RESOURCE_CLEAR; }
	virtual void Upload(QueueImplOpenGL& queue) override {
		UpdateDescription();
	}

	virtual void Download(QueueImplOpenGL& queue) override {}
	virtual void Delete(QueueImplOpenGL& queue) override { delete this; }

	virtual void Execute(QueueImplOpenGL& queue) override {
		GL_GUARD();
		IRender::Resource::ClearDescription& d = GetDescription();

		const Float4& clearColor = d.clearColor;
		if ((d.clearColorBit | d.clearDepthBit | d.clearStencilBit) & IRender::Resource::ClearDescription::CLEAR) {
			glClearColor(clearColor.r(), clearColor.g(), clearColor.b(), clearColor.a());
			glClearDepth(0.0f);
			glClearStencil(0);
			glClear((d.clearColorBit & IRender::Resource::ClearDescription::CLEAR ? GL_COLOR_BUFFER_BIT : 0)
				| (d.clearDepthBit & IRender::Resource::ClearDescription::CLEAR ? GL_DEPTH_BUFFER_BIT : 0)
				| (d.clearStencilBit & IRender::Resource::ClearDescription::CLEAR ? GL_STENCIL_BUFFER_BIT : 0));
		}

		if (queue.device->lastFrameBufferID != 0 && (d.clearColorBit & IRender::Resource::ClearDescription::DISCARD_LOAD)) {
			GLErrorGuard subGuard;
			static const GLuint buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1, GL_COLOR_ATTACHMENT0 + 2, GL_COLOR_ATTACHMENT0 + 3,
				GL_COLOR_ATTACHMENT0 + 4, GL_COLOR_ATTACHMENT0 + 5, GL_COLOR_ATTACHMENT0 + 6, GL_COLOR_ATTACHMENT0 + 7 };
			glInvalidateFramebuffer(GL_FRAMEBUFFER, 8, buffers);
		}

		if (queue.device->lastFrameBufferID != 0 && ((d.clearDepthBit & d.clearStencilBit) & IRender::Resource::ClearDescription::DISCARD_LOAD)) {
			GLErrorGuard subGuard;
			static const GLuint buffer = GL_DEPTH_STENCIL_ATTACHMENT;
			glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &buffer);
		}

		// glDepthMask(d.clearDepthBit & IRender::Resource::ClearDescription::STORE ? GL_TRUE : GL_FALSE);
		// glStencilMask(d.clearStencilBit & IRender::Resource::ClearDescription::STORE ? GL_TRUE : GL_FALSE);
		// GLuint c = d.clearColorBit & IRender::Resource::ClearDescription::STORE ? GL_TRUE : GL_FALSE;
		// glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		queue.device->lastClear = d;
	}
};

template <>
struct ResourceImplOpenGL<IRender::Resource::RenderTargetDescription> : public ResourceBaseImplOpenGLDesc<IRender::Resource::RenderTargetDescription> {
	ResourceImplOpenGL() : vertexArrayID(0), frameBufferID(0) {}
	virtual IRender::Resource::Type GetType() const override { return RESOURCE_RENDERTARGET; }

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
		Resource::ClearDescription& lastClear = queue.device->lastClear;

		// Not back buffer
		if (vertexArrayID == 0) {
			glGenVertexArrays(1, &vertexArrayID);
		}

		if (!d.colorBufferStorages.empty()) {
			// currently formats are not configurable for depth & stencil buffer
			GL_GUARD();
			if (frameBufferID == 0) {
				glGenFramebuffers(1, &frameBufferID);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
			queue.device->lastFrameBufferID = frameBufferID;

			if (d.depthStencilStorage.resource == nullptr) {
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
			} else {
				ResourceImplOpenGL<IRender::Resource::TextureDescription>* t = static_cast<ResourceImplOpenGL<IRender::Resource::TextureDescription>*>(d.depthStencilStorage.resource);
				assert(t->textureID != 0);
				// do not support other types :D
				assert(t->GetDescription().state.layout == IRender::Resource::TextureDescription::DEPTH_STENCIL || t->GetDescription().state.layout == IRender::Resource::TextureDescription::DEPTH);

				if (t->GetDescription().state.sample == IRender::Resource::TextureDescription::RENDERBUFFER) {
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, t->renderbufferID);
				} else {
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, t->textureID, d.depthStencilStorage.mipLevel);
				}
			}

			// create texture slots first
			std::vector<uint32_t> renderBufferSlots;
			for (size_t i = 0; i < d.colorBufferStorages.size(); i++) {
				GL_GUARD();
				IRender::Resource::RenderTargetDescription::Storage& storage = d.colorBufferStorages[i];
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

	virtual void Execute(QueueImplOpenGL& queue) override {
		Resource::RenderTargetDescription& d = GetDescription();
		GL_GUARD();
		Resource::ClearDescription& lastClear = queue.device->lastClear;
		if (queue.device->lastFrameBufferID != 0 && (lastClear.clearColorBit & IRender::Resource::ClearDescription::DISCARD_STORE)) {
			static const GLuint buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1, GL_COLOR_ATTACHMENT0 + 2, GL_COLOR_ATTACHMENT0 + 3,
				GL_COLOR_ATTACHMENT0 + 4, GL_COLOR_ATTACHMENT0 + 5, GL_COLOR_ATTACHMENT0 + 6, GL_COLOR_ATTACHMENT0 + 7 };
			glInvalidateFramebuffer(GL_FRAMEBUFFER, 8, buffers);
		}

		if (queue.device->lastFrameBufferID != 0 && ((lastClear.clearDepthBit & lastClear.clearStencilBit) & IRender::Resource::ClearDescription::DISCARD_STORE)) {
			static const GLuint buffer = GL_DEPTH_STENCIL_ATTACHMENT;
			glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, &buffer);
		}

		queue.device->lastFrameBufferID = frameBufferID;
		glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
		glBindVertexArray(vertexArrayID);
		UShort2Pair range = d.range;

		if (range.second.x() == 0) {
			if (d.colorBufferStorages.empty()) {
				range.second.x() = queue.device->resolution.x();
			} else {
				ResourceImplOpenGL<TextureDescription>* texture = static_cast<ResourceImplOpenGL<TextureDescription>*>(d.colorBufferStorages[0].resource);
				range.second.x() = texture->GetDescription().dimension.x();
			}
		}

		if (range.second.y() == 0) {
			if (d.colorBufferStorages.empty()) {
				range.second.y() = queue.device->resolution.y();
			} else {
				ResourceImplOpenGL<TextureDescription>* texture = static_cast<ResourceImplOpenGL<TextureDescription>*>(d.colorBufferStorages[0].resource);
				range.second.y() = texture->GetDescription().dimension.y();
			}
		}

		glViewport(range.first.x(), range.first.y(), range.second.x() - range.first.x(), range.second.y() - range.first.y());

		if (frameBufferID != 0) {
			const size_t MAX_ID = 8;
			static GLuint idlist[MAX_ID] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1,
			GL_COLOR_ATTACHMENT0 + 2, GL_COLOR_ATTACHMENT0 + 3, GL_COLOR_ATTACHMENT0 + 4,
			GL_COLOR_ATTACHMENT0 + 5, GL_COLOR_ATTACHMENT0 + 6, GL_COLOR_ATTACHMENT0 + 7 };

			if (d.colorBufferStorages.empty()) {
				glDrawBuffer(GL_NONE);
			} else {
				glDrawBuffers((GLsizei)Min(MAX_ID, d.colorBufferStorages.size()), idlist);
			}
		}
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
struct ResourceImplOpenGL<IRender::Resource::DrawCallDescription> : public ResourceBaseImplOpenGLDesc<IRender::Resource::DrawCallDescription> {
	virtual IRender::Resource::Type GetType() const override { return RESOURCE_DRAWCALL; }
	virtual void Upload(QueueImplOpenGL& queue) override {
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
						glVertexAttribPointer(vertexBufferBindingCount, Min(4u, bufferComponent - k), bufferElementType, GL_FALSE, buffer->component * bufferElementSize, reinterpret_cast<void*>((size_t)bufferRange.offset + sizeof(float) * k));
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
				case Resource::BufferDescription::SHARED:
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

template <>
struct ResourceImplOpenGL<IRender::Resource::NotifyDescription> : public ResourceBaseImplOpenGLDesc<IRender::Resource::NotifyDescription> {
	virtual IRender::Resource::Type GetType() const override { return RESOURCE_DRAWCALL; }
	virtual void Upload(QueueImplOpenGL& queue) override {
		UpdateDescription();
	}
	virtual void Download(QueueImplOpenGL& queue) override {}
	virtual void Execute(QueueImplOpenGL& queue) override {
		GetDescription().notifier(queue.device->render, &queue);
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

// Queue
IRender::Queue* ZRenderOpenGL::CreateQueue(Device* device, bool shared) {
	assert(device != nullptr);
	return new QueueImplOpenGL(static_cast<DeviceImplOpenGL*>(device), shared);
}

IRender::Device* ZRenderOpenGL::GetQueueDevice(Queue* queue) {
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	return q->device;
}

void ZRenderOpenGL::PresentQueues(Queue** queues, uint32_t count, PresentOption option) {
	GL_GUARD();
	ClearDeletedQueues();
	void (QueueImplOpenGL::*op)() = &QueueImplOpenGL::ExecuteAll;
	switch (option) {
	case PresentOption::PRESENT_EXECUTE_ALL:
		op = &QueueImplOpenGL::ExecuteAll;
		break;
	case PresentOption::PRESENT_REPEAT_TO_YIELD:
		op = &QueueImplOpenGL::RepeatToYield;
		break;
	case PresentOption::PRESENT_EXECUTE_TO_YIELD:
		op = &QueueImplOpenGL::ExecuteToYield;
		break;
	case PresentOption::PRESENT_CONSUME_YIELD:
		op = &QueueImplOpenGL::ConsumeYield;
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

void ZRenderOpenGL::MergeQueue(Queue* target, Queue* source) {
	assert(target != nullptr);
	assert(source != nullptr);

	QueueImplOpenGL* t = static_cast<QueueImplOpenGL*>(target);
	QueueImplOpenGL* s = static_cast<QueueImplOpenGL*>(source);

	t->Merge(s);
}

void ZRenderOpenGL::YieldQueue(Queue* queue) {
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	assert(queue != nullptr);
	q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_EXECUTE, nullptr));
}

void ZRenderOpenGL::DeleteQueue(Queue* queue) {
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	assert(queue != nullptr);
	deletedQueues.Push(q);
}

// Resource
IRender::Resource* ZRenderOpenGL::CreateResource(Queue* queue, Resource::Type resourceType) {
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
	case Resource::RESOURCE_CLEAR:
		return new ResourceImplOpenGL<Resource::ClearDescription>();
	case Resource::RESOURCE_DRAWCALL:
		return new ResourceImplOpenGL<Resource::DrawCallDescription>();
	case Resource::RESOURCE_NOTIFY:
		return new ResourceImplOpenGL<Resource::NotifyDescription>();
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

void ZRenderOpenGL::SwapResource(Queue* queue, Resource* lhs, Resource* rhs) {
	assert(queue != nullptr);
	assert(lhs != nullptr);
	assert(rhs != nullptr);
	QueueImplOpenGL* q = static_cast<QueueImplOpenGL*>(queue);
	q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_PRESWAP, lhs));
	q->QueueCommand(ResourceCommandImplOpenGL(ResourceCommandImplOpenGL::OP_POSTSWAP, rhs));
}

void ZRenderOpenGL::DeleteResource(Queue* queue, Resource* resource) {
	assert(queue != nullptr);
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
