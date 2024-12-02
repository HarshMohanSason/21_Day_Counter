# 21 Day Counter

## Description

The 21 Day Counter is a program designed to track whether a guest has stayed 21 days or more at a property. Once a guest reaches their 21st day, the program sends a notification as a reminder. The system stores all guest information in a local SQLite database, ensuring data persistence and easy access.

I have not uploaded the .exe files here. You would need to compile to code with g++ compiler along with the resource files and sqlite object files. 

### Steps: 
1. windres resource.rc -o resource.o                                (Compile the resource file first)
2. gcc -c sqlite3.c -o sqlite3.o                                    (Compile the sqlite3.h as well to get the db output file)
3. g++ -c main.cpp -o main.o                                        (Compile the main file to .o as well)
4. g++ main.o sqlite3.o resource.o -lcomctl32 -o 21_day_counter.exe (Compile all the .o files together with -lcomctl32 because the code contains                                                                                dependencies from the Comctl32.dll file so make sure to set the flag)

This will generate all the .exe file which will then look something like this after you add the add the data. 

![Screenshot 2024-12-01 182319](https://github.com/user-attachments/assets/aa05021c-3137-429e-b500-07285ade5a3f)

### To send notification, you need to compile the send_notification.cpp separately. 
1. g++ -c send_notification.cpp -o script.o
2. g++ script.o sqlite3.o -o send_notification.exe

You would then need to add that to the task schedular to set it up for regular notifications everyday when 21 days have been over in the db. It should look something like this. 

![Screenshot 2024-12-01 182651](https://github.com/user-attachments/assets/5534a901-8119-4eca-96a1-bd256cc42a06)

### Make sure to also have the .exe files in the same folder since send_notification also uses the same .db file. Something like this

![Screenshot 2024-12-01 182401](https://github.com/user-attachments/assets/2e43e75d-a2cb-47de-b534-545f806ada6b)

### Note: If you do not have MinGW installed, you would need to get these dll files from somewhere. 

