#ifndef TEXTBOXVALIDTOR_H
#define TEXTBOXVALIDTOR_H

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

class TextBoxValidator {
public:
    static std::string errorMessage; // One error at a time :)

    // Validate if the string is not empty
    static bool IsNotEmpty(const std::string& str) {
		errorMessage = "No input provided. Please provide an input first";
        return !str.empty();
    }
	
	static bool IsValidName(const std:: string& str){
		for (char c: str){
			if(std::isdigit(c)){
				errorMessage = "Guest name cannot contain numbers"; 
				return false;
			}
		}
		if(!std::isupper(str.at(0))){
			errorMessage = "First letter of the name should be all CAPS"; 
			return false;
		}
		if(str.length() > 25)
		{	errorMessage = "Guest name is too long"; 
			return false; 
		}
		return true;
	}

    // Validate if the string is a valid day
    static bool IsValidDay(const std::string& str) {
        for (char c : str) {
            if (!std::isdigit(c)) {  
                errorMessage = "Day can only contain digits."; 
                return false;
            }
        }
        int day = std::stoi(str);
        if (day < 1 || day > 31) {
            errorMessage = "Enter a valid day between 1 and 31.";
            return false; 
        }
        return true;
    }

    // Validate if the string is a valid month
    static bool IsValidMonth(const std::string& str) {
        if (str.length() != 2) {
            errorMessage = "Month must be exactly two digits.";
            return false;
        }
        
        for (char c : str) {
            if (!std::isdigit(c)) {  
                errorMessage = "Month must contain only digits.";
                return false;
            }
        }
        
        int month = std::stoi(str);
        if (month < 1 || month > 12) {
            errorMessage = "Month must be between 01 and 12.";
            return false;
        }
        return true;
    }

    // Validate if the string is a valid year (4 digits)
    static bool IsValidYear(const std::string& str) {
        if (str.length() != 4) {
            errorMessage = "Year must be exactly 4 digits.";
            return false;
        }

        for (char c : str) {
            if (!std::isdigit(c)) {
                errorMessage = "Year must contain only digits.";
                return false;
            }
        }

        int year = std::stoi(str);
        if (year < 1000 || year > 9999) {
            errorMessage = "Year must be between 1000 and 9999.";
            return false;
        }

        return true;
    }

    // Validate the date in the format "MM/DD/YYYY"
    static bool IsValidDateFormat(const std::string& str) {
        // Check for the general format "MM/DD/YYYY"
        if (str.length() != 10) {
            errorMessage = "Date must be in the format MM/DD/YYYY.";
            return false;
        }

        // Split the string into month, day, and year
        std::string monthStr = str.substr(0, 2);    // First two characters (MM)
        std::string dayStr = str.substr(3, 2);      // Next two characters (DD)
        std::string yearStr = str.substr(6, 4);     // Last four characters (YYYY)

        // Validate the components
        if (!IsValidMonth(monthStr)) {
            return false;
        }
        if (!IsValidDay(dayStr)) {
            return false;
        }
        if (!IsValidYear(yearStr)) {
            return false;
        }

        return true;
    }
	
	static bool IsValidFolioNumber(const std::string& str){
		
		if(str.length() > 11)
		{   errorMessage = "Folio too long"; 
			return false; 
		}
		
		//Folio number should start with the property code which is 9482 here. Eg 9482AFH003 is valid
		if(str.substr(0, 4) != "9482")
		{
			errorMessage = "Folio number entered is incorrect, Must start with 9482"; 
			return false;
		}
			return true;
	}
	
};


#endif