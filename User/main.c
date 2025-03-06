#include "stm32f10x.h"   // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "KEY_INTR.h"
#include "Z.h"
#include "F.h"
#include "S.h"
#include "C.h"
//PA0输入要测信号   PA1输出舵机pwm   PA4输出正弦波三角波
//OLED屏幕SDA-PB15 SCL-PB10  按键对应PB7 PB9 PB11 PB13
extern uint8_t  Serial_RxData;
static uint8_t prevRxData = 0;

int main(void)
{
    
    OLED_Init();
    Serial_Init();
    key_INTR_init();  
   

   
    OLED_ShowString(0, 0, "Init OK", OLED_8X16);
    OLED_Update();
  

   
    while (1)
    {
        

      
        if (prevRxData != Serial_RxData)
        {
            prevRxData = Serial_RxData;
            switch (Serial_RxData)
            {
                case 0x5A: // 'Z'
                    Z_init();
                    break;

                case 0x53: // 'S'
                    S_init();
                    break;

                case 0x46: // 'F'
                    F_init();
                    break;

                case 0x43: // 'C'
                    C_init();                                                    
                    break;

                default:                  
                    break;
            }
        }
     
      if (Serial_RxData==0x43)
			{
		    	Measure_Update();
					Delay_ms(20);
			}
         
    }
}
