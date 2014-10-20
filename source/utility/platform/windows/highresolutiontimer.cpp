#include <Windows.h>

#include "utility/HighResolutionTimer.hpp"

struct HighResolutionTimerImpl
{
	LONGLONG start;
	LONGLONG updateFrequency;
};

HighResolutionTimer::HighResolutionTimer() : impl_(new HighResolutionTimerImpl())
{
	LARGE_INTEGER ticksPerSecond;
	QueryPerformanceFrequency(&ticksPerSecond);
	impl_->updateFrequency = ticksPerSecond.QuadPart;

	LARGE_INTEGER tick;
	QueryPerformanceCounter(&tick);
	impl_->start = tick.QuadPart;
}

HighResolutionTimer::~HighResolutionTimer()
{
	delete impl_;
}

double HighResolutionTimer::GetTime()
{
	LARGE_INTEGER tick;
	QueryPerformanceCounter(&tick);
	return static_cast<double>(tick.QuadPart - impl_->start) / impl_->updateFrequency;
}