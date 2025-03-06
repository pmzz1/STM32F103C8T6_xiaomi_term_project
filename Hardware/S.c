#include "stm32f10x.h"
#include "OLED.h"

#define TRI_DC_OFFSET       2048               // 三角波 DAC 中点偏移
#define TRI_VPP_STEP        124                // 对应 0.1V 步进(假设参考电压 3.3V)
#define TRI_AMPLITUDE_STEP  (TRI_VPP_STEP / 2) // 因为峰峰值 = 振幅 * 2，所以每次振幅步进是 VPP_STEP/2
#define TRI_MAX_AMPLITUDE   (4095 - TRI_DC_OFFSET)
#define TRI_MIN_AMPLITUDE   TRI_AMPLITUDE_STEP

#define TRI_FREQ_STEP       1000               // 每次调频步进 1 kHz
#define TRI_FREQ_MAX        10000              // 最高 10 kHz
#define TRI_FREQ_MIN        1000               // 最低 1 kHz

volatile int16_t  Tri_currentAmplitude = 1024;  // 初始振幅
volatile uint32_t Tri_currentFreq      = 1000;  // 初始频率
volatile uint16_t triangle_wave[64];            // DMA 输出的三角波表

const uint16_t base_triangle_wave[64] = {
    0,    128,  256,  384,  512,  640,  768,  896,
    1024, 1152, 1280, 1408, 1536, 1664, 1792, 1920,
    2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944,
    3072, 3200, 3328, 3456, 3584, 3712, 3840, 3968,
    4095, 3968, 3840, 3712, 3584, 3456, 3328, 3200,
    3072, 2944, 2816, 2688, 2560, 2432, 2304, 2176,
    2048, 1920, 1792, 1664, 1536, 1408, 1280, 1152,
    1024, 896,  768,  640,  512,  384,  256,  128
};

void Tri_updateTriangleWave(void)
{
    DMA_Cmd(DMA2_Channel3, DISABLE);

    for (int i = 0; i < 64; i++)
    {
        int32_t deviation = (int32_t)base_triangle_wave[i] - TRI_DC_OFFSET;

        int32_t scaled_dev = (deviation * Tri_currentAmplitude) / 2047;

        int32_t val = TRI_DC_OFFSET + scaled_dev;
        if (val > 4095) val = 4095;
        if (val < 0)    val = 0;

        triangle_wave[i] = (uint16_t)val;
    }

    DMA_Cmd(DMA2_Channel3, ENABLE);
}

void Tri_displayVpp(void)
{
    // Vpp = 2 * 振幅 * (3.3 / 4095)
    float Vpp = 2.0f * ((float)Tri_currentAmplitude * 3.3f) / 4095.0f;
    OLED_ShowFloatNum(88, 0, Vpp, 1, 2, OLED_8X16);
    OLED_Update();
}

void Tri_upVpp(void)
{
    if (Tri_currentAmplitude + TRI_AMPLITUDE_STEP <= TRI_MAX_AMPLITUDE)
    {
        Tri_currentAmplitude += TRI_AMPLITUDE_STEP;
        Tri_updateTriangleWave();
        Tri_displayVpp();
    }
}

void Tri_downVpp(void)
{
    if (Tri_currentAmplitude - TRI_AMPLITUDE_STEP >= TRI_MIN_AMPLITUDE)
    {
        Tri_currentAmplitude -= TRI_AMPLITUDE_STEP;
        Tri_updateTriangleWave();
        Tri_displayVpp();
    }
}

void Tri_displayFreq(void)
{
    OLED_ShowNum(90, 16, (int)Tri_currentFreq, 4, OLED_8X16);
    OLED_Update();
}

void Tri_setFreq(uint32_t freq)
{
    if (freq < TRI_FREQ_MIN) freq = TRI_FREQ_MIN;
    if (freq > TRI_FREQ_MAX) freq = TRI_FREQ_MAX;
    Tri_currentFreq = freq;

    // f_wave = 75000 / (auto_reload + 1)
    uint32_t auto_reload = 75000 / Tri_currentFreq - 1;

    if (auto_reload < 1)     auto_reload = 1;
    if (auto_reload > 65535) auto_reload = 65535;

    TIM_Cmd(TIM2, DISABLE);
    TIM_SetAutoreload(TIM2, auto_reload);
    TIM_GenerateEvent(TIM2, TIM_EventSource_Update);
    TIM_Cmd(TIM2, ENABLE);

    Tri_displayFreq();
}

void Tri_upFreq(void)
{
    if (Tri_currentFreq + TRI_FREQ_STEP <= TRI_FREQ_MAX)
    {
        Tri_setFreq(Tri_currentFreq + TRI_FREQ_STEP);
    }
}

void Tri_downFreq(void)
{
    if (Tri_currentFreq - TRI_FREQ_STEP >= TRI_FREQ_MIN)
    {
        Tri_setFreq(Tri_currentFreq - TRI_FREQ_STEP);
    }
}

void S_init(void)
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
    DMA_InitStructure.DMA_MemoryBaseAddr      = (uint32_t)triangle_wave;
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

    Tri_updateTriangleWave();

    OLED_Clear();
    OLED_ShowString(0, 0, "MODE S", OLED_8X16);

    OLED_ShowString(56, 0, "Vpp:", OLED_8X16);
    Tri_displayVpp();

    OLED_ShowString(48, 16, "Freq:", OLED_8X16);
    Tri_displayFreq();

    OLED_DrawLine(8, 52, 113, 52);
    OLED_DrawLine(108, 48, 113, 52);
    OLED_DrawLine(108, 56, 113, 52);
    OLED_ShowString(113, 37, "t", OLED_8X16);

    OLED_DrawLine(8, 25, 8, 52);
    OLED_DrawLine(8, 25, 4, 30);
    OLED_DrawLine(8, 25, 12, 30);
    OLED_ShowString(14, 25, "V", OLED_6X8);

    OLED_ShowString(54, 56, "1ms", OLED_6X8);
    OLED_ShowString(102, 56, "2ms", OLED_6X8);

    OLED_DrawTriangleWave();

    OLED_Update();
}
