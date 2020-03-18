// FieldTexture.h
// PaintDream (paintdream@paintdream.com)
// 2020-3-17
//

#ifndef __FIELDTEXTURE_H__
#define __FIELDTEXTURE_H__

#include "../FieldComponent.h"
#include "../../../../SnowyStream/Resource/TextureResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		class FieldTexture : public FieldComponent::FieldBase {
		public:
			virtual Bytes operator [] (const Float3& position) const;

			enum TEXTURE_TYPE {
				TEXTURE_2D, TEXTURE_3D, 
			};

		protected:
			TShared<NsSnowyStream::TextureResource> textureResource;
		};
	}
}

#endif // __FIELDTEXTURE_H__