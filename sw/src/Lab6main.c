// Lab6main.c
// Runs on M0G3507QDGS32RQ1
// Mark McDermott, Daniel Valvano and Jonathan Valvano
// Students do not need to use this project. 
// It is here to help with understanding single chip software
// July 4, 2026

// ********NOT A LAUNCHPAD****************
// Specify your hardware connections, feel free to change
// 16 MHz crystal connected across, 10pF C0G capacitors to ground
//   PA5 is HFXIN
//   PA6 is HFXOUT
// PA0  is squarewave output to buzzer
// PA3  is SW5 
// PA10 is SW4 
// PA26 is SW3 
// PA7  is SW2
// PA22 is ADC input from slide pot, ADC0 channel 7
// PA15 is DAC output to speaker
// **********ST7735 TFT and SDC*******************
// ST7735, ECE445L Lab 6, 7 VSSOP-32 M0G3507QDGS32RQ1
// Backlight (pin 10)     to +3.3 V
// MISO (pin 9)           to SPI1 POCI: PA16
// SCK (pin 8)            to SPI1 SCLK: PA17
// MOSI (pin 7)           to SPI1 PICO: PA18
// TFT_CS (pin 6)         to GPIO:  PA27
// CARD_CS (pin 5)        to PA25 (GPIO)
// Data/Command (pin 4)   to PA13 (GPIO), high for data, low for command
// RESET (pin 3)          to PA24 (GPIO)
// VCC (pin 2)            to +3.3 V
// Gnd (pin 1)            to ground

#include <ti/devices/msp/msp.h>
#include <stdio.h>
#include "../inc/LaunchPad.h"
#include "../inc/Clock.h"
#include "../inc/ST7735_SDC.h"
#include "../inc/Timer.h"
#include "../inc/DAC.h"
#include "../inc/ADC.h"

#define BUZ_PORT      GPIOA
#define BUZ_PIN       (1<<0)      // PA0
#define BUZ_INDEX     (PA0INDEX)  // PA0 GPIO

#define LED_PORT      GPIOA
#define LED_PIN       (1<<12)      // PA12
#define LED_INDEX     (PA12INDEX)  // PA12 GPIO
#define LED_Toggle()  LED_PORT->DOUTTGL31_0 = LED_PIN
#define SW5_PORT      GPIOA
#define SW5_PIN       (1<<3)       // PA3
#define SW5_INDEX     (PA3INDEX)   // PA3 GPIO
#define SW4_PORT      GPIOA
#define SW4_PIN       (1<<10)      // PA10
#define SW4_INDEX     (PA10INDEX)  // PA10 GPIO
#define SW3_PORT      GPIOA
#define SW3_PIN       (1<<26)      // PA26
#define SW3_INDEX     (PA26INDEX)  // PA26 GPIO
#define SW2_PORT      GPIOA
#define SW2_PIN       (1<<7)       // PA7
#define SW2_INDEX     (PA7INDEX)   // PA7 GPIO
#define SW(pin) (SW ## pin ##_PORT->DIN31_0&SW ## pin ##_PIN)

// 12-bit 64-element sine wave
const uint16_t Wave[64] = {
  2048,2244,2438,2629,2813,2991,3159,3317,3462,3594,3711,
  3812,3896,3962,4010,4038,4048,4038,4010,3962,3896,3812,
  3711,3594,3462,3317,3159,2991,2813,2629,2438,2244,2048,
  1852,1658,1467,1283,1105,937,779,634,502,385,284,
  200,134,86,58,48,58,86,134,200,284,385,
  502,634,779,937,1105,1283,1467,1658,1852
}; 
uint32_t count2,count3,count4;
void SysTick_Handler(void){
  if(SW(2)){
    DAC_Out(Wave[count2&0x3F]);
    count2++;
  }
}
uint32_t data,distance,time,sum,flag;

void TIMG8_IRQHandler(void){// runs every 1ms
  if((TIMG8->CPU_INT.IIDX) == 1){ // this will acknowledge
    if(SW(3)){
      BUZ_PORT->DOUTTGL31_0 = BUZ_PIN;
      count3++;
    }else{
      BUZ_PORT->DOUTCLR31_0 = BUZ_PIN;
    }
    data = ADC0_In();
    sum += data;
    time++;
    if(time >= 128){
      distance = (2000*sum)>>(7+12); // 0.001cm
      time = 0;
      sum = 0;
      flag = 1; // mailbox flag
    }
  }
}
void LED_Init(void);
void Buzzer_Init(void);
void Switch_Init(void);
void ActivatePortA(void);
void SysTick_InitArm(uint32_t period, uint32_t priority);
int main0(void){ // main0 test of M0G3507QDGS32RQ1 with I/O devices
  __disable_irq(); 
  ActivatePortA();
  Clock_Init_HFXT_16_80MHz(0);  // 0.005% accurate running off external crystal oscillator

  LED_Init();
  Buzzer_Init();
  Switch_Init();
  DAC_Init();
  ADC0_Init(7,ADCVREF_VDDA);
  time = sum = count2 = count3 = count4 = 0;
  TimerG8_IntArm(1000,40,1);   // TimerG0 trigger every 1ms 
  SysTick_InitArm(80000/64,0); // 64kHz

  __enable_irq();
  while(1){
    if(SW(4)){
      LED_Toggle();
      count4++;
    }
    Clock_Delay(40000000); // 500ms
  }
}

