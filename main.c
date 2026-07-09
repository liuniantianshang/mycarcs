#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include <stdio.h>
#include <string.h>

#define assert_param(expr) ((void)0)

/* ===================== RGB??? ===================== */
#define RGB_PORT      GPIOC
#define RGB_CLK       RCC_APB2Periph_GPIOC
#define RGB_LED_R_PIN GPIO_Pin_0
#define RGB_LED_G_PIN GPIO_Pin_1
#define RGB_LED_B_PIN GPIO_Pin_2

#define RGB_SET(R,G,B) GPIO_WriteBit(GPIOC, GPIO_Pin_0, (R ? Bit_SET : Bit_RESET)); \
                       GPIO_WriteBit(GPIOC, GPIO_Pin_1, (G ? Bit_SET : Bit_RESET)); \
                       GPIO_WriteBit(GPIOC, GPIO_Pin_2, (B ? Bit_SET : Bit_RESET));

#define COLOR_RED   0
#define COLOR_GREEN 1
#define COLOR_BLUE  2
#define COLOR_WHITE 3

/* ===================== ???? ===================== */
static uint8_t send_buff[50];
static int16_t target_speed_m1 = 0; // ?? (Motor 1)
static int16_t target_speed_m4 = 0; // ?? (Motor 4)

/* ===================== ???? ===================== */
static void delay_ms(uint32_t ms) {
    uint32_t i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 7200; j++);
}

/* ===================== ??1: ????? (PA9/PA10) ===================== */
static void UART1_Init_Pi(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // PA9 -> USART1_TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA10 -> USART1_RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    // ??????
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

/* ===================== RGB? ===================== */
static void init_RGB_GPIO(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RGB_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin = RGB_LED_R_PIN | RGB_LED_G_PIN | RGB_LED_B_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(RGB_PORT, &GPIO_InitStructure);
    GPIO_SetBits(RGB_PORT, RGB_LED_R_PIN | RGB_LED_G_PIN | RGB_LED_B_PIN);
}

static void RGB_control(uint8_t color) {
    switch (color) {
        case COLOR_RED:   RGB_SET(0, 1, 1) break;
        case COLOR_GREEN: RGB_SET(1, 0, 1) break;
        case COLOR_BLUE:  RGB_SET(1, 1, 0) break;
        case COLOR_WHITE: RGB_SET(0, 0, 0) break;
        default:          RGB_SET(1, 1, 1)
    }
}

/* ????????????? */
void USART1_IRQHandler(void) {
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t rx_data = USART_ReceiveData(USART1);
        
        // ???WASD????
        switch(rx_data) {
            case 'W': case 'w': // ??
                target_speed_m1 = 400;
                target_speed_m4 = 400;
                RGB_control(COLOR_GREEN);
                break;
            case 'S': case 's': // ??
                target_speed_m1 = -400;
                target_speed_m4 = -400;
                RGB_control(COLOR_BLUE);
                break;
            case 'A': case 'a': // ??
                target_speed_m1 = -300;
                target_speed_m4 = 300;
                RGB_control(COLOR_WHITE);
                break;
            case 'D': case 'd': // ??
                target_speed_m1 = 300;
                target_speed_m4 = -300;
                RGB_control(COLOR_WHITE);
                break;
            case 'X': case 'x': // ??
            case ' ':           // ?????
                target_speed_m1 = 0;
                target_speed_m4 = 0;
                RGB_control(COLOR_RED);
                break;
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

/* ===================== ??2: ????? (PA2/PA3) ===================== */
static void UART2_Init_Driver(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // PA2 -> USART2_TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA3 -> USART2_RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    USART_Cmd(USART2, ENABLE);
}

static void Send_Motor_U8(uint8_t Data) {
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    USART_SendData(USART2, Data);
}

static void Send_Motor_ArrayU8(uint8_t *pData, uint16_t Length) {
    while (Length--) {
        Send_Motor_U8(*pData);
        pData++;
    }
}

/* ===================== ????????? ===================== */
static void send_motor_type(uint8_t data) {
    sprintf((char*)send_buff, "$mtype:%d#", data);
    Send_Motor_ArrayU8(send_buff, strlen((char*)send_buff));
}
static void send_motor_deadzone(uint16_t data) {
    sprintf((char*)send_buff, "$deadzone:%d#", data);
    Send_Motor_ArrayU8(send_buff, strlen((char*)send_buff));
}
static void send_pulse_line(uint16_t data) {
    sprintf((char*)send_buff, "$mline:%d#", data);
    Send_Motor_ArrayU8(send_buff, strlen((char*)send_buff));
}
static void send_pulse_phase(uint16_t data) {
    sprintf((char*)send_buff, "$mphase:%d#", data);
    Send_Motor_ArrayU8(send_buff, strlen((char*)send_buff));
}
static void send_wheel_diameter(float data) {
    sprintf((char*)send_buff, "$wdiameter:%.3f#", data);
    Send_Motor_ArrayU8(send_buff, strlen((char*)send_buff));
}
static void send_upload_data(uint8_t all_enc, uint8_t ten_enc, uint8_t speed) {
    sprintf((char*)send_buff, "$upload:%d,%d,%d#", all_enc, ten_enc, speed);
    Send_Motor_ArrayU8(send_buff, strlen((char*)send_buff));
}
static void Contrl_Speed(int16_t m1, int16_t m2, int16_t m3, int16_t m4) {
    sprintf((char*)send_buff, "$spd:%d,%d,%d,%d#", m1, m2, m3, m4);
    Send_Motor_ArrayU8(send_buff, strlen((char*)send_buff));
}



/* ===================== ??? ===================== */
int main(void) {
    // 1. ?????
    init_RGB_GPIO();
    UART2_Init_Driver(); // ????????????
    UART1_Init_Pi();     // ??????????

    RGB_control(COLOR_BLUE); // ??:????
    delay_ms(100);

    // 2. ???????
    Contrl_Speed(0, 0, 0, 0);
    delay_ms(100);
    send_upload_data(0, 0, 0);
    delay_ms(10);

    send_motor_type(1);            // 520??
    delay_ms(100);
    send_pulse_phase(30);          // ???30
    delay_ms(100);
    send_pulse_line(11);           // ???11
    delay_ms(100);
    send_wheel_diameter(67.00);    // ????67mm
    delay_ms(100);
    send_motor_deadzone(1900);     // ??1900
    delay_ms(100);

    RGB_control(COLOR_RED); // ??:?????,????
    
    // 3. ???:?????????????
    while (1) {
        // ??M1(??)?M4(??)
        Contrl_Speed(target_speed_m1, 0, 0, target_speed_m4);
        
        // ??50ms????,??????
        delay_ms(50);
    }
}