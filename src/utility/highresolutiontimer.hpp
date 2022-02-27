#ifndef HIGHRESOLUTIONTIMER_HPP
#define HIGHRESOLUTIONTIMER_HPP

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


#endif /* HIGHRESOLUTIONTIMER_HPP */