#pragma once

#include "hittable.h"
#include "material.h"
#include <cmath>
#include <memory>
#include <utility>

class Sphere : public Hittable
{
	double radius;
	Point3 center;
	std::shared_ptr<Material> mat;

public:
	Sphere(double r, Point3 c, std::shared_ptr<Material> m) // leggi sotto
		: radius(r), center(c), mat(std::move(m))
	{ }

	bool hit(const Ray& ray, double tmin, double tmax, HitRecord& rec) const override
	{
		const auto oc = ray.origin() - center;
		const auto a = dot(ray.direction(), ray.direction());
		const auto b = 2.0 * dot(oc, ray.direction());
		const auto c = dot(oc, oc) - radius * radius;

		const auto delta = b * b - 4 * a * c;

		if (delta < 0)
			return false;

		auto t =  (-b - sqrt(delta)) / (2.0 * a);

		if (t > tmax || t < tmin)
		{
			t = (-b + sqrt(delta)) / (2.0 * a);
			if (t > tmax || t < tmin)
				return false;
		}

		auto const P = ray.at(t);
		auto const n = unit_vector(P - center);
		/*Ottimizzazione della Normale (Bonus): Per calcolare la normale hai usato unit_vector(P - center). Questa funzione calcola una radice quadrata per trovare la lunghezza del vettore. Ma noi sappiamo già che il punto $P$ si trova esattamente sulla superficie della sfera, quindi la distanza dal centro è esattamente il raggio! Puoi calcolare la normale in modo molto più veloce risparmiando calcoli alla CPU:
		auto const n = (P - center) / radius;*/
		
		rec.set_face_normal(ray, n);
		rec.t = t;
		rec.P = P;
		rec.mat = mat;

		return true;
	}

	~Sphere() override = default;
};

/*
* ==========================================
* APPUNTI DA SENIOR DEV: PASS BY VALUE + STD::MOVE (Sink Parameter)
* ==========================================
*
* Quando un costruttore riceve un parametro che verrà "preso in possesso" (memorizzato in una variabile della classe)
* e memorizzato internamente (come questo shared_ptr<Material>), la tecnica
* più efficiente in C++ moderno è: PASSARE PER VALORE + STD::MOVE.
*
* Perché non "const&"?
*    - const& evita la copia in INGRESSO, ma quando poi assegni il parametro
*      al campo membro (es. mat(m)), stai comunque COPIANDO, incrementando
*      in modo atomico il reference count dello shared_ptr (operazione costosa,
*      pensata per la sicurezza multi-thread anche se qui non serve).
*
* Perché "per valore + std::move" è meglio?
*    1. Se il chiamante passa un temporaneo (es. std::make_shared<...>(...)
*       scritto direttamente come argomento), il parametro viene costruito
*       per MOVE, non per copia: zero overhead atomico.
*    2. Se il chiamante passa una variabile esistente che vuole conservare,
*       avviene comunque UNA copia (inevitabile, il chiamante vuole la sua),
*       ma poi std::move nell'initializer list evita la SECONDA copia
*       che si avrebbe con "mat(m)" senza move.
*
* In sintesi: "per valore + move" è sempre uguale o più efficiente di "const&"
* per parametri che finiscono "posseduti" da un campo membro (sink parameter).
* Richiede #include <utility> per std::move.
* ==========================================
*/