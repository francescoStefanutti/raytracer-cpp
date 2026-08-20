#pragma once

#include <cmath>
#include <memory>
#include <cstdlib>
#include <limits>
#include <random>

constexpr double infinity = std::numeric_limits<double>::infinity();

constexpr double pi = 3.1415926535897932385;

inline double deg_to_rad(double deg)
{
	return deg * (pi / 180.0);
}

inline double random_double()
{
	thread_local std::random_device rd;
	thread_local std::mt19937 generator(rd());
	thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
	return distribution(generator); 
}

inline double random_double(double min, double max)
{
	return min + random_double()* (max - min);
}

inline int random_int(int min, int max)
{
	return int(random_double(min, max+1));
}

