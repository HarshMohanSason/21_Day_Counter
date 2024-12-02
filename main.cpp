#include <windows.h>
#include "resource.h"
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <vector>
#include "guest.h"
#include <commctrl.h>
#include <algorithm>
#include <cctype>
#include "text_box_validator.h"
#include "total_number_of_days_stayed_calculator.h"
#include "database_manager.h"

extern "C" {
    #include "sqlite3.h"
}

using namespace std;

//---------------Some global variables--------------

//These will be used entirely for fetching values from dialog boxes or updating the values in the db. 

char g_GuestName[100];
char g_FolioNumber[100];
char g_CheckInDate[100];
char g_CheckOutDate[100];
int total_days_stayed;

//Program name
const char g_szClassName[] = "21 Day Counter";

// global current instance
HINSTANCE hInst;

// global instance for the listView; 
static HWND hWndListView;

// Default initialization for the errorMessage for TextBoxValidator only
string TextBoxValidator::errorMessage = "";

// Using this only to differentiate between what dialog to open after processing search dialog
enum DialogContext {
    SEARCH_GUEST,
    DELETE_GUEST,
	EDIT_GUEST
};
DialogContext currentContext; 


//call back function for adding a new guest 
BOOL CALLBACK AddGuestDlgProc(HWND, UINT, WPARAM, LPARAM);

//call back function to delete guest information
BOOL CALLBACK SearchGuestDlgProc(HWND, UINT, WPARAM, LPARAM);

//call back function to edit guest information
BOOL CALLBACK EditGuestDlgProc(HWND, UINT, WPARAM, LPARAM); 

//----------ADDS THE DATA FROM GUEST INSTANCE TO THE SQL TABLE-----------

//Just a helper function to convert a string to lowercase
string ToLower(const string& str)
{
	string lowerStr = str; 
	transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return tolower(c); });
    return lowerStr;
}

void AddDataInDb(Guest& guest) {
    sqlite3* db = DatabaseManager::getDatabase();
    if (!db) {
        MessageBox(NULL, "Database not open.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    int rc;
    // Create table (if not exists)
    const char * sql_create_table = "CREATE TABLE IF NOT EXISTS guests ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "guest_name TEXT,"
    "folio_number TEXT,"
    "check_in_date TEXT,"
    "check_out_date TEXT,"
    "days_stayed INTEGER)";
	
    rc = sqlite3_exec(db, sql_create_table, nullptr, nullptr, nullptr);
    if (rc) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error creating table", MB_OK | MB_ICONERROR);
        return;
    }

    // Prepare the insert statement
    const char * sql_insert = "INSERT INTO guests (guest_name, folio_number, check_in_date, check_out_date, days_stayed) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt * stmt;
    rc = sqlite3_prepare_v2(db, sql_insert, -1, & stmt, nullptr);

    if (rc == SQLITE_OK) {
        // Bind data to statement
        sqlite3_bind_text(stmt, 1, guest.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, guest.folioNumber.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, guest.checkInDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, guest.checkOutDate.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, guest.totalDaysStayed);

        // Execute the insert statement
        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE) {
            MessageBox(NULL, sqlite3_errmsg(db), "Error inserting data", MB_OK | MB_ICONERROR);
        } else {
            // Success message (optional)
            MessageBox(NULL, "Data inserted successfully!", "Success", MB_OK | MB_ICONINFORMATION);
        }
    } else {
        MessageBox(NULL, sqlite3_errmsg(db), "Error preparing statement", MB_OK | MB_ICONERROR);
    }

    sqlite3_finalize(stmt);  // Finalize the statement
}

void DeleteGuestFromTheListView(const char* guestName) {
    // Locate and delete from the ListView
	
	string lowerGuestName = ToLower(guestName);
	
    LVFINDINFO findInfo = {};
    findInfo.flags = LVFI_STRING;

    int itemIndex = -1;
    for (int i = 0; (itemIndex = ListView_GetNextItem(hWndListView, itemIndex, LVNI_ALL)) != -1; i++) {
        // Get the item text and convert it to lowercase
        char itemText[256];
        ListView_GetItemText(hWndListView, itemIndex, 0, itemText, sizeof(itemText));
        
        // Compare lowercase of guestName with item text
        if (ToLower(itemText) == lowerGuestName) {
            // Item found, delete it from the ListView
            ListView_DeleteItem(hWndListView, itemIndex);

            // Refresh the ListView to reflect the changes
            InvalidateRect(hWndListView, NULL, TRUE);
            UpdateWindow(hWndListView);
            return;
        }
    }
}

