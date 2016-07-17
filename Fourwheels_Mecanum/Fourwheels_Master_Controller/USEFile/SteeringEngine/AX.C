#include"stm32f10x.h"
#include "AX.h"
#include "usart2.h"
#include "led.h"
#define RX_TIMEOUT_COUNT2   500L
#define RX_TIMEOUT_COUNT1  (RX_TIMEOUT_COUNT2*10L)

volatile uint8_t gbpParameter[128];				 //���ݰ�����
volatile uint8_t gbRxBufferReadPointer;
volatile uint8_t gbpRxBuffer[128];				 //���������Ļ�������
volatile uint8_t gbpTxBuffer[128];				// ���ͻ���
volatile uint8_t gbRxBufferWritePointer;	    //	  
volatile uint8_t gbpRxInterruptBuffer[256];	   //	�жϽ��յĻ�������
volatile uint8_t Timer_count;
volatile uint8_t axOline[AX_MAX_NUM];
volatile uint8_t axOlineNum;
volatile uint8_t delayCount;
extern u8 pcneedpos[2];

uint8_t TxPacket(uint8_t bID, uint8_t bInstruction, uint8_t bParameterLength)
   {		//bParameterLengthΪ����PARAMETER���ȣ��������ݰ�����Ϣ(����ָ���)

	
	uint8_t bCount, bPacketLength;
	uint8_t bCheckSum;

	gbpTxBuffer[0] = 0xff;
	gbpTxBuffer[1] = 0xff;
	gbpTxBuffer[2] = bID;
	gbpTxBuffer[3] = bParameterLength + 2;		//Length(Paramter,Instruction,Checksum) 
	gbpTxBuffer[4] = bInstruction;
	for (bCount = 0; bCount < bParameterLength; bCount++)
	 {
		gbpTxBuffer[bCount + 5] = gbpParameter[bCount];
	 }
	bCheckSum = 0;
	bPacketLength = bParameterLength + 4 + 2;					  //bPacketLength����������0XFF	,��checksum
	for (bCount = 2; bCount < bPacketLength - 1; bCount++) //except 0xff,checksum

			{
		bCheckSum += gbpTxBuffer[bCount];
	}
	gbpTxBuffer[bCount] = (uint8_t) (~bCheckSum); //Writing Checksum  with  Bit Inversion
//	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    CLEAR_BUFFER;
	AX_TXD
	for (bCount = 0; bCount < bPacketLength; bCount++) 
	{
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
		USART_SendData(USART3, gbpTxBuffer[bCount]);
	}

	while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);

	AX_RXD
  //  for(i=0;i<0x8fff;i++);
	return (bPacketLength);		   //���ط��͵��������ݳ���
}

/*uint8_t RxPacket(uint8_t bRxPacketLength)
 * bRxPacketLength ����׼�����յ����ݳ���
 * return ʵ�ʷ��ص����ݳ��ȣ�����0XFF��
 * */
