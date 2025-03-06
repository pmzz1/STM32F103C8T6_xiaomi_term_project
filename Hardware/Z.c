#include "stm32f10x.h"
#include "OLED.h"

#define SIN_DC_OFFSET       2048                  // 正弦波用的中点偏移
#define SIN_VPP_STEP        124                   // 每次调幅时，对应 0.1V 的步进(假设 3.3V)
#define SIN_AMPLITUDE_STEP  (SIN_VPP_STEP / 2)    // 峰峰值=振幅×2，因此振幅的步进为 VPP_STEP/2
#define SIN_MAX_AMPLITUDE   (4095 - SIN_DC_OFFSET)
#define SIN_MIN_AMPLITUDE   SIN_AMPLITUDE_STEP

#define SIN_FREQ_STEP       1000                  // 频率每次步进 1 kHz
#define SIN_FREQ_MAX        10000                 // 最大频率 10 kHz
#define SIN_FREQ_MIN        1000                  // 最小频率 1 kHz

volatile int16_t  Sin_currentAmplitude = 1024;   // 初始振幅
volatile uint32_t Sin_currentFreq      = 1000;   // 初始频率
volatile uint16_t Sin_wave[64];                 // DMA 输出的正弦表

const uint16_t Sin_baseSineWave[64] = {
    2048, 2251, 2453, 2651, 2843, 3027, 3201, 3364,
    3513, 3648, 3767, 3870, 3953, 4018, 4064, 4089,
    4094, 4079, 4044, 3988, 3914, 3821, 3710, 3583,
    3440, 3284, 3115, 2936, 2748, 2553, 2353, 2150,
    1945, 1742, 1542, 1347, 1159, 980, 811, 655,
    512, 385, 274, 181, 107, 51, 16, 1,
    6, 31, 77, 142, 225, 328, 447, 582,
    731, 894, 1068, 1252, 1444, 1642, 1844, 2047
};

void Sin_updateSineWave(void)
{
    DMA_Cmd(DMA2_Channel3, DISABLE);
    
    for (int i = 0; i < 64; i++) {
        int32_t deviation = (int32_t)Sin_baseSineWave[i] - SIN_DC_OFFSET;
        
        int32_t scaled_deviation = (deviation * Sin_currentAmplitude) / 2047;
        
        int32_t scaled_value = SIN_DC_OFFSET + scaled_deviation;
        
        if (scaled_value > 4095) scaled_value = 4095;
        if (scaled_value < 0)    scaled_value = 0;
        
        Sin_wave[i] = (uint16_t)scaled_value;
    }
    
    DMA_Cmd(DMA2_Channel3, ENABLE);
}

void Sin_displayVpp(void) 
{
    // Vpp = 2.0 × 振幅 × (3.3 / 4095)
    float Vpp = 2.0f * ((float)Sin_currentAmplitude * 3.3f) / 4095.0f;
    OLED_ShowFloatNum(88, 0, Vpp, 1, 2, OLED_8X16); 
    OLED_Update();
}

void Sin_upVpp(void)
{
    if (Sin_currentAmplitude + SIN_AMPLITUDE_STEP <= SIN_MAX_AMPLITUDE) {
        Sin_currentAmplitude += SIN_AMPLITUDE_STEP;
        Sin_updateSineWave();
        Sin_displayVpp();
    }
}

void Sin_downVpp(void)
{
    if (Sin_currentAmplitude - SIN_AMPLITUDE_STEP >= SIN_MIN_AMPLITUDE) {
        Sin_currentAmplitude -= SIN_AMPLITUDE_STEP;
        Sin_updateSineWave();
        Sin_displayVpp();
    }
}

void Sin_displayFreq(void)
{
    OLED_ShowNum(90, 16, (int)Sin_currentFreq, 4, OLED_8X16);  
    OLED_Update();
}

