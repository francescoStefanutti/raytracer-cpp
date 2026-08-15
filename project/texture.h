#pragma once

#include "vec3.h"
#include "rtw_image.h"
#include "perlin.h"
#include <memory>
#include <cmath>
#include <algorithm>

// Interfaccia astratta: ogni texture concreta sa dire "che colore ha in (u,v,p)"
class Texture
{
public:
	virtual Color value(double u, double v, Point3 p) const = 0;

	virtual ~Texture() = default;
};

// Texture a colore costante, ignora u/v/p
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

// Texture a scacchiera spaziale (usa solo p, ignora u/v)
class CheckerTexture : public Texture
{
	std::shared_ptr<Texture> even; // sotto-texture per le celle "pari"
	std::shared_ptr<Texture> odd;  // sotto-texture per le celle "dispari"
	double scale;                  // dimensione di ogni cella della scacchiera

public:
	CheckerTexture(const std::shared_ptr<Texture>& even, const std::shared_ptr<Texture>& odd, double scale)
		: even(even), odd(odd), scale(scale)
	{ }

	CheckerTexture(const Color& color1, const Color& color2, double scale)
		: CheckerTexture(std::make_shared<SolidColor>(color1), std::make_shared<SolidColor>(color2), scale)
	{ }

	Color value(double u, double v, Point3 p) const override
	{
		auto p_scaled = p / scale; // riscala lo spazio in unità di "celle"
		// somma dei tre indici di cella, pari/dispari decide even/odd
		return int(floor(p_scaled.x) + floor(p_scaled.y) + floor(p_scaled.z)) % 2 ? odd->value(u, v, p) : even->value(u, v, p);
	}
};

// Texture da immagine reale (es. earthmap.jpg), usa solo u/v, ignora p
class ImageTexture : public Texture
{
	std::shared_ptr<RTW_image> image; // shared_ptr perché RTW_image non è copiabile

public:
	ImageTexture(const std::string& path)
		: image(std::make_shared<RTW_image>(path))
	{ }

	Color value(double u, double v, Point3 p) const override
	{
		auto u_clamped = std::clamp(u, 0.0, 1.0);   // protezione contro u fuori range
		auto v_clamped = std::clamp(v, 0.0, 1.0);   // protezione contro v fuori range

		auto v_flipped = 1 - v_clamped;             // v=0 è polo sud, ma riga 0 immagine è in alto

		auto u_pixel_coordinate = u_clamped * image->width();   // u in [0,1] -> colonna pixel
		auto v_pixel_coordinate = v_flipped * image->height();  // v in [0,1] -> riga pixel

		auto color = image->pixel_data(static_cast<int>(u_pixel_coordinate), static_cast<int>(v_pixel_coordinate));

		return Color{ color[0] / 255.0, color[1] / 255.0, color[2] / 255.0 }; // byte 0-255 -> double 0.0-1.0
	}
};


class NoiseTexture : public Texture
{
public:
	enum class Mode
	{
		Smooth, Turbolence, Marble
	};

private:
	Perlin perlin;
	double scale;
	int depth;
	Mode mode;

public:
	NoiseTexture(double scale, Mode mode, int depth = 7)
		: scale(scale), mode(mode), depth(depth)
	{ }


	Color value(double u, double v, Point3 p) const override
	{
		double grey_value;
		switch (mode)
		{
		case Mode::Smooth:
			grey_value = 0.5 * (perlin.noise_smooth_gradient(p * scale) + 1.0);
			break;
		
		case Mode::Turbolence:
			grey_value = perlin.turbolence(p * scale, depth);
			break;

		case Mode::Marble:
			grey_value = 0.5*(1 + sin(scale * p.z + 10 * perlin.turbolence(p, depth)));
			break;
		}

		return { grey_value, grey_value, grey_value };
	}

};


/*
* RIEPILOGO DEL FILE
*
* Texture è la classe astratta comune a tutte le texture: definisce solo
* value(u, v, p) -> Color, cioè "che colore ha la superficie in quel punto".
* I materiali (Lambertian, Metal, Dielectric) possiedono uno shared_ptr<Texture>
* e lo interrogano invece di usare un Color fisso.
*
* SolidColor: caso base, colore costante ovunque, ignora completamente u/v/p.
* Serve anche come "adattatore": i costruttori dei materiali che accettano
* ancora un Color diretto lo avvolgono automaticamente in una SolidColor.
*
* CheckerTexture: pattern a scacchiera nello spazio 3D (non sulla superficie
* 2D via u/v). Divide lo spazio in celle cubiche di lato `scale`, e sceglie
* tra due sotto-texture (even/odd) in base alla parità della somma degli
* indici di cella lungo i tre assi. Essendo composta da Texture (non da
* Color), le sotto-texture possono a loro volta essere immagini, altre
* scacchiere, ecc.
*
* ImageTexture: mappa una vera immagine (caricata via RTW_image) sulla
* superficie usando le coordinate u/v. Il flip di v è necessario perché la
* convenzione delle immagini (riga 0 in alto) è opposta alla convenzione
* usata da get_sphere_uv (v=0 al polo sud). image è tenuta per shared_ptr
* perché RTW_image contiene un unique_ptr internamente e quindi non è
* copiabile: ImageTexture resta comunque copiabile a basso costo (si copia
* solo il puntatore condiviso, non i dati dell'immagine).
*/

