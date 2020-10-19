return {
	Template = "[Runtime]/ShaderResource/CustomMaterialPass",
	Name = "PBR",
	Inputs = {
		-- VertexInputs
		Position = {
			Stages = { "VertexInput" },
			Value = { 0.0, 0.0, 0.0 }, -- float3
			Binding = "POSITION",
		},
		Color = {
			Stages = { "VertexInput" },
			Value = { 1.0, 1.0, 1.0, 1.0 }, -- float3
			Binding = "COLOR",
		},
		Normal = {
			Stages = { "VertexInput" },
			Value = { 0.0, 0.0, 1.0, 0.0 }, -- float3
			Binding = "NORMAL",
		},
		Tangent = {
			Stages = { "VertexInput" },
			Value = { 1.0, 0.0, 0.0, 0.0 },
			Binding = "TANGENT"
		},
		TexCoord = {
			Stages = { "VertexInput" },
			Value = { 0.0, 0.0, 0.0, 0.0 } -- float4
			Binding = "TEXCOORD0"
		},

		-- Uniforms
		WorldMatrix = {
			Stages = { "VertexInstance" },
			Value = {
				1.0, 0.0, 0.0, 0.0
				0.0, 1.0, 0.0, 0.0
				0.0, 0.0, 1.0, 0.0
				0.0, 0.0, 0.0, 1.0
			},
			Binding = "TRANSFORM_WORLD"
		},
		ViewMatrix = {
			Stages = { "VertexUniform" },
			Value = {
				1.0, 0.0, 0.0, 0.0
				0.0, 1.0, 0.0, 0.0
				0.0, 0.0, 1.0, 0.0
				0.0, 0.0, 0.0, 1.0
			},
			Binding = "TRANSFORM_VIEW"
		},
		ViewProjectionMatrix = {
			Stages = { "VertexUniform" },
			Value = {
				1.0, 0.0, 0.0, 0.0
				0.0, 1.0, 0.0, 0.0
				0.0, 0.0, 1.0, 0.0
				0.0, 0.0, 0.0, 1.0
			},
			Binding = "TRANSFORM_VIEWPROJECTION"
		},

		-- Options
		EnableVertexNormal = {
			Stages = { "VertexOptions", "FragmentOptions" },
			Value = false,
			Binding = "Normal",
		},
		EnableVertexTangent = {
			Stages = { "VertexOptions", "FragmentOptions" },
			Value = false,
			Binding = "Tangent"
		},
		EnableVertexColor = {
			Stages = { "VertexOptions", "FragmentOptions" },
			Value = false,
			Binding ="Color"
		},

		-- Varyings
		VaryingPosition = {
			Stages = { "VertexVarying" },
			Value = { 0.0, 0.0, 0.0, 1.0 },
			Binding = "HPOSITION"
		},
		VaryingTexCoord = {
			Stages = { "VertexVarying" },
			Value = { 0.0, 0.0, 0.0, 0.0 },
			Binding = "TexCoord"
		},
		VaryingViewNormal = {
			Stages = { "VertexVarying" },
			Value = { 0.0, 0.0, 1.0 },
			Binding = "Normal"
		},
		VaryingViewTangent = {
			Stages = { "VertexVarying" },
			Value = { 0.0, 0.0, 1.0 },
			Binding = "Tangent"
		},
		VaryingViewBinormal = {
			Stages = { "VertexVarying" },
			Value = { 0.0, 0.0, 1.0 },
			Binding = "Tangent"
		},
		VaryingColor = {
			Stages = { "VertexVarying" },
			Value = { 1.0, 1.0, 1.0, 1.0 },
			Binding = "Color"
		},

		-- Textures
		BaseColorTexture = {
			Stages = { "FragmentUniform" },
			Value = "[Runtime]/TextureResource/MissingBaseColor",
			Binding = ""
		},
		NormalTexture = {
			Stages = { "FragmentUniform" },
			Value = "[Runtime]/TextureResource/MissingNormal",
			Binding = ""
		},
		MaterialTexture = {
			Stages = { "FragmentUniform" },
			Value = "[Runtime]/TextureResource/MissingMaterial",
			Binding = ""
		},

		-- Options
		EnableBaseColorTexture = {
			Stages = { "FragmentOptions" },
			Value = false,
			Binding = "BaseColorTexture",
		},
		EnableNormalTexture = {
			Stages = { "FragmentOptions" },
			Value = false,
			Binding = "NormalTexture"
		},
		EnableMaterialTexture = {
			Stages = { "FragmentOptions" },
			Value = false,
			Binding ="MaterialTexture"
		},
	},
	Shaders = {
		Vertex = [[
			VaryingPosition = mult_vec(ViewProjectionMatrix, Position);
			if (EnableTexCoord) {
				VaryingTexCoord = TexCoord;
			}

			if (EnableVertexColor) {
				VaryingColor = Color;
			}

			if (EnableVertexNormal) {
				float4 normal = Normal * 2.0 / 255.0 - float4(1.0, 1.0, 1.0, 1.0);
				float4x4 viewWorldMatrix = mult_mat(ViewMatrix, WorldMatrix);
				if (EnableVertexTangent) {
					float4 tangent = Tangent * 2.0 / 255.0 - float4(1.0, 1.0, 1.0, 1.0);
					VaryingTangent = mult_vec(float3x3(ViewWorldMatrix), tangent.xyz);
					VaryingBinormal = mult_vec(float3x3(ViewWorldMatrix), normal.xyz);
					VaryingNormal = cross(VaryingBinormal, VaryingTangent);
					VaryingBinormal *= tangent.w;
				} else {
					VaryingNormal = mult_vec(float3x3(ViewWorldMatrix), normal.xyz);
				}
			}
		]],

		Fragment = [[
			if (EnableBaseColorTexture && EnableTexCoord) {
				float4 color = texture(BaseColorTexture, TexCoord.xy);
				if (EnableVertexColor) {
					OutputColor = VaryingColor.xyz * color.xyz;
				} else {
					OutputColor = color.xyz;
				}
				
				Alpha = color.w;
			}

			if (EnableNormalTexture && EnableNormal && EnableTangent) {
				float4 bump = texture(NormalTexture, TexCoord.xy);
				bump.xyz = bump.xyz * 2.0 - float3(1.0, 1.0, 1.0);
				OutputNormal = normalize(VaryingTangent.xyz * bump.x + VaryingBinormal.xyz * bump.y + VaryingNormal.xyz * bump.z);
			} else {
				OutputNormal = VaryingNormal.xyz;
			}

			if (EnableMaterialTexture) {
				float4 material = texture(MaterialTexture, TexCoord.xy);
				Occlusion = material.x;
				Roughness = material.y;
				Metallic = material.z;
			}
		]]
	}
}