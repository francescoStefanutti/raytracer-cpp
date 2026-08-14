#pragma once

#include "vec3.h"
#include "rtw_image.h"
#include <memory>
#include <cmath>
#include <algorithm>

class Texture
{

public:
	virtual Color value(double u, double v, Point3 p) const = 0;

	virtual ~Texture() = default;
};

class SolidColor : public Texture
{
	Color constant_color;

public: 
	SolidColor(Color c)
		: constant_color(c)
	{ }

	SolidColor(double r, double g, double b)
		: SolidColor(Color(r,g,b))
	{ }

	Color value(double u, double v, Point3 p) const override
	{
		return constant_color;
	}

};

class CheckerTexture : public Texture
{
	std::shared_ptr<Texture> even;
	std::shared_ptr<Texture> odd;
	double scale;

public:
	CheckerTexture(const std::shared_ptr<Texture>& even, const std::shared_ptr<Texture>& odd, double scale)
		: even(even), odd(odd), scale(scale)
	{ }

	CheckerTexture(const Color& color1, const Color& color2, double scale)
		: CheckerTexture(std::make_shared<SolidColor>(color1), std::make_shared<SolidColor>(color2), scale)
	{ }

	Color value(double u, double v, Point3 p) const override
	{
		auto p_scaled = p / scale;
		return int(floor(p_scaled.x) + floor(p_scaled.y) + floor(p_scaled.z)) % 2 ? odd->value(u, v, p) : even->value(u, v, p);
	}

};

class ImageTexture : public Texture
{
	std::shared_ptr<RTW_image> image;

public:
	ImageTexture(const std::string& path)
		: image(std::make_shared<RTW_image>(path))
	{ }

	Color value(double u, double v, Point3 p) const override
	{
		auto u_clamped = std::clamp(u, 0.0, 1.0);
		auto v_clamped = std::clamp(v, 0.0, 1.0);

		auto v_flipped = 1 - v_clamped;

		auto u_pixel_coordinate = u_clamped * image->width();
		auto v_pixel_coordinate = v_flipped * image->height();

		auto color = image->pixel_data(static_cast<int>(u_pixel_coordinate), static_cast<int>(v_pixel_coordinate));
		return Color{ color[0] / 255.0, color[1] / 255.0, color[2] / 255.0 };
	}
};