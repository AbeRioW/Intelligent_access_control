#include "matrix_keyboard.h"
#include "stm32f1xx_hal.h"

// 初始化矩阵键盘
void Matrix_Keyboard_Init(void)
{
    // 初始时所有行输出高电平
    HAL_GPIO_WritePin(LINE1_PORT, LINE1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE2_PORT, LINE2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE3_PORT, LINE3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE4_PORT, LINE4_Pin, GPIO_PIN_SET);
}

// 扫描矩阵键盘，返回按键值（1-16），无按键返回0
uint8_t Matrix_Keyboard_Scan(void)
{
    uint8_t key_value = 0;
    
    // 扫描第一行
    HAL_GPIO_WritePin(LINE1_PORT, LINE1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LINE2_PORT, LINE2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE3_PORT, LINE3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE4_PORT, LINE4_Pin, GPIO_PIN_SET);
    
    if (HAL_GPIO_ReadPin(COW1_PORT, COW1_Pin) == GPIO_PIN_RESET) {
        key_value = 1;
    } else if (HAL_GPIO_ReadPin(COW2_PORT, COW2_Pin) == GPIO_PIN_RESET) {
        key_value = 2;
    } else if (HAL_GPIO_ReadPin(COW3_PORT, COW3_Pin) == GPIO_PIN_RESET) {
        key_value = 3;
    } else if (HAL_GPIO_ReadPin(COW4_PORT, COW4_Pin) == GPIO_PIN_RESET) {
        key_value = 4;
    }
    
    // 扫描第二行
    HAL_GPIO_WritePin(LINE1_PORT, LINE1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE2_PORT, LINE2_Pin, GPIO_PIN_RESET);
    
    if (HAL_GPIO_ReadPin(COW1_PORT, COW1_Pin) == GPIO_PIN_RESET) {
        key_value = 5;
    } else if (HAL_GPIO_ReadPin(COW2_PORT, COW2_Pin) == GPIO_PIN_RESET) {
        key_value = 6;
    } else if (HAL_GPIO_ReadPin(COW3_PORT, COW3_Pin) == GPIO_PIN_RESET) {
        key_value = 7;
    } else if (HAL_GPIO_ReadPin(COW4_PORT, COW4_Pin) == GPIO_PIN_RESET) {
        key_value = 8;
    }
    
    // 扫描第三行
    HAL_GPIO_WritePin(LINE2_PORT, LINE2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE3_PORT, LINE3_Pin, GPIO_PIN_RESET);
    
    if (HAL_GPIO_ReadPin(COW1_PORT, COW1_Pin) == GPIO_PIN_RESET) {
        key_value = 9;
    } else if (HAL_GPIO_ReadPin(COW2_PORT, COW2_Pin) == GPIO_PIN_RESET) {
        key_value = 10;
    } else if (HAL_GPIO_ReadPin(COW3_PORT, COW3_Pin) == GPIO_PIN_RESET) {
        key_value = 11;
    } else if (HAL_GPIO_ReadPin(COW4_PORT, COW4_Pin) == GPIO_PIN_RESET) {
        key_value = 12;
    }
    
    // 扫描第四行
    HAL_GPIO_WritePin(LINE3_PORT, LINE3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE4_PORT, LINE4_Pin, GPIO_PIN_RESET);
    
    if (HAL_GPIO_ReadPin(COW1_PORT, COW1_Pin) == GPIO_PIN_RESET) {
        key_value = 13;
    } else if (HAL_GPIO_ReadPin(COW2_PORT, COW2_Pin) == GPIO_PIN_RESET) {
        key_value = 14;
    } else if (HAL_GPIO_ReadPin(COW3_PORT, COW3_Pin) == GPIO_PIN_RESET) {
        key_value = 15;
    } else if (HAL_GPIO_ReadPin(COW4_PORT, COW4_Pin) == GPIO_PIN_RESET) {
        key_value = 16;
    }
    
    // 恢复所有行为高电平
    HAL_GPIO_WritePin(LINE1_PORT, LINE1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE2_PORT, LINE2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE3_PORT, LINE3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LINE4_PORT, LINE4_Pin, GPIO_PIN_SET);
    
    return key_value;
}