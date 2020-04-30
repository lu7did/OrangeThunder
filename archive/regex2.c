#include<stdio.h>
// Importing the POSIX regex library
#include <regex.h> 
void print_result(int return_value){
  if (return_value == 0){
    printf("Pattern found.\n");
  }
  else if (return_value == REG_NOMATCH){
    printf("Pattern not found.\n");
  }
  else{
    printf("An error occured.\n");
  }
}
int main() {
  regex_t regex;
  //int return_value;
  //int return_value2;
  //return_value = regcomp(&regex,"ice",0);
  //return_value = regexec(&regex, "icecream", 0, NULL, 0);
  //return_value2 = regcomp(&regex,"ice",0);
  //return_value2 = regexec(&regex, "frozen yoghurt", 0, NULL, 0);
  //print_result(return_value);
  //print_result(return_value2);

  int rc;
  int e;

  rc=regcomp(&regex,"lu[123456789][abcde]",0);
  print_result(regexec(&regex,"lu2acb",0,NULL,0));
  print_result(regexec(&regex,"lu2hcb",0,NULL,0));
  print_result(regexec(&regex,"cx2acb",0,NULL,0));

  rc=regcomp(&regex,"^py",0);
  print_result(regexec(&regex,"py2acb",0,NULL,0));
  return 0;
}
