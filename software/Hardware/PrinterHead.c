#include "PrinterHead.h"
#include "Delay.h"
#include "MySPI.h"
#include "PrinterMoto.h"
#include "stm32f10x.h"

/* JX-2R-01 */
#define PRINTER_LAT_RCC_GPIO RCC_APB2Periph_GPIOB
#define PRINTER_LAT_PORT GPIOB
#define PRINTER_LAT_PIN GPIO_Pin_1

#define PRINTER_STB_RCC_GPIO RCC_APB2Periph_GPIOB
#define PRINTER_STB_PORT GPIOB
#define PRINTER_STB1_PIN GPIO_Pin_10
#define PRINTER_STB2_PIN GPIO_Pin_11
#define PRINTER_STB3_PIN GPIO_Pin_12
#define PRINTER_STB4_PIN GPIO_Pin_13
#define PRINTER_STB5_PIN GPIO_Pin_14
#define PRINTER_STB6_PIN GPIO_Pin_15
#define PRINTER_STB_PINS                                                                                               \
    PRINTER_STB1_PIN | PRINTER_STB2_PIN | PRINTER_STB3_PIN | PRINTER_STB4_PIN | PRINTER_STB5_PIN | PRINTER_STB6_PIN

#define HEAT_TIME 20
#define LAT_TIME 1
#define DOTLINE_SIZE 48

uint8_t dotLine[DOTLINE_SIZE] = {0};

void PrinterHead_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(PRINTER_LAT_RCC_GPIO, ENABLE);
    RCC_APB2PeriphClockCmd(PRINTER_STB_RCC_GPIO, ENABLE);

    // 初始化 LAT 数据锁存寄存器
    GPIO_InitStructure.GPIO_Pin = PRINTER_LAT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(PRINTER_LAT_PORT, &GPIO_InitStructure);
    // 默认不启用锁存寄存器，高电平关闭
    GPIO_SetBits(PRINTER_LAT_PORT, PRINTER_LAT_PIN);

    // 初始化 选通脉冲引脚
    GPIO_InitStructure.GPIO_Pin = PRINTER_STB_PINS;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(PRINTER_STB_PORT, &GPIO_InitStructure);
    // 默认不启用脉冲，低电平关闭
    GPIO_ResetBits(PRINTER_STB_PORT, PRINTER_STB_PINS);
}
/*
    每行384个点，每个STB控制64个点。
    每行可以打印 384 / 8 = 48 个字节（字符）
    每个STB可以控制 64 / 8 = 8 个字节（字符）

    当经过384个CLK时钟，也就是 384 / 8 = 48次SPI字节交换后，一点行数据传输完毕，目前存放在移位寄存器
    然后，拉低LAT一段时间（查看手册），将数据锁存到锁存器中
    然后拉高STB一段时间进行加热，这样一点行的内容就打印完了（当STB位打开时，在根据每个点位的1或0选择该点是否加热）
*/

void PrinterHead_LAT_Enable(void)
{
    GPIO_ResetBits(PRINTER_LAT_PORT, PRINTER_LAT_PIN);
}

void PrinterHead_LAT_Disable(void)
{
    GPIO_SetBits(PRINTER_LAT_PORT, PRINTER_LAT_PIN);
}

void PrinterHead_Heat_Enable(void)
{
    GPIO_SetBits(PRINTER_STB_PORT, PRINTER_STB_PINS);
}

void PrinterHead_Heat_Disable(void)
{
    GPIO_ResetBits(PRINTER_STB_PORT, PRINTER_STB_PINS);
}

void PrinterHead_Heat_Circle(void)
{
    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB1_PIN, (BitAction)1); // Enable
    Delay_ms(HEAT_TIME);
    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB1_PIN, (BitAction)0); // Disable

    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB2_PIN, (BitAction)1);
    Delay_ms(HEAT_TIME);
    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB2_PIN, (BitAction)0);

    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB3_PIN, (BitAction)1);
    Delay_ms(HEAT_TIME);
    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB3_PIN, (BitAction)0);

    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB4_PIN, (BitAction)1);
    Delay_ms(HEAT_TIME);
    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB4_PIN, (BitAction)0);

    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB5_PIN, (BitAction)1);
    Delay_ms(HEAT_TIME);
    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB5_PIN, (BitAction)0);

    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB6_PIN, (BitAction)1);
    Delay_ms(HEAT_TIME);
    GPIO_WriteBit(PRINTER_STB_PORT, PRINTER_STB6_PIN, (BitAction)0);
}

void PrinterHead_ClearDotLineArray(void)
{
    uint8_t i;
    for (i = 0; i < 48; i++)
    {
        dotLine[i] = 0;
    }
}

void PrinterHead_SendDotLineData(void)
{
    uint8_t i, j;
    uint8_t temp;
    for (i = 0; i < DOTLINE_SIZE; i++)
    {
        temp = MySPI_SwapByte(dotLine[i]);
    }
    PrinterHead_LAT_Enable();
    Delay_ms(LAT_TIME);
    PrinterHead_LAT_Disable();
    PrinterHead_ClearDotLineArray(); // 自动清除dotLine，方便外界继续向dotLine中放入数据
}

void PrinterHead_PrintDotLine(void)
{
    PrinterHead_SendDotLineData();

    // PrinterHead_Heat_Enable();
    // Delay_ms(HEAT_TIME);
    // PrinterHead_Heat_Disable();

    // 电压不够时可以使用这个函数加热
    PrinterHead_Heat_Circle();

    PrinterMoto_Run_Circle(4);
}
