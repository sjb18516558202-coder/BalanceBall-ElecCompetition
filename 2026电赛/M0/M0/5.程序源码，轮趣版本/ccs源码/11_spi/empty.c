#include "ti_msp_dl_config.h"
#include "board.h"
#include "lcd_init.h"
#include "lcd.h"
#include "pic.h"
#include "bsp_spi.h"
#include <stdint.h>
#include <stdio.h>


int main(void)
{
	unsigned char buff[10] = {0};
	
	//开发板初始化 Development board initialization
	board_init();

	delay_ms(100);//等待部署 Waiting for deployment
	
	//读取W25Q32的ID Read the ID of W25Q32
	printf("ID = %X\r\n",W25Q32_readID());

	//读取0地址的5个字节数据到buff Read 5 bytes of data from address 0 to buff
	W25Q32_read(buff, 0, 7);

	//串口输出读取的数据 Serial port outputs the read data
	printf("buff = %s\r\n",buff);

	//往0地址写入5个字节长度的数据 ABCD Write 5 bytes of data ABCD to address 0
	W25Q32_write("Yahboom", 0, 7);
	delay_ms(100);

	//读取0地址的5个字节数据到buff Read 5 bytes of data from address 0 to buff
	W25Q32_read(buff, 0, 7);
	
	//串口输出读取的数据 Serial port outputs the read data
    //	printf("buff = %s\r\n",buff);
				
	SYSCFG_DL_init();
	
	LCD_Init();//LCD初始化 LCD Initialization
	LCD_Fill(0,0,LCD_W,LCD_H,WHITE);	
	LCD_ShowPicture(20,45,120,29,gImage_pic1);
	LCD_ShowString(10,0,"Hello!",BLACK,WHITE,16,0);
	LCD_ShowChinese(50,20,"亚博智能",BLACK,WHITE,16,0);
	
    // 显示 buff 中的内容在同一行上 Display the contents of buff on the same line
    int x = 54;  // 起始 x 坐标 Starting x coordinate
    for (int i = 0; i < 7; i++) 
		{
        char str[2] = {buff[i], '\0'};  // 每次取一个字符 Take one character at a time
        LCD_ShowString(x, 36, str,BLACK, WHITE, 16, 0);  // 显示字符 Display Characters
        x += 8;  // 每个字符的宽度为 8，x 坐标向右偏移 Each character is 8 wide and has an x-coordinate offset to the right.
    }

	while (1)
	{

	}
}