void UpdateGuestFromTheListView(Guest& guest, const char* previousName){
	
	string lowerGuestName = ToLower(previousName);
	
    LVFINDINFO findInfo = {};
    findInfo.flags = LVFI_STRING;

    int itemIndex = -1;
	for (int i = 0; (itemIndex = ListView_GetNextItem(hWndListView, itemIndex, LVNI_ALL)) != -1; i++) {
        // Get the item text and convert it to lowercase
        char itemText[256];
        ListView_GetItemText(hWndListView, itemIndex, 0, itemText, sizeof(itemText));
        
        // Compare lowercase of guestName with item text
        if (ToLower(itemText) == lowerGuestName) {
		ListView_SetItemText(hWndListView, itemIndex, 0, (char*)guest.name.c_str());
	    ListView_SetItemText(hWndListView, itemIndex, 1, (char*)guest.folioNumber.c_str());
	    ListView_SetItemText(hWndListView, itemIndex, 2, (char*)guest.checkInDate.c_str());
	    ListView_SetItemText(hWndListView, itemIndex, 3, (char*)guest.checkOutDate.c_str());
	   
	   // Convert the integer to a string
        string str = to_string(guest.totalDaysStayed);
	    ListView_SetItemText(hWndListView, itemIndex, 4, (char*)str.c_str());
	   // Refresh the ListView to reflect the changes
        InvalidateRect(hWndListView, NULL, TRUE);
        UpdateWindow(hWndListView);
		}
	}
}

//------------------ FUNCTION TO DELETE THE DATA IN THE DATABASE -----------------
void DeleteDataInDb(const char *guestName)
{
	sqlite3* db = DatabaseManager::getDatabase();
	if (!db) {
        MessageBox(NULL, "Database not open.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    int rc;

    const char* delete_query = "DELETE FROM guests WHERE LOWER(guest_name) = LOWER(?)"; 
    sqlite3_stmt* stmt;
	
	rc = sqlite3_prepare_v2(db, delete_query, -1, & stmt, nullptr);

    if (rc != SQLITE_OK) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error preparing statement", MB_OK | MB_ICONERROR);
        return;
    }
	//bind the delete value here
	rc = sqlite3_bind_text(stmt, 1, guestName, -1, SQLITE_STATIC);

	// Execute the DELETE statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error deleting guest", MB_OK | MB_ICONERROR);
	return;
   } else {
	    DeleteGuestFromTheListView(guestName);
        MessageBox(NULL, "Guest deleted successfully!", "Success", MB_OK | MB_ICONINFORMATION);
		return;
	}

    sqlite3_finalize(stmt);
}

//----------------- FUNCTION TO UPDATE THE DATA IN THE DATABASE ----------------
void UpdateDataInDb(Guest guest, const char* previousName)
{
    sqlite3* db = DatabaseManager::getDatabase(); //get the database pointer from the class
	int rc;  
	//If its nullptr, database connection is not open
    if (!db) {
        MessageBox(NULL, "Database connection is not open!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    sqlite3_stmt* stmt; //pointer for the prepared statement
    
    // Guest found, now proceed with the update
    const char* update_query = 
        "UPDATE guests SET guest_name = ?, folio_number = ?, check_in_date = ?, check_out_date = ?, days_stayed = ? WHERE LOWER(guest_name) = LOWER(?)"; 
    
	//prepare the query to be executed. 
    rc = sqlite3_prepare_v2(db, update_query, -1, &stmt, nullptr); 
	
	//throw error if doesn't get prepared. 
    if (rc != SQLITE_OK) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error preparing update query", MB_OK | MB_ICONERROR);
        sqlite3_finalize(stmt);
        return;
    }

    // Bind the updated data to the query (for the SET clause)
    rc = sqlite3_bind_text(stmt, 1, guest.name.c_str(), -1, SQLITE_STATIC);
    rc |= sqlite3_bind_text(stmt, 2, guest.folioNumber.c_str(), -1, SQLITE_STATIC);
    rc |= sqlite3_bind_text(stmt, 3, guest.checkInDate.c_str(), -1, SQLITE_STATIC);
    rc |= sqlite3_bind_text(stmt, 4, guest.checkOutDate.c_str(), -1, SQLITE_STATIC);
    rc |= sqlite3_bind_int(stmt, 5, guest.totalDaysStayed);
	
	//bind the holder value for the previous name to be executed  
    rc |= sqlite3_bind_text(stmt, 6, previousName, -1, SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error binding update query parameters", MB_OK | MB_ICONERROR);
        sqlite3_finalize(stmt);
        return;
    }

    // Execute the update statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error updating data", MB_OK | MB_ICONERROR);
    } else {
        UpdateGuestFromTheListView(guest, previousName);
        MessageBox(NULL, "Guest data updated successfully!", "Success", MB_OK | MB_ICONINFORMATION);
    }

    // Finalize and clean up
    sqlite3_finalize(stmt);
}


