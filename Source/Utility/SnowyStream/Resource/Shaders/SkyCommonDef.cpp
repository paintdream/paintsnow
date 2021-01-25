#include "SkyCommonDef.h"
using namespace PaintsNow;

TObject<IReflect>& DensityProfileLayer::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(width);
		ReflectProperty(exp_term);
		ReflectProperty(exp_scale);
		ReflectProperty(linear_term);
		ReflectProperty(constant_term);
	}

	return *this;
}

TObject<IReflect>& DensityProfile::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(layer_top);
		ReflectProperty(layer_bottom);
	}

	return *this;
}

TObject<IReflect>& AtmosphereParameters::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(solar_irradiance);
		ReflectProperty(sun_angular_radius);
		ReflectProperty(bottom_radius);
		ReflectProperty(top_radius);
		ReflectProperty(rayleigh_density);
		ReflectProperty(rayleigh_scattering);
		ReflectProperty(mie_density);
		ReflectProperty(mie_scattering);
		ReflectProperty(mie_extinction);
		ReflectProperty(mie_phase_function_g);
		ReflectProperty(absorption_density);
		ReflectProperty(absorption_extinction);
		ReflectProperty(ground_albedo);
		ReflectProperty(mu_s_min);
	}

	return *this;
}