void Sin_setFreq(uint32_t freq)
{
    // 限制频率范围
    if (freq < SIN_FREQ_MIN) freq = SIN_FREQ_MIN;
    if (freq > SIN_FREQ_MAX) freq = SIN_FREQ_MAX;
    Sin_currentFreq = freq;
    
    //计算自动重装载值 (ARR)
    //f_wave = 75000 / (auto_reload + 1)
    uint32_t auto_reload = 75000 / Sin_currentFreq - 1;
    
    //防止越界
    if (auto_reload < 1)     auto_reload = 1;
    if (auto_reload > 65535) auto_reload = 65535;
    
    TIM_Cmd(TIM2, DISABLE);
    
    TIM_SetAutoreload(TIM2, auto_reload);
    TIM_GenerateEvent(TIM2, TIM_EventSource_Update);
    
    TIM_Cmd(TIM2, ENABLE);
    
    Sin_displayFreq();
}

void Sin_upFreq(void)
{
    if (Sin_currentFreq + SIN_FREQ_STEP <= SIN_FREQ_MAX) {
        Sin_setFreq(Sin_currentFreq + SIN_FREQ_STEP);
    }
}

void Sin_downFreq(void)
{
    if (Sin_currentFreq - SIN_FREQ_STEP >= SIN_FREQ_MIN) {
        Sin_setFreq(Sin_currentFreq - SIN_FREQ_STEP);
    }
}
void Z_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);   
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);  
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);	
 
    //预分频器 15-1，ARR 75-1 => 计数频率 75 kHz
    TIM_PrescalerConfig(TIM2, 15 - 1, TIM_PSCReloadMode_Update);
    TIM_SetAutoreload(TIM2, 75 - 1);
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);
    
    DAC_InitTypeDef DAC_InitStruct;
    DAC_InitStruct.DAC_Trigger                      = DAC_Trigger_T2_TRGO;
    DAC_InitStruct.DAC_WaveGeneration               = DAC_WaveGeneration_None;
    DAC_InitStruct.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
    DAC_InitStruct.DAC_OutputBuffer                 = DAC_OutputBuffer_Enable;
    DAC_Init(DAC_Channel_1, &DAC_InitStruct);  
    
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)&DAC->DHR12R1;
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryBaseAddr      = (uint32_t)Sin_wave; 
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize          = 64;
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority            = DMA_Priority_Medium;
    DMA_InitStructure.DMA_M2M                 = DMA_M2M_Disable;
    DMA_Init(DMA2_Channel3, &DMA_InitStructure);
    
    TIM_Cmd(TIM2, ENABLE);
    DMA_Cmd(DMA2_Channel3, ENABLE);
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_DMACmd(DAC_Channel_1, ENABLE);
    
    Sin_updateSineWave();
    
    //显示初始化信息
    OLED_Clear();
    OLED_ShowString(0, 0, "MODE Sine", OLED_8X16);

    OLED_ShowString(56, 0, "Vpp:", OLED_8X16);
    Sin_displayVpp();  // 显示当前幅度对应的 Vpp
    
    OLED_ShowString(48, 16, "Freq:", OLED_8X16);
    Sin_displayFreq(); // 显示当前频率
       
    OLED_DrawSine();   
    
    // 简单画坐标系
    OLED_DrawLine(8, 52, 113, 52);   // X轴
    OLED_DrawLine(108, 48, 113, 52);
    OLED_DrawLine(108, 56, 113, 52);
    OLED_ShowString(113, 37, "t", OLED_8X16);
	
    OLED_DrawLine(8, 25, 8, 52);     // Y轴
    OLED_DrawLine(8, 25, 4, 30);
    OLED_DrawLine(8, 25, 12, 30);
    OLED_ShowString(14, 25, "V", OLED_6X8);
	
    OLED_ShowString(54, 56, "1ms", OLED_6X8);
    OLED_ShowString(102, 56, "2ms", OLED_6X8);
    
    OLED_Update();
}
