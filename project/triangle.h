#pragma once

#include "quad.h"
#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "aabb.h"
#include "ray.h"
#include <memory>

class Triangle : public Quad
{

public:
	Triangle(const Point3& Q, Vec3 u, Vec3 v, std::shared_ptr<Material> material)
		:Quad(Q,u,v,material)
	{ }

	bool is_interior(double alfa, double beta, HitRecord& rec) const override
	{
		if (alfa < 0 || beta < 0 || alfa+beta>1) // fuori dai bordi del triangolo
			return false;
		else 
		{
			rec.u = alfa; 
			rec.v = beta;
			return true;
		}
	}

};