#pragma once

#include <memory>
#include "hittable.h"
#include "matrix.h"

class GeneralTransform : public Hittable
{
	std::shared_ptr<Hittable> object; // oggetto originale, non modificato: viene sempre testato nel suo spazio locale
	Matrix4x4 M; // matrice diretta: spazio locale -> spazio mondo (punti)
	Matrix4x4 M_inversed; // matrice inversa: spazio mondo -> spazio locale (per portare il raggio all'indietro)
	Matrix4x4 M_normal; // matrice dedicata alle normali: inversa-trasposta della parte lineare di M
	AABB bbox; // bbox in spazio mondo, cachata una sola volta nel costruttoreù

public:
	GeneralTransform(std::shared_ptr<Hittable> obj, GeneralMatrix matrix)
		: object(std::move(obj)), M(matrix.forward), M_inversed(matrix.inverse), M_normal(matrix.normal)
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
					auto p_transf = M.transform_point(p); // porta il vertice in spazio mondo (M gestisce già scale+rotazione+traslazione)

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
		rec.n = unit_vector(M_normal.transform_vector(rec.n)); // usa M_normal (non M!): con lo scale, la normale si trasforma diversamente dal punto
		rec.t = dot(rec.P - ray.origin(), ray.direction()); // ricalcola t nello spazio giusto

		return true;
	}

	AABB bounding_box() const override
	{
		return bbox;
	}
};

/*
* ==========================================
* COME FUNZIONA GeneralTransform
* ==========================================
* GeneralTransform è l'evoluzione di RigidTransform: stessa identica struttura e
* stesso principio (si trasforma il RAGGIO all'indietro nello spazio locale
* dell'oggetto con M_inversed, si testa l'hit lì, poi si riportano punto e normale
* in spazio mondo), ma qui la trasformazione può includere anche uno SCALE, non
* solo rotazione+traslazione.
*
* Perché serve una terza matrice (M_normal) solo per le normali:
* sotto uno scale non uniforme (es. sx != sy != sz), una normale trasformata con
* la stessa matrice usata per i punti (M) smette di essere perpendicolare alla
* superficie deformata. La regola geometrica corretta è trasformare le normali con
* l'inversa-trasposta della parte lineare (3x3) di M, non con M stessa.
*
* GeneralMatrix (in matrix.h) precalcola questa inversa-trasposta una volta sola,
* per composizione analitica invece che con un'inversione di matrice generica:
* - traslazione pura: non tocca le normali, M_normal = identità.
* - rotazione pura: l'inversa di una rotazione è la sua trasposta, quindi
*   inversa-trasposta = la rotazione stessa, M_normal coincide con M (per questo
*   in RigidTransform, senza scale, si poteva sempre riusare M anche per rec.n).
* - scale puro: una matrice diagonale è simmetrica (trasposta di se stessa), quindi
*   inversa-trasposta si riduce allo scale reciproco (1/sx, 1/sy, 1/sz), identico
*   a M_inversed, senza bisogno di trasporre nulla esplicitamente.
* Comporre trasformazioni (operator* di GeneralMatrix) applica questa stessa logica
* pezzo per pezzo, così M_normal resta sempre coerente con M anche per catene di
* trasformazioni complesse (T*R*S), senza mai invertire una matrice 4x4 generica.
* ==========================================
*/