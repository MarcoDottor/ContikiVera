#include <stdio.h>
#include <string.h>
#include <malloc.h>
#define X 45458163
#define Y 9174229
#define distX 1
#define distY 1

float val;
int xVal, yVal;

void move();
char* replace_char(char* str, char find, char replace);

void main(void) {
	printf("SSS");
    FILE *toRead, *transformRead, *toWrite;
    char* toReadName= "C:\\Users\\mvv12\\Downloads\\Telegram Desktop\\mobile_trace.txt";
    char* transformReadName= "C:\\Users\\mvv12\\Downloads\\Telegram Desktop\\mobile_traceTransform.txt";
    char* toWriteName= "C:\\Users\\mvv12\\Downloads\\Telegram Desktop\\mobile_tracePROVA.txt";

    toRead= fopen(toReadName,"r");
    if(toRead==NULL) printf("NULL");
    transformRead= fopen(transformReadName,"w");
    toWrite= fopen(toWriteName,"w");

    xVal= X;
    yVal=Y;
    
    int i=0;

    while(!feof(toRead)){
    	char *str= malloc (20* sizeof (char));
    	fgets(str, 20, toRead);
    	str=replace_char(str, ',', '.');
    	fprintf(transformRead, "%s",str);
    }
    fclose(toRead);
    fclose(transformRead);
    FILE *toReadRight;
    toReadRight= fopen(transformReadName, "r");
    while(!feof(toReadRight)){    	
        fscanf(toReadRight, "%f", &val);
        move();
        printf("Fuori: %d %d\n\n", xVal, yVal);
    float xV=xVal*0.000001 ,yV=yVal*0.000001;
    printf("x:  %f     y:   %f",xV, yV);
        fprintf(toWrite, "%f %f %f", val, xV, yV);
        printf("\n%d\n",i++);
	}
}

void move(){
	printf("Prima: %d %d\n",xVal, yVal);
    int randomNum= rand()%3;
    if(randomNum==1)    {xVal+=distX;}
    else if(randomNum==2)   {xVal-=distX;}
    randomNum= rand()%3;
    if(randomNum==1)    yVal+=distY;
    else if(randomNum==2)   yVal-=distY;
	printf("Dopo: %d %d\n",xVal, yVal);
}

char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

