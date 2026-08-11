#pragma once

#include <cmath>
#include <memory>
#include <cstdlib>
#include <limits>

constexpr double infinity = std::numeric_limits<double>::infinity();

constexpr double pi = 3.1415926535897932385;

inline double deg_to_rad(double deg)
{
	return deg * (pi / 180.0);
}

inline double random_double()
{
	return rand() / (RAND_MAX + 1.0); // e in un'operazione matematica è presente un numero con la virgola mobile esplicito, il compilatore promuove automaticamente tutta l'operazione a double.
}

inline double random_double(double min, double max)
{
	return min + random_double()* (max - min);
}

