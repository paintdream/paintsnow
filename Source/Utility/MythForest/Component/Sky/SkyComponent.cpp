#include "SkyComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Transform/TransformComponent.h"
#include <utility>

using namespace PaintsNow;

// [Precomputed Atmospheric Scattering](https://hal.inria.fr/inria-00288758/en)
// https://ebruneton.github.io/precomputed_atmospheric_scattering/demo.html
/**
* Copyright (c) 2017 Eric Bruneton
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holders nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/


static const int kLambdaMin = 360;
static const int kLambdaMax = 830;
static const int TRANSMITTANCE_TEXTURE_WIDTH = 256;
static const int TRANSMITTANCE_TEXTURE_HEIGHT = 64;

static const int SCATTERING_TEXTURE_R_SIZE = 32;
static const int SCATTERING_TEXTURE_MU_SIZE = 128;
static const int SCATTERING_TEXTURE_MU_S_SIZE = 32;
static const int SCATTERING_TEXTURE_NU_SIZE = 8;

static const int SCATTERING_TEXTURE_WIDTH = SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
static const int SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
static const int SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;

static const int IRRADIANCE_TEXTURE_WIDTH = 64;
static const int IRRADIANCE_TEXTURE_HEIGHT = 16;

// The conversion factor between watts and lumens.
static const double MAX_LUMINOUS_EFFICACY = 683.0;

// Values from "CIE (1931) 2-deg color matching functions", see
// "http://web.archive.org/web/20081228084047/
//	http://www.cvrl.org/database/data/cmfs/ciexyz31.txt".
static const double CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[380] = {
  360, 0.000129900000, 0.000003917000, 0.000606100000,
  365, 0.000232100000, 0.000006965000, 0.001086000000,
  370, 0.000414900000, 0.000012390000, 0.001946000000,
  375, 0.000741600000, 0.000022020000, 0.003486000000,
  380, 0.001368000000, 0.000039000000, 0.006450001000,
  385, 0.002236000000, 0.000064000000, 0.010549990000,
  390, 0.004243000000, 0.000120000000, 0.020050010000,
  395, 0.007650000000, 0.000217000000, 0.036210000000,
  400, 0.014310000000, 0.000396000000, 0.067850010000,
  405, 0.023190000000, 0.000640000000, 0.110200000000,
  410, 0.043510000000, 0.001210000000, 0.207400000000,
  415, 0.077630000000, 0.002180000000, 0.371300000000,
  420, 0.134380000000, 0.004000000000, 0.645600000000,
  425, 0.214770000000, 0.007300000000, 1.039050100000,
  430, 0.283900000000, 0.011600000000, 1.385600000000,
  435, 0.328500000000, 0.016840000000, 1.622960000000,
  440, 0.348280000000, 0.023000000000, 1.747060000000,
  445, 0.348060000000, 0.029800000000, 1.782600000000,
  450, 0.336200000000, 0.038000000000, 1.772110000000,
  455, 0.318700000000, 0.048000000000, 1.744100000000,
  460, 0.290800000000, 0.060000000000, 1.669200000000,
  465, 0.251100000000, 0.073900000000, 1.528100000000,
  470, 0.195360000000, 0.090980000000, 1.287640000000,
  475, 0.142100000000, 0.112600000000, 1.041900000000,
  480, 0.095640000000, 0.139020000000, 0.812950100000,
  485, 0.057950010000, 0.169300000000, 0.616200000000,
  490, 0.032010000000, 0.208020000000, 0.465180000000,
  495, 0.014700000000, 0.258600000000, 0.353300000000,
  500, 0.004900000000, 0.323000000000, 0.272000000000,
  505, 0.002400000000, 0.407300000000, 0.212300000000,
  510, 0.009300000000, 0.503000000000, 0.158200000000,
  515, 0.029100000000, 0.608200000000, 0.111700000000,
  520, 0.063270000000, 0.710000000000, 0.078249990000,
  525, 0.109600000000, 0.793200000000, 0.057250010000,
  530, 0.165500000000, 0.862000000000, 0.042160000000,
  535, 0.225749900000, 0.914850100000, 0.029840000000,
  540, 0.290400000000, 0.954000000000, 0.020300000000,
  545, 0.359700000000, 0.980300000000, 0.013400000000,
  550, 0.433449900000, 0.994950100000, 0.008749999000,
  555, 0.512050100000, 1.000000000000, 0.005749999000,
  560, 0.594500000000, 0.995000000000, 0.003900000000,
  565, 0.678400000000, 0.978600000000, 0.002749999000,
  570, 0.762100000000, 0.952000000000, 0.002100000000,
  575, 0.842500000000, 0.915400000000, 0.001800000000,
  580, 0.916300000000, 0.870000000000, 0.001650001000,
  585, 0.978600000000, 0.816300000000, 0.001400000000,
  590, 1.026300000000, 0.757000000000, 0.001100000000,
  595, 1.056700000000, 0.694900000000, 0.001000000000,
  600, 1.062200000000, 0.631000000000, 0.000800000000,
  605, 1.045600000000, 0.566800000000, 0.000600000000,
  610, 1.002600000000, 0.503000000000, 0.000340000000,
  615, 0.938400000000, 0.441200000000, 0.000240000000,
  620, 0.854449900000, 0.381000000000, 0.000190000000,
  625, 0.751400000000, 0.321000000000, 0.000100000000,
  630, 0.642400000000, 0.265000000000, 0.000049999990,
  635, 0.541900000000, 0.217000000000, 0.000030000000,
  640, 0.447900000000, 0.175000000000, 0.000020000000,
  645, 0.360800000000, 0.138200000000, 0.000010000000,
  650, 0.283500000000, 0.107000000000, 0.000000000000,
  655, 0.218700000000, 0.081600000000, 0.000000000000,
  660, 0.164900000000, 0.061000000000, 0.000000000000,
  665, 0.121200000000, 0.044580000000, 0.000000000000,
  670, 0.087400000000, 0.032000000000, 0.000000000000,
  675, 0.063600000000, 0.023200000000, 0.000000000000,
  680, 0.046770000000, 0.017000000000, 0.000000000000,
  685, 0.032900000000, 0.011920000000, 0.000000000000,
  690, 0.022700000000, 0.008210000000, 0.000000000000,
  695, 0.015840000000, 0.005723000000, 0.000000000000,
  700, 0.011359160000, 0.004102000000, 0.000000000000,
  705, 0.008110916000, 0.002929000000, 0.000000000000,
  710, 0.005790346000, 0.002091000000, 0.000000000000,
  715, 0.004109457000, 0.001484000000, 0.000000000000,
  720, 0.002899327000, 0.001047000000, 0.000000000000,
  725, 0.002049190000, 0.000740000000, 0.000000000000,
  730, 0.001439971000, 0.000520000000, 0.000000000000,
  735, 0.000999949300, 0.000361100000, 0.000000000000,
  740, 0.000690078600, 0.000249200000, 0.000000000000,
  745, 0.000476021300, 0.000171900000, 0.000000000000,
  750, 0.000332301100, 0.000120000000, 0.000000000000,
  755, 0.000234826100, 0.000084800000, 0.000000000000,
  760, 0.000166150500, 0.000060000000, 0.000000000000,
  765, 0.000117413000, 0.000042400000, 0.000000000000,
  770, 0.000083075270, 0.000030000000, 0.000000000000,
  775, 0.000058706520, 0.000021200000, 0.000000000000,
  780, 0.000041509940, 0.000014990000, 0.000000000000,
  785, 0.000029353260, 0.000010600000, 0.000000000000,
  790, 0.000020673830, 0.000007465700, 0.000000000000,
  795, 0.000014559770, 0.000005257800, 0.000000000000,
  800, 0.000010253980, 0.000003702900, 0.000000000000,
  805, 0.000007221456, 0.000002607800, 0.000000000000,
  810, 0.000005085868, 0.000001836600, 0.000000000000,
  815, 0.000003581652, 0.000001293400, 0.000000000000,
  820, 0.000002522525, 0.000000910930, 0.000000000000,
  825, 0.000001776509, 0.000000641530, 0.000000000000,
  830, 0.000001251141, 0.000000451810, 0.000000000000,
};

// The conversion matrix from XYZ to linear sRGB color spaces.
// Values from https://en.wikipedia.org/wiki/SRGB.
static const double XYZ_TO_SRGB[9] = {
  +3.2406, -1.5372, -0.4986,
  -0.9689, +1.8758, +0.0415,
  +0.0557, -0.2040, +1.0570
};

static double CieColorMatchingFunctionTableValue(double wavelength, int column) {
	if (wavelength <= kLambdaMin || wavelength >= kLambdaMax) {
		return 0.0;
	}
	double u = (wavelength - kLambdaMin) / 5.0;
	int row = static_cast<int>(std::floor(u));
	assert(row >= 0 && row + 1 < 95);
	assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength &&
		CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
	u -= row;
	return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) +
		CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

static double Interpolate(
	const std::vector<double>& wavelengths,
	const std::vector<double>& wavelength_function,
	double wavelength) {
	assert(wavelength_function.size() == wavelengths.size());
	if (wavelength < wavelengths[0]) {
		return wavelength_function[0];
	}
	for (unsigned int i = 0; i < wavelengths.size() - 1; ++i) {
		if (wavelength < wavelengths[i + 1]) {
			double u =
				(wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
			return
				wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
		}
	}
	return wavelength_function[wavelength_function.size() - 1];
}

/*
<p>We can then implement a utility function to compute the "spectral radiance to
luminance" conversion constants (see Section 14.3 in <a
href="https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
Evaluation of 8 Clear Sky Models</a> for their definitions):
*/

