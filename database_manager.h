
#ifndef DATABASEMANGER_H
#define DATABASEMANGER_H
#include <windows.h>
#include "sqlite3.h"

class DatabaseManager {
public:
    // Get the database instance (singleton pattern)
    static sqlite3* getDatabase() {
        if (!db) {
            // Open the database if it's not already open
            if (!openDatabase()) {
                return nullptr; // Return nullptr if the database can't be opened
            }
        }
        return db; // Return the open database connection
    }

    // Close the database when done
    static void closeDatabase() {
        if (db) {
            sqlite3_close(db);
            db = nullptr;
        }
    }

private:
    static sqlite3* db;  // Database connection object

    // Open the database connection
    static bool openDatabase() {
        int rc = sqlite3_open("guest_information.db", &db); // Open the database file
        if (rc) {
            MessageBox(NULL, sqlite3_errmsg(db), "Error opening database", MB_OK | MB_ICONERROR);
            return false;
        }
        return true;
    }

    // Prevent creating an instance of DatabaseManager
    DatabaseManager() {}
    DatabaseManager(const DatabaseManager&) = delete;
    void operator=(const DatabaseManager&) = delete;
};


sqlite3* DatabaseManager::db = nullptr; 

#endif 