#pragma once

#include "Vec3.h"

class Ray
{
	// metto dei valori di default così posso mettere anche il costruttore vuoto di default che mi potrebbe servire se devo inizializzare un ray vuoto
	Point3 orig{};
	Vec3 dir{};
	double tm = 0;

public:
	constexpr Ray() = default;

	Ray(const Point3& origin, const Vec3& direction, double time = 0) // se non passo time il defaul è zero
		: orig(origin), dir(unit_vector(direction)), tm(time) // lo normalizzo perchè mi servirà in futuro che tutti ir aggi sia normalizzati (spcelmente per materiali dielectric)
	{ }

	constexpr Point3 origin() const
	{
		return orig;
	}

	constexpr Vec3 direction() const
	{
		return dir;
	}

	constexpr Point3 at(double t) const
	{
		return orig + t * dir;
	}

	constexpr double time() const
	{
		return tm;
	}
};


//NOTA: Restituiamo Point3 e Vec3 per VALORE e non per reference (const Point3&).
//Visto che un Vec3 pesa solo 24 byte, passarlo per valore permette al compilatore 
//di caricarlo direttamente nei registri ultra-veloci della CPU, evitando il 
//"memory fetch" (lettura in memoria) richiesto dai puntatori/reference. 