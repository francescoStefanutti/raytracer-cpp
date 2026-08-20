#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <execution>
#include <ranges>
#include <algorithm>
#include "essentials.h"
#include "vec3.h"
#include "hittable.h"
#include "color.h"
#include "ray.h"
#include "material.h"
#include "external/stb_image_write.h"

class Camera
{
public:
	// CAMERA
	double aspect_ratio = 16.0 / 9.0;
	int image_width = 400;

	double vfov = 90.0;
	double defocus_angle = 0.0; // con zero abbiamo una pinhole camera, valori maggiori comportamento più natural con sfocatura
	double focus_dist = 1.0;

	Point3 lookfrom{ 0,0,0 };
	Point3 lookat{ 0,0,-1 };
	Vec3 vup{ 0,1,0 };


	//Aliasing
	int samples_per_pixel = 100;

	//Reflection
	int max_depth = 50;

	Color background{ 0,0,0 }; // colore fisso restituito quando un raggio non colpisce nulla (sostituisce il vecchio skybox)


	void Render(const Hittable& world, std::ofstream& image, const std::string& png_filename)
	{
		Initialize();

		image << "P3\n";
		image << image_width << " " << image_height << "\n";
		image << 255 << "\n";

		// creo vettore in cui scriverà i colori sfruttando multi thread. non posso sfruttare l'ottimizazzione di reserve perchè ogni thread deve andare a scrivere nel vettore a un preciso index quindi ho bisogno che vengano inizializzati dei color come default, anche se vuol dire usare più memoria
		std::vector<Color> pixels(image_height * image_width);

		// creo un oggetto range, una sequenza virtuale che si comporta come un contenitore, ma senza memorizzare nulla in memoria: genera i numeri 0, 1, 2, ..., image_height-1 al volo, uno alla volta, quando richiesti tramite i suoi iteratori. serve al std::for_each per funzionare
		auto rows = std::views::iota(0, image_height);

		std::for_each(std::execution::par, rows.begin(), rows.end(), [&](int j)
			{
				for (int i = 0; i <= image_width - 1; i++)
				{
					const auto pixel_center = pixel00_loc + i * pixel_delta_u + j * pixel_delta_v;

					Color pixel_color{ 0,0,0 };
					for (int sample = 0; sample < samples_per_pixel; sample++)
					{
						auto origin = defocus_angle <= 0 ? lookfrom : defocus_disk_sample(); // in questo modo non chiamo inutilmente la funzione quando ho la pinhole camera

						const auto pixel_random = pixel_center + pixel_sample_square();
						const auto ray_direction = pixel_random - origin;

						Ray ray(origin, ray_direction, random_double());

						pixel_color += ray_color(ray, world, max_depth);
					}

					pixels[j * image_width + i] = pixel_color;
				}
			});

		// buffer binario per il PNG: 3 byte (R,G,B) per pixel, riempito nel loop sotto e scritto su disco una sola volta a fine funzione
		std::vector<unsigned char> color_list(image_height * image_width * 3);

		// dopo avere riempito il mio array, ora lo scorro e ogni colore lo stampo applicando le correzioni
		for (int j = 0; j < image_height; j++)
		{
			std::clog << "\rRighe rimanenti: "<< image_height - j << ' ' << std::flush; // leggi commento sotto

			for (int i = 0; i <= image_width - 1; i++)
			{
				auto index = j * image_width + i;
				auto colors = calculate_color(pixels[index], samples_per_pixel); // calcolo (scale/gamma/clamp) fatto una sola volta, condiviso da entrambi gli output

				write_color_ppm(image, colors); // scrittura incrementale, testo, direttamente sullo stream .ppm
				write_color_png(colors, color_list, index * 3); // scrittura nel buffer .png, *3 perchè ogni pixel occupa 3 celle consecutive
			}
		}

		// scrittura del file .png in un colpo solo: il formato è compresso, serve l'immagine intera in memoria (a differenza del .ppm scritto in streaming sopra)
		stbi_write_png(png_filename.c_str(), image_width, image_height , 3, color_list.data(), image_width * 3);

		std::clog << "\rFatto					   \n";
	}


private:
	int image_height;
	Point3 pixel00_loc;
	Vec3 pixel_delta_u;
	Vec3 pixel_delta_v;

	Vec3 w;
	Vec3 u;
	Vec3 v;

	Vec3 defocus_disk_u;
	Vec3 defocus_disk_v;

