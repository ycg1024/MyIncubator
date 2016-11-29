#include "usart.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>

#define U1_IT_MaxBufferSize 128 // usart1�����жϻ����С
#define U2_CMD_BUFFER_LEN 128   // usart2���ͻ���

// usart1�����жϻ���
uint8_t U1_IT_rxBuffer[U1_IT_MaxBufferSize];
// usart1 ���ͻ�����
extern char U1_rxBuffer[128];
// usart1 ���ջ�����
extern char U1_txBuffer[128];
// usart2 ���ͻ�����
extern char U2_rxBuffer[64];
// HMIЭ�����֡
static uint8_t endCmd[3] = {0xff, 0xff, 0xff};
// Ŀ���¶�
extern float TargetTemp;
// �ں����ݴ洢��
extern float SensorTempProcessed[3];
// �봮�����Ĺ����л���־ 0������������¶�  1��ͼ�������������
extern int8_t Usart1TmtChoiceFlag;
// ����������
extern uint16_t HeatPower;
// ���ȹ���
extern uint16_t FanPower;

struct que // usart1�ж϶��нṹ��
{
  uint8_t *head;
  uint8_t *tail;
} rxQue;

/**
	* @brief  USART����, 	usart1 ����ģʽ 115200 8-N-1 ˫�շ� + NVIC + QUE 
	*											usart2 ����ģʽ 115200 8-N-1 �������� ��GPIO����Ȼ��ʼ������
	* @param  ��
	* @return ��
	* @attention ��
	*/
void USART_Config(void)
{
  //usart1��ʼ����GPIO+USART+NVIC+CircleReceive��
  /*************************************************************************************************/
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* config USART1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

  /* USART1 GPIO config */
  /* Configure USART1 Tx (PA.09) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Configure USART1 Rx (PA.10) as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* USART1 mode config */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);

  USART_Cmd(USART1, ENABLE);

  //Usart1 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;

  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ��ʹ��
  NVIC_Init(&NVIC_InitStructure);		  //��ʼ������NVIC�Ĵ���USART1

  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); //�����ж�

  //Usart1 ���ջ��ζ��г�ʼ��
  rxQue.head = U1_IT_rxBuffer;
  rxQue.tail = U1_IT_rxBuffer;
  /*************************************************************************************************/

  //usart2��ʼ����GPIO+USART��
  /*************************************************************************************************/

  /* config USART2 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

  /* USART1 GPIO config */
  /* Configure USART2 Tx (PA.02) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Configure USART2 Rx (PA.3) as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* USART2 mode config */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART2, &USART_InitStructure);

  USART_Cmd(USART2, ENABLE);
  /*************************************************************************************************/
}

/**
	* @brief  �ض���c�⺯��printf��USART1
	* @param  ��
	* @return ��
	* @attention ��
	*/
int fputc(int ch, FILE *f)
{
  /* ����һ���ֽ����ݵ�USART1 */
  USART_SendData(USART1, (uint8_t)ch);

  /* �ȴ�������� */
  while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
    ;

  return (ch);
}

/**
	* @brief  �ض���c�⺯��scanf��USART1
	* @param  ��
	* @return ��
	* @attention ��
	*/
int fgetc(FILE *f)
{
  /* �ȴ�����1�������� */
  while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
    ;

  return (int)USART_ReceiveData(USART1);
}

/**
	* @brief  �Զ�����printf������UASRT2
	* @param  ��
	* @return ��
	* @attention ��
	*/
void usart2Printf(char *fmt, ...)
{
  char buffer[U2_CMD_BUFFER_LEN - 1];
  u8 i = 0;
  u8 len;

  va_list arg_ptr;						//Define convert parameters variable
  va_start(arg_ptr, fmt);					//Init variable
  len = vsnprintf(buffer, U2_CMD_BUFFER_LEN + 1, fmt, arg_ptr); //parameters list format to buffer

  while ((i < U2_CMD_BUFFER_LEN) && (i < len) && (len > 0))
  {
    USART_SendData(USART2, (u8)buffer[i++]);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
      ;
  }
  va_end(arg_ptr);
}

/**
	* @brief  ����HMIЭ���е�ֹͣ֡
	* @param  ��
	* @return ��
	* @attention ��
	*/
void usart1_sendEndCmd(void)
{
  int i = 0;

  for (i = 0; i < 3; i++)
  {
    /* ͨ������1���� */
    USART_SendData(USART1, endCmd[i]);
    /* �ȴ�������� */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
      ;
  }
}

/**
	* @brief  ��ȡ���λ�������е�Ԫ�أ����Ӷ�����ȥ��
	* @param  ��
	* @return ��ȡ�����ַ�
	* @attention Ԥ��Ҫ����uart1_avilable()���ж϶������Ƿ��д�����Ԫ��
	*/
