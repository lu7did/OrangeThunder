#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<fcntl.h> 
#include <sys/select.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/stat.h>

#include<sys/wait.h>
#include<sys/prctl.h>
#include <time.h> 

// --- IPC structures
//--------------------------------------------------------------------------------------------------
// writeQSO in log if available
//--------------------------------------------------------------------------------------------------
void writeLog() {


struct tm* local; 
    time_t t = time(NULL);
    // Get the localtime 
    local = localtime(&t);  
    printf("Local time and date: %s\n",asctime(local)); 


int hour = local->tm_hour;
int year = 1900+local->tm_year;
int month = local->tm_mon+1;
int mday = local->tm_mday;
int min = local->tm_min;

    printf("element %d %d %d %d %d\n",year,month,mday,hour,min);



}
// ======================================================================================================================
// MAIN
// Create IPC pipes, launch child process and keep communicating with it
// ======================================================================================================================
int main(int argc, char** argv)
{

    writeLog();

}
