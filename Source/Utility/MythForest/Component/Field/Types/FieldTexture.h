// FieldTexture.h
// PaintDream (paintdream@paintdream.com)
// 2020-3-17
//

#pragma once
#include "../FieldComponent.h"
#include "../../../../SnowyStream/Resource/TextureResource.h"

namespace PaintsNow {
	class FieldTexture : public FieldComponent::FieldBase {
	public:
		virtual Bytes operator [] (const Float3& position) const;

		enum TEXTURE_TYPE {
			TEXTURE_2D, TEXTURE_3D,
		};

	protected:
		TShared<TextureResource> textureResource;
	};
}