	void Initialize()
	{
		auto camera_to_target = lookfrom - lookat;
		w = unit_vector(camera_to_target);
		u = unit_vector(cross(vup, w));
		v = unit_vector(cross(w, u));

		auto defocus_radius = focus_dist * tan(deg_to_rad(defocus_angle / 2));
		defocus_disk_u = u * defocus_radius;
		defocus_disk_v = v * defocus_radius;


		// VIEWPORT
		image_height = static_cast<int>(image_width * 1 / aspect_ratio) < 1 ? 1 : static_cast<int>(image_width * 1 / aspect_ratio); 	// per evitare che l'altezza sia minore di 1 e craschi
		const auto focal_length = camera_to_target.length();
		const auto viewport_height = 2 * tan(deg_to_rad(vfov / 2)) * focal_length;
		const auto viewport_width = static_cast<double>(image_width)/ image_height * viewport_height; // non usiamo l'aspect ratio perchè per calcolare la viewport height perchè l'altezza in pixel di prima è stata arrotondata (essendo un int), quindi il formato reale della nostra immagine non è più esattamente 16:9 al millimetro. Dobbiamo usare l'aspect ratio "reale" ricalcolandolo così: dividi image_width per image_height (ricordati di fare un static_cast<double> su uno dei due, altrimenti il C++ farà una divisione tra interi e perderai i decimali!).

		const Vec3 viewport_u = viewport_width * u;
		const Vec3 viewport_v = -viewport_height * v;


		// Dobbiamo calcolare la distanza fisica 3D tra un pixel e quello successivo.
		pixel_delta_u = viewport_u / image_width;
		pixel_delta_v = viewport_v / image_height;

		// Ora troviamo le coordinate esatte nello spazio tridimensionale da cui far partire il primo raggio in alto a sx
		const auto viewport_upper_left = lookfrom - focal_length*w - viewport_u * 0.5 - viewport_v * 0.5;
		// prendiamo il primo pixel
		pixel00_loc = viewport_upper_left + pixel_delta_u * 0.5 + pixel_delta_v * 0.5;
	}



	// posso passare hittable perchè anche la list è figlia
	Color ray_color(const Ray& r, const Hittable& objects, int depth)
	{
		HitRecord rec;

		// Se abbiamo superato il limite di rimbalzi, la luce si spegne (nero)
		if (depth <= 0)
		{
			return Color{ 0,0,0 };
		}

		if (objects.hit(r, 0.001, infinity, rec)) // 0.001 per evitare shadow acne e che succeda che il raggio colpisca l'oggetto stesso una volta riflesso
		{
			auto scatter_result = rec.mat->scatter(r, rec); // il materiale rimbalza il raggio?
			auto emitted_result = rec.mat->emitted(rec.u, rec.v, rec.P, rec.front_face); // il materiale emette luce propria? (nero per i materiali non luminosi)

			// se l'optional è pieno c'è stata riflessinoe
			if (scatter_result)
				// colore finale = luce emessa qui + luce riflessa (attenuata) che arriva dal rimbalzo
				return  emitted_result + scatter_result->attenuation * ray_color(scatter_result->scattered, objects, depth - 1); // Il colore finale è viene attuenatuo in base all'albedo della amteriale incontrano quando il raggio rimbalzato
			else
				return emitted_result; // niente rimbalzo (es. una luce): il colore è solo quello emesso, non più forzatamente nero
		}
		else
		{
			return background; // nessun hit: colore di sfondo fisso, non più calcolato dalla direzione del raggio
		}
	}

	// funzione per generare un vettore random di spostamente relativo dal centro del pixel
	Vec3 pixel_sample_square() const
	{
		return pixel_delta_u * (random_double() - 0.5) + pixel_delta_v * (random_double() - 0.5);
	}

	Vec3 defocus_disk_sample()
	{
		Vec3 r = random_in_unit_disk();
		return lookfrom + r.x*defocus_disk_u + r.y*defocus_disk_v ;
	}

};

