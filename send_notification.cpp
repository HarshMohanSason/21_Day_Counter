#include <windows.h>
#include <ctime> 
#include <string> 

extern "C" {
    #include "sqlite3.h"
}

using namespace std;

//get the currentDate from the system
string getCurrentDate() {
    time_t now = time(0);
    tm* localTime = localtime(&now);

    char buffer[11]; // YYYY-MM-DD is 10 chars + null terminator
    strftime(buffer, sizeof(buffer), "%m/%d/%Y", localTime);

    return string(buffer);
}

//Send a message box showing the notification on the windows. 
void sendNotification(string totalGuest)
{
 string guest_text = totalGuest == "1" ? "is 1 guest" : "are" + totalGuest + "guests";
string message = "There " + guest_text + " with 21 days and over, please take action.";
	MessageBox(NULL, message.c_str(), "Alert", MB_OK | MB_ICONEXCLAMATION);
}

//fetch the data from the db
void getDataFromDb(){
	
	sqlite3* db; 
	int rc; 
	
    // Open the database
    rc = sqlite3_open("guest_information.db", &db);
    if (rc != SQLITE_OK) {
        MessageBox(NULL, "Error opening the database", "Error", 
		MB_OK);
        return;
    }
	//query to select the number of rows where the check_out_date equals today's time and days need to be greater than 21
	string query = "SELECT COUNT(*) FROM guests WHERE check_out_date = ? AND days_stayed > 20";
	
	sqlite3_stmt* stmt;

	//Prepare the statement
    rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
        MessageBox(NULL, "Error opening the database", "Error", 
		MB_OK);
        return;
    }
	
	string currentDate = getCurrentDate(); //get the currentDate
	
	sqlite3_bind_text(stmt, 1, currentDate.c_str(), -1, SQLITE_STATIC);
	
	int totalGuests = 0; 
	//if rows are returned, get the count
	if (sqlite3_step(stmt) == SQLITE_ROW) {
        totalGuests = sqlite3_column_int(stmt, 0); // Get the count from the result
    }
	
	sqlite3_finalize(stmt);
        sqlite3_close(db);

    // If there are guests over 21 days, send the notification
    if (totalGuests > 0) {
        sendNotification(to_string(totalGuests));
    }

}
int main(){
	
	getDataFromDb();
	return 0; 
}