//------------- FUNCTION TO SEARCH A GUEST IN THE DATABASE -------------
Guest SearchFolioNumber(const string& guestName) {
    sqlite3* db = DatabaseManager::getDatabase();
    Guest guest = Guest("", "", "", "", 0);  // Create an empty guest instance to return

    if (!db) {
        MessageBox(NULL, "Database not open.", "Error", MB_OK | MB_ICONERROR);
        return guest;
    }

    // SQL query to search for a guest by name
    const char* search_query = "SELECT * FROM guests WHERE LOWER(guest_name) = LOWER(?)";
    sqlite3_stmt* stmt;

    // Prepare the SQL statement
    int rc = sqlite3_prepare_v2(db, search_query, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error preparing the query", MB_OK | MB_ICONERROR);
        sqlite3_finalize(stmt);
        return guest;
    }

    // Bind the guest name to the query (parameter 1)
    rc = sqlite3_bind_text(stmt, 1, guestName.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error binding parameters", MB_OK | MB_ICONERROR);
        sqlite3_finalize(stmt);
        return guest;
    }

    // Execute the statement and fetch results
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        // Fetch the values from the result (note the correct column indices)
        const char* name = (const char*)sqlite3_column_text(stmt, 1); // 0 = guest_name
        const char* folioNumber = (const char*)sqlite3_column_text(stmt, 2); // 1 = folio_number
        const char* check_in_date = (const char*)sqlite3_column_text(stmt, 3); // 2 = check_in_date
        const char* check_out_date = (const char*)sqlite3_column_text(stmt, 4); // 3 = check_out_date
        int no_of_days_stayed = sqlite3_column_int(stmt, 5); // 4 = days_stayed

        // Populate the guest object with data
        guest = Guest(name ? name : "", folioNumber ? folioNumber : "", check_in_date ? check_in_date : "", check_out_date ? check_out_date : "", no_of_days_stayed);
        MessageBox(NULL, "Guest Found!", "Success", MB_OK | MB_ICONINFORMATION);
    } else {
        // No rows returned (no guest found)
        MessageBox(NULL, "No Guest Found!", "Error", MB_OK | MB_ICONERROR);
    }

    // Finalize the statement
    sqlite3_finalize(stmt);

    return guest; //return the guest instance 
}


