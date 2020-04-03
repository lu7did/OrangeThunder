#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

int main(int argc, char** argv)
{
  pid_t pid = 0;
  int inpipefd[2];
  int outpipefd[2];
  char buffer[2048];
  char msg[256];
  int status;

  pipe(inpipefd);
  pipe(outpipefd);
  pid = fork();
  if (pid == 0)
  {
    // Child
    dup2(outpipefd[0], STDIN_FILENO);
    dup2(inpipefd[1], STDOUT_FILENO);
    dup2(inpipefd[1], STDERR_FILENO);

    //ask kernel to deliver SIGTERM in case the parent dies
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    //replace tee with your process
    //execl("/bin/ls","ls -la more ", (char*) NULL);
    execl(getenv("SHELL"),"sh","-c","sudo /usr/local/bin/rtl_sdr -s 1200000 -f 14100000 -D 2 - | csdr convert_u8_f  csdr shift_addition_cc 0.021 | csdr fir_decimate_cc 25 0.05 HAMMING | csdr bandpass_fir_fft_cc 0 0.5 0.05 |  csdr realpart_cf | csdr agc_ff | csdr limit_ff | csdr convert_f_s16 | aplay -r 48000 -f S16_LE -t raw -c 1 ",NULL);
    // Nothing below this line should be executed by child process. If so, 
    // it means that the execl function wasn't successfull, so lets exit:
    exit(1);
  }
  // The code below will be executed only by parent. You can write and read
  // from the child using pipefd descriptors, and you can send signals to 
  // the process using its pid by kill() function. If the child process will
  // exit unexpectedly, the parent process will obtain SIGCHLD signal that
  // can be handled (e.g. you can respawn the child process).

  //close unused pipe ends
  close(outpipefd[0]);
  close(inpipefd[1]);

  // Now, you can write to outpipefd[1] and read from inpipefd[0] :  
  while(1)
  
  {

    int nread=read(inpipefd[0],buffer,1024);
    buffer[nread]=0x00;
    fprintf(stderr,"%s",(char*)buffer);
    //write(STDOUT_FILENO,buffer,nread);

        
    //#printf("Enter message to send\n");
    //#scanf("%s", msg);
    //#if(strcmp(msg, "exit") == 0) break;

    //#write(outpipefd[1], msg, strlen(msg));
    //#read(inpipefd[0], buf, 256);

    //#printf("Received answer: %s\n", buf);
  }

  kill(pid, SIGKILL); //send SIGKILL signal to the child process
  waitpid(pid, &status, 0);
}
