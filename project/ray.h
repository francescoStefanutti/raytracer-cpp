#pragma once

#include "Vec3.h"

class Ray
{
	Point3 orig;
	Vec3 dir;

public:
	constexpr Ray() = default;

	Ray(const Point3& origin, const Vec3& direction)
		: orig(origin), dir(unit_vector(direction)) // lo normalizzo perchè mi servirà in futuro che tutti ir aggi sia normalizzati (spcelmente per materiali dielectric)
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
};


//NOTA: Restituiamo Point3 e Vec3 per VALORE e non per reference (const Point3&).
//Visto che un Vec3 pesa solo 24 byte, passarlo per valore permette al compilatore 
//di caricarlo direttamente nei registri ultra-veloci della CPU, evitando il 
//"memory fetch" (lettura in memoria) richiesto dai puntatori/reference. 