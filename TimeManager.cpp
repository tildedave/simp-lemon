#include "TimeManager.h"
#include <iostream>
#include <iomanip>

TimeManager::TimeManager()
{
}

TimeManager::~TimeManager()
{
}

void 
TimeManager::start(const std::string& str) {
	gettimeofday(&(timevalMap_[currentName_][str].first), NULL);
}

void 
TimeManager::stop(const std::string& str) {
	gettimeofday(&(timevalMap_[currentName_][str].second), NULL);
}

void 
TimeManager::setName(const std::string& name) {
	currentName_ = name;
}

void 
TimeManager::unsetName() {
	currentName_ = "";
}
	
void 
TimeManager::outputTimes() {
	std::cout << std::setfill('-') << std::setw(60) << "" << std::endl;
	std::cout << std::setfill(' ') << std::right << std::setw(30) << "TIMES" << std::endl;
	std::cout << std::setfill('-') << std::setw(60) << "" << std::endl;
	std::cout << std::setfill(' ');
	for(std::map<std::string, std::map<std::string, std::pair<timeval, timeval> > >::iterator itTimeMap(timevalMap_.begin());
		itTimeMap != timevalMap_.end();
		++itTimeMap) {
		std::string currentCategory = itTimeMap->first;
		if (currentCategory.length() > 0)
			std::cout << currentCategory << ":" << std::endl;
		for(std::map< std::string, std::pair<timeval, timeval> >::iterator itCategory(itTimeMap->second.begin()); 
			itCategory != itTimeMap->second.end(); 
			++itCategory) {
			double diff = itCategory->second.second.tv_sec - itCategory->second.first.tv_sec;
			diff += (itCategory->second.second.tv_usec - itCategory->second.first.tv_usec) / (1000.0 * 1000.0);
			std::cout << "\t" << std::left << std::setw(25) << itCategory->first << ": " << diff << "s" << std::endl;
		}
	}
	std::cout << std::setfill('-') << std::setw(60) << "" << std::endl;
	std::cout << std::setfill(' ');
}
