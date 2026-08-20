#pragma once
#include "hittable.h"
#include "hittable_list.h"
#include "aabb.h"
#include <vector>
#include <algorithm>
#include <memory>

// Nodo di una Bounding Volume Hierarchy: divide ricorsivamente gli oggetti in due sotto-alberi per rendere l'intersezione raggio-scena sub-lineare.
class BVH_node : public Hittable
{
	std::shared_ptr<Hittable> left;   // sotto-albero sinistro (BVH_node o oggetto foglia)
	std::shared_ptr<Hittable> right;  // sotto-albero destro (BVH_node o oggetto foglia)
	AABB bbox;                        // box che racchiude tutto ciò che sta sotto questo nodo

public:
	// Costruisce ricorsivamente il nodo a partire dalla porzione [start, end) del vettore. objects è condiviso e modificato in-place da tutte le chiamate ricorsive (per riferimento).
	BVH_node(std::vector<std::shared_ptr<Hittable>>& objects, size_t start, size_t end)
	{
		// bbox = unione dei box di tutti gli oggetti della porzione (serve per longest_axis() e resta valido come bbox finale del nodo, senza doverlo ricalcolare dopo).
		for (size_t i = start; i < end; i++ )
		{
			bbox= AABB(bbox, objects[i]->bounding_box());
		}

		if (end - start == 1)
		{
			// Caso base: un solo oggetto, duplicato su entrambi i figli per evitare controlli su puntatori nulli dentro hit().
			left = objects[start];
			right = objects[start];
		}
		else if(end - start == 2)
		{
			// Caso base: due oggetti, uno per figlio, nessun ordinamento necessario.
			left = objects[start];
			right = objects[end-1];
		}
		else
		{
			// Caso generale: si sceglie l'asse più lungo del bbox (split più efficace rispetto a un asse casuale), si ordina la porzione lungo quell'asse in base al punto di inizio (min) del bounding box di ciascun oggetto, poi si divide a metà e si ricorre sulle due sotto-porzioni.
			auto axis = bbox.longest_axis();
			std::sort(objects.begin() + start, objects.begin() + end, [&](const std::shared_ptr<Hittable>& a, const std::shared_ptr<Hittable>& b)
				{
					return a->bounding_box().axis_interval(axis).min < b->bounding_box().axis_interval(axis).min;
				});

			auto mid = int(start+(end - start) / 2);
			left = std::make_shared<BVH_node>(objects, start, mid);
			right = std::make_shared<BVH_node>(objects, mid, end);
		}
	}

	// Costruttore comodo: prende una Hittable_list PER VALORE (copia locale, usa-e-getta) così può riordinarla senza toccare la lista originale del chiamante.
	BVH_node(Hittable_list list)
		: BVH_node(list.get_objects(), 0, list.get_objects().size())
	{ }

	bool hit(const Ray& ray, double tmin, double tmax, HitRecord& rec) const override
	{
		// Test economico sul box dell'intero nodo: se il raggio lo manca, manca sicuramente anche tutto ciò che c'è sotto (scarto immediato).
		if (!bbox.hit(ray, Interval{ tmin, tmax }))
			return false;

		// temp_rec1/temp_rec2: appunti "usa e getta" di left/right, MAI scritti direttamente su rec (vedi nota in fondo al file).
		HitRecord temp_rec1;
		HitRecord temp_rec2;
		auto hit_left = left->hit(ray, tmin, tmax, temp_rec1);
		// Controlla il destro: se sinistra ha già trovato un colpo, restringe il range a [tmin, temp_rec1.t] così right->hit() scarta da solo eventuali colpi più lontani.
		auto hit_right = hit_left ? right->hit(ray, tmin, temp_rec1.t, temp_rec2) : right->hit(ray, tmin, tmax, temp_rec2);

		// rec viene scritto solo ora, con dati certi: mai in base a un hit() che ha già fallito.
		if (hit_left && hit_right)
			rec = temp_rec1.t < temp_rec2.t ? temp_rec1 : temp_rec2;
		else if (hit_left && !hit_right)
			rec = temp_rec1;
		else if (!hit_left && hit_right)
			rec = temp_rec2;

		return (hit_left || hit_right);
	}

	AABB bounding_box() const override
	{
		return bbox;
	}
};

// ==========================================
// APPUNTI: COME SCENDE/RISALE LA RICORSIONE IN hit()
// ==========================================
// left->hit(...) e right->hit(...) non sono "controlli superficiali": sono chiamate
// complete a hit(), che grazie al polimorfismo eseguono BVH_node::hit() (se il figlio
// è un altro nodo interno) o Sphere::hit() (se il figlio è una foglia). L'esecuzione
// del nodo chiamante resta "in pausa" finché la chiamata non ritorna, esattamente come
// una normale funzione ricorsiva (fattoriale, Fibonacci, ecc.).
//
// La discesa prosegue quindi di nodo in nodo, ciascuno con il proprio bbox.hit() come
// guardia, finché non si raggiungono le foglie (oggetti concreti come Sphere), che
// eseguono il test diretto senza ricorsione ulteriore. Da lì si risale un livello alla
// volta: ogni nodo combina i risultati di left e right e restituisce il verdetto al
// proprio chiamante, fino a tornare alla radice e poi a ray_color().
// ==========================================

// ==========================================
// APPUNTI: PERCHÉ temp_rec1/temp_rec2 E NON "rec" DIRETTO
// ==========================================
// hit() riceve "rec" per riferimento dal chiamante (un altro BVH_node, o ray_color()).
// Se left->hit(...) o right->hit(...) scrivessero direttamente su quel "rec" esterno,
// un oggetto che usa rec come appunti di lavoro prima di decidere se restituire true o
// false (es. ConstantMedium, che deve calcolare dove il raggio entra/esce dal volume
// PRIMA di sapere se ci sarà scattering) potrebbe "sporcare" rec anche quando alla fine
// restituisce false. Se questo capita nel ramo destro DOPO che il ramo sinistro ha già
// trovato un hit valido, il valore vero (del sinistro) andrebbe perso silenziosamente,
// pur restituendo "true" alla fine (grazie a hit_left) — un bug difficile da notare,
// perché l'immagine sembra quasi giusta ma con materiali/colori sbagliati qua e là.
//
// Per questo left e right scrivono sempre su una loro copia locale (temp_rec1/2), "usa
// e getta": rec (quello vero, esterno) viene toccato una volta sola, alla fine, e solo
// con dati che sappiamo per certo provenire da un hit() che ha risposto true. Stesso
// principio già applicato in Hittable_list::hit() (con un solo temp_rec, dato che lì si
// itera su una lista invece che confrontare solo due rami) e in ConstantMedium::hit().
// ==========================================