#include "TimeUtil.h"
#include <iostream>

TimeUtil::TimeUtil()
{
}

TimeUtil::~TimeUtil()
{
}

TimeUtil::TimeUtil(const TimeUtil& tu) 
{
	this->startTime = tu.startTime;
	this->stopTime = tu.stopTime;
}
	
void 
TimeUtil::start() {
	gettimeofday(&this->startTime, NULL);
}

void 
TimeUtil::stop() {
	gettimeofday(&this->stopTime, NULL);	
}

std::ostream& operator<<(std::ostream& os, const TimeUtil& tu) {
	double diff = tu.stopTime.tv_sec - tu.startTime.tv_sec;
	diff += (tu.stopTime.tv_usec - tu.startTime.tv_usec) / (1000.0 * 1000.0);
	return os << diff << "s";
}
