#ifndef CUSTOMER_H
#define CUSTOMER_H

void customerMenu(int connectionFD);
int loginCustomer(int connectionFD, int accountNumber, char *password);
void withdrawMoney(int connectionFD, int accountNumber);
void depositMoney(int connectionFD, int accountNumber);
void customerBal(int connectionFD, int accountNumber);
void applyLoan(int connectionFD, int accountNumber);
void transferFunds(int connectionFD, int accountNumber, int destAcc, float amt);
void addFeedback(int connectionFD);
void transactionHistory(int connectionFD, int accountNumber);
int changePassword(int connectionFD, int accountNumber);
void logout(int connectionFD, int id);

int writeBytes, readBytes, key, loginOffset;
char readBuffer[4096], writeBuffer[4096];

void customerMenu(int connectionFD){
    struct Customer newCustomer;
    int accountNumber;
    int destAcc;
    int response = 0;
    char password[20];
    char customerName[20];
    char newPassword[20];
    float amount;

label1:
    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "\nEnter account number: ");
    writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
    if(writeBytes == -1)
    {
        printf("Error sending data\n");
    }
    else
    {
        // read account number
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
        if(readBytes == -1)
        {
            printf("Error reading data\n");
        }
        else
        {
            accountNumber = atoi(readBuffer);
        }
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer,  "Enter password: ");
    writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
    if(writeBytes == -1)
    {
        printf("Error sending data\n");
    }
    else
    {
        // Read Password
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
        strcpy(password, readBuffer);

        if (loginCustomer(connectionFD, accountNumber, password))
        {            
            while(1)
            {   
                bzero(writeBuffer, sizeof(writeBuffer));
                strcpy(writeBuffer, CUSMENU);
                writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
                if(writeBytes == -1)
                {
                    printf("Error writing to client\n");
                }
                else
                {
                    printf("Menu sent to client\n");
                    bzero(readBuffer, sizeof(readBuffer));
                    int readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
                    if(readBytes == -1)
                    {
                        printf("Error reading client data\n");
                    }
                    else if(readBytes == SIGINT){
                        printf("SIGINT");
                    }
                    else
                    {
                        int choice = atoi(readBuffer);
                        printf("Customer entered: %d\n", choice);

                        switch(choice)
                        {
                            case 1:
                                depositMoney(connectionFD, accountNumber);
                                break;
                            case 2:
                                withdrawMoney(connectionFD, accountNumber);
                                break;
                            case 3:
                                customerBal(connectionFD, accountNumber);;
                                break;
                            case 4:
                                applyLoan(connectionFD, accountNumber);
                                break;
                            case 9:
                                bzero(writeBuffer, sizeof(writeBuffer));
                                strcpy(writeBuffer, "Enter dest account number: ");
                                write(connectionFD, writeBuffer, sizeof(writeBuffer));

                                bzero(readBuffer, sizeof(readBuffer));
                                read(connectionFD, readBuffer, sizeof(readBuffer));
                                destAcc = atoi(readBuffer);
                                
                                float amt;
                                bzero(writeBuffer, sizeof(writeBuffer));
                                strcpy(writeBuffer, "Enter amount: ");
                                write(connectionFD, writeBuffer, sizeof(writeBuffer));
                                bzero(readBuffer, sizeof(readBuffer));
                                read(connectionFD, readBuffer, sizeof(readBuffer));

                                amt = atof(readBuffer);
                                transferFunds(connectionFD, accountNumber, destAcc, amt);
                                break;
                            case 6:
                                response = changePassword(connectionFD, accountNumber);
                                if(!response)
                                {
                                    bzero(writeBuffer, sizeof(writeBuffer));
                                    strcpy(writeBuffer, "Unable to change password^");
                                    write(connectionFD, writeBuffer, sizeof(writeBuffer));
                                    read(connectionFD, readBuffer, sizeof(readBuffer));
                                }
                                else
                                { 
                                    bzero(writeBuffer, sizeof(writeBuffer));
                                    strcpy(writeBuffer, "Password changed successfully^");
                                    write(connectionFD, writeBuffer, sizeof(writeBuffer));
                                    read(connectionFD, readBuffer, sizeof(readBuffer));                                   
                                }                                    
                                goto label1;
                            case 7:
                                // View Transaction
                                transactionHistory(connectionFD, accountNumber);
                                break;
                            case 8:
                                // Add Feedback
                                addFeedback(connectionFD);
                                break;
                            case 5:
                                // Logout
                                printf("%d logged out!\n", accountNumber);
                                logout(connectionFD, accountNumber);
                                return;
                            default:
                                write(connectionFD, "Invalid Choice from customer menu\n", sizeof("Invalid Choice from customer menu\n"));                                
                        }
                    }
                }
            }
        }
        else
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            bzero(readBuffer, sizeof(readBuffer));
            strcpy(writeBuffer, "\nInvalid ID or Password^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
            goto label1;
        }
    }
}

