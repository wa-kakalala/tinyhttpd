#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){
    char * method = NULL;
    char *query_string = NULL;
    printf("Content-Type:text/html\r\n\r\n");

    method = getenv("REQUEST_METHOD");
    if(method == NULL){
        printf("<html> <p> UNKNOWN METHOD</p></html>");
        return;
    }
    if(strcasecmp(method,"POST") == 0){
        printf("<html> <p> UNSUPPORTED METHOD</p></html>");
        return 0;
    }
    query_string = getenv("QUERY_STRING");
    if(query_string == NULL){
        printf("<html> <p> PARAMETER ERROR </p></html>");
        return 0;
    } 
    int a = 0;
    int b = 0;
    int i = 0;
    char * start = query_string+2;
    char * end = NULL;
    // format: a=xxx&b=xxx
    for(i = 0;i<strlen(query_string);i++){
        if(*(query_string+i) == '&'){
            end = query_string+i;
            break;
        }
    }
    *end = '\0';
    a = atoi(start);
    start = end +3;
    b = atoi(start);
    printf("<html> <p> %d + %d = %d  </p></html>",a,b,a+b);
    return 0;
}
