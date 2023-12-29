
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <stdio.h>  /* for puts() */
#include <string.h> /* for memset() */
#include <unistd.h> /* for sleep() */
#include <stdlib.h> /* for EXIT_SUCCESS */

#include <signal.h> /* for `struct sigevent` and SIGEV_THREAD */
#include <time.h>   /* for timer_create(), `struct itimerspec`,
                     * timer_t and CLOCK_REALTIME 
                     */
char kbhit(void);
struct gpiod_line *lineRed;
void init_timer(void);
char kbhit(void);
int fd,c;
char a[5];

                     
   timer_t timerid;
   struct sigevent sev;
   struct itimerspec trigger;
 pthread_cond_t cv;
pthread_mutex_t lock;

void *thread(void *v) {
	for(;;){
	
	pthread_mutex_lock(&lock);
	pthread_cond_wait(&cv, &lock);
	pthread_mutex_unlock(&lock);
	    
		fd=open("adc/in_voltage5_raw",O_RDONLY);
		read(fd,&a,sizeof(a));
		c=atoi(a);
		printf( "the value is %d \n",c);
		close(fd);

}
	return NULL;
}

void Tstimer_thread(union sigval sv) 
{
		pthread_mutex_lock(&lock);
		pthread_cond_signal(&cv);
		pthread_mutex_unlock(&lock);
        
		
        /* Will print "100ms elapsed." ! Avode printing when in RT*/
        puts("100ms elapsed.");
		
        timer_settime(timerid, 0, &trigger, NULL);
}

int main(void) 
{

		init_timer();
        /* Wait 10 seconds under the main thread. When the timer expires,
         * a message will be printed to the standard output by the newly
         * created notification thread.
         */
		pthread_t *t;
		t = (pthread_t *) malloc(sizeof(pthread_t));
		pthread_create(t, NULL, thread, NULL);	
		
        while(kbhit()!='q');
        

        /* Delete (destroy) the timer */
        timer_delete(timerid);
        free(t);

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
        trigger.it_value.tv_nsec = 100000000;  // 100ms

        /* Arm the timer. No flags are set and no old_value will be retrieved.
         */
        timer_settime(timerid, 0, &trigger, NULL);
        
}
