#结课作业——实战（作业要求）

1. 认真研究所提供的OLED的参考资料，实现在OLED上显示图形和文字。

2. 利用USART功能，通过个人电脑的串口调试助手，给Mini开发板STM32F103RC单片机发送指令，同时在
配发的OLED屏幕上显示出适当的信息。指令由一位字符构成，分以下4种情况：

3. 指令为‘Z’，利用单片机的DAC及DMA功能，生成正弦波，频率为1 kHz，电压峰峰值为3.3V。利用
OLED显示正弦波的示意波形，横轴为时间，时长2ms，纵轴为电压，范围3.3V。此时屏幕可显示2个完整
周期。按动按键可使正弦波参数发生变化，K1、K2以0.1V为步长，使正弦波的峰峰值在0~3.3V之间进行调
整；K3、K4以1kHz为步长，使正弦波的频率在1kHz~10kHz的范围内调整。屏幕显示峰峰值和频率。

4. 指令为‘S’，利用单片机的DAC及DMA功能，生成三角波（不要使用DAC模块的波形生成功能），频率
为1 kHz，电压峰峰值为3.3V。利用OLED显示三角波的示意波形。其余要求与要求2相同。

5. 指令为‘F’，利用单片机的定时器比较输出功能，生成方波，频率为50Hz（周期20ms)，脉宽1.5ms。利
用OLED显示2个周期的方波。按动按键可使方波参数发生变化，K1、K2以0.1ms为步长，使方波的脉宽在
1.0ms~2.0ms之间进行变化。屏幕显示脉宽。连接舵机，观察舵机角度的变化。

6. 指令为‘C’，利用单片机的ADC和DMA功能，实现对上述波形的测量。测量的波形在OLED屏幕上显示，
按动按键可使测量结果的显示效果发生变化，其中K1、K2用于调整屏幕纵坐标方向单位尺寸的电压幅度，
K3、K4用于调整屏幕横坐标方向单位尺寸的时长。

7. 按键通过外部中断来实现对参数的调整。

8. 整整两天才弄完，感谢舍友带饭。