// ======================= Login system =======================
int loginCustomer(int connectionFD, int accountNumber, char *password) {
    struct Customer customer;
    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);

    if (file == -1) {
        printf("Error opening file!\n");
        return 0;
    }

    sema = initializeSemaphore(accountNumber);

    setupSignalHandlers();

    if (sem_trywait(sema) == -1) {
        if (errno == EAGAIN) {
            printf("Customer with account number %d is already logged in!\n", accountNumber);
        } else {
            perror("sem_trywait failed");
        }
        close(file);
        return 0;
    }

    lseek(file, 0, SEEK_SET);
    while(read(file, &customer, sizeof(customer)) != 0)
    {
        if (customer.accountNumber == accountNumber && strcmp(customer.password, crypt(password, HASHKEY)) == 0 && customer.activeStatus == 1) {
            printf("Customer whose acc no.: %d loggedIn\n", accountNumber);
            close(file);
            return 1;
        }
    }

    // sem_post(sema);

    snprintf(semName, 50, "/sem_%d", accountNumber);

    sem_t *sema = sem_open(semName, 0);
    if (sema != SEM_FAILED) {
        sem_post(sema);
        sem_close(sema); 
        sem_unlink(semName);    
    }

    close(file);
    return 0;
}

// ======================= Deposit Money =======================
void depositMoney(int connectionFD, int accountNumber){
    char readBuffer[4096], writeBuffer[4096], transactionBuffer[1024];

    struct Customer customer;
    struct trans_histroy th;

    time_t s, val = 1;
	struct tm* current_time;
	s = time(NULL);
	current_time = localtime(&s);

    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    lseek(fp, 0, SEEK_END);
  
    int found = 0;
    float depositAmount;

    if (file == -1) {
        printf("Error opening file!\n");
        return;
    }

    while(read(file, &customer, sizeof(customer)) != 0) {
        if (customer.accountNumber == accountNumber) {
            break;
        }
    }
    int offset = lseek(file, -sizeof(struct Customer), SEEK_CUR);

    struct flock fl = {F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
    int lockStatus = fcntl(file, F_SETLKW, &fl);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter deposit amount: ");
    writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));

    if(writeBytes == -1)
    {
        printf("Error writing to client\n");
    }
    else
    {
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
        if(readBytes == -1)
        {
            printf("Error reading client data\n");
        }
        else
        {
            depositAmount = atof(readBuffer);
            printf("Customer with acc no.: %d deposited %.2f\n", accountNumber, depositAmount);

            // lseek(file, 0, SEEK_SET);
            // while(read(file, &customer, sizeof(customer)) != 0) {
            //     if (customer.accountNumber == accountNumber) {
            //         break;
            //     }
            // }
            // lseek(file, -sizeof(struct Customer), SEEK_CUR);

            customer.balance += depositAmount;
            
            bzero(transactionBuffer, sizeof(transactionBuffer));
            sprintf(transactionBuffer, "%.2f deposited at %02d:%02d:%02d %d-%d-%d\n", depositAmount, current_time->tm_hour, current_time->tm_min,current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);
            
            bzero(th.hist, sizeof(th.hist));
            strcpy(th.hist, transactionBuffer);
            th.acc_no = customer.accountNumber;
            write(fp, &th, sizeof(th));
            write(file, &customer, sizeof(customer));


            fl.l_type = F_UNLCK;
            fcntl(file, F_SETLK, &fl);
            
            close(fp);
            close(file);

            bzero(readBuffer, sizeof(readBuffer));
            bzero(writeBuffer, sizeof(writeBuffer));
            sprintf(writeBuffer, "Deposit successful!^");
            write(connectionFD, writeBuffer, sizeof(writeBuffer));
            read(connectionFD, readBuffer, sizeof(readBuffer));
        }
    }
    return;
}

