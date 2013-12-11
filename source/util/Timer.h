#pragma once

#include <map>
#include <vector>
#include <string>
#include <stdint.h>

#ifdef __Linux__
#include <sys/time.h>
#else
#include <Windows.h>
#endif

class SimulationTimer
{
public:
	void Start()
	{
#ifdef __LINUX__ || __APPLE__
		gettimeofday(&start, NULL);
#else
		LARGE_INTEGER ticksPerSecond;
		QueryPerformanceFrequency(&ticksPerSecond);
		updateFrequency = static_cast<double>(ticksPerSecond.QuadPart);

		LARGE_INTEGER tick;
		QueryPerformanceCounter(&tick);
		start = tick.QuadPart;
#endif
	}

	double GetTime()
	{
#ifdef __LINUX__ || __APPLE__
		gettimeofday(&now, NULL);
		return (now.tv_sec - start.tv_sec) * 1000000 + (now.tv_usec - start.tv_usec);
#else
		QueryPerformanceCounter(&tick);
		return (tick.QuadPart - start) / updateFrequency;
#endif
	}

private:
#ifdef __LINUX__ || __APPLE__
	struct timeval start;
	struct timeval now;
#elif defined(_WIN32)
	int64_t start;
	LARGE_INTEGER tick;
	double updateFrequency;
#else
#error "Unknown platform"
#endif
};