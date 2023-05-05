#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include <stdlib.h>

#define NUM_POSITIONS 5
#define SW1 BIT0
#define SW2 BIT1

unsigned short sqColors[] = {COLOR_RED, COLOR_GREEN, COLOR_ORANGE, COLOR_BLUE};
#define NUM_SQCOLORS 4
#define BG_COLOR COLOR_BLACK
  
// WARNING: LCD DISPLAY USES P1.0.  Do not touch!!! 

#define LED BIT6		/* note that bit zero req'd for display */

#define SWITCHES 15
#define TIME_COUNT 250

int redrawScreen = 1;
volatile int wdt_timer = 0;
char game_started = 0;
char game_active = 0;
volatile int rand_post_timer = 0;
volatile int delay_timer = 0;

static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void 
switch_init()			/* setup switch */
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}
char painted = 0;
char painted1 = 0;
void
switch_interrupt_handler()
{
  char p2val = switch_update_interrupt_sense();
	p2val = ~p2val & SWITCHES;
	if(p2val & BIT0){
		game_started = 1;
		rand_post_timer = wdt_timer + TIME_COUNT + (rand() % TIME_COUNT);;
		fillRectangle(110,140,10,10,COLOR_GREEN);
		P1DIR |= LED;
		P1OUT |= LED;
	}
	if(p2val & BIT1){
		P1DIR &= ~LED;
		P1OUT &= ~LED;
		if(game_started){
			game_started = game_active = painted = painted1 = 0;
		}
		clearScreen(COLOR_BLACK);
		fillRectangle(110,140,10,10,COLOR_RED);
		// TODO : implement stat and history viewer
  }
}

void wdt_c_handler()
{
	if(game_started  & ~game_active){
		if(~painted){
			fillRectangle(55,70,10,10,COLOR_RED);
			painted = 1;
		}
	}
	if(game_started & game_active){
		if(~painted1){
			fillRectangle(55,70,10,10,COLOR_GREEN);
			painted1 = 1;
		}	
	}
}
  
void update_shape();

void main()
{
  configureClocks();
  lcd_init();
  switch_init();
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x18);	              /**< GIE (enable interrupts) */
  
  clearScreen(COLOR_BLACK);
}

    
    
void
update_shape()
{
	//fillRectangle(0,10,130,10,COLOR_RED);
	//fillRectangle(0,0,130,160,COLOR_RED);
}


/* Switch on S2 */
void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler();	/* single handler for all switches */
  }
}
/** WDT Handler */
void
__interrupt_vec(WDT_VECTOR) WDT(){
	if(game_started){
		wdt_timer++;
	}
	else{
		wdt_timer = 0;
	}
	if(wdt_timer == rand_post_timer){
		game_active = 1;
	}
	if(game_active){
		delay_timer++;
	}
	wdt_c_handler();
}
