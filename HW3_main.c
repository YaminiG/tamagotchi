

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <stdlib.h>
#include <LED_HAL.h>
#include <Timer_HAL.h>


#define CHILD_YEARS 1
#define ADULT_YEARS 12

#define RADIUS_EGG 10
#define RADIUS_CHILD 11
#define RADIUS_ADULT 13

#define HALF_CHILD 6
#define THIRD_CHILD 4
#define HALF_ADULT 7
#define THIRD_ADULT 4

#define RADIUS_3 3
#define RADIUS_2 2
#define RADIUS_1 1

#define COL_0 16
#define COL_1 32
#define COL_2 48

#define ROW_0 32
#define ROW_1 48
#define ROW_2 64

#define CENTER_X 64
#define CENTER_Y 64
#define LEFT_X 32
#define RIGHT_X 96
#define LEFT_OF_CENTER_X 56
#define RIGHT_OF_CENTER_X 72

#define EGG_PLAY_TARGET 20
#define PLAY_TARGET 10

// Global parameters with current application settings

typedef enum {egg, child, adult} age_t;
typedef enum {black, red, green, yellow, blue, magenta, cyan, white} color_t;
typedef enum {baud9600, baud19200, baud38400, baud57600} UARTBaudRate_t;

UARTBaudRate_t currentBaudRate;
extern Graphics_Image tomb11BPP_COMP_RLE4;
unsigned n_char_rec;

typedef struct Tamagotchi
{
    age_t age;
    int years;
    int x;
    int y;
    int energy;
    int happy;
    int playCount;
} Tamagotchi;

Tamagotchi game_tama;


//-----------------------------------------------------------------------
// Character Graphics API
//
// The 128*128 pixel screen is partitioned in a grid of 8 rows of 16 characters
// Each character is a plotted in a rectangle of 8 pixels (wide) by 16 pixels (high)
//
// The lower-level graphics functions are taken from the Texas Instruments Graphics Library
//
//            C Application        (this file)
//                   |
//                 GRLIB           (graphics library)
//                   |
//             CrystalFontz Driver (this project, LcdDriver directory)
//                   |
//                font data        (this project, fonts directory)

Graphics_Context g_sContext;

void InitGraphics() {
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    Graphics_initContext(&g_sContext,
                         &g_sCrystalfontz128x128,
                         &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_ROYAL_BLUE);
    GrContextFontSet(&g_sContext, &g_sFontCmtt16);
    Graphics_clearDisplay(&g_sContext);
}

void LCDClearDisplay() {
    Graphics_clearDisplay(&g_sContext);
}

void LCDDrawChar(unsigned row, unsigned col, int8_t c) {
    Graphics_drawString(&g_sContext,
                        &c,
                        1,
                        8 * (col % 16),
                        16 * (row % 8),
                        OPAQUE_TEXT);
}

//------------------------------------------
// UART API
//
// We are using EUSCI_A0, which is a user UART in MSP432 that is accessible
// on your laptop as 'XDS 110 Class Application/User UART'. It usually shows up as COM3.
//
// The low-level UART functions are taken from the MSP432 Driverlib, Chapter 24.
//
// The Baud Rate Computation Procedure is taken from the
// User Guide MSP432P4 Microcontroller, Chapter 24, page 915
//
//  Baud rate computation:
//  - System Clock SMCLK in MSP432P4 is 3MHz (Default)
//  - Baud Rate Division Factor N = 3MHz / Baudrate
//      Eg. N9600 = 30000000 / 9600 = 312.5
//  - If N>16 -> oversampling mode
//      Baud Rate Divider              UCBF  = FLOOR(N/16)     = INT(312.5/16)     = 19
//      First modulation stage select  UCBRF = FRAC(N/16) * 16 = FRAC(312.5/16)*16 = 8


eUSCI_UART_Config uartConfig = {
     EUSCI_A_UART_CLOCKSOURCE_SMCLK,               // SMCLK Clock Source = 3MHz
     19,                                           // UCBR   = 19
     8,                                            // UCBRF  = 8
     0xAA,                                         // UCBRS  = 0xAA
     EUSCI_A_UART_NO_PARITY,                       // No Parity
     EUSCI_A_UART_LSB_FIRST,                       // LSB First
     EUSCI_A_UART_ONE_STOP_BIT,                    // One stop bit
     EUSCI_A_UART_MODE,                            // UART mode
     EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION // Oversampling
};

