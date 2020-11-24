// GLSLShaderGenerator.h
// PaintDream (paintdream@paintdream.com)
// 2020-8-19
//

#pragma once
#include "../../../Interface/IRender.h"
#include "../../../Interface/IShader.h"

namespace PaintsNow {
	class GLSLShaderGenerator : public IReflect {
	public:
		GLSLShaderGenerator(IRender::Resource::ShaderDescription::Stage s, uint32_t& pinputIndex, uint32_t& poutputIndex, uint32_t& ptextureIndex);

		static const String& GetFrameCode();
		void Complete();
		void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override;
		void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override;

		IRender::Resource::ShaderDescription::Stage stage;
		String declaration;
		String initialization;
		String finalization;

		uint32_t debugVertexBufferIndex;
		uint32_t& inputIndex;
		uint32_t& outputIndex;
		uint32_t& textureIndex;

		std::vector<std::pair<const IShader::BindBuffer*, String> > bufferBindings;
		std::vector<std::pair<const IShader::BindTexture*, String> > textureBindings;

	protected:
		std::map<const IShader::BindBuffer*, std::pair<String, String> > mapBufferDeclaration;
		std::map<const IShader::BindBuffer*, bool> mapBufferEnabled;
	};
}

