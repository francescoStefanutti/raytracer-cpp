#include <iostream>
#include <fstream>
#include <cmath>
#include <memory>
#include "vec3.h"
#include "color.h"
#include "ray.h"
#include "sphere.h"
#include "hittable_list.h"
#include "camera.h"
#include "material.h"
#include "bvh.h"
#include "texture.h"
#include "quad.h"
#include "triangle.h"
#include "matrix.h"
#include "transform.h"
#include "constant_medium.h"

int main()
{
	std::ofstream image("image.ppm");

	std::unique_ptr<BVH_node> tree;

	try
	{
		// BOUNDARY
		auto white = std::make_shared<Lambertian>(Color{ 1,1,1 });
		auto boundary_volume = box({ -0.7, -0.5, 1.0 }, { 0.7, 0.7, 2.0 }, white);

		// --- TEXTURE ---
		auto checkered_texture = std::make_shared<CheckerTexture>(Color{ 0.2, 0.5, 0.9 }, Color{ 0.6, 0.1, 0.2 }, 1.0);
		auto world_texture = std::make_shared<ImageTexture>("images/earthmap.jpg");
		auto noise_turb = std::make_shared<NoiseTexture>(4, NoiseTexture::Mode::Turbulence, 5);
		auto noise_marble = std::make_shared<NoiseTexture>(5, NoiseTexture::Mode::Marble, 7);

		// --- MATERIALI ---
		auto ground_material = std::make_shared<Lambertian>(checkered_texture);
		auto earth_material = std::make_shared<Lambertian>(world_texture);
		auto glass = std::make_shared<Dielectric>(1.5);
		auto glass_green = std::make_shared<Dielectric>(1.5, Color{ 0,1,0 });
		auto gold = std::make_shared<Metal>(Color{ 1.0,0.8,0.2 }, 0.1);
		auto copper = std::make_shared<Metal>(Color{ 0.7,0.4,0.2 }, 0.5);
		auto noise_marble_material = std::make_shared<Lambertian>(noise_marble);
		auto noise_turb_material = std::make_shared<Metal>(noise_turb, 0.3);
		auto light_material = std::make_shared<DiffuseLight>(Color{ 4,4,4 });
		auto bouncing_yellow = std::make_shared<Lambertian>(Color{ 0.9,0.9,0.05 });
		auto bouncing_magenta = std::make_shared<Metal>(Color{ 0.9,0.2,0.9 }, 0.1);

		Hittable_list world;
		world.reserve(13);

		// Terreno: sfera enorme, a scacchi
		world.add(std::make_shared<Sphere>(1000, Point3{ 0,-1000.5,-1 }, ground_material));

		// Fila di sfere in primo piano, spaziate lungo x, stessa profondità z=-1
		world.add(std::make_shared<Sphere>(0.5, Point3{ -3,0,-1 }, earth_material));
		world.add(std::make_shared<Sphere>(0.5, Point3{ -1.5,0,-1 }, glass));
		world.add(std::make_shared<Sphere>(0.5, Point3{ 0,0,-1 }, glass_green));
		world.add(std::make_shared<Sphere>(0.5, Point3{ 1.5,0,-1 }, gold));
		world.add(std::make_shared<Sphere>(0.5, Point3{ 3,0,-1 }, copper));

		// Sfera con noise marble, leggermente arretrata
		world.add(std::make_shared<Sphere>(0.6, Point3{ 0,0.1,-2.5 }, noise_marble_material));

		// Quad verticale con Metal, ruotato intorno alla propria asse y usando trasformazioni
		auto quad_original = std::make_shared<Quad>(Point3{ 4.5,-0.5,-3 }, Vec3{ 0,2,0 }, Vec3{ 1.5,0,1 }, copper);
		Point3 center_quad{ 5.25, 0.5, -2.5 };
		auto matrix = RigidTransform::translation(center_quad) * RigidTransform::rotation(0, 90, 0) * RigidTransform::translation(-center_quad);
		world.add(std::make_shared<Transform>(quad_original, matrix));

		// Triangolo con noise turbulence (metallico), a sinistra, ben visibile
		world.add(std::make_shared<Triangle>(Point3{ -4.5,0,-2 }, Vec3{ 2,0,0 }, Vec3{ 1,2,0 }, noise_turb_material));

		// Luce: pannello grande sopra tutta la scena
		world.add(std::make_shared<Quad>(Point3{ -2,3,-3 }, Vec3{ 4,0,0 }, Vec3{ 0,0,4 }, light_material));

		// Due sfere in movimento (motion blur): centro dinamico, spostamento piccolo e verticale.
		world.add(std::make_shared<Sphere>(0.3, Point3{ -3.8,0.3,0.3 }, Point3{ -3.8,0.6,0.3 }, bouncing_yellow));
		world.add(std::make_shared<Sphere>(0.3, Point3{ 3.8,0.3,0.3 }, Point3{ 3.8,0.6,0.3 }, bouncing_magenta));

		// Volume di gas
		world.add(std::make_shared<ConstantMedium>(boundary_volume, 2, Color{ 0.2, 0.5, 0.7 }));

		tree=  std::make_unique<BVH_node>(world);
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	Camera camera;
	camera.background = { 0.05, 0.05, 0.08 }; // sfondo scuro ma non nero puro, per far risaltare la luce
	camera.aspect_ratio = 16.0 / 9.0;
	camera.image_width = 600;
	camera.samples_per_pixel = 200; // più campioni: con una luce vera serve più campionamento per ridurre il rumore
	camera.max_depth = 50;

	camera.lookfrom = { 0, 3, 7 };
	camera.lookat = { 0, 0.3, -1.5 };
	camera.vfov = 45.0;
	camera.defocus_angle = 0.6;
	camera.focus_dist = (camera.lookfrom - camera.lookat).length();

	camera.Render(*tree, image, "output.png");
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