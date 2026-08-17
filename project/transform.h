#pragma once

#include <memory>
#include "hittable.h"
#include "matrix.h"

class Transform : public Hittable
{
	std::shared_ptr<Hittable> object;
	Matrix4x4 M;
	Matrix4x4 M_inversed;
	AABB bbox;

public:
	Transform(std::shared_ptr<Hittable> object, RigidTransform matrix)
		: object(object), M(matrix.forward), M_inversed(matrix.inverse)
	{
		auto object_bbox = object->bounding_box();
		double x_vals[2] = { object_bbox.axis_interval(0).min, object_bbox.axis_interval(0).max };
		double y_vals[2] = { object_bbox.axis_interval(1).min, object_bbox.axis_interval(1).max };
		double z_vals[2] = { object_bbox.axis_interval(2).min, object_bbox.axis_interval(2).max };

		double x_min = infinity;
		double y_min = infinity;
		double z_min = infinity;
		double x_max = -infinity;
		double y_max = -infinity;
		double z_max = -infinity;
	
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				for (int k = 0; k < 2; k++)
				{
					Point3 p = { x_vals[i], y_vals[j], z_vals[k] };

					auto p_transf = M.transform_point(p);
					
					x_min = p_transf.x < x_min ? p_transf.x : x_min;
					y_min = p_transf.y < y_min ? p_transf.y : y_min;
					z_min = p_transf.z < z_min ? p_transf.z : z_min;

					x_max = p_transf.x > x_max ? p_transf.x : x_max;
					y_max = p_transf.y > y_max ? p_transf.y : y_max;
					z_max = p_transf.z > z_max ? p_transf.z : z_max;
				}
			}
		}

		bbox = AABB({ x_min, x_max }, { y_min, y_max }, { z_min, z_max });
	}

	bool hit(const Ray& ray, double tmin, double tmax, HitRecord& rec) const override
	{
		Ray ray_transf(M_inversed.transform_point(ray.origin()), M_inversed.transform_vector(ray.direction()), ray.time());

		if (!object->hit(ray_transf, tmin, tmax, rec))
			return false;

		rec.P = M.transform_point(rec.P);
		rec.n = M.transform_vector(rec.n);

		return true;
	}

	AABB bounding_box() const override
	{
		return bbox;
	}

};