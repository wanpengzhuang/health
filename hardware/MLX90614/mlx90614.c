#include "mlx90614.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ACK	 0 
#define	NACK 1 
#define SA				0x5A //0x00 //设备地址
#define RAM_ACCESS		0x00 //RAM access command RAM
#define EEPROM_ACCESS	0x20 //EEPROM access command EEPROM
#define RAM_TOBJ1		0x07 //To1 address in the eeprom  -70.01 ~ 382.19





/*******************************************************************************
 * Function Name  : SMBus_ReadMemory
 * Description    : READ DATA FROM RAM/EEPROM
 * Input          : slaveAddress, command
 * Output         : None
 * Return         : Data
*******************************************************************************/
u16 SMBus_ReadMemory(u8 slaveAddress, u8 command)
{
    u16 data;			// Data storage (DataH:DataL)
    u8 Pec;				// PEC byte storage
    u8 DataL=0;			// Low data byte storage
    u8 DataH=0;			// High data byte storage
    u8 arr[6];			// Buffer for the sent bytes
    u8 PecReg;			// Calculated PEC byte storage
    u8 ErrorCounter;	// Defines the number of the attempts for communication with MLX90614
	uint8_t buff[3];
	
    ErrorCounter=0x00;				// Initialising of ErrorCounter
	slaveAddress <<= 1;	//2-7??±íê?′ó?úμ??·
	
    //do
    //{
//repeat:
        //gd_i2c_read_cmdspdata(slaveAddress, command, buff, 3);
		gd_i2c_read(slaveAddress, command, buff, 3,1);
		//buff[0]=gd_i2c_read_cmddata(slaveAddress, command);
		//buff[1]=gd_i2c_read_cmddata(slaveAddress, command+1);
		//buff[2]=gd_i2c_read_cmddata(slaveAddress, command+2);
		DataL = buff[0];
		DataH = buff[1];
		Pec = buff[2];

        arr[5] = slaveAddress;		//
        arr[4] = command;			//
        arr[3] = slaveAddress+1;	//Load array arr
        arr[2] = DataL;				//
        arr[1] = DataH;				//
        arr[0] = 0;					//
        PecReg=PEC_Calculation(arr);//Calculate CRC
    //}
    //while(PecReg != Pec);		//If received and calculated CRC are equal go out from do-while{}

    data = (DataH<<8) | DataL;	//data=DataH:DataL
    return data;
}

/*******************************************************************************
* Function Name  : PEC_calculation
* Description    : Calculates the PEC of received bytes
* Input          : pec[]
* Output         : None
* Return         : pec[0]-this byte contains calculated crc value
* CRC校验
*******************************************************************************/
u8 PEC_Calculation(u8 pec[])
{
    u8 	crc[6];
    u8	BitPosition=47;
    u8	shift;
    u8	i;
    u8	j;
    u8	temp;

    do
    {
        /*Load pattern value 0x000000000107*/
        crc[5]=0;
        crc[4]=0;
        crc[3]=0;
        crc[2]=0;
        crc[1]=0x01;
        crc[0]=0x07;

        /*Set maximum bit position at 47 ( six bytes byte5...byte0,MSbit=47)*/
        BitPosition=47;

        /*Set shift position at 0*/
        shift=0;

        /*Find first "1" in the transmited message beginning from the MSByte byte5*/
        i=5;
        j=0;
        while((pec[i]&(0x80>>j))==0 && i>0)
        {
            BitPosition--;
            if(j<7)
            {
                j++;
            }
            else
            {
                j=0x00;
                i--;
            }
        }/*End of while */

        /*Get shift value for pattern value*/
        shift=BitPosition-8;

        /*Shift pattern value */
        while(shift)
        {
            for(i=5; i<0xFF; i--)
            {
                if((crc[i-1]&0x80) && (i>0))
                {
                    temp=1;
                }
                else
                {
                    temp=0;
                }
                crc[i]<<=1;
                crc[i]+=temp;
            }/*End of for*/
            shift--;
        }/*End of while*/

        /*Exclusive OR between pec and crc*/
        for(i=0; i<=5; i++)
        {
            pec[i] ^=crc[i];
        }/*End of for*/
    }
    while(BitPosition>8); /*End of do-while*/

    return pec[0];
}

 /*******************************************************************************
 * Function Name  : SMBus_ReadTemp
 * Description    : Calculate and return the temperature
 * Input          : None
 * Output         : None
 * Return         : SMBus_ReadMemory(0x00, 0x07)*0.02-273.15
*******************************************************************************/
float SMBus_ReadTemp(void)
{   
	float temp;
	
	lab1:
	//temp = SMBus_ReadMemory(SA, RAM_ACCESS|RAM_TOBJ1)*0.02-273.15;
	temp = SMBus_ReadMemory(SA,RAM_ACCESS|RAM_TOBJ1)*0.02-273.15;
	//temp = gd_i2c_read_cmddata(SA, RAM_ACCESS|RAM_TOBJ1)*0.02-273.15;
	if(temp > 0 && temp < 100)
	{
		return temp;
	}
	else
	{
		goto lab1;
	}
	
}



void mlx90614_Init(void)
{

	//如果需要初始化代码，则在这里添加
}

