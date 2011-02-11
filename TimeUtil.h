#ifndef TIMEUTIL_H_
#define TIMEUTIL_H_

#include <time.h>
#include <sys/time.h>
#include <iostream>

class TimeUtil
{
private:
	struct timeval startTime;
	struct timeval stopTime;
	
public:
	friend std::ostream& operator<<(std::ostream& os, const TimeUtil& tu);
	
	TimeUtil();
	TimeUtil(const TimeUtil& tu);
	
	virtual ~TimeUtil();
	
	
	
	void start();
	void stop();
};

#endif /*TIMEUTIL_H_*/