uint8_t RxPacket(uint8_t bRxPacketLength)
 {

	unsigned long ulCounter;
	uint8_t bCount, bLength, bChecksum;
	uint8_t bTimeout;
	bTimeout = 0;
	/*����ѭ��
	 * �����ܵ�ָ�����ȵ����ݻ��߳�ʱ��ʱ���˳�ѭ��*/
	for (bCount = 0; bCount < bRxPacketLength; bCount++) 
	{
		ulCounter = 0;
		/*�����жϽ�������
		 * �������ڵȴ�״̬
		 * ���жϻ���������û������gbRxBufferReadPointer == gbRxBufferWritePointer
		 * ʱ�����������
		 * ����ʱ�����жϻ��������������ݵ�ʱ�������ȴ�*/
		while (gbRxBufferReadPointer == gbRxBufferWritePointer) 
		{						   
			if (ulCounter++ > RX_TIMEOUT_COUNT1) 
			{
				bTimeout = 1;
				break;
			}
		}
		if (bTimeout) break;
		gbpRxBuffer[bCount] = gbpRxInterruptBuffer[gbRxBufferReadPointer++];
	} ///��������ݶ����ж϶����зŵ�RxBuffer������

	bLength = bCount;
	bChecksum = 0;
	/*Ĭ�������TxPacket��RxPacket����ʹ��
	 * ����һ�����ݰ�����BROADCASTING_IDʱ�з��ص�����*/
	if (gbpTxBuffer[2] != BROADCASTING_ID)
	 {
		if (bTimeout && (bRxPacketLength != 255)) 
		{				 //��ʱ����δ������
			//TxDString("\r\n [Error:RxD Timeout]");
			CLEAR_BUFFER; //��ս��ջ�����
			return 0;
	    }

		if (bLength > 3) //checking is available.				 //�������ݳ�������Ҫ����3λ
		{
			if (gbpRxBuffer[0] != 0xff || gbpRxBuffer[1] != 0xff) 
			{
				//TxDString("\r\n [Error:Wrong Header]");
				CLEAR_BUFFER;	  //��ս��ջ�����
				return 0;
			}
			if (gbpRxBuffer[2] != gbpTxBuffer[2]) 
			{
				//TxDString("\r\n [Error:TxID != RxID]");
				CLEAR_BUFFER;
				return 0;
			}
			if (gbpRxBuffer[3] != bLength - 4) 		 //���ܵ������ݳ���-4������length
			{					//Ĭ��״̬�����Ȳ�����255
				//TxDString("\r\n [Error:Wrong Length]");
				CLEAR_BUFFER;
				return 0;
			}
			for (bCount = 2; bCount < bLength; bCount++)
				bChecksum += gbpRxBuffer[bCount];
			if (bChecksum != 0xff) 
			{
				//TxDString("\r\n [Error:Wrong CheckSum]");
				CLEAR_BUFFER;
				return 0;
			}
		}
	}
//	USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
	return bLength;
}


//uint8_t axPing(uint8_t bID) {
//unsigned char bTxPacketLength, bRxPacketLength;
//bTxPacketLength = TxPacket(bCount, INST_PING, 0);
//bRxPacketLength = RxPacket(255); //����׼�����պö������
//if (bRxPacketLength == DEFAULT_RETURN_PACKET_SIZE)
//	;
//}

uint8_t axTorqueOff(uint8_t bID) ///�ͷŶ��������
{
	gbpParameter[0] = P_TORQUE_ENABLE; //Address of Torque
	gbpParameter[1] = 0; //Writing Data
	TxPacket(bID, INST_WRITE, 2);
	if (RxPacket(DEFAULT_RETURN_PACKET_SIZE) == DEFAULT_RETURN_PACKET_SIZE)
		return OK;
	else
		return NoSuchServo;
}

uint8_t axTorqueOn(uint8_t bID) ///ʹ�ܶ��������
{
	gbpParameter[0] = P_TORQUE_ENABLE; //Address of Torque
	gbpParameter[1] = 1; //Writing Data
	TxPacket(bID, INST_WRITE, 2);
	if (RxPacket(DEFAULT_RETURN_PACKET_SIZE) == DEFAULT_RETURN_PACKET_SIZE)
		return OK;
	else
		return NoSuchServo;
}

uint8_t axSendPosition(uint8_t bID, uint16_t target_pos, uint16_t target_speed)	  //��ĳ�����λ��
///���͸�ĳһ�����һ��Ҫ�ƶ�����λ�ã������������� 1ID 2λ�� 3�ٶ�
{
	gbpParameter[0] = P_GOAL_POSITION_L; //Address of Firmware Version
	gbpParameter[1] = target_pos; //Writing Data P_GOAL_POSITION_L
	gbpParameter[2] = target_pos >> 8; //Writing Data P_GOAL_POSITION_H
	gbpParameter[3] = target_speed; //Writing Data P_GOAL_SPEED_L
	gbpParameter[4] = target_speed >> 8; //Writing Data P_GOAL_SPEED_H
	TxPacket(bID, INST_WRITE, 5);
	if (RxPacket(DEFAULT_RETURN_PACKET_SIZE + 1) == DEFAULT_RETURN_PACKET_SIZE)
	{
	   return OK;
	}
	
	else
	{		
			return NoSuchServo;
	}
	
}

/*uint16_t axReadPosition(uint8_t bID)
 * parameter
 * 		bID �����
 * return value
 * 		���λ��
 * 		0xffff��ʾ������
 * */