// ======================= View Balance =======================
void customerBal(int connectionFD, int accountNumber){
    char readBuffer[4096], writeBuffer[4096];
    struct Customer customer;
    int file = open(CUSPATH, O_RDONLY);
    if (file == -1) {
        printf("Error opening file!\n");
        return ;
    }
    float updatedBalance = 0;

    lseek(file, 0, SEEK_SET);
    while(read(file, &customer, sizeof(customer)) != 0)
    {
        if (customer.accountNumber == accountNumber) {
            updatedBalance = customer.balance;
            break;
        }
    }
    close(file);
    
    printf("Current balance of %d: %.2f\n", accountNumber, updatedBalance);
    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));
    sprintf(writeBuffer, "The current balance is: %.2f^", updatedBalance);
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));

    return;
}

// ======================= Withdraw Money =======================
void withdrawMoney(int connectionFD, int accountNumber){
    char readBuffer[4096], writeBuffer[4096], transactionBuffer[1024];
    struct Customer customer;
    struct trans_histroy th;

    time_t s, val = 1;
	struct tm* current_time;
	s = time(NULL);
	current_time = localtime(&s);

    int found = 0;
    float withdrawAmount;

    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    lseek(fp, 0, SEEK_END);

    while (read(file, &customer, sizeof(customer)) != 0)
    {
        if(customer.accountNumber == accountNumber)
        {
            break;
        }
    }
    int offset = lseek(file, -sizeof(struct Customer), SEEK_CUR);

    struct flock fl = {F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid()};
    int lockStatus = fcntl(file, F_SETLKW, &fl);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter the amount to withdraw: ");
    writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));

    if(writeBytes == -1)
    {
        printf("Error writing to client\n");
    }
    else
    {
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
        if(readBytes == -1)
        {
            printf("Error reading from client\n");
        }
        else
        {
            if(lockStatus == -1)
            {
                printf("Unable to lock file\n");
                exit(0);
            }

            withdrawAmount = atof(readBuffer);

            // lseek(file, 0, SEEK_SET);
            // while(read(file, &customer, sizeof(customer)) != 0) {
            //     if (customer.accountNumber == accountNumber) {
            //         break;
            //     }
            // }
            // lseek(file, -sizeof(struct Customer), SEEK_CUR);

            printf("requested to withdraw from %d account number: %.2f\n", accountNumber, withdrawAmount);

            if (customer.balance < withdrawAmount){
                bzero(writeBuffer, sizeof(writeBuffer));
                bzero(readBuffer, sizeof(readBuffer));
                
                printf("Insufficient balance in account: %d\n", accountNumber);

                sprintf(writeBuffer, "Insufficient funds!^");
                writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
                read(connectionFD, readBuffer, sizeof(readBuffer));
                if(writeBytes == -1)
                {
                    printf("Unable to write to client\n");
                }

                fl.l_type = F_UNLCK;
                fcntl(file, F_SETLK, &fl);

                close(fp);
                close(file);
                return;
            }
            customer.balance -= withdrawAmount;

            bzero(transactionBuffer, sizeof(transactionBuffer));
            sprintf(transactionBuffer, "%.2f withdraw at %02d:%02d:%02d %d-%d-%d\n", withdrawAmount, current_time->tm_hour, current_time->tm_min,current_time->tm_sec, (current_time->tm_year)+1900, (current_time->tm_mon)+1, current_time->tm_mday);

            bzero(th.hist, sizeof(th.hist));
            strcpy(th.hist, transactionBuffer);
            th.acc_no = customer.accountNumber;
            write(fp, &th, sizeof(th));
            write(file, &customer, sizeof(customer));

            fl.l_type = F_UNLCK;
            fcntl(file, F_SETLK, &fl);

            close(fp);
            close(file);

            bzero(readBuffer, sizeof(readBuffer));
            bzero(writeBuffer, sizeof(writeBuffer));
            printf("New balance of %d account number: %.2f\n", accountNumber, customer.balance);
            sprintf(writeBuffer, "Withdrawal successful!^");

            writeBytes = write(connectionFD, writeBuffer, sizeof(writeBuffer));
            readBytes = read(connectionFD, readBuffer, sizeof(readBuffer));
            if(writeBytes == -1)
            {
                printf("Error writing to client\n");
            }
        }
    }
    return;
}

