

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

char kbhit(void);
int i=0;

int main(int argc, char **argv)
{
	const char *chipname ="gpiochip2";
	struct gpiod_chip *chip;
	struct gpiod_line *lineRed;
	
		// Open GPIO 
		chip = gpiod_chip_open_by_name(chipname);
		// Open GPIO lines
		lineRed = gpiod_chip_get_line(chip, 3);
		//lineGreen = gpiod_chip_get_line(chip, 25);
		//lineYellow = gpiod_chip_get_line(chip, 5);
		//lineButton = gpiod_chip_get_line(chip, 6);
		// Open LED lines for output
		gpiod_line_request_output(lineRed, "led", 0);
		//gpiod_line_request_output(lineGreen, "example1", 0);
		//gpiod_line_request_output(lineYellow, "example1", 0);
		// Open switch line for input
		//gpiod_line_request_input(lineButton, "example1");
		// Blink LEDs in a binary pattern
		
		
		while (kbhit() != 'q') 
		{
			gpiod_line_set_value(lineRed, (i & 1) != 0);
			sleep(1);
			i++;
	    }
	    gpiod_line_release(lineRed);
	    gpiod_chip_close(chip);
        return 0;
}
