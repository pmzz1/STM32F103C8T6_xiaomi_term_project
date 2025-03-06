#include "stm32f10x.h"
#include "OLED.h"     
#include "Delay.h"      

#define ADC_BUFFER_SIZE 128
#define VERTICAL_SCALE_MIN   1
#define VERTICAL_SCALE_MAX   5

// 横向缩放 1--5 倍 (1 表示每个采样点占1个像素, 5 表示每个采样点占5个像素)
#define HORIZONTAL_SCALE_MIN 1
#define HORIZONTAL_SCALE_MAX 5

extern volatile uint16_t g_ADCBuffer[ADC_BUFFER_SIZE];
volatile uint16_t g_ADCBuffer[ADC_BUFFER_SIZE] = {0};
static uint16_t s_DrawBuffer[ADC_BUFFER_SIZE];
static uint8_t g_verticalScale   = 1;  // 范围 1--5
static uint8_t g_horizontalScale = 1;  // 范围 1--5

void C_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;    
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    DMA_InitTypeDef DMA_InitStructure;
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr  = (uint32_t)&ADC1->DR;       
    DMA_InitStructure.DMA_MemoryBaseAddr      = (uint32_t)g_ADCBuffer;     
    DMA_InitStructure.DMA_DIR                 = DMA_DIR_PeripheralSRC;     
    DMA_InitStructure.DMA_BufferSize          = ADC_BUFFER_SIZE;           
    DMA_InitStructure.DMA_PeripheralInc       = DMA_PeripheralInc_Disable; 
    DMA_InitStructure.DMA_MemoryInc           = DMA_MemoryInc_Enable;      
    DMA_InitStructure.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_HalfWord; 
    DMA_InitStructure.DMA_MemoryDataSize      = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode                = DMA_Mode_Circular;          
    DMA_InitStructure.DMA_Priority            = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M                 = DMA_M2M_Disable;            
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);

		DMA_Cmd(DMA1_Channel1, ENABLE);

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);  // ADC 时钟 = PCLK2/6=12MHz

    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;  
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;   
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None; 
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = 1;        
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_55Cycles5);

    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));

		ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
		OLED_Clear();
	  OLED_ShowString(0,0,"MODE C",OLED_8X16);
		OLED_Update();
		
}

void Measure_Update(void)
{
    DMA_Cmd(DMA1_Channel1, DISABLE);

    uint16_t remain = DMA_GetCurrDataCounter(DMA1_Channel1);  
    uint16_t writePos = (ADC_BUFFER_SIZE - remain) % ADC_BUFFER_SIZE;
    uint16_t idx = 0;
    
	
		for (uint16_t i = writePos; i < ADC_BUFFER_SIZE; i++)
    {
        s_DrawBuffer[idx++] = g_ADCBuffer[i];
    }
    for (uint16_t i = 0; i < writePos; i++)
    {
        s_DrawBuffer[idx++] = g_ADCBuffer[i];
    }
    DMA_Cmd(DMA1_Channel1, ENABLE);


   OLED_ClearArea(0,16,128,48);

    const uint8_t xMax     = 128;       			    // 屏幕宽
    const uint8_t yBottom  = 63;        			   	// 底部像素
    const uint8_t yTop     = 16;      				    // 波形上边界
    const uint8_t waveH    = yBottom - yTop; 	    // 47像素高度

    for(uint16_t i = 0; i < ADC_BUFFER_SIZE; i++)
    {
        // 计算屏幕 x 坐标
        uint16_t x = i * g_horizontalScale;
        if(x >= xMax) 
            break;   // 超过屏幕宽度就不绘制

        //将 0~4095(假设12位ADC) 映射到 y=16--63
        float ratio = (float)s_DrawBuffer[i] / 4095.0f;  
        float maxRange = (float)waveH / (float)g_verticalScale;  
        uint8_t y = yBottom - (uint8_t)(ratio * maxRange);
        if(y < yTop)    y = yTop;
        if(y > yBottom) y = yBottom;
      
        OLED_DrawPoint(x, y);
    }
   
   //在顶部显示 "V=x H=y"
    OLED_ShowString(70, 0, "V=", OLED_8X16);
    OLED_ShowNum(86, 0, g_verticalScale, 1, OLED_8X16);

    OLED_ShowString(100, 0, "H=", OLED_8X16);
    OLED_ShowNum(116, 0, g_horizontalScale, 1, OLED_8X16);
   
    OLED_Update();
}


void Measure_VScale_Up(void)
{
    if (g_verticalScale < VERTICAL_SCALE_MAX)
    {
        g_verticalScale++;
    }
}

void Measure_VScale_Down(void)
{
    if (g_verticalScale > VERTICAL_SCALE_MIN)
    {
        g_verticalScale--;
    }
}

void Measure_HScale_Up(void)
{
    if (g_horizontalScale < HORIZONTAL_SCALE_MAX)
    {
        g_horizontalScale++;
    }
}

void Measure_HScale_Down(void)
{
    if (g_horizontalScale > HORIZONTAL_SCALE_MIN)
    {
        g_horizontalScale--;
    }
}