// ======================= Apply loan =======================
void applyLoan(int connectionFD, int accountNumber){

    // reading & writing loan counter
    struct Counter ct;
    int file = open(COUNTERPATH, O_RDWR, 0644);
    read(file, &ct, sizeof(ct));
    int lc = ct.count;
    printf("Number of loans: %d\n", lc);
    lseek(file, 0, SEEK_SET);
    ct.count = lc+1;
    write(file, &ct, sizeof(ct));
    close(file);

    char readBuffer[4096], writeBuffer[4096];
    struct LoanDetails ld;
    int file1 = open(LOANPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    
    int amount;

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter Loan Amount: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    lseek(file1, 0, SEEK_END);
    read(connectionFD, readBuffer, sizeof(readBuffer));
    
    amount = atoi(readBuffer);
    printf("Applied loan of %d from acc no.: %d\n", amount, accountNumber);

    ld.empID = -1;
    ld.accountNumber = accountNumber;
    ld.loanAmount = amount;
    ld.status = 0; // Requested
    ld.loanID = lc+1;

    int response = write(file1, &ld, sizeof(ld));
    close(file1);
    if(response <= 0)
    {
        bzero(readBuffer, sizeof(readBuffer));
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Unable to apply for loan!^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
    }
    else
    {        
        bzero(readBuffer, sizeof(readBuffer));
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Loan applied^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));
    }
    return;
}