void InitUART() {
    UART_initModule(EUSCI_A0_BASE, &uartConfig);
    UART_enableModule(EUSCI_A0_BASE);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
        GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
}

void UARTSetBaudRate(UARTBaudRate_t t) {
    currentBaudRate = t;
    switch (currentBaudRate){
    case baud9600:
        uartConfig.clockPrescalar = 19;
        uartConfig.firstModReg = 8;
        uartConfig.secondModReg = 0xAA;
        break;
    case baud19200:
        uartConfig.clockPrescalar = 9;
        uartConfig.firstModReg = 12;
        uartConfig.secondModReg = 0x44;
        break;
    case baud38400:
        uartConfig.clockPrescalar = 4;
        uartConfig.firstModReg = 14;
        uartConfig.secondModReg = 0x10;
        break;
    case baud57600:
        uartConfig.clockPrescalar = 3;
        uartConfig.firstModReg = 4;
        uartConfig.secondModReg = 0x04;
        break;
    }

    InitUART();
}

void UARTToggleBaudRate() {
    switch (currentBaudRate){
    case baud9600:
        UARTSetBaudRate(baud19200);
        break;
    case baud19200:
        UARTSetBaudRate(baud38400);
        break;
    case baud38400:
        UARTSetBaudRate(baud57600);
        break;
    case baud57600:
        UARTSetBaudRate(baud9600);
        break;
    }
}

bool UARTHasChar() {
    return (UART_getInterruptStatus (EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
                == EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);
}

uint8_t UARTGetChar() {
    if (UARTHasChar())
        return UART_receiveData(EUSCI_A0_BASE);
    else
        return 0;
}

bool UARTCanSend() {
    return (UART_getInterruptStatus (EUSCI_A0_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG)
                == EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG);
}

void UARTPutChar(uint8_t t) {
    while (!UARTCanSend()) ;
    UART_transmitData(EUSCI_A0_BASE,t);
}

//------------------------------------------
// Red LED API
void InitRedLED() {
    GPIO_setAsOutputPin    (GPIO_PORT_P1,    GPIO_PIN0);   // red LED on Launchpad
    GPIO_setOutputLowOnPin (GPIO_PORT_P1,    GPIO_PIN0);
}

void RedLEDToggle() {
    GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
}

//------------------------------------------
// Display API
void ClearDisplay() {
    LCDClearDisplay();
}

//-----------------------------------------------------------------------
// TAMAGOTCHI FUNCTIONS

// Egg Drawing
void DrawTamagotchiEgg(){
   Graphics_fillCircle(&g_sContext, 63, 63, 15);
   Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW_GREEN);
   Graphics_fillCircle(&g_sContext,52,61,4);
   Graphics_fillCircle(&g_sContext,68,71,4);
   Graphics_fillCircle(&g_sContext,65,51,4);





}

//Child Drawing Functions
void DrawChildBody(){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_YELLOW_GREEN);
    Graphics_fillCircle(&g_sContext, 63, 63, 15);





}

void DrawChildEars(){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DARK_SEA_GREEN);
    Graphics_fillCircle(&g_sContext, 48, 63, 3);
    Graphics_fillCircle(&g_sContext, 78, 63, 3);


    }



void DrawChildEyes(){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_fillCircle(&g_sContext,58 , 60, 3);
    Graphics_fillCircle(&g_sContext,68 , 60, 3);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_fillCircle(&g_sContext,58 , 60, 1);
    Graphics_fillCircle(&g_sContext,68 , 60, 1);












}
void DrawChildMouth(){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DEEP_PINK);
    Graphics_Rectangle mouth = {60,
                                   70,
                                   67,
                                   72};
    Graphics_fillRectangle(&g_sContext, &mouth);





}

void DrawChildArms(){

}

void DrawChildLegs(){

}

