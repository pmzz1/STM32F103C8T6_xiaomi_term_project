#include "stm32f10x.h"  // Device header
#include "OLED.h"

// 默认占空比对应的初值1.5ms
uint16_t setpwm = 1500;

extern uint8_t Serial_RxData;

void F_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_InternalClockConfig(TIM2);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period        = 20000 - 1;  // ARR 20,000 (对应 20ms)
    TIM_TimeBaseInitStructure.TIM_Prescaler     = 72 - 1;     // PSC  72 (时钟 72MHz/72 = 1MHz)
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
    // PWM 频率 = 72,000,000 / [(ARR + 1) X (PSC + 1)] = 50

    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = setpwm;  // CCR 初始值 1.5ms
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);
    // 占空比 = CCR/(ARR + 1)，例如 setpwm=1500 -- 1.5ms

    TIM_Cmd(TIM2, ENABLE);

    OLED_Clear();
    OLED_ShowString(0,   0,  "MODE F", OLED_8X16);
    OLED_ShowString(40, 16,  "PW:",    OLED_8X16);
    OLED_ShowString(100,16,  "ms",     OLED_8X16);
    OLED_ShowFloatNum(64, 16, 1.5, 1, 1, OLED_8X16);

    OLED_DrawLine(8,  52, 113, 52);  
    OLED_DrawLine(108,48, 113, 52);
    OLED_DrawLine(108,56, 113, 52);
    OLED_ShowString(113, 37, "t",  OLED_8X16);

    OLED_DrawLine(8,  25,  8,  52);
    OLED_DrawLine(8,  25,  4,  30);
    OLED_DrawLine(8,  25, 12,  30);
    OLED_ShowString(14, 22, "V",  OLED_6X8);

    OLED_ShowString(54,  56, "10ms", OLED_6X8);
    OLED_ShowString(102, 56, "20ms", OLED_6X8);

    OLED_DrawSquarewave(setpwm);
    OLED_Update();
}

void PWM_SetCompare2(uint16_t Compare)
{
    TIM_SetCompare2(TIM2, Compare);  
}

void pwm_set_key_up(void)
{
    setpwm += 100;  
    if (setpwm > 2000) {
        setpwm = 2000;  // 上限 2.0ms
    }
    OLED_ShowFloatNum(64, 16, ((float)setpwm)/1000, 1, 1, OLED_8X16);
    PWM_SetCompare2(setpwm);
    OLED_DrawSquarewave(setpwm);
}

void pwm_set_key_down(void)
{
    setpwm -= 100;  
    if (setpwm < 1000) {
        setpwm = 1000;  // 下限 1.0ms
    }
    OLED_ShowFloatNum(64, 16, ((float)setpwm)/1000, 1, 1, OLED_8X16);
    PWM_SetCompare2(setpwm);
    OLED_DrawSquarewave(setpwm);
}
