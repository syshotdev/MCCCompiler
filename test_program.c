#include <stdio.h>
#include <stdlib.h>
// Example file for testing my Abstract Syntax Tree designs
typedef struct{
  int date;
  char *color;
  struct car *next; // The next one as in the next one in traffic
} car;


int main(){
  car *ford = malloc(sizeof(car));
  ford->date = 1996;
  ford->color = "red";
  ford->next = NULL;

  printf("Car date: %d\nCar color:%s", ford->date, ford->color);

  return 0;
}
