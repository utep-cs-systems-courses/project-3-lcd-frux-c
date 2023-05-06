#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include <stdlib.h>
#include <stdio.h>

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
#define PLAY_HISTORY 5

int redrawScreen = 1;
volatile int wdt_timer = 0;
char game_started = 0;
char game_active = 0;
volatile int rand_post_timer = 0;
volatile int delay_timer = 0;
int playCounter = 0;
int playMs[PLAY_HISTORY] = {0};

void add_play_history();
void print_play_history();
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
		P1DIR |= LED;
		P1OUT |= LED;
		clearScreen(COLOR_BLACK);
		drawString5x7(5,130,"Game Started,",COLOR_WHITE,BG_COLOR);
		drawString5x7(5,140,"Press BTN2 on green.",COLOR_WHITE,BG_COLOR);
	}
	if(p2val & BIT1){
		P1DIR &= ~LED;
		P1OUT &= ~LED;
		if(game_started & game_active){
			int caught_time = delay_timer;
			add_play_history(caught_time * 4); // 1 tick is 4 ms	
		}
		game_started = game_active = painted = painted1 = delay_timer = 0;
		clearScreen(COLOR_BLACK);
		drawString5x7(5,150,"Idle",COLOR_WHITE,BG_COLOR);
		// TODO : implement stat and history viewer
  }
	if(p2val & BIT2){
		if(~game_started){
			print_play_history();
		}
	}
}

void add_play_history(int ms){
		int min = ++playCounter<PLAY_HISTORY?playCounter:PLAY_HISTORY;
		for(int i = min-1; i >= 1; i--){
			playMs[i] = playMs[i-1];
		}
		playMs[0] = ms;
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

void print_play_history(){
	clearScreen(COLOR_BLACK);
	char header[12];
	sprintf(header,"Last %d Plays",PLAY_HISTORY);
	drawString5x7(10,10,header,COLOR_RED,BG_COLOR);
	for(int i = 0; i < PLAY_HISTORY; i++){
		if(!playMs[i]) break;
		char str[6];
		sprintf(str,"%d. %dms",i+1,playMs[i]);
		drawString5x7(10,30 + (i*10),str,COLOR_GREEN,BG_COLOR);
	}
}
void main()
{
  configureClocks();
  lcd_init();
  switch_init();
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x18);	              /**< GIE (enable interrupts) */
  
  clearScreen(COLOR_BLACK);
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
