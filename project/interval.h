#pragma once

#include <algorithm>
#include "essentials.h"

class Interval
{
public:

	double min = infinity;
	double max = -infinity;

	Interval() = default;

	Interval(double min, double max)
		: min(min), max(max)
	{ }

	Interval(const Interval& interval1,const Interval& interval2)
		: min(std::min(interval1.min, interval2.min)), max(std::max(interval1.max, interval2.max))
	{ }

	bool contains(double x) const
	{
		return x<=max && x>=min;
	}

	double size() const
	{
		return max - min;
	}

	double clamp(double x) const
	{
		return std::clamp(x, min, max);
	}
};