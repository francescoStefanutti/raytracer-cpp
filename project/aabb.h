#pragma once
#include "interval.h"
#include "ray.h"

// Axis-Aligned Bounding Box: parallelepipedo allineato agli assi, definito come intersezione di tre intervalli (uno per asse x/y/z). Usato per test di intersezione rapidi ed economici prima di testare gli oggetti veri.
class AABB
{
	Interval x, y, z;
public:
	// Box vuoto di default: ogni Interval() parte con min=+inf, max=-inf, quindi non serve una costante AABB::empty separata.
	AABB() = default;
	AABB(Interval x, Interval y, Interval z)
		: x(x), y(y), z(z)
	{ }
	// Costruisce il box che racchiude strettamente altri due box (unione).
	AABB(const AABB& box1, const AABB& box2)
		: x(box1.x, box2.x), y(box1.y, box2.y), z(box1.z, box2.z)
	{ }

	// Metodo "slab": per ogni asse calcola l'intervallo di t in cui il raggio è dentro quella slab, poi interseca via via con ray_t. Se in un asse qualsiasi l'intervallo risultante collassa (min >= max), il raggio manca il box.
	bool hit(const Ray& r, Interval ray_t) const
	{
		for (int i = 0; i<3; i++ )
		{
			const Interval& axis = axis_interval(i);
			auto t0 = (axis.min - r.origin()[i]) / r.direction()[i];
			auto t1 = (axis.max - r.origin()[i]) / r.direction()[i];
			auto t_in = std::min(t0, t1);
			auto t_out = std::max(t0, t1);
			ray_t.min = t_in > ray_t.min ? t_in : ray_t.min; // troviamo il massimo di tutti i minimi
			ray_t.max = t_out < ray_t.max ? t_out : ray_t.max; // troviamo il minimo di tutti i massimi
			if (ray_t.min >= ray_t.max)
				return false;
		}
		return true;
	}

	// Restituisce l'Interval dell'asse n (0=x, 1=y, 2=z).
	const Interval& axis_interval(int n) const
	{
		if (n == 1) return y;
		if (n == 2) return z;
		return x;
	}

	// Indice (0/1/2) dell'asse più lungo del box: usato dal BVH per scegliere lungo quale asse dividere gli oggetti, invece di un asse casuale.
	int longest_axis() const
	{
		if (x.size() > y.size())
			if (x.size() > z.size())
				return 0; // x vince su y e su z
			else
				return 2; // x vince su y, ma z vince su x
		else
			if (y.size() > z.size())
				return 1; // y vince su x e su z
			else
				return 2; // y vince su x, ma z vince su y
	}
};