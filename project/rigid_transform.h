#pragma once

#include <memory>
#include "hittable.h"
#include "matrix.h"

class RigidTransform : public Hittable
{
	std::shared_ptr<Hittable> object; // oggetto originale, non modificato: viene sempre testato nel suo spazio locale
	Matrix4x4 M; // matrice diretta: spazio locale -> spazio mondo
	Matrix4x4 M_inversed; // matrice inversa: spazio mondo -> spazio locale
	AABB bbox; // bbox in spazio mondo, cachata una sola volta nel costruttoreù

public:
	RigidTransform(std::shared_ptr<Hittable> obj, RigidMatrix matrix)
		: object(std::move(obj)), M(matrix.forward), M_inversed(matrix.inverse)
	{
		auto object_bbox = object->bounding_box(); // bbox originale, ancora in spazio locale
		double x_vals[2] = { object_bbox.axis_interval(0).min, object_bbox.axis_interval(0).max };
		double y_vals[2] = { object_bbox.axis_interval(1).min, object_bbox.axis_interval(1).max };
		double z_vals[2] = { object_bbox.axis_interval(2).min, object_bbox.axis_interval(2).max };
		
		double x_min = infinity;
		double y_min = infinity;
		double z_min = infinity;
		double x_max = -infinity;
		double y_max = -infinity;
		double z_max = -infinity;

		// genera le 8 combinazioni min/max sui tre assi (gli 8 vertici del bbox locale)
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				for (int k = 0; k < 2; k++)
				{
					Point3 p = { x_vals[i], y_vals[j], z_vals[k] };
					auto p_transf = M.transform_point(p); // porta il vertice in spazio mondo

					// aggiorna il min/max accumulato con questo vertice trasformato
					x_min = p_transf.x < x_min ? p_transf.x : x_min;
					y_min = p_transf.y < y_min ? p_transf.y : y_min;
					z_min = p_transf.z < z_min ? p_transf.z : z_min;
					
					x_max = p_transf.x > x_max ? p_transf.x : x_max;
					y_max = p_transf.y > y_max ? p_transf.y : y_max;
					z_max = p_transf.z > z_max ? p_transf.z : z_max;
				}
			}
		}
		bbox = AABB({ x_min, x_max }, { y_min, y_max }, { z_min, z_max }); // bbox allineata agli assi, più piccola possibile, che racchiude l'oggetto trasformato
	}

	bool hit(const Ray& ray, double tmin, double tmax, HitRecord& rec) const override
	{
		// porta il raggio dallo spazio mondo allo spazio locale dell'oggetto (origine = punto, direzione = vettore: si trasformano diversamente)
		Ray ray_transf(M_inversed.transform_point(ray.origin()), M_inversed.transform_vector(ray.direction()), ray.time());
		
		if (!object->hit(ray_transf, tmin, tmax, rec)) // test di intersezione normale, nello spazio locale
			return false;
		
		rec.P = M.transform_point(rec.P); // riporta il punto di hit in spazio mondo
		rec.n = M.transform_vector(rec.n); // riporta la normale in spazio mondo (ok senza inversa-trasposta: nessuno scaling, solo rotazione/traslazione)
		
		return true;
	}

	AABB bounding_box() const override
	{
		return bbox;
	}
};

/*
* ==========================================
* COME FUNZIONA RigidTransform
* ==========================================
* RigidTransform avvolge un Hittable esistente (object) e gli applica una trasformazione
* rigida (rotazione + traslazione, niente scaling) senza dover modificare l'oggetto
* originale, che resta invariato e continua a "vivere" nel proprio spazio locale.
*
* L'idea chiave: invece di trasformare l'oggetto (impossibile, è già costruito),
* si trasforma il RAGGIO all'indietro, dallo spazio mondo allo spazio locale
* dell'oggetto, usando M_inversed. Lì l'oggetto è ancora "dov'era originariamente",
* quindi il suo hit() normale funziona senza modifiche. Se c'è un colpo, il punto
* e la normale calcolati (che sono in spazio locale) vengono riportati in spazio
* mondo con la matrice diretta M, prima di restituire il risultato.
*
* Punti e vettori si trasformano in modo diverso: un punto è affetto dalla
* traslazione (transform_point), un vettore/direzione no, perché rappresenta solo
* un orientamento (transform_vector). Per questo l'origine del raggio e la
* direzione usano due metodi diversi, così come rec.P (punto) e rec.n (vettore).
*
* RigidMatrix (definita in matrix.h) tiene sempre M e M_inversed accoppiate
* e coerenti tra loro, così chi usa RigidTransform non deve mai calcolare l'inversa
* a mano: la fabbrica statica (translation/rotation) o la composizione (operator*)
* la calcolano automaticamente nell'ordine e con i segni corretti.
* ==========================================
*/