// -----------------THIS FUNCTION CREATES THE LIST VIEW -----------
HWND CreateListView(HWND hWndParent) {

    HWND hWndList; // Handle to the list view window
    RECT rcl; // Rectangle for setting the size of the window
    // Ensure that the common control DLL is loaded.
    int index;
    char szText[MAX_PATH]; // Place to store some text
    LV_COLUMN lvC; // List View Column structure
    LV_ITEM lvI; // List view item structure
    InitCommonControls(); //Initialize the common controls

    // Get the size and position of the parent window
    GetClientRect(hWndParent, & rcl);
    hWndList = CreateWindowEx(0L,
        WC_LISTVIEW, // list view class
        "", // no default text
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT |
        LVS_EDITLABELS | WS_EX_CLIENTEDGE,
        0, 0,
        rcl.right - rcl.left, rcl.bottom - rcl.top,
        hWndParent,
        (HMENU) ID_LISTVIEW,
        hInst,
        NULL);

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT; // left align the column
    lvC.cx = 140; // width of the column, in pixels
    lvC.pszText = szText;

    //Set the column header values 
    for (index = 0; index <= 5; index++) {
        lvC.iSubItem = index;
        LoadString(hInst,
            IDS_GUEST_NAME + index,
            szText,
            sizeof(szText)); //Load the values from the resource file 
        if (ListView_InsertColumn(hWndList, index, & lvC) == -1)
            return NULL;
    }

    lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0; //
    lvI.stateMask = 0; //

    //-------------PROCEEDING TO FETCH THE DATA FROM SQL IN ORDER TO DISPLAY IN THE LIST VIEW -----------------
    sqlite3 * db;
    int rc;

    rc = sqlite3_open("guest_information.db", & db); //open the db

    if (rc) {
        MessageBox(NULL, sqlite3_errmsg(db), "Error opening database", MB_OK | MB_ICONERROR);
    }

    const char * sql = "SELECT * FROM guests"; //select all the values from the table guest
    sqlite3_stmt * stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, & stmt, nullptr);

    int rowIndex = 0; // Row index for ListView

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

        lvI.iItem = rowIndex; // Row index in ListView

        lvI.iSubItem = 0; // First column (name of the guest)
        lvI.pszText = (char * ) sqlite3_column_text(stmt, 1); //Guest name in the column is the 1st value in column in the db
        ListView_InsertItem(hWndList, & lvI); // Insert row into ListView

        // For the remaining columns (2 to 5), set the values
        for (int i = 2; i <= 5; i++) {
            lvI.iSubItem = i - 1;

            if (i == 5) //the last value is no of days which is an integer
            {
                int no_of_days_stayed = sqlite3_column_int(stmt, i);

                // Convert the integer to a string
                string str = to_string(no_of_days_stayed);

                //set the pszText value 
                lvI.pszText = (char * ) str.c_str();
            } else {
                const char * columnText = (const char * ) sqlite3_column_text(stmt, i);
                if (columnText) {
                    lvI.pszText = (char * ) columnText; // Set the text value for the column
                }
            }

            // Update the corresponding ListView item
            ListView_SetItemText(hWndList, rowIndex, i - 1, lvI.pszText);
        }

        rowIndex++; // Increment row index for the next item
    }
    return (hWndList);
}

//--------------THIS FUNCTION WILL UPDATE THE LIST UI IMMIDIATELY IN ORDER TO SHOW TO NEWLY ADDED GUEST INFO ------------------
void UpdateListAfterAddingNewGuest(HWND hWndListView,
    const Guest & newGuest) {
    LV_ITEM lvI;
    lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;

    // Get the current number of items in the ListView (new row index)
    int rowIndex = ListView_GetItemCount(hWndListView);

    lvI.iItem = rowIndex; // Use the next available row index

    // Set text for the first column (Guest Name)
    lvI.iSubItem = 0;
    lvI.pszText = (char * ) newGuest.name.c_str();
    ListView_InsertItem(hWndListView, & lvI); // Insert the item into ListView

    ListView_SetItemText(hWndListView, rowIndex, 1, (char * ) newGuest.folioNumber.c_str()); // Folio Number
    ListView_SetItemText(hWndListView, rowIndex, 2, (char * ) newGuest.checkInDate.c_str()); // Check-In Date
    ListView_SetItemText(hWndListView, rowIndex, 3, (char * ) newGuest.checkOutDate.c_str()); // Check-Out Date

    string str = to_string(newGuest.totalDaysStayed);
    ListView_SetItemText(hWndListView, rowIndex, 4, (char * ) str.c_str()); // Days Stayed

    //Refresh the window
    InvalidateRect(hWndListView, NULL, TRUE);

    //Update the window to see immediate changes
    UpdateWindow(hWndListView);
}

