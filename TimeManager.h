#ifndef TIMEMANAGER_H_
#define TIMEMANAGER_H_

#include <string>
#include <map>
#include <time.h>
#include <sys/time.h>
#include <utility>

class TimeManager
{
protected:
	std::map<std::string, std::map<std::string, std::pair<timeval, timeval> > > timevalMap_;
	std::string currentName_;
	
public:
	TimeManager();
	virtual ~TimeManager();
	
	void start(const std::string& str);
	void stop(const std::string& str);

	void setName(const std::string& name);
	void unsetName();
	
	void outputTimes();
};

#endif /*TIMEMANAGER_H_*/
