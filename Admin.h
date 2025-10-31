#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <crypt.h>
#include <semaphore.h>
//#include "../Structures/allStruct.h"

#define ADMINNAME "RamJi"
#define PASSWORD "2002"
#define EMPPATH "../Data/employees.txt"
#define CUSPATH "../Data/customers.txt"
#define HASHKEY "123@GoodByeHashing@123"

extern sem_t *sema;
extern char semName[50];
extern char readBuffer[4096], writeBuffer[4096];

sem_t *initializeSemaphore(int id);
void setupSignalHandlers();
void cleanupSemaphore(int signum);

int addEmployee(int connectionFD);
void modifyCE(int connectionFD, int modifyChoice);
void manageRole(int connectionFD);

// ================ Add New Bank Employee ================
int addEmployee(int connectionFD)
{
    struct Employee emp;
    char fname[20], lname[20], pwd[20];
    int empID;

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Employee ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    empID = atoi(readBuffer);

    // Check if employee ID already exists
    int fd_check = open(EMPPATH, O_RDONLY);
    if (fd_check != -1) {
        struct Employee tmp;
        while (read(fd_check, &tmp, sizeof(tmp)) == sizeof(tmp)) {
            if (tmp.empID == empID) {
                bzero(writeBuffer, sizeof(writeBuffer));
                strcpy(writeBuffer, "Error: Employee ID already exists!^");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));
                close(fd_check);
                return 0;
            }
        }
        close(fd_check);
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter First Name: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(fname, readBuffer);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Last Name: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(lname, readBuffer);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(pwd, readBuffer);

    int fd = open(EMPPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("open employees");
        return 0;
    }

    emp.empID = empID;
    strcpy(emp.firstName, fname);
    strcpy(emp.lastName, lname);
    strcpy(emp.password, crypt(pwd, HASHKEY));
    emp.role = 1; // Default role: Employee

    write(fd, &emp, sizeof(emp));
    close(fd);

    printf("Admin added employee ID %d\n", empID);
    return 1;
}

// ===================== Modify Customer / Employee ==================
void modifyCE(int connectionFD, int modifyChoice)
{
    if(modifyChoice == 1)
    {
        printf("Admin choose 1 - Modify Customer\n");
        int file = open(CUSPATH, O_CREAT | O_RDWR , 0644);
        if(file == -1)
        {
            printf("Error opening file!\n");
            return ;
        }

        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Enter Account Number: ");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        int accNo;
        bzero(readBuffer, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        accNo = atoi(readBuffer);
        printf("Admin entered account number: %d\n", accNo);

        struct Customer c;    
        lseek(file, 0, SEEK_SET);

        int srcOffset = -1, sourceFound = 0;

        while (read(file, &c, sizeof(c)) != 0)
        {
            if(c.accountNumber == accNo)
            {
                srcOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
                sourceFound = 1;
            }
            if(sourceFound)
                break;
        }

        struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Customer), getpid()};
        fcntl(file, F_SETLKW, &fl1);

        if(sourceFound)
        {
            char newName[20];
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter New Name: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            strcpy(newName, readBuffer);
            printf("Admin entered new Name: %s\n", newName);

            strcpy(c.customerName, newName);

            write(file, &c, sizeof(c));

            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Customer name updated successfully!^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }
        else
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Invalid Account Number\n^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }
        fl1.l_type = F_UNLCK;
        fl1.l_whence = SEEK_SET;
        fl1.l_start = srcOffset;
        fl1.l_len = sizeof(struct Customer);
        fl1.l_pid = getpid();

        fcntl(file, F_UNLCK, &fl1);
        close(file);
        return ;
    }

    else if(modifyChoice == 2)
    {
        printf("Admin choose 2 - Modify Employee\n");
        struct Employee emp;
        int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
        if(file == -1)
        {
            printf("Error opening file!\n");
            return ;
        }

        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Enter Employee ID: ");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        int id;
        bzero(readBuffer, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        id = atoi(readBuffer);
        printf("Admin entered Employee ID: %d\n", id);

        lseek(file, 0, SEEK_SET);

        int srcOffset = -1, sourceFound = 0;

        while (read(file, &emp, sizeof(emp)) != 0)
        {
            if(emp.empID == id)
            {
                srcOffset = lseek(file, -sizeof(struct Employee), SEEK_CUR);
                sourceFound = 1;
            }
            if(sourceFound)
                break;
        }

        struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Employee), getpid()};
        fcntl(file, F_SETLKW, &fl1);

        if(sourceFound)
        {
            char newName[20];
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Enter New First Name: ");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            strcpy(newName, readBuffer);

            strcpy(emp.firstName, newName);
            printf("Admin entered new Name: %s\n", newName);
            printf("Admin changed name of employee: %d\n", id);

            write(file, &emp, sizeof(emp));

            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Employee name updated successfully!^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }
        else
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Invalid Employee ID^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));

            bzero(readBuffer, sizeof(readBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));        
        }
        
        fl1.l_type = F_UNLCK;
        fl1.l_whence = SEEK_SET;
        fl1.l_start = srcOffset;
        fl1.l_len = sizeof(struct Employee);
        fl1.l_pid = getpid();

        fcntl(file, F_UNLCK, &fl1);
        close(file);

        return ;   
    }
}

