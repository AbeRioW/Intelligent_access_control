#ifndef __MATRIX_KEYBOARD_H
#define __MATRIX_KEYBOARD_H

#include "stdint.h"
#include "gpio.h"

// 矩阵键盘引脚定义
// 使用gpio.c中定义的引脚宏
#define LINE1_PORT GPIOB
#define LINE2_PORT GPIOB
#define LINE3_PORT GPIOB
#define LINE4_PORT GPIOB

#define COW1_PORT GPIOB
#define COW2_PORT GPIOB
#define COW3_PORT GPIOB
#define COW4_PORT COW4_GPIO_Port

// 函数声明
void Matrix_Keyboard_Init(void);
uint8_t Matrix_Keyboard_Scan(void);

#endif