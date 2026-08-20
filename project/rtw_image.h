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
		if (!load(image_path)) // controllo esplicito: caricamento fallito -> l'oggetto non deve nascere in uno stato "vuoto" inosservato
			throw std::runtime_error("texture non trovata"); // vedi nota in fondo al file sul perché throw e non solo un return
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
		if (bdata.empty()) // guardia di sicurezza: niente dati caricati, niente calcoli sugli indici (vedi nota in fondo)
			return { 255,0,255 }; // magenta "sentinella": segnala visivamente una texture mancante invece di UB
	
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

// ==========================================
// APPUNTI: PERCHÉ throw NEL COSTRUTTORE, E PERCHÉ IL CONTROLLO IN pixel_data()
// ==========================================
// Prima, il costruttore chiamava load() ignorando il valore restituito (bool).
// Se il file non veniva trovato (es. percorso relativo sbagliato, working
// directory diversa da quella attesa), l'oggetto RTW_image nasceva comunque,
// ma "vuoto" (image_width = 0, bdata vuoto) -- senza nessun segnale d'errore.
// Il problema emergeva solo molto più tardi, alla prima pixel_data(): con
// image_width = 0, "image_width - 1" vale -1, e std::clamp(x, 0, -1) riceve un
// intervallo invertito (minimo > massimo) -- comportamento non garantito dal
// C++ (undefined behavior), fino a un accesso fuori dai limiti di un bdata mai
// riempito. Risultato tipico: crash difficile da collegare alla causa reale
// (un file mancante), scoperto magari a metà di un render lungo.
//
// throw std::runtime_error(...) nel costruttore sposta l'errore al momento
// giusto: se il file non si carica, l'oggetto RTW_image non viene creato
// affatto (l'eccezione interrompe la costruzione), e il programma si ferma
// subito con un messaggio chiaro -- invece di proseguire silenziosamente con
// una texture "fantasma" che esploderà altrove, più avanti, in un punto del
// codice che non c'entra nulla col vero problema (il percorso del file).
//
// Il controllo "if (bdata.empty())" dentro pixel_data() resta comunque, come
// seconda linea di difesa: se in futuro qualcuno costruisse un RTW_image in un
// modo che aggira il costruttore attuale (o gestisse l'eccezione e continuasse
// a usare l'oggetto vuoto), pixel_data() non farebbe comunque UB -- restituisce
// un colore sentinella riconoscibile a vista (magenta), invece di un calcolo
// su un intervallo invertito. Due controlli, in due punti diversi, con due
// scopi diversi: uno impedisce all'oggetto di esistere rotto, l'altro rende
// sicura la funzione anche nel caso (oggi non previsto, ma non impossibile)
// in cui un oggetto rotto venga comunque usato.
// ==========================================