// ======================= Money Transfer =======================
void transferFunds(int connectionFD, int sourceAccount, int destAccount, float amount) {
    char readBuffer[4096], writeBuffer[4096], transactionBuffer[1024];

    struct Customer cs;
    struct trans_histroy th;
    int isTransfer = 0;

    time_t s, val = 1;
	struct tm* current_time;
	s = time(NULL);
	current_time = localtime(&s);

    int file = open(CUSPATH, O_CREAT | O_RDWR, 0644);
    int fp = open(HISTORYPATH, O_RDWR | O_APPEND | O_CREAT, 0644);
    lseek(fp, 0, SEEK_END);
  
    int sourceFound = 0, destFound = 0;
    int srcOffset = -1, dstOffset = -1;

    while (read(file, &cs, sizeof(cs)) != 0)
    {
        if(cs.accountNumber == sourceAccount)
        {
            srcOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            read(file, &cs, sizeof(cs));
            sourceFound = 1;
        }
        if(cs.accountNumber == destAccount)
        {
            dstOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            read(file, &cs, sizeof(cs));
            destFound = 1;
        }

        if(sourceFound && destFound)
            break;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    lseek(file, srcOffset, SEEK_SET);
    read(file, &cs, sizeof(cs));

    if (cs.balance < amount) {
        bzero(writeBuffer, sizeof(writeBuffer));
        bzero(readBuffer, sizeof(readBuffer));
        printf("Insufficient funds\n");
        strcpy(writeBuffer, "Insufficient funds in the source account.^");
        write(connectionFD, writeBuffer, sizeof(writeBuffer));
        read(connectionFD, readBuffer, sizeof(readBuffer));

        fl1.l_type = F_UNLCK;
        fcntl(file, F_SETLK, &fl1);
        close(file);
        return;
    }

    printf("Current balance: %.2f\n", cs.balance);
    cs.balance -= amount; // Deduct from source account
    float srcBalance = cs.balance;

    bzero(transactionBuffer, sizeof(transactionBuffer));
    printf("%.2f transferred into acc no %d from acc no %d\n", amount, destAccount, sourceAccount);
    sprintf(transactionBuffer,"%.2f transferred into acc no %d at %02d:%02d:%02d %d-%d-%d", amount, destAccount, current_time->tm_hour,current_time->tm_min,current_time->tm_sec,(current_time->tm_year)+1900,(current_time->tm_mon)+1,current_time->tm_mday);
    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = sourceAccount;
    write(fp, &th, sizeof(th));

    lseek(file, -sizeof(struct Customer), SEEK_CUR);
    write(file, &cs, sizeof(cs));

    // Locking Destination Account
    struct flock fl2 = {F_WRLCK, SEEK_SET, dstOffset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl2);

    lseek(file, dstOffset, SEEK_SET);
    read(file, &cs, sizeof(cs));
    
    cs.balance += amount;

    lseek(file, -sizeof(struct Customer), SEEK_CUR);
    write(file, &cs, sizeof(cs));

    fl1.l_type = F_UNLCK;
    fl1.l_whence = SEEK_SET;
    fl1.l_start = srcOffset;
    fl1.l_len = sizeof(struct Customer);
    fl1.l_pid = getpid();

    fcntl(file, F_UNLCK, &fl1);

    fl2.l_type = F_UNLCK;
    fl2.l_whence = SEEK_SET;
    fl2.l_start = dstOffset;
    fl2.l_len = sizeof(struct Customer);
    fl2.l_pid = getpid();

    fcntl(file, F_UNLCK, &fl2);

    bzero(transactionBuffer, sizeof(transactionBuffer));
    sprintf(transactionBuffer,"%.2f credited by acc no %d at %02d:%02d:%02d %d-%d-%d", amount, sourceAccount, current_time->tm_hour,current_time->tm_min,current_time->tm_sec,(current_time->tm_year)+1900,(current_time->tm_mon)+1,current_time->tm_mday);
    bzero(th.hist, sizeof(th.hist));
    strcpy(th.hist, transactionBuffer);
    th.acc_no = destAccount;
    write(fp, &th, sizeof(th));

    close(file);

    bzero(writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    printf("Current balance of acc no %d: %.2f\n", sourceAccount, srcBalance);
    sprintf(writeBuffer, "Current Balance of %d is: %.2f^", sourceAccount, srcBalance);
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    return;
}

// ======================= View Transaction History =======================
void transactionHistory(int connectionFD, int accountNumber){
    char tempBuffer[4096];
    struct trans_histroy th;
    int maxTrans = 0;

    int file = open(HISTORYPATH, O_RDONLY | O_CREAT, 0644);

    bzero(writeBuffer, sizeof(writeBuffer));
    while(read(file, &th, sizeof(th)) != 0)
    {
        if(th.acc_no == accountNumber && maxTrans < 10)
        {
            maxTrans++;
            bzero(tempBuffer, sizeof(tempBuffer));
            strcpy(tempBuffer, th.hist);
            strcat(writeBuffer, tempBuffer);
        }
    }
    close(file);

    bzero(readBuffer, sizeof(readBuffer));
    strcat(writeBuffer, "^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ======================= Add Feedback =======================
void addFeedback(int connectionFD){
    
    struct FeedBack fb;

    int file = open(FEEDPATH, O_CREAT | O_RDWR | O_APPEND, 0644);
    if(file == -1)
    {
        printf("Error in opening file\n");
    }

    int choice;
    bzero(writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    strcpy(writeBuffer, "Enter Feedback:\n1. Good\n2. Bad\n3. Worse\n");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    
    read(connectionFD, readBuffer, sizeof(readBuffer));
    choice = atoi(readBuffer);

    if(choice == 1)
    {
        strcpy(fb.feedback, "Nice");
    }
    else if(choice == 2)
    {
        strcpy(fb.feedback, "Needs Improvement");
    }
    else
    {
        strcpy(fb.feedback, "Not at all good");
    }
    write(file, &fb, sizeof(fb));
    close(file);

    bzero(writeBuffer, sizeof(writeBuffer));
    bzero(readBuffer, sizeof(readBuffer));
    strcpy(writeBuffer, "^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ======================= Change Password =======================
int changePassword(int connectionFD, int accountNumber){
    char readBuffer[4096], writeBuffer[4096];

    char newPassword[20];

    struct Customer c;
    int file = open(CUSPATH,  O_CREAT | O_RDWR, 0644);
    
    lseek(file, 0, SEEK_SET);

    int srcOffset = -1, sourceFound = 0;

    while (read(file, &c, sizeof(c)) != 0)
    {
        if(c.accountNumber == accountNumber)
        {
            srcOffset = lseek(file, -sizeof(struct Customer), SEEK_CUR);
            sourceFound = 1;
        }
        if(sourceFound)
            break;
    }

    struct flock fl1 = {F_WRLCK, SEEK_SET, srcOffset, sizeof(struct Customer), getpid()};
    fcntl(file, F_SETLKW, &fl1);

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "Enter password: ");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));
    
    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
    strcpy(newPassword, readBuffer);

    strcpy(c.password, crypt(newPassword, HASHKEY));
    write(file, &c, sizeof(c));

    fl1.l_type = F_UNLCK;
    fl1.l_whence = SEEK_SET;
    fl1.l_start = srcOffset;
    fl1.l_len = sizeof(struct Customer);
    fl1.l_pid = getpid();

    fcntl(file, F_UNLCK, &fl1);
    close(file);

    printf("Customer %d changed password\n", accountNumber);
    return 1;
}

// ======================= Logout =======================
void logout(int connectionFD, int id){

    // struct session s;

    // int fd = open("../Data/temp.txt", O_CREAT | O_RDWR, 0644);
    // int fd1 = open("../Data/login.txt", O_RDONLY);

    // lseek(fd, 0, SEEK_SET);
    // lseek(fd1, 0, SEEK_SET);
    // while (read(fd1, &s, sizeof(s)) != 0)
    // {
    //     if(s.id != id)
    //     {
    //         write(fd, &s, sizeof(s));
    //     }
    // }
    
    // close(fd);
    // close(fd1);
    // remove("../Data/login.txt");
    // rename("../Data/temp.txt", "../Data/login.txt");

    snprintf(semName, 50, "/sem_%d", id);

    sem_t *sema = sem_open(semName, 0);
    if (sema != SEM_FAILED) {
        sem_post(sema);
        sem_close(sema); 
        sem_unlink(semName);    
    }

    bzero(writeBuffer, sizeof(writeBuffer));
    strcpy(writeBuffer, "^");
    write(connectionFD, writeBuffer, sizeof(writeBuffer));

    bzero(readBuffer, sizeof(readBuffer));
    read(connectionFD, readBuffer, sizeof(readBuffer));
}

// ======================= Session Check ==================
// int sessionCheck(int id)
// {
//     struct session s;
//     int fd = open("../Data/login.txt", O_CREAT | O_RDWR, 0644);
//     if(fd == -1)
//     {
//         printf("Unable to open login.txt file\n");
//         return 0;
//     }

//     while (read(fd, &s, sizeof(s)) != 0)
//     {
//         if(s.id == id)
//         {
//             close(fd);
//             return 0;
//         }
//     }
//     lseek(fd, 0, SEEK_END);
//     s.id = id;
//     write(fd, &s, sizeof(s));
//     close(fd);
//     return 1;
// }

#endif