//-------------THIS IS A CALLBACK FOR THE MAIN WINDOW -----------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hWndListView = CreateListView(hwnd);
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_ADD_NEW_GUEST:
            //Create the dialog box to add new guest
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ADD_NEW_GUEST_DIALOG), hwnd, AddGuestDlgProc);
            break;

        case ID_DELETE_GUEST:
		    currentContext = DELETE_GUEST;
            //Create the dialog box to first search the guest that needs to be deleted 
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_SEARCH_GUEST_DIALOG), hwnd, SearchGuestDlgProc);
            break;

        case ID_SEARCH_GUEST:
		    currentContext = SEARCH_GUEST;
            //Create a dialog box to search a guest 
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_SEARCH_GUEST_DIALOG), hwnd, SearchGuestDlgProc);
            break;
		case ID_EDIT_GUEST_INFO:
			currentContext = EDIT_GUEST; 
			//Create the dialog box to first search the guest that needs to be edited. 
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_SEARCH_GUEST_DIALOG), hwnd, SearchGuestDlgProc);
        break;
		}
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

//-----------THIS IS A CALLBACK FOR THE ADD NEW GUEST INFO DIALOG BOX ------------
BOOL CALLBACK AddGuestDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    switch (Message) {

    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDOK_ADD_NEW_GUEST: {
            //Get all the values from the edit boxes
            GetDlgItemText(hwnd, ID_GUEST_NAME, g_GuestName, 100);
            GetDlgItemText(hwnd, ID_FOLIO_NUMBER, g_FolioNumber, 100);
            GetDlgItemText(hwnd, ID_CHECK_IN_DATE, g_CheckInDate, 100);
            GetDlgItemText(hwnd, ID_CHECK_OUT_DATE, g_CheckOutDate, 100);
			
            // Check the guest name before proceeding
            if (!TextBoxValidator::IsNotEmpty(g_GuestName) || !TextBoxValidator::IsValidName(g_GuestName)) {
                MessageBox(hwnd, TextBoxValidator::errorMessage.c_str(), "Input Error", MB_ICONEXCLAMATION | MB_OK);
                return FALSE; // Exit without proceeding
            }
            // Check the FolioNumber before proceeding
            if (!TextBoxValidator::IsNotEmpty(g_FolioNumber) || !TextBoxValidator::IsValidFolioNumber(g_FolioNumber)) {
                MessageBox(hwnd, TextBoxValidator::errorMessage.c_str(), "Input Error", MB_ICONEXCLAMATION | MB_OK);
                return FALSE;
              
            }
            // Check the CheckInDate and checkout date before proceeding
            if (!TextBoxValidator::IsNotEmpty(g_CheckInDate) || !TextBoxValidator::IsValidDateFormat(g_CheckInDate) || !TextBoxValidator::IsNotEmpty(g_CheckOutDate) || !TextBoxValidator::IsValidDateFormat(g_CheckOutDate)) {
                MessageBox(hwnd, TextBoxValidator::errorMessage.c_str(), "Input Error", MB_ICONEXCLAMATION | MB_OK);
                return FALSE;
            }
            //If everything is validated, proceed to add data	
            else {
		 
				int total_days_stayed = TotalNumberOfDaysStayedCalculator::CalculateDaysDifference(string(g_CheckInDate), string(g_CheckOutDate));
                //create instance for the guest information
                if(total_days_stayed <= 0)
				{
				 MessageBox(hwnd, "Check out date cannot be less than or equal to the check in date", "Input Error", MB_ICONEXCLAMATION | MB_OK);
                 return FALSE;	
				}
	            if (total_days_stayed > 30)
			    {
				MessageBox(hwnd, "Guest cannot stay more than 28 days. Please take action immediately", "Exceeds 28 days", MB_ICONEXCLAMATION | MB_OK);
                return FALSE;
			    }
				else{
				Guest instance = Guest(
                    string(g_GuestName),
                    string(g_FolioNumber),
                    string(g_CheckInDate),
                    string(g_CheckOutDate),
                    total_days_stayed
                );
                AddDataInDb(instance); //add the data to the db
                UpdateListAfterAddingNewGuest(hWndListView, instance); //update the list UI
                EndDialog(hwnd, IDOK_ADD_NEW_GUEST); //Exit out of the dialog
				}
            }
		break;
        }
        case IDCANCEL_ADD_NEW_GUEST:
            //Just exit the dialog if user decides to just cancel
            EndDialog(hwnd, IDCANCEL_ADD_NEW_GUEST);
            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK DeleteGuestDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
 
    switch (Message) {

    case WM_INITDIALOG: {
        Guest * guest = reinterpret_cast < Guest * > (lParam);
        SetDlgItemText(hwnd, ID_GUEST_NAME, guest -> name.c_str());
        SetDlgItemText(hwnd, ID_FOLIO_NUMBER, guest -> folioNumber.c_str());
        SetDlgItemText(hwnd, ID_CHECK_IN_DATE, guest -> checkInDate.c_str());
        SetDlgItemText(hwnd, ID_CHECK_OUT_DATE, guest -> checkOutDate.c_str());
        string days = to_string(guest -> totalDaysStayed);
        SetDlgItemText(hwnd, ID_NO_OF_DAYS_STAYED, days.c_str());
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(guest));
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
		case IDOK_DELETE_GUEST: {
		    Guest* guest = reinterpret_cast<Guest*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			
			DeleteDataInDb(guest->name.c_str());
			EndDialog(hwnd, IDOK_DELETE_GUEST);
			break;
		}
        case IDCANCEL_DELETE_GUEST:
            EndDialog(hwnd, IDCANCEL_DELETE_GUEST);
            break;
        
		}
	break;
	case WM_CLOSE: // Handle the close button (X)
        EndDialog(hwnd, IDCANCEL); // Equivalent to canceling the operation
        break;
    default:
        return FALSE;
    }
    return TRUE;

}