// ===================== Manager Role ==================
void manageRole(int connectionFD)
{
    int file = open(EMPPATH, O_CREAT | O_RDWR, 0644);
    if(file == -1)
    {
        printf("Error opening file!\n");
        return ;
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Employee ID: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    int id;
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    id = atoi(readBuffer);

    struct Employee emp;
    int employeeFound = 0;
    long offset = -1;

    while(read(file, &emp, sizeof(emp)) != 0)
    {
        if(emp.empID == id)
        {
            offset = lseek(file, 0, SEEK_CUR) - sizeof(struct Employee);
            employeeFound = 1;
            break;
        }
    }

    if(employeeFound)
    {
        int choice;
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Enter 1 to make manager\nEnter 2 to make employee: ");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        bzero(readBuffer, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        choice = atoi(readBuffer);

        // Lock the record
        struct flock fl = {F_WRLCK, SEEK_SET, offset, sizeof(struct Employee), getpid()};
        fcntl(file, F_SETLKW, &fl);

        // Read the employee again to get current data
        lseek(file, offset, SEEK_SET);
        read(file, &emp, sizeof(emp));
        
        // Update role
        if(choice == 1)
        {
            printf("Admin made %d manager\n", id);
            emp.role = 0; // Manager
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Employee promoted to Manager!^");
        }
        else if(choice == 2)
        {
            printf("Admin made %d employee\n", id);
            emp.role = 1; // Employee
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Manager demoted to Employee!^");
        }
        else
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, "Invalid choice!^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            
            fl.l_type = F_UNLCK;
            fcntl(file, F_SETLK, &fl);
            close(file);
            return;
        }

        // Write back the updated employee
        lseek(file, offset, SEEK_SET);
        write(file, &emp, sizeof(emp));

        // Unlock
        fl.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl);
        
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
    }
    else
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Invalid Employee ID^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
    }
    
    close(file);
    return;
}

// =================== Admin Menu ===================
void adminMenu(int connectionFD)
{
    printf("DEBUG: Admin menu started\n");
    
    sema = initializeSemaphore(0);
    if(sema == SEM_FAILED) {
        perror("Semaphore initialization failed");
        return;
    }

    setupSignalHandlers();

    if (sem_trywait(sema) == -1) {
        if (errno == EAGAIN) {
            printf("Admin is already logged in!\n");
        } else {
            perror("sem_trywait failed");
        }
        sem_close(sema);
        return;
    }
    
    printf("DEBUG: Semaphore locked successfully\n");
    
    int flag = 0;
    char password[20];
    
label1:
    bzero(writeBuffer, sizeof(writeBuffer));
    if(flag)
    {
        strcat(writeBuffer, "\nInvalid credential\n");
        flag = 0;
    }
    strcat(writeBuffer, "Enter password: ");
    
    printf("DEBUG: Sending password prompt to client\n");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(password, readBuffer);
    
    printf("DEBUG: Received password: %s\n", password);

    if(strcmp(PASSWORD, password) == 0)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        strcpy(writeBuffer, "\nLogin Successful^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        printf("DEBUG: Admin login successful\n");
    }
    else
    {
        printf("DEBUG: Admin login failed - expected: %s, got: %s\n", PASSWORD, password);
        flag = 1;
        goto label1;
    }

    while(1)
    {
        int modifyChoice;
        int choice;

        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ADMINMENU);
        write(connectionFD, writeBuffer, sizeof(writeBuffer));

        bzero(readBuffer, sizeof(readBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
        choice = atoi(readBuffer);
        printf("Admin entered: %d\n", choice);

        switch (choice) {
            case 1:
                // Add new bank employee
                if(addEmployee(connectionFD))
                {
                    bzero(writeBuffer, sizeof(writeBuffer));
                    bzero(readBuffer, sizeof(readBuffer));
                    strcpy(writeBuffer, "Employee successfully added\n^");
                    write(connectionFD, writeBuffer, sizeof(writeBuffer));
                    read(connectionFD, readBuffer, sizeof(readBuffer));
                }
                else
                {
                    bzero(writeBuffer, sizeof(writeBuffer));
                    bzero(readBuffer, sizeof(readBuffer));
                    strcpy(writeBuffer, "Failed to add employee\n^");
                    write(connectionFD, writeBuffer, sizeof(writeBuffer));
                    read(connectionFD, readBuffer, sizeof(readBuffer));
                }
                break;
            case 2:
                // Modify Customer/Employee details
                bzero(writeBuffer, sizeof(writeBuffer));
                strcpy(writeBuffer, "Enter 1 to Modify Customer\nEnter 2 to Modify Employee: ");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));
                
                bzero(readBuffer, sizeof(readBuffer));
                read(connectionFD, readBuffer, sizeof(readBuffer));
                modifyChoice = atoi(readBuffer);

                modifyCE(connectionFD, modifyChoice);
                break;
            case 3:
                // Manage User Roles
                manageRole(connectionFD);
                break;
            
            case 4:
                // Change password - Not implemented yet
                bzero(writeBuffer, sizeof(writeBuffer));
                strcpy(writeBuffer, "Change password feature not implemented yet^");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));
                read(connectionFD, readBuffer, sizeof(readBuffer));
                break;
            case 5:
                // Logout
                printf("DEBUG: Admin logging out\n");
                sem_post(sema);
                sem_close(sema);
                return;
            default:
                bzero(writeBuffer, sizeof(writeBuffer));
                bzero(readBuffer, sizeof(readBuffer));
                strcpy(writeBuffer, "Invalid choice! Please try again.^");
                write(connectionFD, writeBuffer, sizeof(writeBuffer));
                read(connectionFD, readBuffer, sizeof(readBuffer));
        }
    }    
}