uint16_t axReadPosition(uint8_t bID) 			  //��ȡĳ�������λ��
{
	unsigned int Position;
	gbpParameter[0] = P_PRESENT_POSITION_L; //λ�����ݵ���ʼ��ַ #define P_GOAL_POSITION_L (30) �μ������ֲ�
	gbpParameter[1] = 2; //��ȡ����
	TxPacket(bID, INST_READ, 2);
	if(RxPacket(DEFAULT_RETURN_PACKET_SIZE + gbpParameter[1]) != DEFAULT_RETURN_PACKET_SIZE + gbpParameter[1])
	{		 //?????????????
		Position = 0xffff;
	}
	else
	{
	    pcneedpos[0]=gbpRxBuffer[6];
		pcneedpos[1]=gbpRxBuffer[5];		//�����ݴ�����������λ��
		Position = ((unsigned int) gbpRxBuffer[6]) << 8;
		Position += gbpRxBuffer[5];
	
	}
	return Position;
}

void axTorqueOffAll(void)					//�ͷ����ж������
 {
	gbpParameter[0] = P_TORQUE_ENABLE;
	gbpParameter[1] = 0x00;
	TxPacket(BROADCASTING_ID, INST_WRITE, 2);

 }

void axTorqueOnAll(void) 				  //ʹ�����ж������
{
	gbpParameter[0] = P_TORQUE_ENABLE;
	gbpParameter[1] = 0x01;
	TxPacket(BROADCASTING_ID, INST_WRITE, 2);
}


/*void axScan(void)
 * Return Values
 * 		���߶������
 * ���ܼ��ID�ķ�Χ0~AX_MAX_NUM-1
 * ������ߵĶ�����뱣����ȫ������axOline[30]
 * ������ߵĶ����������ȫ������axOlineNum
 * */

void getServoConnective(void) 
{							 //PING����Ӽ��ſ�ʼ
	uint8_t bCount;
	axOlineNum = 0;
	for (bCount = 0; bCount < AX_MAX_NUM - 1; bCount++) ///��һ��ѭ����ѯÿһ�������״̬Ϊ�˿�������ֻɨ��0x00~0x1f
	{
		TxPacket(bCount, INST_PING, 0);
		if (RxPacket(255) == DEFAULT_RETURN_PACKET_SIZE) //������ذ��ĳ�����ȷ
		axOline[axOlineNum++] = bCount;
	}

	Packet_Reply(USART2, ServoIDNumber, (uint8_t *) &axOline[0], axOlineNum);

}


void moveServoPosition(uint8_t *p, uint8_t num) 
{
	uint8_t i, err = OK;
	uint16_t pos;
	for (i = 0; i < num / 3; i++) {
		pos = (*(p + (3 * i) + 2)) * 256 + (*(p + (3 * i) + 1));
		err |= axSendPosition(*p + (3 * i), pos, DEFAULTSPEED);
	}

	Packet_Reply(USART2, err, PNULL, 0);
}


void moveServoPosWithSpeed(uint8_t *p, uint8_t num) 
{
	uint8_t i, err = OK;
	uint16_t pos, speed;
	for (i = 0; i < num / 5; i++) 
	{
		pos = *(p + (5 * i) + 2) * 256 + *(p + (5* i) +1);
		speed = *(p + (5* i) + 4) * 256 + *(p + (5 * i)+3 );
		err |= axSendPosition(*(p + (5 * i)), pos, speed);
	}
	Packet_Reply(USART2, err, PNULL, 0);
}

/*GetServoPosition(uint8_t *p,uint8_t num)
 * parameter:
 * 		*p
 * 			ָ����Ч���������ͷָ��
 * 		num
 * 			��������Ч����
 * */
