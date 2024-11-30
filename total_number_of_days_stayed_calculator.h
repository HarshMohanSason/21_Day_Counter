#ifndef TOTALNUMBEROFDAYSSTAYEDCALCULATOR_H
#define TOTALNUMBEROFDAYSSTAYEDCALCULATOR_H

#include <chrono>
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

class TotalNumberOfDaysStayedCalculator{
	
private:
    // Function to parse the date in MM/DD/YYYY format into std::tm structure
    static bool ParseDate(const std::string& dateStr, std::tm& tm) {
        std::stringstream ss(dateStr);
        char sep; // for separator between MM, DD, YYYY
        ss >> tm.tm_mon >> sep >> tm.tm_mday >> sep >> tm.tm_year;

        // Validate the date format and adjust tm values
        if (ss.fail()) {
            return false;
        }

        tm.tm_year -= 1900; // tm_year is years since 1900
        tm.tm_mon -= 1;     // tm_mon is 0-based (0 = January)
        return true;
    }
	

public:
    // Function to calculate the number of days between two dates
    static int CalculateDaysDifference(const std::string& startDate, const std::string& endDate) {
        std::tm startTm = {}, endTm = {};

        // Parse the dates
        ParseDate(startDate, startTm);
		ParseDate(endDate, endTm);

        // Convert tm to time_t
        time_t startTime = std::mktime(&startTm);
        time_t endTime = std::mktime(&endTm);

        
        // Calculate the difference in seconds
        double seconds = difftime(endTime, startTime);

        // Convert seconds to days (24 hours * 60 minutes * 60 seconds = 86400 seconds in a day)
        return seconds / 86400; // 86400 = 24 * 60 * 60
    }
};



#endif