#include "bsp_spi.h"


uint8_t spi_read_write_byte(uint8_t dat)
{
        uint8_t data = 0;

        //发送数据 Sending Data
        DL_SPI_transmitData8(SPI_INST,dat);
        //等待SPI总线空闲 Wait for the SPI bus to be idle
        while(DL_SPI_isBusy(SPI_INST));
        //接收数据 Receiving Data
        data = DL_SPI_receiveData8(SPI_INST);
        //等待SPI总线空闲 Wait for the SPI bus to be idle
        while(DL_SPI_isBusy(SPI_INST));

        return data;
}
//读取芯片ID
//返回值如下:
//0XEF13,表示芯片型号为W25Q80
//0XEF14,表示芯片型号为W25Q16
//0XEF15,表示芯片型号为W25Q32
//0XEF16,表示芯片型号为W25Q64
//0XEF17,表示芯片型号为W25Q128
//读取设备ID
//Read chip ID
//The return value is as follows:
//0XEF13, indicating the chip model is W25Q80
//0XEF14, indicating the chip model is W25Q16
//0XEF15, indicating the chip model is W25Q32
//0XEF16, indicating the chip model is W25Q64
//0XEF17, indicating the chip model is W25Q128
//Read device ID
uint16_t W25Q32_readID(void)
{
    uint16_t  temp = 0;
    //将CS端拉低为低电平 Pull the CS terminal to a low level
    SPI_CS(0);
    //发送指令90h Send command 90h
    spi_read_write_byte(0x90);//发送读取ID命令 Send the Read ID command
    //发送地址  000000H Send address 000000H
    spi_read_write_byte(0x00);
    spi_read_write_byte(0x00);
    spi_read_write_byte(0x00);

    //接收数据 Receiving Data
    //接收制造商ID Receive Manufacturer ID
    temp |= spi_read_write_byte(0xFF)<<8;
    //接收设备ID Receive device ID
    temp |= spi_read_write_byte(0xFF);
    //恢复CS端为高电平 Restore CS terminal to high level
    SPI_CS(1);
    //返回ID Return ID
    return temp;
}

//发送写使能 Send write enable
void W25Q32_write_enable(void)
{
    //拉低CS端为低电平 Pull the CS terminal to low level
    SPI_CS(0);
    //发送指令06h Send command 06h
    spi_read_write_byte(0x06);
    //拉高CS端为高电平 Pull the CS terminal high
    SPI_CS(1);
}

void W25Q32_wait_busy(void)
{
    unsigned char byte = 0;
    do
     {
        //拉低CS端为低电平 Pull the CS terminal to low level
        SPI_CS(0);
        //发送指令05h Send command 05h
        spi_read_write_byte(0x05);
        //接收状态寄存器值 Receive Status Register Value
        byte = spi_read_write_byte(0Xff);
        //恢复CS端为高电平 Restore CS to high level
        SPI_CS(1);
        //判断BUSY位是否为1 如果为1说明在忙，重新读写BUSY位直到为0
		// Check if BUSY is 1. If it is 1, it means it is busy. Re-read and write BUSY until it is 0
     }while( ( byte & 0x01 ) == 1 );
}

/**********************************************************
 * 函 数 名 称：W25Q32_erase_sector
 * 函 数 功 能：擦除一个扇区
 * 传 入 参 数：addr=擦除的扇区号
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：addr=擦除的扇区号，范围=0~15
 * Function name: W25Q32_erase_sector
 * Function: Erase a sector
 * Input parameter: addr = erased sector number
 * Function return: None
 * Author: LC
 * Note: addr = erased sector number, range = 0~15 
**********************************************************/
void W25Q32_erase_sector(uint32_t addr)
{
        //计算扇区号，一个扇区4KB=4096  Calculate the sector number, one sector 4KB=4096
        addr *= 4096;
        W25Q32_write_enable();  //写使能 Write enable
        W25Q32_wait_busy();     //判断忙，如果忙则一直等待 Judge busy, if busy, wait
        //拉低CS端为低电平 Pull down the CS end to a low level
        SPI_CS(0);
        //发送指令20h Send instruction 20h
        spi_read_write_byte(0x20);
        //发送24位扇区地址的高8位 Send the high 8 bits of the 24-bit sector address
        spi_read_write_byte((uint8_t)((addr)>>16));
        //发送24位扇区地址的中8位 Send the middle 8 bits of the 24-bit sector address
        spi_read_write_byte((uint8_t)((addr)>>8));
        //发送24位扇区地址的低8位 Send the low 8 bits of the 24-bit sector address
        spi_read_write_byte((uint8_t)addr);
        //恢复CS端为高电平 Restore the CS end to a high level
        SPI_CS(1);
        //等待擦除完成 Wait for erase to complete
        W25Q32_wait_busy();
}

