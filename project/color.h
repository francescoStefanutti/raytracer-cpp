#pragma once
#include "vec3.h"
#include <iostream>
#include <algorithm> // per metodo nativo di costrizione dei limiti
#include <vector>

// Colore finale di un pixel, già pronto per l'output: tre interi 0-255, indipendenti dal formato file
struct RGB
{
	int r, g, b;
};

inline double linear_to_gamma(double linear_component)
{
	return std::sqrt(linear_component);
}

inline RGB calculate_color(Color pixel_color, int samples_per_pixel)
{
	// scale serve perchè facciamo la media di tutti i pixel presi nei samples
	auto scale = 1.0 / samples_per_pixel;
	auto pixel_color_scaled = pixel_color * scale;

	// gamma correction
	Vec3 pixel_color_gammacorrected;
	pixel_color_gammacorrected.x = linear_to_gamma(pixel_color_scaled.x);
	pixel_color_gammacorrected.y = linear_to_gamma(pixel_color_scaled.y);
	pixel_color_gammacorrected.z = linear_to_gamma(pixel_color_scaled.z);

	// clamp per evitare overflow (colori > 1.0 con luci intense) poi scala a 0-255
	int r = static_cast<int>(std::clamp(pixel_color_gammacorrected.x, 0.0, 0.999) * 255.999);
	int g = static_cast<int>(std::clamp(pixel_color_gammacorrected.y, 0.0, 0.999) * 255.999);
	int b = static_cast<int>(std::clamp(pixel_color_gammacorrected.z, 0.0, 0.999) * 255.999);
	return { r,g,b };
}

// Scrive un pixel come testo leggibile, nel formato PPM (P3): "r g b\n"
inline void write_color_ppm(std::ostream& out, RGB color)
{
	out << color.r << " " << color.g << " " << color.b << "\n";
}

// Scrive un pixel come 3 byte grezzi consecutivi in un buffer condiviso, alla posizione data
inline void write_color_png(RGB color, std::vector<unsigned char>& color_list, int index)
{
	color_list[index] = color.r;
	color_list[index+1] = color.g;
	color_list[index + 2] = color.b;
}

/*
* ==========================================
* APPUNTI: PIPELINE DI CONVERSIONE COLORE E DOPPIO OUTPUT (PPM + PNG)
* ==========================================
* calculate_color() è il punto UNICO in cui un colore grezzo (double, floating point,
* accumulato sommando i vari samples in Camera::Render) viene trasformato nel colore
* finale da scrivere su file: media dei samples (scale), correzione gamma
* (linear_to_gamma, approssimazione con sqrt della curva gamma 2.0), clamp nel range
* [0, 0.999] per evitare overflow quando un pixel supera l'intensità 1.0 (tipico vicino
* a sorgenti di luce), e conversione finale a interi 0-255.
*
* Questo calcolo va fatto una sola volta per pixel: il risultato (RGB) viene poi
* smistato a due funzioni "sottili" che non ricalcolano nulla, si limitano a scrivere
* lo stesso dato in due destinazioni diverse:
* - write_color_ppm: scrittura testuale incrementale, un pixel alla volta, direttamente
*   sullo stream del file .ppm (formato non compresso, storicamente semplice da
*   implementare, ma file molto più pesanti).
* - write_color_png: scrittura binaria in un buffer std::vector<unsigned char> tenuto
*   in memoria (3 celle consecutive per pixel: R,G,B). Il buffer viene riempito durante
*   il loop ma scritto su disco solo alla fine, in un'unica chiamata a stbi_write_png
*   (in camera.h): il formato PNG è compresso (DEFLATE), quindi l'encoder ha bisogno di
*   vedere l'intera immagine in memoria per comprimerla, non può scrivere in streaming
*   come il testo PPM.
* ==========================================
*/