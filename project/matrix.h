
#pragma once

#include <array>
#include "vec3.h"

class Matrix4x4
{
	std::array<std::array<double, 4>, 4> m;

public:
	Matrix4x4()
		: m{{ {1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1} }}
	{ }

	Matrix4x4(const std::array<std::array<double, 4>, 4>& values)
		: m(values)
	{ }


	Point3 transform_point(const Point3& p) const
	{ 
		auto x = m[0][0]*p.x + m[0][1]*p.y + m[0][2]*p.z + m[0][3];
		auto y = m[1][0]*p.x + m[1][1]*p.y + m[1][2]*p.z + m[1][3];
		auto z = m[2][0]*p.x + m[2][1]*p.y + m[2][2]*p.z + m[2][3];
		return Point3{x, y, z};
	}


	Vec3 transform_vector(const Vec3& v) const
	{ 
		auto x = m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z;
		auto y = m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z;
		auto z = m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z;
		return Vec3{x, y, z};
	}

	static Matrix4x4 translation(const Vec3& offset)
	{
		std::array<std::array<double, 4>, 4> values = { {
			{1,0,0,offset.x},
			{0,1,0,offset.y},
			{0,0,1,offset.z},
			{0,0,0,1}
			} 
		};

		return Matrix4x4(values);
	}

	static Matrix4x4 rotation_z(double teta)
	{
		std::array<std::array<double, 4>, 4> values = { {
			{cos(deg_to_rad(teta)),sin(deg_to_rad(teta)),0,0},
			{-sin(deg_to_rad(teta)),cos(deg_to_rad(teta)),0,0},
			{0,0,1,0},
			{0,0,0,1}
			} 
		};

		return Matrix4x4(values);
	}

	static Matrix4x4 rotation_y(double teta)
	{
		std::array<std::array<double, 4>, 4> values = { {
			{cos(deg_to_rad(teta)),0,sin(deg_to_rad(teta)),0},
			{0,1,0,0},
			{-sin(deg_to_rad(teta)),0,cos(deg_to_rad(teta)),0},
			{0,0,0,1}
			} 
		};

		return Matrix4x4(values);
	}

	static Matrix4x4 rotation_x(double teta)
	{
		std::array<std::array<double, 4>, 4> values = { {
			{1,0,0,0},
			{0,cos(deg_to_rad(teta)),sin(deg_to_rad(teta)),0},
			{0,-sin(deg_to_rad(teta)),cos(deg_to_rad(teta)),0},
			{0,0,0,1}
			} 
		};

		return Matrix4x4(values);
	}

	static Matrix4x4 rotation(double ax, double ay, double az)
	{
		return rotation_x(ax) * rotation_y(ay) * rotation_z(az);
	}

	Matrix4x4 operator*(const Matrix4x4& other) const
	{
		std::array<std::array<double,4>,4> r = {{ {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} }};
		for(int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				for(int k = 0; k<4; k++ )
				{
					r[i][j] += m[i][k] * other.m[k][j];
				}
			}
		}

		return r;
	}
};


struct RigidTransform
{
	Matrix4x4 forward;
	Matrix4x4 inverse;

	static RigidTransform translation(const Vec3& offset) 
	{
		return { Matrix4x4::translation(offset), Matrix4x4::translation(-offset) };
	}

	static RigidTransform rotation(double ax, double ay, double az) 
	{
		return { Matrix4x4::rotation(ax, ay, az), Matrix4x4::rotation_z(-az)*Matrix4x4::rotation_y(-ay)*Matrix4x4::rotation_x(-ax) };
	}

	RigidTransform operator*(const RigidTransform& other) const
	{
		return{ forward * other.forward, other.inverse*inverse };
	}
};