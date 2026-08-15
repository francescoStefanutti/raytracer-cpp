#pragma once

#include <array>
#include <algorithm>
#include <numeric>
#include <random>
#include "vec3.h"

class Perlin
{
	std::array<int, 256> perm_x;
	std::array<int, 256> perm_y;
	std::array<int, 256> perm_z;

	std::array<double, 256> rand_double;
	std::array<Vec3, 256> rand_vec;


public:
	Perlin()
		: perm_x(generate_permutation()),  perm_y(generate_permutation()), perm_z(generate_permutation()), rand_double(generate_random_doubles()), rand_vec(generate_random_vec())
	{ }

	static std::array<int, 256> generate_permutation() 
	{
		std::array<int, 256> p;
		std::iota(p.begin(), p.end(), 0);

		std::random_device rd;
		std::mt19937 gen(rd());

		std::shuffle(p.begin(), p.end(), gen);

		return p;
	}

	static std::array<double, 256> generate_random_doubles() 
	{
		std::array<double, 256> p;

		std::random_device rd;
		std::mt19937 gen(rd());

		std::uniform_real_distribution<double> dist(0.0, 1.0);

		for (int i = 0; i < p.size(); i++)
		{
			p[i] = dist(gen);
		}

		return p;
	}

	static std::array<Vec3, 256> generate_random_vec() 
	{
		std::array<Vec3, 256> p;

		for (int i = 0; i < p.size(); i++)
		{
			p[i] = random_unit_vector();
		}

		return p;
	}

	double noise(const Point3& p) const
	{
		auto x = static_cast<int>(floor(p.x) );
		auto y = static_cast<int>( floor(p.y) );
		auto z = static_cast<int>( floor(p.z) );

		auto i = x & 255;
		auto j = y & 255;
		auto k = z & 255;

		auto perm_value_x = perm_x[i];
		auto perm_value_y = perm_y[j];
		auto perm_value_z = perm_z[k];

		auto index = perm_value_x ^ perm_value_y ^ perm_value_z;

		return rand_double[index];
	}

	double noise_smooth(const Point3& p) const
	{
		auto x = static_cast<int>(floor(p.x) );
		auto y = static_cast<int>( floor(p.y) );
		auto z = static_cast<int>( floor(p.z) );

		auto u = p.x - x;
		auto v = p.y - y;
		auto w = p.z - z;

		auto i = x & 255;
		auto j = y & 255;
		auto k = z & 255;

		double accum = 0.0;

		for (int di = 0; di < 2; di++)
		{
			for (int dj = 0; dj < 2; dj++)
			{
				for (int dk = 0; dk < 2; dk++)
				{
					auto u_weight = di ? u : 1 - u;
					auto v_weight = dj ? v : 1 - v;
					auto w_weight = dk ? w : 1 - w;

					auto perm_value_x = perm_x[(i+di)&255];
					auto perm_value_y = perm_y[(j+dj)&255];
					auto perm_value_z = perm_z[(k+dk)&255];

					auto index = perm_value_x ^ perm_value_y ^ perm_value_z;

					auto value = rand_double[index];

					accum += u_weight * v_weight * w_weight * value;

				}
			}
		}

		return accum;	 
	}

	double noise_smooth_gradient(const Point3& p) const
	{
		auto x = static_cast<int>(floor(p.x) );
		auto y = static_cast<int>( floor(p.y) );
		auto z = static_cast<int>( floor(p.z) );

		auto u = p.x - x;
		auto v = p.y - y;
		auto w = p.z - z;

		auto u_smooth = u*u*(3-2*u);
		auto v_smooth = v*v*(3-2*v);
		auto w_smooth = w*w*(3-2*w);

		auto i = x & 255;
		auto j = y & 255;
		auto k = z & 255;

		double accum = 0.0;

		for (int di = 0; di < 2; di++)
		{
			for (int dj = 0; dj < 2; dj++)
			{
				for (int dk = 0; dk < 2; dk++)
				{
					auto u_weight = di ? u_smooth : 1 - u_smooth;
					auto v_weight = dj ? v_smooth : 1 - v_smooth;
					auto w_weight = dk ? w_smooth : 1 - w_smooth;

					auto perm_value_x = perm_x[(i+di)&255];
					auto perm_value_y = perm_y[(j+dj)&255];
					auto perm_value_z = perm_z[(k+dk)&255];

					auto index = perm_value_x ^ perm_value_y ^ perm_value_z;

					auto value = rand_vec[index];

					accum += u_weight*v_weight*w_weight*dot(value, { u - di, v - dj, w - dk });
				}
			}
		}

		return accum;	 
	}


	double turbolence(const Point3& p, int depth) const
	{
		double sum_noise = 0;
		auto p_copy = p;
		auto weight = 1.0;

		for(int i = 0; i<depth; i++)
		{
			sum_noise += weight*noise_smooth_gradient(p_copy);
			weight *= 0.5;
			p_copy *= 2;
		}

		return std::fabs(sum_noise);
	}


};