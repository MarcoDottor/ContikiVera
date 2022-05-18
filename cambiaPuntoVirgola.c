
#include <stdio.h>
#include <string.h>
#include <malloc.h>

float val;
int xVal, yVal;

void move();
char* replace_char(char* str, char find, char replace);
void foo(char* n1, char* n2);

void main(void) {
	foo("/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos2A","/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos2");
	foo("/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos3A","/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos3");
	foo("/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos4A","/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos4");
	foo("/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos5A","/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos5");
	/*foo();
	foo();
	foo();
	foo();*/
}

char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

void foo(char *s1, char *s2){	
    FILE *toRead, *toWrite;
    //char* toReadName= "/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos1A";
    //char* toWriteName= "/home/user/contiki-ng-mw-2122/examples/rpl-udp/valPos1";

   	char* str= malloc (40*sizeof (char));
	toRead= fopen(s1, "r");
	toWrite= fopen(s2, "w");

	while(!feof(toRead)){
		fgets(str, 40, toRead);
		str=replace_char(str, '.',',');
		fprintf(toWrite,"%s",str);
	}
	fclose(toRead);	fclose(toWrite);
}
