#pragma once

#include "ray.h"
#include "aabb.h"
#include <memory>

// forward declaration invece che dare l'include perchè material ha l'include di hittable a sua volta e avremmo una ipendenza circolare
class Material;

struct HitRecord
{
	Point3 P;
	double t;
	Vec3 n;
	bool front_face;
	std::shared_ptr<Material> mat;

	// calcolo di direzione della normale
	void set_face_normal(const Ray& r, const Vec3& outward_normal)
	{
		if (dot(r.direction(), outward_normal) > 0)
		{
			front_face = false;
			n = -outward_normal;
		}
		else
		{
			front_face = true;
			n = outward_normal;
		}

	}

};

class Hittable
{
public:
	virtual bool hit(const Ray& ray, double tmin, double tmax, HitRecord& rec) const = 0 ;

	virtual AABB bounding_box() const = 0;

	virtual ~Hittable() = default;
};