void DrawTamagotchiChild(){
    DrawChildBody();
    DrawChildEars();
    DrawChildEyes();
    DrawChildMouth();
    DrawChildArms();
    DrawChildLegs();
}

// Adult Drawaing Functions
void DrawAdultBody(){

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_LAWN_GREEN);
       Graphics_fillCircle(&g_sContext, 63, 63, 20);


}

void DrawAdultEars(){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GOLDENRON );
              Graphics_fillCircle(&g_sContext, 39, 63, 4);
              Graphics_fillCircle(&g_sContext, 87, 63, 4);

}

void DrawAdultEyes(){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
          Graphics_fillCircle(&g_sContext,58 , 60, 4);
          Graphics_fillCircle(&g_sContext,68 , 60, 4);
          Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
          Graphics_fillCircle(&g_sContext,58 , 60, 2);
          Graphics_fillCircle(&g_sContext,68 , 60, 2);
}

void DrawAdultMouth(){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_DEEP_PINK);
          Graphics_Rectangle mouth = {60,
                                         72,
                                         67,
                                         74};
          Graphics_fillRectangle(&g_sContext, &mouth);

}

void DrawAdultArms(){

}

void DrawAdultLegs(){

}

DrawTamagotchiAdult(){
    DrawAdultBody();
    DrawAdultEars();
    DrawAdultEyes();
    DrawAdultMouth();
    DrawAdultArms();
    DrawAdultLegs();
}

void DrawMeterNames(){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_ROYAL_BLUE);

    char text[16] = "";

    sprintf(text, "Age:");
    Graphics_drawString(&g_sContext, (int8_t *) text, -1, 0, 0, true);
    sprintf(text, "Energy:");
    Graphics_drawString(&g_sContext, (int8_t *) text, -1, 0, 96, true);
    sprintf(text, "Happy:");
    Graphics_drawString(&g_sContext, (int8_t *) text, -1, 0, 112, true);
}

void WriteAge(){
char text[16] = "";
if(game_tama.age==egg){
    sprintf(text, "Egg");
    Graphics_drawString(&g_sContext, (int8_t *) text, -1, 50, 0, true);
}
else if(game_tama.age==child){
    sprintf(text, "Child");
        Graphics_drawString(&g_sContext, (int8_t *) text, -1, 50, 0, true);
}
else if(game_tama.age==adult){
    sprintf(text, "Adult");
        Graphics_drawString(&g_sContext, (int8_t *) text, -1, 50, 0, true);
}


}

void WriteEnergyMeter(){
    char text[16] = "";
    int num = game_tama.energy;
    int anchor = 60;
    int i = 0;
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_ROYAL_BLUE);
            sprintf(text,"*****    ");
            Graphics_drawString(&g_sContext, (int8_t *) text, -1, 60, 96, true);
            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    for(i = 0;i<num;i++){
        sprintf(text, "*");
        Graphics_drawString(&g_sContext, (int8_t *) text, -1, anchor, 96, true);
        anchor = anchor + 10;

    }


}

void WriteHappyMeter(){
    char text[16] = "";

    int num = game_tama.happy;
    int anchor = 60;
        int i = 0;
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_ROYAL_BLUE);
                   sprintf(text,"*****   ");
                   Graphics_drawString(&g_sContext, (int8_t *) text, -1, 60, 112, true);
                   Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        for(i = 0;i<num;i++){

            sprintf(text, "*");
            Graphics_drawString(&g_sContext, (int8_t *) text, -1, anchor, 112, true);
            anchor = anchor + 10;

        }

}

//Draw the tamagotchi with the correct age
void DrawTamagotchi(){
    if (game_tama.age == egg){
        DrawTamagotchiEgg();
    }
    else if (game_tama.age == child){
        DrawTamagotchiChild();
    }
    else if (game_tama.age == adult){
        DrawTamagotchiAdult();
    }
}

void EraseTamagotchi(){
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_ROYAL_BLUE);
      // Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_ROYAL_BLUE);

    Graphics_fillCircle(&g_sContext, 63, 63, 35);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);



}

DrawTombstone(){
    Graphics_drawImage(&g_sContext, &tomb11BPP_COMP_RLE4, 0, 0);

}

