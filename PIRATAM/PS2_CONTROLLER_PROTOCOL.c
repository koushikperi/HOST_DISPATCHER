nclude "constants.h"

//pin out
#define ps2CLK P3_7
#define ps2ATTN P2_0
#define ps2CMD P3_2 //P2_6
#define ps2DATA P3_3 //P2_7
#define ps2ACK P2_1

void delayMicroSeconds( long ms ) {
	int i,j;

	for(j = 0;j<ms;j++)
		for(i = 0; i < 6;i++);//10 micro
}

/*Reads a byte from the ps2Data port and returns it as an int
* Read on falling edge of clock so that the signal has some time to stabalize
*/
int Get_Byte_From_PS2_Controller(void)
{
	int byte = 0;
	int j;

	for(j = 0; j < 8; j++)
	{
		ps2CLK = 0;
		delayMicroSeconds(1);//microseconds 25
		//printf("%d ",byte);
		byte = (byte<<1)|ps2DATA;
		ps2CLK = 1;
		delayMicroSeconds(1);
	}
	//printf("\n");
	return byte;
}

int Send_Data_and_get_ID()
{
	int Data = 0x42, temp = 0, bit1,j;
	int ID1 = 0;

	for(j = 0;j<8;j++)
	{
		ID1 = ID1 << 1;
		bit1 = ps2DATA;		
		ps2CLK = 0;
		ID1 = ID1|bit1;
		delayMicroSeconds(1);
		
		ps2CMD = Data&0x01;
		
		ps2CLK = 1;
		
		Data = Data>>1;
		delayMicroSeconds(1);

	}
	return ID1;
}

void Send_Byte_To_Controller(int byte_to_send)
{
	int j;

	for(j = 0; j< 8; j++)
	{
		ps2CLK = 0;
		delayMicroSeconds(1);//microseconds 16.4
		ps2CMD = byte_to_send&0x01;
		ps2CLK = 1;
		byte_to_send = byte_to_send>>1;
		delayMicroSeconds(1);
	}
}


/* Microcontroller has to send acknowledge pin to low for one clock cycle 
*  after each 8 bits are sent.
*/
void Acknowledge(void)
{
	ps2ACK = 0;
	delayMicroSeconds(1);
	ps2CLK = 1;
	delayMicroSeconds(1);
	ps2CLK = 0;
	ps2ACK = 1;
}


//Reference: http://www.mikroe.com/forum/viewtopic.php?t=8792
unsigned char readStateofController(void)
//int main()
{
	int status = 0, j, up = 0, down = 0;
	int ID = 0, FirstByte = 0, SecondByte = 0, ThirdByte = 0, FourthByte = 0, FifthByte = 0, SixthByte = 0;
	/*Controller sends 0x5A which is going to be stored in dataACK*/
	int dataACK;
	unsigned char state;

	/*Setup for testing the program
	* Needs to be removed/commented for integration with beacon code
	*/
	CLKREG=0x00; // TPS=0000B
	setbaud_timer2(TIMER_2_RELOAD); // Initialize serial port using timer 2 
	if(VERBOSE)printf("Begin transmission\n");
	
	//beginning of transmission
	ps2ACK = 1;
	ps2CLK = 1;
	ps2ATTN = 0; //activate controller
	ps2CMD = 1;
	delayMicroSeconds(1); //delay for the lulz
	
	/*Send byte 0x01 to issue start command*/
	//if(VERBOSE) printf("Sending byte to controller\n");
	Send_Byte_To_Controller(0x01); //signal start of communication cycle
	Acknowledge();
	ps2CMD = 1;
    delayMicroSeconds(250);
	/*Controller replies with ID which we read and store, and microcontroller
	* is sending 0x42 to request data
	*/
	ID = Send_Data_and_get_ID();
	Acknowledge();
	ps2CMD = 1;
	delayMicroSeconds(250);
	//if(VERBOSE)printf("ID read is: %d\n", ID);

	/*Controller 	
 reply with 0x5A to tell us that it is sending the data
	*
	*/
	dataACK = Get_Byte_From_PS2_Controller();
	Acknowledge();
	//if(VERBOSE)printf("DataAck is: %d. This should be 0x5A\n",dataACK);
	
	/*Now, the controller transmits the data bits
	* In the case where ID is 0x41, the controller is in
	* digital mode so it will only send two bytes
	*/
	for(j = 0;j<1;j++)//two bytes
	{
		FirstByte = Get_Byte_From_PS2_Controller();
		//delayMicroSeconds(250);
		SecondByte = Get_Byte_From_PS2_Controller();
		ThirdByte = Get_Byte_From_PS2_Controller();
		FourthByte = Get_Byte_From_PS2_Controller();
		FifthByte = Get_Byte_From_PS2_Controller();
		SixthByte = Get_Byte_From_PS2_Controller();
		
		
	}	

	ps2ATTN = 1; //end transmition and deactivate controller
	
	/* Return protocol
	*	For manual control-
	*		0.Stop
	*		1.forward
	*		2.backwards
	*		3.right
	*		4.left
	*
	*	For mapping
	*		| Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
	*		   Up	  Down	  Right	  Left	   /\		X	   |_|      O
	*
	*		  |______Analog Stick_______|       |_____For Mapping______|
	*
	*/

	
	/*if(VERBOSE) printf("L2: %3d  R2: %3d  L1:%3d  R1:%3d  /\\:%3d  O:%3d  X:%3d  |_|:%3d  \n", 
		(SecondByte&0x80), (SecondByte&0x40), 
		(SecondByte&0x20), (SecondByte&0x10),
		(SecondByte&0x08), (SecondByte&0x04),
		(SecondByte&0x02), (SecondByte&0x01));
	*/
	

	//This is for analog stick
	state = 0;

	if(FifthByte==0x00)
	{
		if(VERBOSE)printf("Robot goes Left\n");
		state =  (0x01<<4)|state;
	}
	if(FifthByte==0xFF)
	{
		if(VERBOSE)printf("Robot goes Right\n");
		state =  (0x01<<5)|state;;
	}
	if(SixthByte==0x00)
	{
		if(VERBOSE)printf("Robot goes Forward\n");
		state =  (0x01<<7)|state;
	}
	if(SixthByte==0xFF)
	{
		if(VERBOSE)printf("Robot goes Backwards\n");
		state = (0x01<<6)|state;
	}
	//This is for the button pad
	//printf("SecondByte is: %d\n",SecondByte);
	if (!(SecondByte&0x08))
	{
		if(VERBOSE)printf("Robot goes forward ( Triangle pressed )\n");
		state = (0x01<<3)|state;
	}
	if (!(SecondByte&0x04))
	{
		if(VERBOSE)printf("Robot goes left ( Circle pressed )\n");
		state = (0x01)|state;
	}
	if (!(SecondByte&0x02))
	{
		if(VERBOSE)printf("Robot goes backwards ( X pressed )\n");
		state = (0x01<<2)|state;
	}
	if (!(SecondByte&0x01))
	{
		if(VERBOSE)printf("Robot goes right ( Square pressed )\n");
		state = (0x01<<1)|state;
	}
		//printf("State is %d: \n",state);
		return state;
}

