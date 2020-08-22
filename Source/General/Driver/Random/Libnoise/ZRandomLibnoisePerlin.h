// ZRandomLibnoisePerlin.h
// By PaintDream
//

#pragma once
#include "../../../Interface/IRandom.h"
#include "Core/module/perlin.h"

namespace PaintsNow {
	class ZRandomLibnoisePerlin final : public IRandom {
	public:
		virtual void Seed(long seed);
		virtual void SetConfig(const String& parameter, double value);
		virtual double GetConfig(const String& parameter) const;
		virtual double GetValue(const double* coords, size_t dimension);

	private:
		noise::module::Perlin perlin;
	};
}

