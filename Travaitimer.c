//à tester sur BBB

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <gpiod.h>
#include <stdio.h>  /* for puts() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for sleep() */
#include <stdlib.h> /* for EXIT_SUCCESS */
#include <signal.h> /* for `struct sigevent` and SIGEV_THREAD */
#include <time.h>                       
	void init_timer(void);
	char kbhit(void);                   
	timer_t timerid;
   struct sigevent sev;
   struct itimerspec trigger;
   
   struct gpiod_chip *chip;
   struct gpiod_line *lineRed;
   int i = 0;    
   
void Tstimer_thread(union sigval sv) 
{
       
        //--------------------toggle GPIO
        gpiod_line_set_value(lineRed,i%2);
        i++;
        
		//------------------------------
		
        /* Will print "100ms elapsed." ! Avode printing when in RT*/
        puts("100ms elapsed.");
		//sleep(1);
        timer_settime(timerid, 0, &trigger, NULL);
}
void led(void)
   {
	const char *chipname ="gpiochip2";
	

		// Open GPIO 
		chip = gpiod_chip_open_by_name(chipname);
		// Open GPIO lines
		lineRed = gpiod_chip_get_line(chip, 3);
		//lineGreen = gpiod_chip_get_line(chip, 25);
		//lineYellow = gpiod_chip_get_line(chip, 5);
		//lineButton = gpiod_chip_get_line(chip, 6);
		// Open LED lines for output
		gpiod_line_request_output(lineRed,"Travaitimer", 0);
		//gpiod_line_request_output(lineGreen, "example1", 0);
		//gpiod_line_request_output(lineYellow, "example1", 0);
		// Open switch line for input
		//gpiod_line_request_input(lineButton, "example1");
		// Blink LEDs in a binary pattern
		
	    }
void clear()
	{
		gpiod_line_release(lineRed);
	    gpiod_chip_close(chip);
	}


int main(void) 
{
		led();
		init_timer();
        /* Wait 10 seconds under the main thread. When the timer expires,
         * a message will be printed to the standard output by the newly
         * created notification thread.
         */
        //sleep(10);
		while(kbhit()!='q');
		
			 /* Delete (destroy) the timer */
        timer_delete(timerid);
        clear();

        return EXIT_SUCCESS;
}

void init_timer(void)
{
        /* Set all `sev` and `trigger` memory to 0 */
        memset(&sev, 0, sizeof(struct sigevent));
        memset(&trigger, 0, sizeof(struct itimerspec));

        /* 
         * Set the notification method as SIGEV_THREAD:
         *
         * Upon timer expiration, `sigev_notify_function` (thread_handler()),
         * will be invoked as if it were the start function of a new thread.
         *
         */
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = &Tstimer_thread;
        
        /* Create the timer. In this example, CLOCK_REALTIME is used as the
         * clock, meaning that we're using a system-wide real-time clock for
         * this timer.
         */
        timer_create(CLOCK_REALTIME, &sev, &timerid);

        /* Timer expiration will occur withing 100ms seconds after being armed
         * by timer_settime().
         */
        trigger.it_value.tv_sec = 0;
        trigger.it_value.tv_nsec = 500000000;  // 500ms

        /* Arm the timer. No flags are set and no old_value will be retrieved.
         */
        timer_settime(timerid, 0, &trigger, NULL);
        
}