BOOL CALLBACK SearchGuestDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    switch (Message) {

    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDSEARCH_GUEST: {

			//get the guest name from the edit box
            GetDlgItemText(hwnd, ID_GUEST_NAME, g_GuestName, 100);
			
            // Check the FolioNumber before proceeding
            if (!TextBoxValidator::IsNotEmpty(g_GuestName)) {
                MessageBox(hwnd, TextBoxValidator::errorMessage.c_str(), "Input Error", MB_ICONEXCLAMATION | MB_OK);
                return FALSE;
            } else {
               
                Guest guest = SearchFolioNumber(g_GuestName);
				
				if(guest.name.length() > 0){
					
					if(currentContext == DELETE_GUEST){
                    DialogBoxParam(GetModuleHandle(NULL),
                    MAKEINTRESOURCE(ID_DELETE_GUEST_DIALOG),
                    hwnd,
                    DeleteGuestDlgProc,
                    reinterpret_cast < LPARAM > ( & guest)); //pass the guest as LPARAM
				    }
					else if(currentContext == SEARCH_GUEST){
					DialogBoxParam(GetModuleHandle(NULL),
                    MAKEINTRESOURCE(ID_DISPLAY_STATIC_SEARCH_DIALOG),
                    hwnd,
                    DeleteGuestDlgProc,
                    reinterpret_cast < LPARAM > ( & guest));
					}
					else
					{
					DialogBoxParam(GetModuleHandle(NULL),
                    MAKEINTRESOURCE(ID_EDIT_GUEST_INFO_DIALOG),
                    hwnd,
                    EditGuestDlgProc,
                    reinterpret_cast < LPARAM > ( & guest));	
					}
				}
          }
		    break;
        }

        case IDCANCEL_SEARCH_GUEST:
			EndDialog(hwnd, IDCANCEL_SEARCH_GUEST);
            break;
        }
	break;
    default:
        return FALSE;
    }
    return TRUE;
}

