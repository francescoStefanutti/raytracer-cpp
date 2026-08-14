#pragma once

#include "ray.h"
#include "hittable.h"
#include "color.h"
#include "texture.h"
#include <memory>
#include <optional>
#include <algorithm>
#include <cmath>

struct ScatterResult
{
	Color attenuation;
	Ray scattered;
};

class Material
{
public:
	virtual std::optional<ScatterResult> scatter(const Ray& r_in, const HitRecord& rec) const = 0;

	virtual ~Material() = default;
};

class Lambertian : public Material
{
	std::shared_ptr<Texture> tex;

public:
	Lambertian(std::shared_ptr<Texture> texture)
		: tex(texture)
	{ }

	Lambertian(Color constant)
		: tex(std::make_shared<SolidColor>(constant))
	{ }

	std::optional<ScatterResult> scatter(const Ray& r_in, const HitRecord& rec) const override
	{
		//  Calcolo la direzione del rimbalzo casuale
		Vec3 direction = rec.n + random_unit_vector(); // rec.p - rec.p si annullano			
		direction = direction.near_zero() ? rec.n : direction; // per evitare che la direction sia nulla. se è nulla la sostituiamo con rec.n
		Ray scattered(rec.P, direction, r_in.time());
		
		auto albedo = tex->value(rec.u, rec.v, rec.P);

		return ScatterResult{ albedo, scattered };
	}

};

class Metal : public Material
{
	std::shared_ptr<Texture> tex;
	double fuzz;

public:
	Metal(std::shared_ptr<Texture> texture, double fuzz)
		: tex(texture), fuzz(std::clamp(fuzz,0.0,1.0))
	{ }

	Metal(Color constant, double fuzz)
		: tex(std::make_shared<SolidColor>(constant)), fuzz(std::clamp(fuzz,0.0,1.0))
	{ }

	std::optional<ScatterResult> scatter(const Ray& r_in, const HitRecord& rec) const override
	{
		//  Calcolo la direzione del rimbalzo preciso (normalizzato) e sommo una fuzziness casuale
		Vec3 direction = unit_vector(reflect(r_in.direction(), rec.n)) + fuzz*random_unit_vector();
		Ray scattered(rec.P, direction, r_in.time());

		// faccio controllo per evitare che raggio riflesso vado in direzione opposta alla normale (utile in futuro per fuzziness)
		if (dot(scattered.direction(), rec.n) > 0)
		{
			auto albedo = tex->value(rec.u, rec.v, rec.P);
			return ScatterResult{ albedo, scattered };
		}
		else
			return std::nullopt;
	}

};

class Dielectric : public Material
{
	double ir; //refraction_index
	std::shared_ptr<Texture> tex;


	//formula di Schlick: funzione che calcola quella probabilità di riflessione in base all'angolo
	static double reflectance(double cosine, double refraction_ratio)
	{
		auto r_zero = std::pow((1 - refraction_ratio) / (1 + refraction_ratio), 2); //riflettanza a incidenza zero
		return r_zero + (1 - r_zero) * std::pow(1 - cosine, 5);
	}

public:
	Dielectric(double ir, std::shared_ptr<Texture> texture)
		:ir(ir), tex(texture)
	{ }

	Dielectric(double ir)
		: ir(ir), tex(std::make_shared<SolidColor>(Color{1,1,1}))
	{ }

	Dielectric(double ir, Color constant)
		: ir(ir), tex(std::make_shared<SolidColor>(constant))
	{ }

	std::optional<ScatterResult> scatter(const Ray& r_in, const HitRecord& rec) const override
	{
		// in base a dove sta puntando la normale capisco da che materialen a che materiale sto passando e di conseguenza il giusto ir
		auto refraction_ratio = rec.front_face ? 1.0 / ir : ir;

		auto cos_theta = std::fmin(dot(unit_vector(-r_in.direction()), rec.n), 1.0); // abbiamo fatto il clamp per evitare diventi maggiore di uno
		auto sin_theta = sqrt(1 - cos_theta * cos_theta);
		
		//condizione per la riflessione interna totale
		bool cannot_refract = refraction_ratio*sin_theta > 1;

		Vec3 direction;
		if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
			direction = reflect(r_in.direction(), rec.n);
		else
			direction = refract(r_in.direction(), rec.n, refraction_ratio);

		Ray scattered(rec.P, direction, r_in.time());

		auto albedo = tex->value(rec.u, rec.v, rec.P);

		return ScatterResult{ albedo, scattered, };
	}

};

/*
* ==========================================
* APPUNTI DA SENIOR DEV: STATIC SU UN METODO DI CLASSE
* ==========================================
*
* "static" davanti a un metodo di classe ha un significato diverso rispetto
* a "static" su una funzione libera globale (dove limita la visibilità al
* singolo file/translation unit) o su una variabile locale/di classe.
*
* Su un METODO, "static" significa che quel metodo appartiene alla CLASSE
* nel suo complesso, non a una singola istanza:
*
*    - NON riceve un puntatore "this" implicito. Di conseguenza NON PUÒ
*      accedere a campi membro non-static (es. albedo, refraction_index)
*      né chiamare altri metodi non-static, perché non "sa" a quale
*      istanza specifica riferirsi.
*
*    - Si chiama senza bisogno di un oggetto creato, usando il nome della
*      classe: Dielectric::reflectance(cos_theta, ratio), invece di
*      oggetto.reflectance(...).
*
*    - È essenzialmente una funzione libera "raggruppata" dentro il
*      namespace della classe: utile quando la logica è concettualmente
*      legata a quella classe (usata solo lì), ma non ha bisogno di
*      leggere/scrivere lo stato di un'istanza specifica.
*
* Esempio pratico: reflectance(cosine, refraction_ratio) riceve tutto ciò
* che le serve come parametri, senza leggere this->refraction_index o
* altri campi -> è pura logica di calcolo, candidata perfetta per essere
* static.
* ==========================================
*/