EngraveTombstone(){

}

//Clear and redraw the whole display
void UpdateWholeDisplay() {
    ClearDisplay();
    DrawTamagotchi();
    DrawMeterNames();
    WriteAge();
    WriteEnergyMeter();
    WriteHappyMeter();
}

void DecrementEnergy(){
    if (game_tama.energy > 0){
        game_tama.energy--;
        WriteEnergyMeter();
    }
}

void DecrementHappy(){
    if (game_tama.happy > 0){
        game_tama.happy--;
        WriteHappyMeter();
    }
    else
        DecrementEnergy();
}

void IncrementEnergy(){
    if (game_tama.energy < 5){
       game_tama.energy++;
        WriteEnergyMeter();
    }
}

void IncrementHappy(){
    if (game_tama.happy < 5){
        game_tama.happy++;
        WriteHappyMeter();
    }
}

JumpLeft(){
    if (game_tama.x == RIGHT_X){
        game_tama.playCount++;
        EraseTamagotchi();
        game_tama.x = CENTER_X;
        DrawTamagotchi();
    }
    else if (game_tama.x == CENTER_X){
        game_tama.playCount++;
        EraseTamagotchi();
        game_tama.x = LEFT_X;
        DrawTamagotchi();
    }
}

JumpRight(){
    if (game_tama.x == LEFT_X){
        game_tama.playCount++;
        EraseTamagotchi();
        game_tama.x = CENTER_X;
        DrawTamagotchi();
    }
    else if (game_tama.x == CENTER_X){
        game_tama.playCount++;
        EraseTamagotchi();
        game_tama.x = RIGHT_X;
        DrawTamagotchi();
    }
}

//grow older in years and potentially evolve to next age
void GrowOlder(){
    game_tama.years++;
    WriteAge();

    if (game_tama.age == egg) {
        if (game_tama.years > CHILD_YEARS){
            EraseTamagotchi();
            game_tama.age = child;
            game_tama.x = CENTER_X;
            DrawTamagotchi();
        }
    }
    else if (game_tama.age == child){
        if (game_tama.years > ADULT_YEARS && game_tama.energy > 3 && game_tama.happy > 3){
            EraseTamagotchi();
            game_tama.age = adult;
            game_tama.x = CENTER_X;
            DrawTamagotchi();
        }
    }
}

//Check if tamagotchi is alive
int IsAlive(){
    if (game_tama.energy==0 && game_tama.happy==0)
        return 0;
    else
        return 1;
}

//Game over screen
void DisplayDeathScreen(){



    DrawTombstone();
    EngraveTombstone();
}

void processChar(uint8_t c) {
    UARTPutChar(c);

    if (c == 'f' || c == 'F')
        IncrementEnergy();
    else if (c == 'p')
        JumpLeft();
    else if (c == 'P')
        JumpRight();

    if (game_tama.playCount >= PLAY_TARGET){
        IncrementHappy();
        DecrementEnergy();
        game_tama.playCount = 0;
    }
}

void SetupNewGame(){
    game_tama.age = egg;
    game_tama.years = 0;
    game_tama.x = CENTER_X;
    game_tama.y = CENTER_Y-4;
    game_tama.energy = 3;
    game_tama.happy = 3;
    game_tama.playCount = 0;

    UpdateWholeDisplay();
    StartOneShot10sTimer();
}

//-----------------------------------------------------------------------


int main(void) {
    uint8_t c;

    WDT_A_hold(WDT_A_BASE);

    InitGraphics();
    InitUART();
    InitLEDs();
    Init10sTimer();

    SetupNewGame();

    n_char_rec = 0;

    while (1) {

        if (UARTHasChar()) {

            c = UARTGetChar();
            n_char_rec++;
            if (game_tama.age > egg){
                processChar(c);
            }
            UARTPutChar(c);
        }

        if (OneShot10sTimerExpired()){
            if (IsAlive()){
                GrowOlder();

                if (game_tama.age > egg){
                    DecrementHappy();
                }
                StartOneShot10sTimer();
            }
            else{
                DisplayDeathScreen();
                while(1);
            }
        }
    }
}
