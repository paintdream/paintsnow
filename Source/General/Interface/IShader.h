// IShader.h -- shader interface
// By PaintDream (paintdream@paintdream.com)
// 2014-12-3
//

#pragma once

#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IReflect.h"
#include "IRender.h"
#include <string>
#include <map>

namespace PaintsNow {
	class IShader : public IReflectObjectComplex {
	public:
		virtual String GetShaderText();
		static int WorkGroupSize;
		static int NumWorkGroups;
		static int LocalInvocationID;
		static int WorkGroupID;
		static int GlobalInvocationID;
		static int LocalInvocationIndex;

		class MetaShader : public TReflected<MetaShader, MetaNodeBase> {
		public:
			MetaShader(IRender::Resource::ShaderDescription::Stage shaderType);
			MetaShader operator = (IRender::Resource::ShaderDescription::Stage shaderType);

			template <class T, class D>
			inline const MetaShader& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef MetaShader Type;
			};

			typedef MetaShader Type;

			TObject<IReflect>& operator () (IReflect& reflect) override;
			IRender::Resource::ShaderDescription::Stage shaderType;
		};

		template <class M>
		class TMetaBinding : public TReflected<TMetaBinding<M>, MetaNodeBase> {
		public:
			typedef TReflected<TMetaBinding<M>, MetaNodeBase> BaseClass;
			TMetaBinding(const M& m = M()) : description(m) {}

			template <class T, class D>
			inline const TMetaBinding& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef TMetaBinding Type;
			};

			typedef TMetaBinding Type;

			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);

				if (reflect.IsReflectProperty()) {
					ReflectProperty(description)[Runtime];
				}

				return *this;
			}

			M description;
		};

		template <class M>
		class TMetaBindingResource : public TReflected<TMetaBindingResource<M>, TMetaBinding<M> > {
		public:
			TMetaBindingResource() : resource(nullptr) {}
			IRender::Resource* resource;
		};

		template <class M>
		class BindConst : public TReflected<BindConst<M>, TMetaBinding<M> > {
		public:
			typedef TReflected<BindConst<M>, TMetaBinding<M> > BaseClass;
			BindConst(const M& m = 0) : BaseClass(m) {}

			template <class T, class D>
			inline const BindConst& FilterField(T* t, D* d) const {
				const_cast<D&>(this->description) = *d;
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindConst Type;
			};

			typedef BindConst Type;
		};

		class BindOption : public TReflected<BindOption, TMetaBinding<const bool*> > {
		public:
			BindOption(bool& value) { description = &value; }
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindOption& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindOption Type;
			};

			typedef BindOption Type;
		};

		class BindInput : public TReflected<BindInput, TMetaBinding<uint32_t> > {
		public:
			enum SCHEMA {
				GENERAL, COMPUTE_GROUP, LOCAL,
				INDEX, POSITION, NORMAL, BINORMAL, TANGENT, COLOR, COLOR_INSTANCED, BONE_INDEX, BONE_WEIGHT,
				TRANSFORM_WORLD, TRANSFORM_WORLD_NORMAL, BONE_TRANSFORMS, TRANSFORM_VIEW, 
				TRANSFORM_VIEWPROJECTION, TRANSFORM_VIEWPROJECTION_INV, TRANSFORM_LAST_VIEWPROJECTION, 
				LIGHT, UNITCOORD, MAINTEXTURE, TEXCOORD };

			BindInput(uint32_t t = GENERAL, const TWrapper<UInt2>& q = TWrapper<UInt2>()) : BaseClass(t), subRangeQueryer(q) {}
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindInput& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindInput Type;
			};

			typedef BindInput Type;
			TWrapper<UInt2> subRangeQueryer;
		};

		class BindOutput : public TReflected<BindOutput, TMetaBinding<uint32_t> > {
		public:
			enum SCHEMA { GENERAL, LOCAL, HPOSITION, DEPTH, COLOR, TEXCOORD };
			BindOutput(uint32_t t = GENERAL) : BaseClass(t) {}
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindOutput& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindOutput Type;
			};

			typedef BindOutput Type;
		};

		class BindTexture : public TReflected<BindTexture, TMetaBindingResource<IRender::Resource::TextureDescription> > {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindTexture& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindTexture Type;
			};

			typedef BindTexture Type;
		};

		class BindBuffer : public TReflected<BindBuffer, TMetaBindingResource<IRender::Resource::BufferDescription> > {
		public:
			TObject<IReflect>& operator () (IReflect& reflect) override {
				BaseClass::operator () (reflect);
				return *this;
			}

			template <class T, class D>
			inline const BindBuffer& FilterField(T* t, D* d) const {
				return *this; // do nothing
			}

			template <class T, class D>
			struct RealType {
				typedef BindBuffer Type;
			};

			typedef BindBuffer Type;
		};
	};
}