uint8_t usart1_read(void)
{
  uint8_t temp = 0;
  if (usart1_avilable() == 0)
  {
    return 0;
  }
  if (rxQue.head != (U1_IT_rxBuffer + U1_IT_MaxBufferSize - 1))
  {
    temp = *rxQue.head;
    rxQue.head++;
  }
  else
  {
    temp = *rxQue.head;
    rxQue.head = U1_IT_rxBuffer;
  }

  return temp;
}

/**
	* @brief  ��ȡ���λ�������е�Ԫ�أ���ȥ��Ԫ��
	* @param  ��
	* @return ��ȡ�����ַ�
	* @attention Ԥ��Ҫ����uart1_avilable()���ж϶������Ƿ��д�����Ԫ��
	*/
uint8_t usart1_peek(void)
{
  uint8_t temp = 0;
  if (usart1_avilable() == 0)
  {
    return 0;
  }

  temp = *rxQue.head;

  return temp;
}

/**
	* @brief  �ж�usart1�Ļ��λ���������Ƿ���δ������Ԫ��
	* @param  ��
	* @return ��
	* @attention ��
	*/
int usart1_avilable(void)
{
  if (rxQue.head != rxQue.tail)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}
/**
	* @brief  usart1���жϺ��������ļ��ϣ������USART1_IRQHandler()��
	* @param  ��
	* @return ��
	* @attention ��
	*/
void USART1_IRQHandler_ex(void)
{
  if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
  {
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
      ;
    //		temp=USART_ReceiveData(USART1);
    if (rxQue.tail != (U1_IT_rxBuffer + U1_IT_MaxBufferSize - 1))
    {
      *rxQue.tail = USART_ReceiveData(USART1);
      rxQue.tail++;
    }
    else
    {
      *rxQue.tail = USART_ReceiveData(USART1);
      rxQue.tail = U1_IT_rxBuffer;
    }
  }
}

/**
	* @brief  usart1�Ľ�������
	* @param  ��
	* @return ��
	* @attention ��
	*/
int usart1_RevTask(void)
{
  if (usart1_avilable() > 0)
  {
    while (usart1_peek() != 0xff)
    {
      strcatchar(U1_rxBuffer, (char)(usart1_read()));
      Delay_us(200);
    }
    usart1_read();
    Delay_us(200);
    usart1_read();
    Delay_us(200);
    usart1_read();

    if (U1_rxBuffer[0] == 0x03)
    {
      if (U1_rxBuffer[1] == 0x07)
      {
        if (U1_rxBuffer[2] == 0x71)
        {
          if (U1_rxBuffer[3] == 0x00)
          {
						
          }
          if (U1_rxBuffer[3] == 0x01)
          {
          }
        }
      }
    }
  }
  strclr(U1_rxBuffer);
  return 1;
}
/**
	* @brief  usart1�ķ�������
	* @param  ��
	* @return ��
	* @attention ��
	*/
int usart1_TmtTask(void)
{
	if(Usart1TmtChoiceFlag == 0)
	{
		printf("main.t0.txt=%.1f",SensorTempProcessed[0]);
		sendHMIEndCmd();
		printf("main.h0.val=%d", HeatPower);
		sendHMIEndCmd();
		printf("main.h1.val=%d", FanPower);
		sendHMIEndCmd();
	}
	else if(Usart1TmtChoiceFlag == 1)
	{
		printf("add 1,0,%d",(int)(SensorTempProcessed[0] * 3));
		sendHMIEndCmd();
		printf("add 1,1,%d",(int)(TargetTemp * 3));
		sendHMIEndCmd();
	}
	else
	{
		;
	}
  return 1;
}
/**
	* @brief  ��char��׷�ӵ��ַ�������
	* @param  ��
	* @return 0��failed 1��successful
	* @attention ��
	*/
int strcatchar(char *front, char rear)
{
  uint8_t pos = 0;

  pos = strlen(front);
  front[pos] = rear;
  front[pos + 1] = '\0';

  return 1;
}
/**
	* @brief  ����ַ���
	* @param  ��
	* @return 0��failed 1��successful
	* @attention ��
	*/
int strclr(char *str)
{
  str[0] = '\0';
  return 1;
}
/**
	* @brief  ����HMIЭ��ֹͣ֡
	* @param  ��
	* @return 0��failed 1��successful
	* @attention ��
	*/
int sendHMIEndCmd(void)
{
	printf("%X%X%X",endCmd[0],endCmd[1],endCmd[2]);
	return 1;
}
/**
	* @brief  ��
	* @param  ��
	* @return ��
	* @attention ��
	*/

/*********************************************END OF FILE**********************/