static const double kLambdaR = 680.0;
static const double kLambdaG = 550.0;
static const double kLambdaB = 440.0;

// The returned constants are in lumen.nm / watt.
static void ComputeSpectralRadianceToLuminanceFactors(
	const std::vector<double>& wavelengths,
	const std::vector<double>& solar_irradiance,
	double lambda_power, double* k_r, double* k_g, double* k_b) {
	*k_r = 0.0;
	*k_g = 0.0;
	*k_b = 0.0;
	double solar_r = Interpolate(wavelengths, solar_irradiance, kLambdaR);
	double solar_g = Interpolate(wavelengths, solar_irradiance, kLambdaG);
	double solar_b = Interpolate(wavelengths, solar_irradiance, kLambdaB);
	int dlambda = 1;
	for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
		double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
		double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
		double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
		const double* xyz2srgb = XYZ_TO_SRGB;
		double r_bar =
			xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
		double g_bar =
			xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
		double b_bar =
			xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
		double irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
		*k_r += r_bar * irradiance / solar_r *
			pow(lambda / kLambdaR, lambda_power);
		*k_g += g_bar * irradiance / solar_g *
			pow(lambda / kLambdaG, lambda_power);
		*k_b += b_bar * irradiance / solar_b *
			pow(lambda / kLambdaB, lambda_power);
	}
	*k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
	*k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}