// ----------------------- FUNCTION TO EDIT THE DIALOG ---------------------------
BOOL CALLBACK EditGuestDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

    switch (Message) {
    case WM_INITDIALOG: {
		Guest* guest = reinterpret_cast<Guest*>(lParam);
        if (guest) {
            SetDlgItemText(hwnd, ID_GUEST_NAME, guest->name.c_str());
            SetDlgItemText(hwnd, ID_FOLIO_NUMBER, guest->folioNumber.c_str()); 
            SetDlgItemText(hwnd, ID_CHECK_IN_DATE, guest->checkInDate.c_str()); 
            SetDlgItemText(hwnd, ID_CHECK_OUT_DATE, guest->checkOutDate.c_str());   
            string days = to_string(guest->totalDaysStayed);
		    SetDlgItemText(hwnd, ID_NO_OF_DAYS_STAYED, days.c_str());
			//Store the data using SetWindowLongPtr in the memory. For some reason, I cannot use the guest pointer if initialzied outside the INITDIALOG. 
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(guest));
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDEDIT_GUEST: {
			//retreive the guest instance using GWLP_USERDATA to process further 
			Guest* guest = reinterpret_cast<Guest*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		    char tempGuestName[100], tempFolioNumber[100], tempCheckInDate[100], tempCheckOutDate[100];
            // Get the values from the edit boxes
            GetDlgItemText(hwnd, ID_GUEST_NAME, tempGuestName, sizeof(tempGuestName));
            GetDlgItemText(hwnd, ID_FOLIO_NUMBER, tempFolioNumber, sizeof(tempFolioNumber));
            GetDlgItemText(hwnd, ID_CHECK_IN_DATE, tempCheckInDate, sizeof(tempCheckInDate));
            GetDlgItemText(hwnd, ID_CHECK_OUT_DATE, tempCheckOutDate, sizeof(tempCheckOutDate));
			
            // Check the guest name before proceeding
            if (!TextBoxValidator::IsNotEmpty(tempGuestName) || !TextBoxValidator::IsValidName(tempGuestName)) {
                MessageBox(hwnd, TextBoxValidator::errorMessage.c_str(), "Input Error", MB_ICONEXCLAMATION | MB_OK);
                return FALSE; // Exit without proceeding
            }

            // Check the FolioNumber before proceeding
            if (!TextBoxValidator::IsNotEmpty(tempFolioNumber) || !TextBoxValidator::IsValidFolioNumber(tempFolioNumber)) {
                MessageBox(hwnd, TextBoxValidator::errorMessage.c_str(), "Input Error", MB_ICONEXCLAMATION | MB_OK);
                return FALSE;
            }

            // Check the CheckInDate and CheckOutDate before proceeding
            if (!TextBoxValidator::IsNotEmpty(tempCheckInDate) || !TextBoxValidator::IsValidDateFormat(tempCheckInDate) ||
                !TextBoxValidator::IsNotEmpty(tempCheckOutDate) || !TextBoxValidator::IsValidDateFormat(tempCheckOutDate)) {
                MessageBox(hwnd, TextBoxValidator::errorMessage.c_str(), "Input Error", MB_ICONEXCLAMATION | MB_OK);
                return FALSE;
            }

            // Calculate the total number of days stayed
            int total_days_stayed = TotalNumberOfDaysStayedCalculator::CalculateDaysDifference(tempCheckInDate, tempCheckOutDate);
            if (total_days_stayed <= 0) {
                MessageBox(hwnd, "Check out date cannot be less than or equal to the check in date", "Input Error", MB_ICONEXCLAMATION | MB_OK);
                return FALSE;
            }
			if (total_days_stayed > 30)
			{
			    MessageBox(hwnd, "Guest cannot stay more than 28 days. Please take action immediately", "Exceeds 28 days", MB_ICONEXCLAMATION | MB_OK);
                return FALSE;
			}
            // Create a new instance of the guest with updated information
            Guest instance(
                string(tempGuestName),
                string(tempFolioNumber),
                string(tempCheckInDate),
                string(tempCheckOutDate),
                total_days_stayed
            );

            UpdateDataInDb(instance,guest->name.c_str());
            EndDialog(hwnd, IDEDIT_GUEST);
            break;
        }

        case IDCANCEL_EDIT_GUEST:
            EndDialog(hwnd, IDCANCEL_EDIT_GUEST);
            break;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

// --------------- MAIN ENTRY FUNCTION ------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;
    //Registering the Window Class
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MYMENU);
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassEx( & wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    hInst = hInstance; //setting the global window instance here
    // Creating the Window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "21 Day Counter",
        WS_OVERLAPPEDWINDOW,
        0, 0, 700, 600,
        NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	    // Open the database when the application starts
    if (!DatabaseManager::getDatabase()) {
        MessageBox(NULL, "Error opening database!", "Error", MB_OK | MB_ICONERROR);
        return 0;  // Exit if the database can't be opened
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    //The Message Loop
    while (GetMessage( & Msg, NULL, 0, 0) > 0) {
        TranslateMessage( & Msg);
        DispatchMessage( & Msg);
    }
	
	DatabaseManager::closeDatabase();
	
    return Msg.wParam;
}