void getServoPosition(uint8_t *p, uint8_t num) 
{
	uint8_t i;
	uint8_t retdata[100];
	uint16_t tmp;
	switch (num) {
	case 1:
		if (*p == BROADCASTING_ID) { //��ȡȫ�����pos
			for (i = 0; i < axOlineNum; i++) {
				retdata[i*3] = axOline[i];
				tmp = axReadPosition(axOline[i]);
				retdata[i*3+1] =tmp & 0xff;
				retdata[i*3+2] =(tmp & 0xff00) >> 8; 
			}
			
		} else {
			retdata[0] = *p;
			tmp = axReadPosition(*p);
			retdata[1] = tmp & 0xff;
			retdata[2] = (tmp&0xff00) >> 8;
		}
		Packet_Reply(USART2, ResServoPosInfo, (uint8_t *) &retdata[0],
					axOlineNum * 3);
		break;
	//case 2:
	default:
		for (i = 0; i < num; i++) {
			retdata[i*3] = *p;
			tmp = axReadPosition(*p);
			p++;
			retdata[i*3+1] = tmp & 0xff;
			retdata[i*3+2] = (tmp & 0xff00) >> 8;
		}
		Packet_Reply(USART2, ResServoPosInfo, (uint8_t *) &retdata,
				num * 3);
		break;
	}		  
}

void enableServo(uint8_t *p, uint8_t num) 
{
	uint8_t err = OK;
	uint8_t i, *p0;
	p0 = p;
	switch (num) 
	{
	case 1:
		if (*p0 == BROADCASTING_ID) { //��ȡȫ�����pos
			axTorqueOnAll();
			err = OK;
			Packet_Reply(USART2, err, PNULL, 0);

		} else {
			err = axTorqueOn(*p0);
			Packet_Reply(USART2, err, PNULL, 0);
		}
		break;
	default:
		for (i = 0; i < num; i++)
			err |= axTorqueOn(*p0++);
		Packet_Reply(USART2, err, PNULL, 0);
		break;
	}
}

void disableServo(uint8_t *p, uint8_t num) 
{
	uint8_t err = OK;
	uint8_t i, *p0;
	p0 = p;
	switch (num) {
	case 1:
		if (*p0 == BROADCASTING_ID) { //��ȡȫ�����pos
			axTorqueOffAll();
			err = OK;
			Packet_Reply(USART2, err, PNULL, 0);
		} else {
			err = axTorqueOff(*p0++);
			Packet_Reply(USART2, err, PNULL, 0);
		}
		break;
	case 2:
		for (i = 0; i < num; i++)
			err |= axTorqueOff(*p0);
		Packet_Reply(USART2, err, PNULL, 0);
		break;
	}
}

void executeInstruction(uint8_t *p, uint8_t num) 
{
	uint8_t err;
	uint8_t i;
	//Send
	AX_TXD
	for (i = 0; i < num; i++) 
	{
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
			;
		USART_SendData(USART3, *p++);
	}
	while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
		;
	//Rec
	AX_RXD
	if (RxPacket(DEFAULT_RETURN_PACKET_SIZE) == DEFAULT_RETURN_PACKET_SIZE)
		err = OK;
	else
		err = NoSuchServo;
	Packet_Reply(USART2, err, PNULL, 0);
	
}
/***********************
*void playMicroAction(uint8_t *p,uint16_t poolSize)
 * parameter:
 * 		*p
 * 			ָ����׵�ַָ��
 * 		poolSize
 * 			ָ��ش�С*
 * void playMicroAction(uint8_t *p, uint16_t poolSize) {
	 * FF FF FE LENGTH INST_SYNC_WRITE(0X83)
	 * 1E(1st write add) 04(write bytes)
	 * ID1 	POSL   POSH   SPEEDL   SPEEDH
	 * ID1 	POSL   POSH   SPEEDL   SPEEDH
	 * ...
	 * ...
	 * IDN 	POSL   POSH   SPEEDL   SPEEDH
	 * CHECKSUM
	 * hence all package length is LENGTH(*(P+3))+4
	 *
*	uint16_t packageLength, j, time;
	uint16_t packageNum, i;
//	TimeInterval = (*p) * 10;												 //�о�Ӧ��*10																
	packageLength = (uint16_t)(*(p+1)+(uint16_t)*(p+2)*256);		//���������� ������Σ�
	packageNum = (poolSize-3)/ packageLength;  
	p = p+3;
	AX_TXD;
	for (i = 0; i < packageNum; i++) {
		//������ʱ����
		MiniActionBeginFlag = 1;
		while(NewKeyActionFlag != 1)
			;
		for (j = 0; j <packageLength; j++) {
			while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
				;
			USART_SendData(USART3, *p++);
		}
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
			;
		NewKeyActionFlag = 0;
	}
	AX_RXD;
}  
*********************/


