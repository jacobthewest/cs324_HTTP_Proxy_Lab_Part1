/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

int main()
{
    char host[100];
    char port[40];
    char theUrl[100];
    char uri[100];
    strcpy(theUrl, "http://www.example.com:8080/index.html");
    char bigArray[100];
    strcpy(bigArray, theUrl);
    char* restOfTheRequest = bigArray;

    char* tempString = strtok_r(restOfTheRequest, "/", &restOfTheRequest); // tempString now equals http:  www.example.com:8080 index.html
    restOfTheRequest = restOfTheRequest + 1;
    if(strstr(tempString, "http:") != NULL) {
        tempString = strtok_r(restOfTheRequest, " ", &restOfTheRequest); 
    }
    
    char anotherBigArray[100];
    strcpy(anotherBigArray, tempString);
    restOfTheRequest = anotherBigArray; // restOfTheRequest = www.example.com:8080/index.html
    tempString = strtok_r(restOfTheRequest, "/", &restOfTheRequest);
    
    char dummyArray[200];
    strcpy(dummyArray, tempString);
    restOfTheRequest = dummyArray;
    if(strstr(tempString, ":")) { // Port is in the hostname
        tempString = strtok_r(restOfTheRequest, "/", &restOfTheRequest);
        strcpy(host, tempString);
        tempString = strtok_r(restOfTheRequest, "\0", &restOfTheRequest);
        fprintf(stdout, "Printing: \n");
        tempString[strlen(tempString) - 1] = "\0";
        fprintf(stdout, "%s\n", tempString);
        fprintf(stdout, "%s\n", restOfTheRequest);
        fflush(stdout);
        strcpy(uri, tempString);
    } else { // No port in the hostname
        strcpy(port, "80");
    }

    return 0;
}