
struct timeval start;
struct timeval now;

void a()
{
	gettimeofday(&now, NULL);
	//return ((now.tv_sec - start.tv_sec) * 1000000 + (now.tv_usec - start.tv_usec)) / 1000000.0;

	gettimeofday(&start, NULL);
}