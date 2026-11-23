#include <time.h>

#include "utility/highresolutiontimer.hpp"

namespace transm {

/* CLOCK_MONOTONIC only moves forward. gettimeofday, used here before, is the wall
 * clock, which the system may set back (WSL resyncs it), giving negative intervals. */
struct HighResolutionTimerImpl
{
	struct timespec start;
	struct timespec now;
};


HighResolutionTimer::HighResolutionTimer() : impl_(new HighResolutionTimerImpl())
{
	clock_gettime(CLOCK_MONOTONIC, &impl_->start);
}

HighResolutionTimer::~HighResolutionTimer()
{
	delete impl_;
}

double HighResolutionTimer::GetTime()
{
	clock_gettime(CLOCK_MONOTONIC, &impl_->now);
	return (double)(impl_->now.tv_sec - impl_->start.tv_sec) + (impl_->now.tv_nsec - impl_->start.tv_nsec) / 1e9;
}

} // namespace transm