SkyComponent::SkyComponent(const TShared<MeshResource>& res, const TShared<BatchComponent>& batch) : BaseClass(res, batch) {
	assert(res);
	Flag().fetch_or(COMPONENT_SHARED, std::memory_order_relaxed); // can be shared among different entities
}

void SkyComponent::UpdateMaterial() {

}

TObject<IReflect>& SkyComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(atmosphereParameters);
		ReflectProperty(transmittance_texture)[Runtime];
		ReflectProperty(scattering_texture)[Runtime];
		ReflectProperty(irradiance_texture)[Runtime];
	}

	return *this;
}

static TShared<TextureResource> NewTexture2D(SnowyStream& snowyStream, const String& path, int width, int height) {
	TShared<TextureResource> textureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), path, false, ResourceBase::RESOURCE_VIRTUAL);
	IRender::Queue* renderQueue = snowyStream.GetResourceQueue();
   
	IRender::Resource::TextureDescription& description = textureResource->description;
	description.dimension = UShort3(safe_cast<uint16_t>(width), safe_cast<uint16_t>(height), 0);
	description.state.format = IRender::Resource::TextureDescription::FLOAT;
	description.state.layout = IRender::Resource::TextureDescription::RGBA;
	description.state.sample = IRender::Resource::TextureDescription::LINEAR;
	description.state.addressU = description.state.addressV = description.state.addressW = IRender::Resource::TextureDescription::CLAMP;
	textureResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
	textureResource->GetResourceManager().InvokeUpload(textureResource(), renderQueue);

	return textureResource;
}

static TShared<TextureResource> NewTexture3D(SnowyStream& snowyStream, const String& path, int width, int height, int depth, bool half) {
	TShared<TextureResource> textureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), path, false, ResourceBase::RESOURCE_VIRTUAL);
	IRender::Queue* renderQueue = snowyStream.GetResourceQueue();

	IRender::Resource::TextureDescription& description = textureResource->description;
	description.dimension = UShort3(safe_cast<uint16_t>(width), safe_cast<uint16_t>(height), safe_cast<uint16_t>(depth));
	description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	description.state.format = half ? IRender::Resource::TextureDescription::HALF : IRender::Resource::TextureDescription::FLOAT;
	description.state.layout = IRender::Resource::TextureDescription::RGBA;
	description.state.sample = IRender::Resource::TextureDescription::LINEAR;
	description.state.addressU = description.state.addressV = description.state.addressW = IRender::Resource::TextureDescription::CLAMP;
	textureResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
	textureResource->GetResourceManager().InvokeUpload(textureResource(), renderQueue);

	return textureResource;
}

void SkyComponent::Initialize(Engine& engine, Entity* entity) {
	SnowyStream& snowyStream = engine.snowyStream;
	transmittance_texture = NewTexture2D(snowyStream, ResourceBase::GenerateLocation("SkyTransmittance", this), TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
	scattering_texture = NewTexture3D(snowyStream, ResourceBase::GenerateLocation("SkyScattering", this), SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH, true);
	irradiance_texture = NewTexture2D(snowyStream, ResourceBase::GenerateLocation("SkyIrradiance", this), IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);

	BaseClass::Initialize(engine, entity);
}

void SkyComponent::Uninitialize(Engine& engine, Entity* entity) {
	BaseClass::Uninitialize(engine, entity);
}

size_t SkyComponent::ReportGraphicMemoryUsage() const {
	return BaseClass::ReportGraphicMemoryUsage();
}
