
#define CompileDate __DATE__
#define CompileTime __TIME__
char CompileDateStamp[] = CompileDate;//This is how we get a compile date and time stamp into the program
char CompileTimeStamp[] = CompileTime;

char startMsg[] = "CRT_SCOPE (Ver_1.60) ";	//Program Revision Text


#include <Arduino.h>	//Provided as part of the Arduino IDE

#include <DueTimer.h>	//Timer library for DUE; download this library from the Arduino.org site
						//Timer library is also available from author at https://github.com/ivanseidel/DueTimer
						
//UNCOMMENT ONE OF THE FOLLOWING TWO INCLUDE STATEMENTS TO SET THE PATH TO THESE THE LIBRARY PROGRAMS

#include <XYscope.h>	//UNCOMMENT THIS LINE for PUBLIC XYscope Drivers & Graphics Functions for Arduino Graphics Engine (AGI)
						//Use this INCLUDE if you store XYscope library routines inside of ...\Library\XYscope folder

//#include "XYscope.h"	//UNCOMMENT THIS LINE for PRIVATE XYscope Drivers & Graphics Functions for Arduino Graphics Engine (AGI)
						//Use this INCLUDE if you keep XYscope library routines in same directory as mainline.ino Directory

//Download this library from GitHub

XYscope XYscope;

// 	+---------Begin Critical Interrupt Service Routines ------------+
//	|  These routines MUST be declared as shown at the top of the	|
//	|      user's main line code for all XYscope Projects!			|
// 	+---------------------------------------------------------------+
//	|																|
//	V																V

void DACC_Handler(void) {
	//	DACC_Handler Interrupt Service Routine. This routine
	//	provides a 'wrapper' to link the AVR DAC INTERRUPT
	//	to the 'XYscope-class' ISR routine.
	XYscope.dacHandler();	//Link the AVR DAC ISR/IRQ to the XYscope.
							//It is called whenever the DMA controller
							//'DAC_ready_for_More_data' event occurs
}

void paintCrt_ISR(void) {
	//	paintCrtISR  Interrupt Service Routine. This routine
	//	provides a 'wrapper' to link the Timer3.AttachedInterrupt()
	//	function to the 'XYscope-class' ISR routine.
	XYscope.initiateDacDma();	//Start the DMA transfer to paint the CRT screen
}

//	V																V
//	|																|
// 	+---------- END Critical Interrupt Service Routines ------------+

//	Define/initialize critical global constants and variables

double dacClkRateHz, dacClkRateKHz;

int EndOfSetup_Ptr;
double TimeForRefresh;


char shiftVal = 0;
uint32_t dispTimer = 0;
int MovingX = 2048, MovingY = 2048;


int enabSecondHand;	//Flag to turn "Radar Scope clock Second-hand" demo feature on/off
					//0=Disable second-hand animation, <>0=ENABLE second-hand animation


float angle = 0;

#include <malloc.h>	//Required for RAM usage monitoring routines

// The following lines are used for RAM monitoring routines
extern char _end;
extern "C" char *sbrk(int i);
char *ramstart = (char *) 0x20070000;
char *ramend = (char *) 0x20088000;


void setup() {
	// Mainline program SETUP routine.
	//
	//	Passed Parameters	NONE
	//
	//	Returns: NOTHING
	//
	//	20170708 Ver 0.0	E.Andrews	First cut
	//						(Updated throughout development cycle without version change)

	//Set Serial Monitor must be setup to agree with this initializtion.
	//  [X] Autoscroll
	//  "No Line Ending"
	//  "115200 baud"
	Serial.begin(115200);
	
	//Send startup messages out to Serial monitor port...
	Serial.println("");
	Serial.print(startMsg);
	Serial.print(" (");
	Serial.print(CompileDateStamp);
	Serial.print(" ");
	Serial.print(CompileTimeStamp);
	Serial.println(")");

	double DmaFreq=800000;		//800000 (Hz) is the default startup value for the DMA frequency.
								//You can try various alternate values to find an optimal
								//value that works best for your scope, setup, & application.
								//Use program menu option "c" & "C" to vary the frequency while
								//watching your scope display; Using "c" & "C" options will allow you
								//to see how frequency changes effect the display quality in real-time.

	XYscope.begin(DmaFreq);

	//Timer3 is used as the CRT refresh timer.  This timer is setup inside of XYscope.begin( ).
	//However, paintCRT_ISR must be "attached" to timer 3.  To be properly link to the
	//refresh-screen XYscope interupt service routine, it must be linked in the Arduino 
	//setup() code as follows:
	
	Timer3.attachInterrupt(paintCrt_ISR);

	//Here is just some stuff to paint onto CRT at startup
	//v----------BEGIN SETUP SPLASH SCREEN ---------------v
	// ArduinoSplash();				//Paint an Arduino logo
	// int xC = 1800, yC = 2800;		//Set values of XY center coordinates for start of text
	// int textSize = 400;				//Set Text Size (in pixels)
	// bool const UndrLined = true;	//Turn underline ON
	// int textBright = 150;
	// XYscope.printSetup(xC - 150, yC + 50 + 700, textSize, textBright);
	// XYscope.print((char *)"AGI", UndrLined);
	// xC = 100;
	// yC = 2900;
	// textSize = 250;
	// XYscope.printSetup(xC + 50, yC + 50 + textSize, textSize, textBright);
	// XYscope.setFontSpacing(XYscope.prop);			//Select Proportional Spacing
	// if (XYscope.getFontSpacing() != XYscope.mono)	//Adjust coordinate in currently active spacing mode is 'prop'
	// 	XYscope.printSetup(xC + 50 + 500, yC + 50 + textSize, textSize,
	// 			textBright);
	// XYscope.print((char *)"Arduino Graphics Interface", false);	//(false=No underline)
	// XYscope.setFontSpacing(XYscope.mono);

	// XYscope.autoSetRefreshTime();
	// XYscope.plotRectangle(0, 0, 4095, 4095);

	// XYscope.printSetup(350, 275, 175, 100);
	// XYscope.print(startMsg);XYscope.print((char *)" LibRev:");XYscope.print(XYscope.getLibRev(),2);

	// //Now put the compile date & time onto the screen
	// //The time and data stamp come from #define statements at the top of the program...
	// XYscope.printSetup(1100, 100, 150, 100);
	// XYscope.print((char *)"(");
	// XYscope.print(CompileDateStamp);
	// XYscope.print((char *)"  ");
	// XYscope.print(CompileTimeStamp);
	// XYscope.print((char *)")");

	// //^----------BEGIN SETUP SPLASH SCREEN ---------------^

	// //Send option menu out to PC via Serial.print
	// Serial.println();
	// Print_CRT_Scope_Menu();
	// PrintStatsToConsole();

}

void loop() {	

  XYscope.print((char *)"Arduino Graphics Interface", false);
	Serial.println(" DONE PeakToPeak_Horiz Sq Wave ");
}