// IL SEGRETO DELL'ILLUMINAZIONE GLOBALE E DELL'ALBEDO:
// L'oggetto NON ha un colore proprio. Il suo colore dipende dalla luce che il raggio riflesso riesce a catturare.
// 
// 1. La chiamata ricorsiva ray_color(...) "mette in pausa" il calcolo, lancia il nuovo raggio 'scattered'
//    nella scena e aspetta che questo colpisca qualcosa (un'altra superficie, una sorgente di luce, o lo
//    sfondo fisso 'background') per sapere di che colore è la luce in arrivo.
// 2. Il '0.5' (nell'esempio storico con lo skybox) rappresentava l'Albedo (Riflettanza) del materiale.
//    Indica che questa superficie opaca assorbe una parte dell'energia luminosa e riflette il resto.
// 3. Quando il raggio torna indietro con un colore, questo viene moltiplicato per l'attenuazione del
//    materiale. Se un raggio rimane incastrato facendo molti rimbalzi, subirà questa moltiplicazione
//    (perdendo energia) a ogni impatto, diventando sempre più scuro e generando ombre ultra-realistiche.
//
// ILLUMINAZIONE CON MATERIALI EMISSIVI (DiffuseLight):
// Prima, l'unica fonte di luce della scena era lo skybox (un gradiente calcolato dalla direzione del
// raggio quando non colpiva nulla). Ora lo sfondo è un colore fisso (spesso nero), e la luce nella scena
// proviene SOLO da oggetti a cui è stato assegnato esplicitamente un materiale emissivo:
// - emitted(u,v,p): quanta luce propria produce il materiale in quel punto (nero per Lambertian/Metal/
//   Dielectric, un colore acceso, spesso >1, per DiffuseLight).
// - scatter(...): il raggio rimbalza da qui? Per una luce, no (restituisce nullopt) -- il percorso del
//   raggio termina lì, il colore finale è solo l'emissione.
// Il colore di un punto di hit è quindi: emitted + attenuation * (colore che arriva rimbalzando).
// Un raggio che, rimbalzando a caso, finisce per colpire una luce "riporta indietro" quel colore acceso
// lungo tutta la catena di rimbalzi intermedi, ognuno dei quali lo attenua un po' -- esattamente come la
// luce vera che rimbalza sugli oggetti prima di raggiungere l'occhio/camera. Se invece nessun rimbalzo
// casuale colpisce mai una luce, quel campione contribuisce con il colore di background (spesso nero):
// per questo, con luci piccole, serve un numero alto di samples_per_pixel per ottenere un'immagine pulita.



// PARALLELIZZAZIONE DEL RENDERING (CPU multithreading via std::execution::par)
//
// Il calcolo di ogni pixel è indipendente da quello degli altri pixel (embarrassingly
// parallel): non serve nessuna sincronizzazione tra i thread mentre calcolano.
// Il problema era solo NEL PUNTO in cui scrivevamo il risultato: scrivere
// direttamente nel file (std::ofstream) da thread diversi contemporaneamente
// causerebbe una race condition (output intrecciato/corrotto), perché lo stream
// è stato condiviso e non è pensato per accessi concorrenti.
//
// Soluzione: separare il CALCOLO (parallelo) dalla SCRITTURA su file (sequenziale).
// 1) Ogni thread calcola una riga (indice j) e scrive il colore grezzo in una zona
//    di memoria propria e non sovrapposta ad altri thread: pixels[j*image_width+i].
//    Poiché ogni riga j occupa un blocco di indici esclusivo (nessun j diverso può
//    produrre lo stesso indice), non serve alcun mutex: il problema sparisce per
//    come è strutturato l'accesso, non per sincronizzazione difensiva.
// 2) std::for_each(std::execution::par, ...) è bloccante: quando la chiamata
//    ritorna, TUTTI i thread hanno finito e "pixels" è completamente riempito.
// 3) Solo a quel punto un ciclo sequenziale (un solo thread, nessun rischio di
//    race condition) scorre "pixels" in ordine e applica calculate_color() +
//    write_color_ppm()/write_color_png() -- che fanno scaling per samples_per_pixel,
//    gamma correction e clamp, poi smistano lo stesso risultato verso i due file di
//    output -- scrivendo il risultato finale sempre nell'ordine corretto riga per riga.


// OUTPUT DOPPIO: PPM (streaming, testo) + PNG (buffer in memoria, binario compresso)
//
// I due formati hanno requisiti opposti sul MOMENTO in cui si può scrivere:
// - .ppm (P3) è testo semplice, non compresso: ogni pixel può essere scritto sullo
//   stream non appena calcolato, riga per riga, senza bisogno di vedere il resto
//   dell'immagine. Per questo write_color_ppm scrive direttamente dentro il loop.
// - .png è un formato binario compresso (DEFLATE, lo stesso algoritmo dello zip):
//   l'encoder ha bisogno dell'immagine intera in memoria per comprimerla bene, quindi
//   non si può scrivere in streaming pixel per pixel. Per questo color_list accumula
//   tutti i byte durante il loop, e stbi_write_png viene chiamata una sola volta,
//   a loop terminato, quando il buffer è completamente pieno.
//
// In entrambi i casi il colore (calculate_color) viene calcolato UNA SOLA VOLTA per
// pixel: le due funzioni write_color_ppm/write_color_png non ricalcolano nulla, si
// limitano a smistare lo stesso risultato RGB verso una destinazione diversa (stream
// di testo vs cella di un vector di byte), evitando di duplicare la logica di
// gamma correction/clamp in due punti del codice.