// place scope or login analyzer to PA0 or PA10
// each the following five clock options should create 50 Hz squarewave
int main1(void){ // main1 test of clocks and PLL
  __disable_irq(); 
  ActivatePortA(); 
  // default is 32MHz SYSOSC    // 0.7% accurate running off internal oscillator
  // Clock_Init(OSCFREQ4MHZ);   // 0.7% accurate running off internal oscillator
  // Clock_Init(OSCFREQ32MHZ);  // 0.7% accurate running off internal oscillator
  // Clock_Init80MHz(0);        // 0.7% accurate running off internal oscillator
  // Clock_Init_HFXT_16MHz();   // 0.005% accurate running off external crystal oscillator
  Clock_Init_HFXT_16_80MHz(0);  // 0.005% accurate running off external crystal oscillator
  Buzzer_Init();
  LED_Init();
  ADC0_Init(7,ADCVREF_VDDA);
  time = sum = count2 = count3 = count4 = 0;
  TimerG8_IntArm(1000,40,1);   // TimerG0 trigger every 1ms 

  while(1){
    LED_Toggle();
    BUZ_PORT->DOUTTGL31_0 = BUZ_PIN;
    count4++;
    Clock_Delay(Clock_Freq()/100); // 10ms
  } 
} 

int main(void){ // main2 test of LCD
  __disable_irq(); 
  ActivatePortA(); 
  Clock_Init_HFXT_16_80MHz(0);  // 0.005% accurate running off external crystal oscillator
  Buzzer_Init();
  LED_Init();
  TIMG0->COUNTERREGS.CTRCTL &= ~0x01; // disarm if not using SDC
  ST7735_InitR(INITR_REDTAB); //INITR_REDTAB for AdaFruit, INITR_BLACKTAB for SPI HiLetgo ST7735R
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_SetCursor(0, 0);  
  ST7735_OutString("ECE445L Lab 6\n");
  ST7735_OutString("Valvano");
  ST7735_PlotClear(0,2000);  // range from 0 to 2000 0.001cm
  ADC0_Init(7,ADCVREF_VDDA);
  time = sum = count2 = count3 = count4 = flag=0;
 // TimerG8_IntArm(1000,40,1);   // TimerG0 trigger every 1ms 
  BUZ_PORT->DOUTSET31_0 = BUZ_PIN;
  __enable_irq();
  while(1){
    if(flag){
      flag = 0;
      LED_Toggle();
      count4++;
      ST7735_PlotPoint(distance);
      ST7735_PlotNextErase();
    }
  } 
} 




void Buzzer_Init(void){
   // PA0 is squarewave output to buzzer
  IOMUX->SECCFG.PINCM[BUZ_INDEX]  = (uint32_t) 0x00000081;
  BUZ_PORT->DOE31_0 |= BUZ_PIN;
  BUZ_PORT->DOUTCLR31_0 = BUZ_PIN;
}
void Switch_Init(void){
// PA3, PA7, PA10, PA26
  IOMUX->SECCFG.PINCM[SW2_INDEX]  = (uint32_t) 0x00050081; // input, pull down;
  IOMUX->SECCFG.PINCM[SW3_INDEX]  = (uint32_t) 0x00050081; // input, pull down;
  IOMUX->SECCFG.PINCM[SW4_INDEX]  = (uint32_t) 0x00050081; // input, pull down;
  IOMUX->SECCFG.PINCM[SW5_INDEX]  = (uint32_t) 0x00050081; // input, pull down;
}

void ActivatePortA(void){
    // Reset t GPIO peripherals
    // RSTCLR
    //   bits 31-24 unlock key 0xB1
    //   bit 1 is Clear reset sticky bit
    //   bit 0 is reset gpio port
  GPIOA->GPRCM.RSTCTL = (uint32_t)0xB1000003;

    // Enable power to two GPIO peripherals
    // PWREN
    //   bits 31-24 unlock key 0x26
    //   bit 0 is Enable Power
  GPIOA->GPRCM.PWREN = (uint32_t)0x26000001;

  Clock_Delay(24); // time for gpio to power up
}
void LED_Init(void){
  IOMUX->SECCFG.PINCM[LED_INDEX]  = (uint32_t) 0x00000081;
  LED_PORT->DOE31_0 |= LED_PIN;
  LED_PORT->DOUTCLR31_0 = LED_PIN;
}
void LED_Out(uint32_t data){
  if(data){
    LED_PORT->DOUTSET31_0 = LED_PIN; // on
  }else{
    LED_PORT->DOUTCLR31_0 = LED_PIN; // off
  }

}


// ------------------    SysTick_Init     ------------------------------------
// Initialize Systick periodic interrupts
// Input:  interrupt period
//            Units of period are 12.5ns
//            Maximum is 2^24-1
//            Minimum is determined by length of ISR
// Output: none
void SysTick_InitArm(uint32_t period, uint32_t priority){
  SysTick->CTRL  = 0x00;      // disable during initialization
  SysTick->LOAD  = period-1;  // set reload register
  //The ARM Cortex-M0+ only implements the most significant 2 bits of each 8-bit priority field (giving the 4 priority levels).
  SCB->SHP[1]    = (SCB->SHP[1]&(~0xC0000000))|priority<<30;    // set priority (bits 31,30)
  SysTick->VAL   = 0;         // clear count, cause reload
  SysTick->CTRL  = 0x07;      // Enable SysTick IRQ and SysTick Timer
}

// Executed every 1 ms, armed by ST7735_InitR
// this should not trigger because you should disarm G0 timer
// remove this ISR if using the SDC
void TIMG0_IRQHandler(void){
  if((TIMG0->CPU_INT.IIDX) == 1){ // this will acknowledge
// the SDC interface will use this
  }
}