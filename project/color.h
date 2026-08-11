#pragma once

#include "Vec3.h"

#include<iostream>
#include<algorithm> // per metodo nativo di costrizione dei limiti

inline double linear_to_gamma(double linear_component)
{
	return std::sqrt(linear_component);
}

inline void write_color(std::ostream& out, Color pixel_color, int samples_per_pixel)
{
	// scale serve perchè facciamo la media di tutti i pixel presi nei samples
	auto scale = 1.0 / samples_per_pixel;
	auto pixel_color_scaled = pixel_color * scale;

	// gamma correction
	Vec3 pixel_color_gammacorrected;
	pixel_color_gammacorrected.x = linear_to_gamma(pixel_color_scaled.x);
	pixel_color_gammacorrected.y = linear_to_gamma(pixel_color_scaled.y);
	pixel_color_gammacorrected.z = linear_to_gamma(pixel_color_scaled.z);

	// qua devo calcolare ogni compoentne con la funzione?

	int r = static_cast<int>(std::clamp(pixel_color_gammacorrected.x, 0.0, 0.999) * 255.999);
	int g = static_cast<int>(std::clamp(pixel_color_gammacorrected.y, 0.0, 0.999) * 255.999);
	int b = static_cast<int>(std::clamp(pixel_color_gammacorrected.z, 0.0, 0.999) * 255.999);

	out << r << " " << g << " " << b << "\n";
}


