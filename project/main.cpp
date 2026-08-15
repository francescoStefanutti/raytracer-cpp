#include <iostream>
#include <fstream>
#include <cmath>
#include <memory>
#include "Vec3.h"
#include "color.h"
#include "ray.h"
#include "sphere.h"
#include "hittable_list.h"
#include "camera.h"
#include "material.h"
#include "bvh.h"
#include "texture.h"

int main()
{
	std::ofstream image("image.ppm");

	auto checkered_texture = std::make_shared<CheckerTexture>(Color{ 0.2, 0.5, 0.9 }, Color{ 0.6, 0.1, 0.2 }, 2);
	auto world_texture = std::make_shared<ImageTexture>("images/earthmap.jpg");
	auto noise_texture = std::make_shared<NoiseTexture>(1, NoiseTexture::Mode::Turbolence, 5);

	auto wood = std::make_shared<Lambertian>(Color{ 0.3,0.2,0.1 });
	auto checkered = std::make_shared<Lambertian>(checkered_texture);
	auto gold = std::make_shared<Metal>(Color{ 1.0,1.0,0.0 }, 0.4);
	auto copper = std::make_shared<Metal>(Color{ 0.7,0.4,0.2 }, 0.8);
	auto glass = std::make_shared<Dielectric>(1.5);
	auto glass_green = std::make_shared<Dielectric>(1.5, Color{0,1,0});
	auto world_material = std::make_shared<Lambertian>(world_texture);
	auto noise_material = std::make_shared<Metal>(noise_texture, 0.3);

	Hittable_list world;
	world.reserve(6);
	world.add(std::make_shared<Sphere>(100, Point3{ 0,-100.5,-1 }, noise_material));
	world.add(std::make_shared<Sphere>(0.5, Point3{ 0,0,-1 }, world_material));
	world.add(std::make_shared<Sphere>(0.5, Point3{ -1.5,0,-1 }, glass));
	world.add(std::make_shared<Sphere>(0.5, Point3{ 1.5,0,-1 }, glass_green));
	world.add(std::make_shared<Sphere>(0.2, Point3{ 2,0,-2 }, Point3{ 2,0.5,-2 }, wood));
	world.add(std::make_shared<Sphere>(0.2, Point3{ -2,0,-2 }, Point3{ -2,0.5,-2 }, gold));
	
	BVH_node tree(world);

	Camera camera;
	camera.lookfrom = {-3, 3, 1};
	camera.vfov = 40.0;
	camera.defocus_angle = 5.0;
	camera.focus_dist = (camera.lookfrom - camera.lookat).length(); // distanza giusta per mettere afuoco la palla di rame
	camera.Render(tree, image);
}

/*
* ==========================================
* APPUNTI DA SENIOR DEV: LOG DEL TERMINALE IN TEMPO REALE
* ==========================================
* Questa riga crea un "indicatore di progresso" aggiornando sempre la stessa riga:
* 
* - std::clog: Flusso dedicato a log e diagnostica. È best practice usarlo al posto 
*   di std::cout per i messaggi, così da non sporcare l'output reale del programma.
* 
* - '\r' (Carriage Return): Riporta il cursore all'inizio della riga senza andare a 
*   capo. Qualsiasi cosa stampata dopo sovrascriverà il testo precedente.
* 
* - lo spazio extra (' '): Serve a "pulire" i caratteri in eccesso. Se passi da 
*   "100" a "99", lo spazio sovrascrive lo zero che altrimenti resterebbe a video ("990").
* 
* - std::flush: Forza il terminale a stampare immediatamente il testo a schermo. 
*   Senza questo comando, il terminale aspetterebbe di vedere un '\n' prima di 
*   mostrare qualsiasi cosa, rendendo inutile il nostro \r.
* ==========================================
*/
/*
* ==========================================
* APPUNTI DA SENIOR DEV: CONSTEXPR vs CONST E LA MAGIA DI AUTO
* ==========================================
* 
* 1. La regola d'oro di "constexpr" (Default to constexpr):
*    - Se un valore è fisso o calcolabile PRIMA che il programma parta (a compile-time), 
*      usa SEMPRE "constexpr" al posto di "const".
*    - Perché? Perché il compilatore ti fa da guardia del corpo: se provi a metterci 
*      dentro un'operazione che richiede il runtime (es. divisioni o operatori ternari 
*      complessi), bloccherà subito la compilazione.
*    - "const" invece si limita a dire "questa variabile non si modifica dopo la 
*      creazione", ma non garantisce che sia calcolata prima.
* 
* 2. L'errore classico con constexpr:
*    - Non puoi usarlo se il valore dipende da calcoli dinamici o da condizioni 
*      (come image_height = (image_height < 1) ? 1 : image_height). In quel caso, 
*      si usa "const".
* 
* 3. Perché usare "const auto" anche quando conosciamo il tipo (es. const Vec3):
*    - Coerenza architetturale: uniforma lo stile del codice quando si maneggiano 
*      oggetti geometrici complessi o risultati di funzioni.
*    - Performance: A livello di codice macchina generato, "const auto" e "const Vec3" 
*      producono esattamente lo stesso identico binario (zero overhead).
* ==========================================
*/ 