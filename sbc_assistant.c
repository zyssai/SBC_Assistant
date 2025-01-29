// install from github
// https://wiki.odroid.com/odroid-n2/application_note/gpio/wiringpi#tab__github_repository

// Remove overlays in /media/boot/config.ini if needed

// Compile with
// gcc -o sbc_assistant sbc_assistant.c $(pkg-config --cflags --libs libwiringpi2)

// Launch crontab -e
// Add the following to execute script every minute,
// save output to log, and remove old log every day at 22am.

// MAILTO=""
// * * * * * /home/odroid/sbc_assistant >> /home/odroid/sbc_assistant.log
// 0 22 * * * mv /home/zyssai/sbc_assistant/sbc_assistant.log /home/zyssai/sbc_assistant/sbc_assistant.log.old


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

  time_t rawtime;
  struct tm *timeinfo;
  char buffer[80];

  // Get the current time
  time(&rawtime);

  // Convert to local time (CET if the system is set correctly)
  timeinfo = localtime(&rawtime);

  // Format the time as "3 December 2024 15:30:45"
  strftime(buffer, sizeof(buffer), "%d %B %Y %H:%M:%S", timeinfo);

  // Display the formatted time with the gpio pin value
  printf("%s, Value of pin is %d\n", buffer, value);

  // Shutdown system if gpio falls to 0
  if (value == 0)
  {
    printf("shutdown!\n");
  // Flush output to ensure it is saved in log
    fflush(stdout);
    system("shutdown -P now");
  }
  return 0;
}
