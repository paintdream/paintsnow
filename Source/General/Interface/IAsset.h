// IAsset.h -- Asset data exchangement
// By PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#ifndef __IASSET_H__
#define __IASSET_H__


#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IFilterBase.h"
#include "../../Core/Interface/IType.h"
#include "../../General/Interface/IRender.h"
#include <list>
#include <vector>


namespace PaintsNow {
	namespace IAsset {
		enum Type { TYPE_BOOL, TYPE_FLOAT, TYPE_FLOAT2, TYPE_FLOAT3, TYPE_FLOAT4, TYPE_MATRIX3, TYPE_MATRIX4, TYPE_TEXTURE };
		struct TextureIndex {
			TextureIndex(uint32_t i = 0) : index(i) {}
			uint32_t index;
		};

		template <class T>
		struct MapType {};

		template <>
		struct MapType<bool> { enum { type = TYPE_BOOL }; };
		template <>
		struct MapType<float> { enum { type = TYPE_FLOAT }; };
		template <>
		struct MapType<TextureIndex> { enum { type = TYPE_TEXTURE }; };
		template <>
		struct MapType<Float2> { enum { type = TYPE_FLOAT2 }; };
		template <>
		struct MapType<Float3> { enum { type = TYPE_FLOAT3 }; };
		template <>
		struct MapType<Float4> { enum { type = TYPE_FLOAT4 }; };
		template <>
		struct MapType<MatrixFloat3x3> { enum { type = TYPE_MATRIX3 }; };
		template <>
		struct MapType<MatrixFloat4x4> { enum { type = TYPE_MATRIX4 }; };

		class MeshGroup : public TReflected<MeshGroup, IReflectObjectComplex> {
		public:
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			String name;
			uint32_t primitiveOffset;
			uint32_t primitiveCount;
		};

		class TexCoord : public TReflected<TexCoord, IReflectObjectComplex> {
		public:
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			std::vector<Float4> coords;
		};

		class MeshCollection : public TReflected<MeshCollection, IReflectObjectComplex> {
		public:
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			std::vector<UInt3> indices;
			std::vector<Float3> vertices;
			std::vector<UChar4> normals;
			std::vector<UChar4> tangents;
			std::vector<UChar4> colors;
			std::vector<UChar4> boneIndices;
			std::vector<UChar4> boneWeights;
			std::vector<TexCoord> texCoords;
			std::vector<MeshGroup> groups;

			static void CalulateNormals(Float3* outputNormals, const Float3* vertices, const Int3* faces, size_t vertexCount, size_t faceCount);
		};


		enum INTERPOLATE { INTERPOLATE_NONE, INTERPOLATE_LINEAR, INTERPOLATE_HERMITE, INTERPOLATE_BEZIER };
		enum FILTER { FILTER_NONE, FILTER_TRANSPARENT, FILTER_BLEND, FILTER_ADDITIVE, FILTER_ADD_ALPHA, FILTER_MODULATE };

		template <class T>
		class InterpolateValue {
		public:
			bool operator < (const InterpolateValue& rhs) const {
				return time < rhs.time;
			}

			T value;
			T inTan;
			T outTan;
			float time;
		};

		template <class T>
		class Sequence : public TReflected<Sequence<T>, IReflectObjectComplex> {
		public:
			typedef TReflected<Sequence<T>, IReflectObjectComplex> BaseClass;
			virtual TObject<IReflect>& operator () (IReflect& reflect) {
				BaseClass::operator () (reflect);
				if (reflect.IsReflectProperty()) {
					ReflectProperty(frames);
					ReflectProperty(interpolate);
				}

				return *this;
			}

#ifdef _MSC_VER
			typedef typename InterpolateValue<T> Frame;
#else
			typedef InterpolateValue<T> Frame;
#endif
			std::vector<Frame> frames;
			Frame staticFrame;
			INTERPOLATE interpolate;
		};

		class Material : public TReflected<Material, IReflectObjectComplex> {
		public:
			class Variable : public TReflected<Variable, IReflectObjectComplex> {
			public:
				virtual TObject<IReflect>& operator () (IReflect& reflect) override;
				Variable();
				template <class T>
				Variable(const String& k, const T& value) {
					key.Assign((uint8_t*)k.data(), k.size());
					*this = value;
				}

				template <class T>
				T Parse(UniqueType<T>) const {
					T ret;

#if defined(_MSC_VER) && _MSC_VER <= 1200
					assert((Type)MapType<std::decay<T>::type>::type == type);
#else
					assert((Type)MapType<typename std::decay<T>::type>::type == type);
#endif
					memcpy(&ret, value.GetData(), sizeof(T));
					return ret;
				}
				
				template <class T>
				Variable& operator = (const T& object) {
#if defined(_MSC_VER) && _MSC_VER <= 1200
					type = (Type)MapType<std::decay<T>::type>::type;
#else
					type = (Type)MapType<typename std::decay<T>::type>::type;
#endif
					value.Resize(sizeof(T));
					memcpy(value.GetData(), &object, sizeof(T));
					return *this;
				}

				Bytes key;
				Bytes value;
				Type type;
			};

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			std::vector<Variable> variables;
			IRender::Resource::RenderStateDescription state;
		};

		typedef Sequence<Float4> RotSequence;
		typedef Sequence<Float3> TransSequence;
		typedef Sequence<Float3> ScalingSequence;

		class BoneAnimation : public TReflected<BoneAnimation, IReflectObjectComplex> {
		public:
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			class Joint : public TReflected<Joint, IReflectObjectComplex> {
			public:
				virtual TObject<IReflect>& operator () (IReflect& reflect) override;

				String name;
				MatrixFloat4x4 offsetMatrix;

				int id;
				int parent;
			};

			class Channel : public TReflected<Channel, IReflectObjectComplex> {
			public:
				virtual TObject<IReflect>& operator () (IReflect& reflect) override;
				int jointIndex;
				RotSequence rotSequence;
				TransSequence transSequence;
				ScalingSequence scalingSequence;
			};

			class Clip : public TReflected<Clip, IReflectObjectComplex> {
			public:
				virtual TObject<IReflect>& operator () (IReflect& reflect) override;
				String name;
				float fps;
				float duration;
				float rarity;
				float speed;
				bool loop;

				std::vector<Channel> channels;
			};

			std::vector<Joint> joints;
			std::vector<Clip> clips;
		};
	}
}


#endif // __IASSET_H__
