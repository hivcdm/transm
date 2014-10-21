#pragma once

namespace transm {

struct HighResolutionTimerImpl;

class HighResolutionTimer
{
public:
	HighResolutionTimer();

	~HighResolutionTimer();

	double GetTime();

private:
	HighResolutionTimerImpl *impl_;
};

} // namespace transm
