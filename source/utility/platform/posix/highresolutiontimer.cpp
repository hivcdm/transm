#include <sys/time.h>

#include "utility/highresolutiontimer.hpp"

struct HighResolutionTimerImpl
{
	struct timeval start;
	struct timeval now;
};


HighResolutionTimer::HighResolutionTimer() : impl_(new HighResolutionTimerImpl())
{
	gettimeofday(&impl_->start, nullptr);
}

HighResolutionTimer::~HighResolutionTimer()
{
	delete impl_;
}

double HighResolutionTimer::GetTime()
{
	gettimeofday(&impl_->now, nullptr);
	return ((impl_->now.tv_sec - impl_->start.tv_sec) * 1000000 + (impl_->now.tv_usec - impl_->start.tv_usec)) / 1000000.0;
}
