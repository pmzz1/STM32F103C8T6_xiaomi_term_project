#include "stm32f10x.h"  // Device header
#include "OLED.h"
#include "F.h"
#include "Z.h"
#include "S.h"
#include "C.h"

extern uint8_t Serial_RxData;

void key_INTR_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    // GPIO 配置：PB7、PB9、PB11、PB13 上拉输入
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // EXTI 配置：PB7 -- EXTI7，PB9 -- EXTI9，PB11 -- EXTI11，PB13 -- EXTI13
    EXTI_InitTypeDef EXTI_InitStruct;
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7);
    EXTI_InitStruct.EXTI_Line    = EXTI_Line7;
    EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStruct);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource9);
    EXTI_InitStruct.EXTI_Line = EXTI_Line9;
    EXTI_Init(&EXTI_InitStruct);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);
    EXTI_InitStruct.EXTI_Line = EXTI_Line11;
    EXTI_Init(&EXTI_InitStruct);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource13);
    EXTI_InitStruct.EXTI_Line = EXTI_Line13;
    EXTI_Init(&EXTI_InitStruct);

    // NVIC 配置：PB7 PB9 属于 EXTI9_5_IRQn，PB11 PB13 属于 EXTI15_10_IRQn
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    NVIC_InitStruct.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_Init(&NVIC_InitStruct);
}

void EXTI9_5_IRQHandler(void)
{
    // PB7 中断
    if (EXTI_GetITStatus(EXTI_Line7) != RESET)
    {
        switch (Serial_RxData)
        {
            case 0x5A:  // "Z"
                Sin_upVpp();
                break;
            case 0x53:  // "S"
                Tri_upVpp();
                break;
            case 0x46:  // "F"
                pwm_set_key_up();
                break;
            case 0x43:  // "C"
							  Measure_VScale_Up();
            default:
                break;
        }
        EXTI_ClearITPendingBit(EXTI_Line7);
    }

    // PB9 中断
    if (EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
        switch (Serial_RxData)
        {
            case 0x5A:  // "Z"
                Sin_downVpp();
                break;
            case 0x53:  // "S"
                Tri_downVpp();
                break;
            case 0x46:  // "F"
                pwm_set_key_down();
                break;
            case 0x43:  // "C"
							  Measure_VScale_Down();
            default:
                break;
        }
        EXTI_ClearITPendingBit(EXTI_Line9);
    }
}


void EXTI15_10_IRQHandler(void)
{
    // PB11 中断
    if (EXTI_GetITStatus(EXTI_Line11) != RESET)
    {
        switch (Serial_RxData)
        {
            case 0x5A:  // "Z"
                Sin_upFreq();
                break;
            case 0x53:  // "S"
                Tri_upFreq();
                break;
            case 0x46:  // "F"
            case 0x43:  // "C"
							  Measure_HScale_Up();
            default:
                break;
        }
        EXTI_ClearITPendingBit(EXTI_Line11);
    }

    // PB13 中断
    if (EXTI_GetITStatus(EXTI_Line13) != RESET)
    {
        switch (Serial_RxData)
        {
            case 0x5A:  // "Z"
                Sin_downFreq();
                break;
            case 0x53:  // "S"
                Tri_downFreq();
                break;
            case 0x46:  // "F"
							
            case 0x43:  // "C"
							  Measure_HScale_Down();
            default:
                break;
        }
        EXTI_ClearITPendingBit(EXTI_Line13);
    }
}
