#pragma once
#include "hittable.h"
#include "material.h"
#include "essentials.h"
#include <memory>
#include <cmath>

class ConstantMedium : public Hittable
{
	std::shared_ptr<Hittable> object;   // il boundary geometrico (es. un box) - mai reso visibile direttamente
	AABB bbox;
	double inv_neg_density;             // -1/density, usato per il campionamento esponenziale
	std::shared_ptr<Isotropic> mat;     // phase function: il "vero" materiale visibile del fumo

public:
	// Overload con Texture: costruisce l'Isotropic internamente, incapsulando il dettaglio implementativo
	ConstantMedium(std::shared_ptr<Hittable> obj, double density, const std::shared_ptr<Texture>& tex)
		: object(std::move(obj)),  bbox(object->bounding_box()), inv_neg_density(-1/density), mat(std::make_shared<Isotropic>(tex))
	{ }
	
	// Overload con Color grezzo: stessa idea, comodità per un colore uniforme
	ConstantMedium(std::shared_ptr<Hittable> obj, double density, Color color)
		: object(std::move(obj)),  bbox(object->bounding_box()), inv_neg_density(-1/density), mat(std::make_shared<Isotropic>(color))
	{ }

	bool hit(const Ray& ray, double tmin, double tmax, HitRecord& rec) const override
	{
		// Primo attraversamento del boundary: dove il raggio ENTRA nel volume
		HitRecord temp_rec1;
		auto first_hit = object->hit(ray, -infinity, infinity, temp_rec1);
		if (!first_hit)
			return false;
		
		auto t_first = temp_rec1.t;

		// Secondo attraversamento: dove il raggio ESCE dal volume (cerco oltre il primo punto)
		HitRecord temp_rec2;
		auto second_hit = object->hit(ray, t_first + 0.0001, infinity, temp_rec2);
		if (!second_hit)
			return false;
	
		auto t_second = temp_rec2.t;

		// Clamping dell'intervallo [t_first, t_second] contro [tmin, tmax] richiesto dal chiamante
		t_first = t_first < tmin ? tmin : t_first;
		t_second = t_second > tmax ? tmax : t_second;;
		
		if (t_first >= t_second)
			return false; // nessuna sovrapposizione: il raggio non attraversa il volume in questo range

		t_first = t_first < 0 ? 0 : t_first; // gestisce il caso in cui l'origine del raggio sia già dentro il volume

		auto boundary_length = t_second - t_first; // quanto "spazio" ha il raggio per scatterare dentro il volume
		auto hit_distance = inv_neg_density * std::log(random_double(0.0, 1.0)); // distanza campionata da distribuzione esponenziale

		if (hit_distance > boundary_length)
			return false; // il raggio attraversa il volume senza scatterare

		// Scattering avvenuto: popolo rec con un punto/hit "sintetico" dentro il volume
		rec.t = t_first + hit_distance;
		rec.P = ray.at(rec.t);
		rec.n = { 1, 0, 0 };      // arbitraria: nel fumo la normale non ha significato fisico
		rec.front_face = true;    // arbitrario, stesso motivo
		rec.mat = mat;            // qui entra in gioco la vera phase function isotropica
		
		return true;
	}

	AABB bounding_box() const override
	{
		return bbox;
	}
};