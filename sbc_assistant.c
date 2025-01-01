// install from github
// https://wiki.odroid.com/odroid-n2/application_note/gpio/wiringpi#tab__github_repository

// Remove overlays in /media/boot/config.ini if needed

// Compile with
// gcc -o sbc_assistant sbc_assistant.c $(pkg-config --cflags --libs libwiringpi2)

// Launch crontab -e
// Add this to execute script every minute

// MAILTO=""
// * * * * * /home/odroid/sbc_assistant >> /home/odroid/sbc_assistant.log



// This script uses pin 12 (wiringPi 1)

#include <wiringPi.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main() {
  wiringPiSetup();
  pinMode(1, INPUT);
  int value = 0;
  value = digitalRead(1);
  time_t t;
  time(&t);
  printf("Value of pin is %d at %s", value, ctime(&t));
  if (value == 0)
  {
    printf("shutdown!\n");
    system("shutdown -P now");
  }
  return 0;
}
