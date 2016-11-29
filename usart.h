/**
	******************************************************************************
	* @file    usart.h
	* @author  ycg1024
	* @version V1.0.0
	* @date    26-Oct-2016
	* @brief    
	*          
	*
	******************************************************************************
	* @attention
	*
	******************************************************************************
	*/

#ifndef __USART1_H
#define __USART1_H

#include "stm32f10x.h"


void USART_Config(void);
//int fputc(int ch, FILE *f);
void usart2Printf(char *fmt, ...);
int usart1_avilable(void);
uint8_t usart1_read(void);
uint8_t usart1_peek(void);
void USART1_IRQHandler_ex(void);
int usart1_RevTask(void);
int usart1_TmtTask(void);
int strcatchar(char *front, char rear);
int strclr(char *str);
int sendHMIEndCmd(void);

#endif /* __USART1_H */


/******************* (C) COPYRIGHT 2016 ycg1024 ***********END OF FILE*********/