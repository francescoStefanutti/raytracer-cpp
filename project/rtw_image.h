#pragma once
#include <memory>              // std::unique_ptr
#include <vector>              // std::vector
#include <iostream>
#include <array>               // std::array
#include <algorithm>           // std::clamp
#include "external/stb_image.h"

class RTW_image
{
	std::unique_ptr<unsigned char, decltype(&stbi_image_free)> fdata; 	// puntatore "smart" ai dati grezzi restituiti da stb_image, con deleter custom
	std::vector<unsigned char> bdata; 	// copia dei dati in un vector, usata per l'accesso sicuro ai pixel
	int image_width = 0;          // larghezza immagine, 0 = non caricata
	int image_height = 0;         // altezza immagine, 0 = non caricata
	int bytes_per_scanline = 0;   // quanti byte occupa una riga intera
	static constexpr int bytes_per_pixel = 3; // fissiamo sempre 3 canali (RGB)

public:
	RTW_image(const std::string& image_path)
		: fdata(nullptr, stbi_image_free) // inizializzazione esplicita: nessun dato ancora, deleter impostato
	{ 
		load(image_path); // delega il caricamento vero e proprio a load()
	}

	bool load(const std::string& image_path)
	{
		int n; // canali originali del file, non usato oltre (li forziamo a 3)
		auto raw = stbi_load(image_path.c_str(), &image_width, &image_height, &n, bytes_per_pixel);		// tenta il caricamento: width/height/n vengono riempiti dalla funzione stessa

		if (raw == nullptr)
			return false; // fallito: l'oggetto resta nello stato "vuoto" di default
		else
		{
			fdata.reset(raw); // fdata adotta il puntatore grezzo, se ne farà carico lui
			bytes_per_scanline = bytes_per_pixel * image_width; // byte totali per una riga
			bdata.assign(fdata.get(), fdata.get() + (image_width * image_height * bytes_per_pixel));// copia lineare di tutti i byte dell'immagine dentro bdata
			return true; // caricamento riuscito
		}
	}

	[[nodiscard]] int width() const
	{
		return image_width; // espone la larghezza a chi usa la classe
	}

	[[nodiscard]] int height() const
	{
		return image_height; // espone l'altezza a chi usa la classe
	}

	[[nodiscard]] std::array<unsigned char, 3> pixel_data(int x, int y) const
	{
		auto x_clamped = std::clamp(x, 0, image_width - 1);  // x sempre dentro i limiti validi
		auto y_clamped = std::clamp(y, 0, image_height - 1); // y sempre dentro i limiti validi
		auto i = y_clamped * bytes_per_scanline + x_clamped * bytes_per_pixel; // indice del primo byte del pixel
		return { bdata[i], bdata[i + 1], bdata[i + 2] }; // R,G,B del pixel richiesto
	}
};

/*
* Funzionamento generale:
*
* - fdata possiede la memoria allocata da stb_image (un unico blocco contiguo
*   di byte, organizzato riga per riga, 3 byte per pixel). Essendo allocata da
*   una libreria esterna, va liberata con la sua funzione dedicata
*   (stbi_image_free), da cui il deleter custom nello unique_ptr.
*
* - load() prova a caricare il file indicato. In caso di fallimento esce subito
*   senza toccare alcun campo, lasciando l'oggetto nello stato iniziale "vuoto"
*   (width/height a 0). In caso di successo, prende possesso dei dati grezzi,
*   calcola quanti byte occupa una riga, e ne fa una copia in bdata (un
*   std::vector, più semplice e sicuro da indicizzare rispetto al puntatore
*   grezzo di fdata).
*
* - pixel_data(x, y) è l'unico punto in cui viene fatto il clamping delle
*   coordinate: se x o y cadono fuori dai limiti validi (per arrotondamenti a
*   monte), vengono "schiacciati" dentro il range corretto con std::clamp,
*   invece di rischiare una lettura fuori dai limiti del vector. L'indice del
*   pixel nel blocco lineare di byte si calcola con: riga * byte-per-riga +
*   colonna * byte-per-pixel. Il risultato è un array di 3 byte (R,G,B), scelto
*   al posto di una struct dedicata perché i tre valori sono omogenei (stesso
*   tipo, stesso ruolo), a differenza di un caso come Sphere::UV dove u e v
*   hanno significati distinti.
*/