void TxPacketBroadSynWrite(uint8_t bInstruction, uint8_t bParameterLength) {
	uint8_t bCount, bPacketLength;
	gbpTxBuffer[0] = 0xff;
	gbpTxBuffer[1] = 0xff;
	gbpTxBuffer[2] = 0xfe;
	gbpTxBuffer[3] = bParameterLength + 2;
//Length(Paramter,Instruction,Checksum) 
	gbpTxBuffer[4] = bInstruction;
	for (bCount = 0; bCount < bParameterLength; bCount++) {
		gbpTxBuffer[bCount + 5] = gbpParameter[bCount];
	}
	bPacketLength = bParameterLength + 5;											 //����������

	AX_TXD
	for (bCount = 0; bCount < bPacketLength; bCount++) {
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
			;
		USART_SendData(USART3, gbpTxBuffer[bCount]);
	}
	while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
		;

	AX_RXD;
}

void changeServoID(uint8_t *p, uint8_t num)
{
	gbpParameter[0] = P_ID; //���ID��ַ
	gbpParameter[1] = *p;
	if(num == 1){							 //һ��ֻ�ܸ�1���ƶ���ID
	TxPacket(BROADCASTING_ID,INST_WRITE,2);
	getServoConnective();
	}
	else{
		Packet_Reply(USART2, CheckError, (void *)0, 0);
	}
}


uint8_t axSendSpeed(uint8_t bID, uint16_t target_speed)
///���͸�ĳһ�����һ��Ҫ�ƶ�����λ�ã������������� 1ID 2λ�� 3�ٶ�
{
	gbpParameter[0] = P_GOAL_SPEED_L; //Address of Firmware Version
	gbpParameter[3] = target_speed; //Writing Data P_GOAL_SPEED_L
	gbpParameter[4] = target_speed >> 8; //Writing Data P_GOAL_SPEED_H
	TxPacket(bID, INST_WRITE, 3);
	if (RxPacket(DEFAULT_RETURN_PACKET_SIZE) == DEFAULT_RETURN_PACKET_SIZE)
		return OK;
	else
		return NoSuchServo;
}

//  ff Length_L	  Length_H	 0	 2	   InstrType	****	   Check_Sum
//InstrType�����ظ�ָ�����ͣ�data�����������ݵ�ָ��,length����Ҫ���͵�Ҳ�����ҵ��¼ҷ��ظ��ҵ����ݳ��ȣ�Ҫ����͸���λ��
void Packet_Reply(USART_TypeDef* USARTx, unsigned char InstrType,unsigned char * data, unsigned int length) 						
	{			   //lengthֻ����data�ĳ���

	 unsigned char Length_H, Length_L, Check_Sum = 0;
	 unsigned int j = 0;
	 Length_H = (length + 6) >> 8;
	 Length_L = (length + 6);
	 
	 RS485_TX_MODE		   //��Ƭ������485����	
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)	 //�������ݼĴ����ձ�־λ
	 ;

	 USART_SendData(USARTx, 0XFF); //��ͷ
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
	 ;

	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
	 ;

	 USART_SendData(USARTx, Length_L); //����l
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
	 ;
	 Check_Sum = Check_Sum + Length_L;
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
	 ;

	 USART_SendData(USARTx, Length_H); //����2
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
	 ;
	 Check_Sum = Check_Sum + Length_H;

	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
	 ;

	 USART_SendData(USARTx, 0); //ID =0,�������ֶ��
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
	 ;
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
	 ;

	 USART_SendData(USARTx, 2); //�������ǻظ���Ϣ
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
	 ;
	 Check_Sum = Check_Sum + 2;

	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
	 ;

	 USART_SendData(USARTx, InstrType); //����ָ������
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
	 ;
	 Check_Sum = Check_Sum + InstrType;

	 //������Ч����
	 for (j = 0; j < length; j++) 
	 {
		 while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
		 ;
	
		 USART_SendData(USARTx, *(data + j));
		 Check_Sum = Check_Sum + *(data + j);
		 while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
		 ;
	 }
	 Check_Sum = ~Check_Sum; //����У���

	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
	 ;

	 USART_SendData(USARTx, Check_Sum); //����ָ������
	 while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
	 ;

	 Check_Sum = 0;
     RS485_RX_MODE		 //����485 
}