/**********************************************************
 * 函 数 名 称：W25Q32_write
 * 函 数 功 能：写数据到W25Q32进行保存
 * 传 入 参 数：buffer=写入的数据内容        addr=写入地址        numbyte=写入数据的长度
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：无
 * Function name: W25Q32_write
 * Function function: Write data to W25Q32 for storage
 * Input parameters: buffer = data content to be written addr = write address numbyte = length of written data
 * Function return: None
 * Author: LC
 * Notes: None
**********************************************************/
void W25Q32_write(uint8_t* buffer, uint32_t addr, uint16_t numbyte)
{
    unsigned int i = 0;
    //擦除扇区数据 Erase sector data
    W25Q32_erase_sector(addr/4096);
    //写使能 Write enable
    W25Q32_write_enable();
    //忙检测 Busy detection
    W25Q32_wait_busy();
    //写入数据 Write data
    //拉低CS端为低电平 Pull CS to low level
    SPI_CS(0);
    //发送指令02h Send instruction 02h
    spi_read_write_byte(0x02);
    //发送写入的24位地址中的高8位 
	// Send the high 8 bits of the 24-bit address to be written
    spi_read_write_byte((uint8_t)((addr)>>16));
    //发送写入的24位地址中的中8位 
	// Send the middle 8 bits of the 24-bit address to be written
    spi_read_write_byte((uint8_t)((addr)>>8));
    //发送写入的24位地址中的低8位 
	// Send the low 8 bits of the 24-bit address to be written
    spi_read_write_byte((uint8_t)addr);
    //根据写入的字节长度连续写入数据buffer 
	// Continuously write data buffer according to the length of the written byte
    for(i=0;i<numbyte;i++)
    {
        spi_read_write_byte(buffer[i]);
    }
    //恢复CS端为高电平 Restore CS end to high level
    SPI_CS(0);
    //忙检测 Busy detection
    W25Q32_wait_busy();
}

/**********************************************************
 * 函 数 名 称：W25Q32_read
 * 函 数 功 能：读取W25Q32的数据
 * 传 入 参 数：buffer=读出数据的保存地址  read_addr=读取地址   read_length=读去长度
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：无
 * Function name: W25Q32_read
 * Function function: Read W25Q32 data
 * Input parameters: buffer = storage address of read data read_addr = read address read_length = read length
 * Function return: None
 * Author: LC
 * Notes: None
**********************************************************/
void W25Q32_read(uint8_t* buffer,uint32_t read_addr,uint16_t read_length)
{
        uint16_t i;
        //拉低CS端为低电平 Pull the CS end to a low level
        SPI_CS(0);
        //发送指令03h Send instruction 03h
        spi_read_write_byte(0x03);
        //发送24位读取数据地址的高8位
	    // Send the high 8 bits of the 24-bit read data address
        spi_read_write_byte((uint8_t)((read_addr)>>16));
        //发送24位读取数据地址的中8位
		// Send the middle 8 bits of the 24-bit read data address
        spi_read_write_byte((uint8_t)((read_addr)>>8));
        //发送24位读取数据地址的低8位
		// Send the low 8 bits of the 24-bit read data address
        spi_read_write_byte((uint8_t)read_addr);
        //根据读取长度读取出地址保存到buffer中
		// Read the address according to the read length and save it in the buffer
        for(i=0;i<read_length;i++)
        {
            buffer[i]= spi_read_write_byte(0XFF);
        }
        //恢复CS端为高电平 Restore the CS end to a high level
        SPI_CS(1);
}

