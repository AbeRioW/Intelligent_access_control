/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "matrix_keyboard.h"
#include "RC522.h"
#include "AS608.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_flash_ex.h"
#include "string.h"
#include "AS608.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

// NFC卡片ID存储相关定义
#define MAX_NFC_IDS 10 // 最多存储10个NFC卡片ID
#define NFC_ID_SIZE 5 // NFC卡片ID大小（字节）
#define FLASH_PAGE_SIZE 0x400 // STM32F103C8T6的FLASH页面大小（1KB）
#define FLASH_START_ADDR 0x0801F000 // FLASH存储起始地址（选择最后一个页面）

uint8_t nfc_id_list[MAX_NFC_IDS][NFC_ID_SIZE]; // NFC卡片ID列表
uint8_t nfc_id_count = 0; // 已存储的NFC卡片ID数量
uint8_t saved_pin[4] = {0, 0, 0, 0}; // 存储的密码

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  从FLASH读取NFC卡片ID列表
  * @param  无
  * @retval 无
  */
void ReadNFCIDsFromFlash(void)
{
  // 从FLASH读取NFC卡片ID数量
  nfc_id_count = *((uint16_t *)FLASH_START_ADDR);
  
  // 检查数量是否合法
  if (nfc_id_count > MAX_NFC_IDS)
  {
    nfc_id_count = 0;
    return;
  }
  
  // 从FLASH读取NFC卡片ID列表
  for (uint8_t i = 0; i < nfc_id_count; i++)
  {
    for (uint8_t j = 0; j < NFC_ID_SIZE; j++)
    {
      nfc_id_list[i][j] = *((uint16_t *)(FLASH_START_ADDR + 2 + i * NFC_ID_SIZE * 2 + j * 2));
    }
  }
}

/**
  * @brief  向FLASH写入NFC卡片ID列表
  * @param  无
  * @retval 无
  */
void WriteNFCIDsToFlash(void)
{
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t PageError = 0;
  
  // 解锁FLASH
  HAL_FLASH_Unlock();
  
  // 配置擦除参数
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_START_ADDR;
  EraseInitStruct.NbPages = 1;
  
  // 擦除FLASH页面
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
  {
    // 擦除失败
    HAL_FLASH_Lock();
    return;
  }
  
  // 写入NFC卡片ID数量
  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_START_ADDR, nfc_id_count) != HAL_OK)
  {
    // 写入失败
    HAL_FLASH_Lock();
    return;
  }
  
  // 写入NFC卡片ID列表
  for (uint8_t i = 0; i < nfc_id_count; i++)
  {
    for (uint8_t j = 0; j < NFC_ID_SIZE; j++)
    {
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_START_ADDR + 2 + i * NFC_ID_SIZE * 2 + j * 2, nfc_id_list[i][j]) != HAL_OK)
      {
        // 写入失败
        HAL_FLASH_Lock();
        return;
      }
    }
  }
  
  // 锁定FLASH
  HAL_FLASH_Lock();
}

/**
  * @brief  检查NFC卡片ID是否已存在
  * @param  id: 要检查的NFC卡片ID
  * @retval 1: 已存在，0: 不存在
  */
uint8_t CheckNFCIDExists(uint8_t *id)
{
  for (uint8_t i = 0; i < nfc_id_count; i++)
  {
    if (memcmp(nfc_id_list[i], id, NFC_ID_SIZE) == 0)
    {
      return 1;
    }
  }
  return 0;
}

/**
  * @brief  添加新的NFC卡片ID到列表
  * @param  id: 要添加的NFC卡片ID
  * @retval 1: 添加成功，0: 添加失败（已满）
  */
uint8_t AddNFCID(uint8_t *id)
{
  if (nfc_id_count >= MAX_NFC_IDS)
  {
    return 0; // 已满
  }
  
  // 复制ID到列表
  memcpy(nfc_id_list[nfc_id_count], id, NFC_ID_SIZE);
  nfc_id_count++;
  
  // 写入FLASH
  WriteNFCIDsToFlash();
  
  return 1;
}

/**
  * @brief  从列表中删除NFC卡片ID
  * @param  id: 要删除的NFC卡片ID
  * @retval 1: 删除成功，0: 删除失败（未找到）
  */
uint8_t RemoveNFCID(uint8_t *id)
{
  uint8_t found = 0;
  
  // 查找ID在列表中的位置
  for (uint8_t i = 0; i < nfc_id_count; i++)
  {
    if (memcmp(nfc_id_list[i], id, NFC_ID_SIZE) == 0)
    {
      // 找到ID，将后面的ID前移
      for (uint8_t j = i; j < nfc_id_count - 1; j++)
      {
        memcpy(nfc_id_list[j], nfc_id_list[j + 1], NFC_ID_SIZE);
      }
      nfc_id_count--;
      found = 1;
      break;
    }
  }
  
  if (found)
  {
    // 写入FLASH
    WriteNFCIDsToFlash();
  }
  
  return found;
}

/**
  * @brief  从FLASH读取密码
  * @param  pin: 存储密码的数组
  * @retval 无
  */
void ReadPINFromFlash(uint8_t *pin)
{
  // 从FLASH读取密码
  for (uint8_t i = 0; i < 4; i++)
  {
    pin[i] = *((uint16_t *)(FLASH_START_ADDR + 2 + MAX_NFC_IDS * NFC_ID_SIZE * 2 + i * 2));
  }
  
  // 检查密码是否有效（如果FLASH中没有存储密码，使用默认密码0000）
  if (pin[0] > 9 || pin[1] > 9 || pin[2] > 9 || pin[3] > 9)
  {
    pin[0] = 0;
    pin[1] = 0;
    pin[2] = 0;
    pin[3] = 0;
  }
}

/**
  * @brief  向FLASH写入密码
  * @param  pin: 要写入的密码数组
  * @retval 无
  */
void WritePINToFlash(uint8_t *pin)
{
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t PageError = 0;
  
  // 解锁FLASH
  HAL_FLASH_Unlock();
  
  // 配置擦除参数
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_START_ADDR;
  EraseInitStruct.NbPages = 1;
  
  // 擦除FLASH页面
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK)
  {
    // 擦除失败
    HAL_FLASH_Lock();
    return;
  }
  
  // 写入NFC卡片ID数量
  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_START_ADDR, nfc_id_count) != HAL_OK)
  {
    // 写入失败
    HAL_FLASH_Lock();
    return;
  }
  
  // 写入NFC卡片ID列表
  for (uint8_t i = 0; i < nfc_id_count; i++)
  {
    for (uint8_t j = 0; j < NFC_ID_SIZE; j++)
    {
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_START_ADDR + 2 + i * NFC_ID_SIZE * 2 + j * 2, nfc_id_list[i][j]) != HAL_OK)
      {
        // 写入失败
        HAL_FLASH_Lock();
        return;
      }
    }
  }
  
  // 写入密码
  for (uint8_t i = 0; i < 4; i++)
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_START_ADDR + 2 + MAX_NFC_IDS * NFC_ID_SIZE * 2 + i * 2, pin[i]) != HAL_OK)
    {
      // 写入失败
      HAL_FLASH_Lock();
      return;
    }
  }
  
  // 锁定FLASH
  HAL_FLASH_Lock();
}

/**
  * @brief  密码验证函数
  * @param  无
  * @retval 1: 验证成功，0: 验证失败
  */
uint8_t VerifyPIN(void)
{
  OLED_Clear();
  OLED_ShowString(0, 0, (uint8_t*)"Enter PIN code:", 8, 1);
  OLED_ShowString(0, 16, (uint8_t*)"Press #16 to confirm", 8, 1);
  OLED_Refresh();
  
  // 等待按键释放
  while (Matrix_Keyboard_Scan() != 0)
  {
    HAL_Delay(10);
  }
  
  // PIN码输入逻辑
  uint8_t pin_code[4] = {0};
  uint8_t pin_index = 0;
  uint8_t exit_flag = 0;
  
  while (!exit_flag)
  {
    uint8_t pin_key = Matrix_Keyboard_Scan();
    if (pin_key != 0)
    {
      if (pin_key == 16)
      {
        // 16号按键确认
        if (pin_index == 4)
        {
          exit_flag = 1;
        }
      }
      else if (pin_index < 4)
      {
        // 按键映射：1-3->1-3, 5-7->4-6, 9-11->7-9, 14->0
        uint8_t digit = 0;
        if (pin_key >= 1 && pin_key <= 3)
        {
          digit = pin_key; // 1->1, 2->2, 3->3
        }
        else if (pin_key >= 5 && pin_key <= 7)
        {
          digit = pin_key - 1; // 5->4, 6->5, 7->6
        }
        else if (pin_key >= 9 && pin_key <= 11)
        {
          digit = pin_key - 2; // 9->7, 10->8, 11->9
        }
        else if (pin_key == 14)
        {
          digit = 0; // 14->0
        }
        
        if (digit >= 0 && digit <= 9)
        {
          pin_code[pin_index] = digit;
          OLED_ShowNum(pin_index * 20, 8, digit, 1, 8, 1);
          OLED_Refresh();
          pin_index++;
        }
      }
      // 等待按键释放
      while (Matrix_Keyboard_Scan() != 0)
      {
        HAL_Delay(10);
      }
    }
    HAL_Delay(100);
  }
  
  // 验证PIN码
  uint8_t pin_match = 1;
  
  for (uint8_t i = 0; i < 4; i++)
  {
    if (pin_code[i] != saved_pin[i])
    {
      pin_match = 0;
      break;
    }
  }
  
  OLED_Clear();
  OLED_ShowString(0, 0, (uint8_t*)"Verifying PIN...", 8, 1);
  OLED_Refresh();
  HAL_Delay(1000);
  
  if (!pin_match)
  {
    // PIN码错误
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"PIN Error", 8, 1);
    OLED_ShowString(0, 8, (uint8_t*)"Access Denied", 8, 1);
    OLED_Refresh();
    HAL_Delay(2000);
    
    // 返回主页面
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
    OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
    OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
    OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
    OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
    OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
    OLED_ShowString(0, 56, (uint8_t*)"Press #8 to Change PIN", 8, 1);
    OLED_Refresh();
    return 0;
  }
  
  return 1;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
   OLED_Init();
  OLED_ShowString(0,0,(uint8_t*)"Initializing...",8,1);
  OLED_Refresh();
  HAL_Delay(1000);
   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // LAY1连接到PB13
	 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // LAY1连接到PB13
  
  // 初始化后PWM输出SG90舵机的中间位置信号（90度）
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1500); // 设置脉冲值为1500，对应1.5ms脉冲宽度
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2); // 启动PWM输出
  
  PCD_Init(); // 初始化RC522
  AS608_Init(); // 初始化AS608
  
  ReadNFCIDsFromFlash(); // 从FLASH读取NFC卡片ID列表
  ReadPINFromFlash(saved_pin); // 从FLASH读取密码
  
  // AS608握手
  OLED_Clear();
  OLED_ShowString(0,0,(uint8_t*)"Checking AS608...",8,1);
  OLED_Refresh();
  
  uint32_t as608_addr = 0xffffffff;
  uint8_t handshakeResult = AS608_HandShake(&as608_addr);
  
  if (handshakeResult == AS608_ACK_OK)
  {
    // 握手成功
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"AS608 Handshake", 8, 1);
    OLED_ShowString(0, 8, (uint8_t*)"Success!", 8, 1);
    OLED_Refresh();
    HAL_Delay(2000);
    
    // 进入主页面
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
    OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
    OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
    OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
    OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
    OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
    OLED_Refresh();
  }
  else
  {
    // 握手失败
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"AS608 Handshake", 8, 1);
    OLED_ShowString(0, 8, (uint8_t*)"Failed!", 8, 1);
    OLED_Refresh();
    HAL_Delay(2000);
	while(1);
  }
  Matrix_Keyboard_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		  // 初始化后PWM输出SG90舵机的中间位置信号（90度）
		#if 1
    static uint8_t last_key = 0;
    uint8_t current_key = Matrix_Keyboard_Scan();
    
    // 键盘处理
    if (current_key != last_key && current_key != 0)
    {
      if (current_key == 4)
      {
        // 4号按键，进入NFC卡片注册模式
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Place NFC Card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"to Register", 8, 1);
        OLED_Refresh();
        
        // 等待按键释放
        while (Matrix_Keyboard_Scan() != 0)
        {
          HAL_Delay(10);
        }
        
        // 等待NFC卡片
        uint8_t card_type[2] = {0};
        uint8_t card_id[5] = {0};
        uint8_t found = 0;
        
        while (!found)
        {
          if (PCD_Request(PICC_REQIDL, card_type) == PCD_OK)
          {
            if (PCD_Anticoll(card_id) == PCD_OK)
            {
              // 读取到卡片ID
              OLED_Clear();
              OLED_ShowString(0, 0, (uint8_t*)"NFC ID:", 8, 1);
              for (uint8_t i = 0; i < 4; i++)
              {
                OLED_ShowNum(i * 24, 16, card_id[i], 2, 8, 1);
              }
              OLED_Refresh();
              
              // 检查ID是否已存在
              if (CheckNFCIDExists(card_id))
              {
                // 已注册
                OLED_ShowString(0, 24, (uint8_t*)"Already Registered", 8, 1);
                OLED_Refresh();
                HAL_Delay(2000);
              }
              else
              {
                // 添加新ID
                if (AddNFCID(card_id))
                {
                  // 添加成功
                  OLED_ShowString(0, 24, (uint8_t*)"Registered Successfully", 8, 1);
                  OLED_Refresh();
                  HAL_Delay(2000);
                }
                else
                {
                  // 存储已满
                  OLED_ShowString(0, 24, (uint8_t*)"Storage Full", 8, 1);
                  OLED_Refresh();
                  HAL_Delay(2000);
                }
              }
              found = 1;
            }
          }
          HAL_Delay(100);
        }
        
        // 返回主页面
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
        OLED_Refresh();
      }
      else if (current_key == 12)
      {
        // 12号按键，进入NFC卡片删除模式
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Place NFC Card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"to Delete", 8, 1);
        OLED_Refresh();
        
        // 等待按键释放
        while (Matrix_Keyboard_Scan() != 0)
        {
          HAL_Delay(10);
        }
        
        // 等待NFC卡片
        uint8_t card_type[2] = {0};
        uint8_t card_id[5] = {0};
        uint8_t found = 0;
        
        while (!found)
        {
          if (PCD_Request(PICC_REQIDL, card_type) == PCD_OK)
          {
            if (PCD_Anticoll(card_id) == PCD_OK)
            {
              // 读取到卡片ID
              OLED_Clear();
              OLED_ShowString(0, 0, (uint8_t*)"NFC ID:", 8, 1);
              for (uint8_t i = 0; i < 4; i++)
              {
                OLED_ShowNum(i * 24, 16, card_id[i], 2, 8, 1);
              }
              OLED_Refresh();
              
              // 检查ID是否存在
              if (CheckNFCIDExists(card_id))
              {
                // 删除ID
                if (RemoveNFCID(card_id))
                {
                  // 删除成功
                  OLED_ShowString(0, 24, (uint8_t*)"Deleted Successfully", 8, 1);
                  OLED_Refresh();
                  HAL_Delay(2000);
                }
              }
              else
              {
                // 卡片未注册
                OLED_ShowString(0, 24, (uint8_t*)"Invalid Card", 8, 1);
                OLED_Refresh();
                HAL_Delay(2000);
              }
              found = 1;
            }
          }
          HAL_Delay(100);
        }
        
        // 返回主页面
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
        OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
        OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
        OLED_Refresh();
      }
      else if (current_key == 13)
      {
        // 13号按键，进入指纹注册模式
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Fingerprint", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Enrollment", 8, 1);
        OLED_Refresh();
        
        // 等待按键释放
        while (Matrix_Keyboard_Scan() != 0)
        {
          HAL_Delay(10);
        }
        
        // 生成一个简单的指纹ID（从1开始）
        static uint16_t fp_id = 1;
        
        // 显示提示信息，给用户足够的准备时间
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Fingerprint", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Enrollment", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Place finger", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"on sensor", 8, 1);
        OLED_Refresh();
        
        // 给用户3秒的准备时间
        HAL_Delay(3000);
        
        // 调用AS608_Enroll函数进行指纹注册
        uint8_t result = AS608_Enroll(fp_id);
        
        if (result == AS608_ACK_OK)
        {
          // 注册成功
          OLED_Clear();
          OLED_ShowString(0, 0, (uint8_t*)"Enrollment", 8, 1);
          OLED_ShowString(0, 8, (uint8_t*)"Success!", 8, 1);
          char fp_id_str[10];
          sprintf(fp_id_str, "ID: %d", fp_id);
          OLED_ShowString(0, 16, (uint8_t*)fp_id_str, 8, 1);
          OLED_Refresh();
          HAL_Delay(2000);
          fp_id++;
          if (fp_id >= AS608_MAX_FINGER_NUM)
          {
            fp_id = 1; // 重置ID
          }
        }
        else
        {
          // 注册失败
          OLED_Clear();
          OLED_ShowString(0, 0, (uint8_t*)"Enrollment", 8, 1);
          OLED_ShowString(0, 8, (uint8_t*)"Failed!", 8, 1);
          OLED_ShowString(0, 16, (uint8_t*)AS608_GetErrorString(result), 8, 1);
          OLED_Refresh();
          HAL_Delay(2000);
        }
        
        // 返回主页面
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
        OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
        OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
        OLED_Refresh();
      }
      else if (current_key == 8)
      {
        // 8号按键，进入密码修改模式
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Change PIN", 8, 1);
        OLED_Refresh();
        
        // 等待按键释放
        while (Matrix_Keyboard_Scan() != 0)
        {
          HAL_Delay(10);
        }
        
        // 1. 输入当前密码
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Enter current PIN:", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 to confirm", 8, 1);
        OLED_Refresh();
        
        uint8_t current_pin[4] = {0};
        uint8_t pin_index = 0;
        uint8_t exit_flag = 0;
        
        while (!exit_flag)
        {
          uint8_t pin_key = Matrix_Keyboard_Scan();
          if (pin_key != 0)
          {
            if (pin_key == 16)
            {
              // 16号按键确认
              if (pin_index == 4)
              {
                exit_flag = 1;
              }
            }
            else if (pin_index < 4)
            {
              // 按键映射：1-3->1-3, 5-7->4-6, 9-11->7-9, 14->0
              uint8_t digit = 0;
              if (pin_key >= 1 && pin_key <= 3)
              {
                digit = pin_key; // 1->1, 2->2, 3->3
              }
              else if (pin_key >= 5 && pin_key <= 7)
              {
                digit = pin_key - 1; // 5->4, 6->5, 7->6
              }
              else if (pin_key >= 9 && pin_key <= 11)
              {
                digit = pin_key - 2; // 9->7, 10->8, 11->9
              }
              else if (pin_key == 14)
              {
                digit = 0; // 14->0
              }
              
              if (digit >= 0 && digit <= 9)
              {
                current_pin[pin_index] = digit;
                OLED_ShowNum(pin_index * 20, 8, digit, 1, 8, 1);
                OLED_Refresh();
                pin_index++;
              }
            }
            // 等待按键释放
            while (Matrix_Keyboard_Scan() != 0)
            {
              HAL_Delay(10);
            }
          }
          HAL_Delay(100);
        }
        
        // 2. 验证当前密码
        uint8_t pin_match = 1;
        
        for (uint8_t i = 0; i < 4; i++)
        {
          if (current_pin[i] != saved_pin[i])
          {
            pin_match = 0;
            break;
          }
        }
        
        if (!pin_match)
        {
          // 密码错误
          OLED_Clear();
          OLED_ShowString(0, 0, (uint8_t*)"PIN Error", 8, 1);
          OLED_ShowString(0, 8, (uint8_t*)"Access Denied", 8, 1);
          OLED_Refresh();
          HAL_Delay(2000);
          
          // 返回主页面
          OLED_Clear();
          OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
          OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
          OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
          OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
          OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
          OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
          OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
          OLED_ShowString(0, 56, (uint8_t*)"Press #8 to Change PIN", 8, 1);
          OLED_Refresh();
          continue;
        }
        
        // 3. 输入新密码
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Enter new PIN:", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 to confirm", 8, 1);
        OLED_Refresh();
        
        uint8_t new_pin1[4] = {0};
        pin_index = 0;
        exit_flag = 0;
        
        while (!exit_flag)
        {
          uint8_t pin_key = Matrix_Keyboard_Scan();
          if (pin_key != 0)
          {
            if (pin_key == 16)
            {
              // 16号按键确认
              if (pin_index == 4)
              {
                exit_flag = 1;
              }
            }
            else if (pin_index < 4)
            {
              // 按键映射：1-3->1-3, 5-7->4-6, 9-11->7-9, 14->0
              uint8_t digit = 0;
              if (pin_key >= 1 && pin_key <= 3)
              {
                digit = pin_key; // 1->1, 2->2, 3->3
              }
              else if (pin_key >= 5 && pin_key <= 7)
              {
                digit = pin_key - 1; // 5->4, 6->5, 7->6
              }
              else if (pin_key >= 9 && pin_key <= 11)
              {
                digit = pin_key - 2; // 9->7, 10->8, 11->9
              }
              else if (pin_key == 14)
              {
                digit = 0; // 14->0
              }
              
              if (digit >= 0 && digit <= 9)
              {
                new_pin1[pin_index] = digit;
                OLED_ShowNum(pin_index * 20, 8, digit, 1, 8, 1);
                OLED_Refresh();
                pin_index++;
              }
            }
            // 等待按键释放
            while (Matrix_Keyboard_Scan() != 0)
            {
              HAL_Delay(10);
            }
          }
          HAL_Delay(100);
        }
        
        // 4. 再次输入新密码确认
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Confirm new PIN:", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 to confirm", 8, 1);
        OLED_Refresh();
        
        uint8_t new_pin2[4] = {0};
        pin_index = 0;
        exit_flag = 0;
        
        while (!exit_flag)
        {
          uint8_t pin_key = Matrix_Keyboard_Scan();
          if (pin_key != 0)
          {
            if (pin_key == 16)
            {
              // 16号按键确认
              if (pin_index == 4)
              {
                exit_flag = 1;
              }
            }
            else if (pin_index < 4)
            {
              // 按键映射：1-3->1-3, 5-7->4-6, 9-11->7-9, 14->0
              uint8_t digit = 0;
              if (pin_key >= 1 && pin_key <= 3)
              {
                digit = pin_key; // 1->1, 2->2, 3->3
              }
              else if (pin_key >= 5 && pin_key <= 7)
              {
                digit = pin_key - 1; // 5->4, 6->5, 7->6
              }
              else if (pin_key >= 9 && pin_key <= 11)
              {
                digit = pin_key - 2; // 9->7, 10->8, 11->9
              }
              else if (pin_key == 14)
              {
                digit = 0; // 14->0
              }
              
              if (digit >= 0 && digit <= 9)
              {
                new_pin2[pin_index] = digit;
                OLED_ShowNum(pin_index * 20, 8, digit, 1, 8, 1);
                OLED_Refresh();
                pin_index++;
              }
            }
            // 等待按键释放
            while (Matrix_Keyboard_Scan() != 0)
            {
              HAL_Delay(10);
            }
          }
          HAL_Delay(100);
        }
        
        // 5. 检查两次输入是否一致
        uint8_t pin_equal = 1;
        for (uint8_t i = 0; i < 4; i++)
        {
          if (new_pin1[i] != new_pin2[i])
          {
            pin_equal = 0;
            break;
          }
        }
        
        if (!pin_equal)
        {
          // 两次输入不一致
          OLED_Clear();
          OLED_ShowString(0, 0, (uint8_t*)"PIN Mismatch", 8, 1);
          OLED_ShowString(0, 8, (uint8_t*)"Try again", 8, 1);
          OLED_Refresh();
          HAL_Delay(2000);
        }
        else
        {
          // 两次输入一致，保存新密码到FLASH
          memcpy(saved_pin, new_pin1, 4);
          WritePINToFlash(saved_pin);
          OLED_Clear();
          OLED_ShowString(0, 0, (uint8_t*)"PIN Changed", 8, 1);
          OLED_ShowString(0, 8, (uint8_t*)"Success!", 8, 1);
          OLED_Refresh();
          HAL_Delay(2000);
        }
        
        // 返回主页面
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
        OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
        OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
        OLED_ShowString(0, 56, (uint8_t*)"Press #8 to Change PIN", 8, 1);
        OLED_Refresh();
      }
      else if (current_key == 15)
      {
        // 15号按键，进入指纹删除模式
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Fingerprint", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Deletion", 8, 1);
        OLED_Refresh();
        
        // 等待按键释放
        while (Matrix_Keyboard_Scan() != 0)
        {
          HAL_Delay(10);
        }
        
        // 显示提示信息
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Place finger", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"to delete", 8, 1);
        OLED_Refresh();
        
        // 给用户3秒的准备时间
        HAL_Delay(3000);
        
        // 验证指纹并获取其ID
        uint16_t fp_id = 0;
        uint16_t score = 0;
        uint8_t verify_result = AS608_VerifyFinger(&fp_id, &score);
        
        if (verify_result == AS608_ACK_OK)
        {
          // 指纹验证成功，删除该指纹
          uint8_t delete_result = AS608_DeleteChar(fp_id, 1);
          
          if (delete_result == AS608_ACK_OK)
          {
            // 删除成功
            OLED_Clear();
            OLED_ShowString(0, 0, (uint8_t*)"Deletion", 8, 1);
            OLED_ShowString(0, 8, (uint8_t*)"Success!", 8, 1);
            char fp_id_str[10];
            sprintf(fp_id_str, "ID: %d", fp_id);
            OLED_ShowString(0, 16, (uint8_t*)fp_id_str, 8, 1);
            OLED_Refresh();
            HAL_Delay(2000);
          }
          else
          {
            // 删除失败
            OLED_Clear();
            OLED_ShowString(0, 0, (uint8_t*)"Deletion", 8, 1);
            OLED_ShowString(0, 8, (uint8_t*)"Failed!", 8, 1);
            OLED_ShowString(0, 16, (uint8_t*)AS608_GetErrorString(delete_result), 8, 1);
            OLED_Refresh();
            HAL_Delay(2000);
          }
        }
        else
        {
          // 指纹验证失败
          OLED_Clear();
          OLED_ShowString(0, 0, (uint8_t*)"Deletion", 8, 1);
          OLED_ShowString(0, 8, (uint8_t*)"Failed!", 8, 1);
          OLED_ShowString(0, 16, (uint8_t*)"Finger not found", 8, 1);
          OLED_Refresh();
          HAL_Delay(2000);
        }
        
        // 返回主页面
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
        OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
        OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
        OLED_ShowString(0, 56, (uint8_t*)"Press #8 to Change PIN", 8, 1);
        OLED_Refresh();
      }
      else if (current_key == 16)
      {
        // 16号按键，进入PIN码开门模式
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Enter PIN code:", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 to confirm", 8, 1);
        OLED_Refresh();
        
        // 等待按键释放
        while (Matrix_Keyboard_Scan() != 0)
        {
          HAL_Delay(10);
        }
        
        // PIN码输入逻辑
        uint8_t pin_code[4] = {0};
        uint8_t pin_index = 0;
        uint8_t exit_flag = 0;
        
        while (!exit_flag)
        {
          uint8_t pin_key = Matrix_Keyboard_Scan();
          if (pin_key != 0)
          {
            if (pin_key == 16)
            {
              // 16号按键确认
              if (pin_index == 4)
              {
                // 验证PIN码
                uint8_t pin_match = 1;
                
                for (uint8_t i = 0; i < 4; i++)
                {
                  if (pin_code[i] != saved_pin[i])
                  {
                    pin_match = 0;
                    break;
                  }
                }
                
                OLED_Clear();
                OLED_ShowString(0, 0, (uint8_t*)"Verifying PIN...", 8, 1);
                OLED_Refresh();
                HAL_Delay(1000);
                
                if (pin_match)
                {
                  // PIN码正确
                  OLED_Clear();
                  OLED_ShowString(0, 0, (uint8_t*)"PIN Verified", 8, 1);
                  OLED_ShowString(0, 8, (uint8_t*)"Door Unlocked", 8, 1);
                  OLED_Refresh();
                  
                  // 控制SG90舵机，从0度转动到90度
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500); // 设置脉冲值为500，对应0度
                  HAL_Delay(1000); // 等待舵机到位
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1500); // 设置脉冲值为1500，对应90度
                  HAL_Delay(1000); // 等待舵机到位
                  
                  HAL_Delay(2000);
                }
                else
                {
                  // PIN码错误
                  OLED_Clear();
                  OLED_ShowString(0, 0, (uint8_t*)"PIN Error", 8, 1);
                  OLED_ShowString(0, 8, (uint8_t*)"Access Denied", 8, 1);
                  OLED_Refresh();
                  HAL_Delay(2000);
                }
              }
              exit_flag = 1;
            }
            else if (pin_index < 4)
            {
              // 按键映射：1-3->1-3, 5-7->4-6, 9-11->7-9, 14->0
              uint8_t digit = 0;
              if (pin_key >= 1 && pin_key <= 3)
              {
                digit = pin_key; // 1->1, 2->2, 3->3
              }
              else if (pin_key >= 5 && pin_key <= 7)
              {
                digit = pin_key - 1; // 5->4, 6->5, 7->6
              }
              else if (pin_key >= 9 && pin_key <= 11)
              {
                digit = pin_key - 2; // 9->7, 10->8, 11->9
              }
              else if (pin_key == 14)
              {
                digit = 0; // 14->0
              }
              
              if (digit != 0 || pin_key == 14)
              {
                pin_code[pin_index] = digit;
                pin_index++;
                
                // 显示输入的PIN码（显示明文）
                OLED_Clear();
                OLED_ShowString(0, 0, (uint8_t*)"Enter PIN code:", 8, 1);
                for (uint8_t i = 0; i < pin_index; i++)
                {
                  OLED_ShowNum(10 + i*16, 8, pin_code[i], 1, 8, 1);
                }
                OLED_ShowString(0, 16, (uint8_t*)"Press #16 to confirm", 8, 1);
                OLED_Refresh();
                
                // 等待按键释放
                while (Matrix_Keyboard_Scan() != 0)
                {
                  HAL_Delay(10);
                }
              }
            }
          }
          HAL_Delay(50); // 扫描间隔
        }
        
        // 等待16号按键释放
        while (Matrix_Keyboard_Scan() != 0)
        {
          HAL_Delay(10);
        }
        
        // 返回主页面
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
        OLED_Refresh();
      }
      else
      {
        // 其他按键
        OLED_Clear();
        OLED_ShowString(0,0,(uint8_t*)"Key: ",8,1);
        OLED_ShowNum(40,0,current_key,2,8,1);
        OLED_Refresh();
      }
      last_key = current_key;
    }
    else if (current_key == 0)
    {
      last_key = 0;
    }
    
    // NFC卡片检测
    static uint8_t last_card[4] = {0};
    uint8_t card_type[2] = {0};
    uint8_t card_id[5] = {0};
    
    if (PCD_Request(PICC_REQIDL, card_type) == PCD_OK)
    {
      if (PCD_Anticoll(card_id) == PCD_OK)
      {
        // 检查卡片ID是否变化
        if (memcmp(card_id, last_card, 4) != 0)
        {
          OLED_Clear();
          OLED_ShowString(0,0,(uint8_t*)"NFC Card ID:",8,1);
          for (int i = 0; i < 4; i++)
          {
            OLED_ShowNum(10 + i*20, 8, card_id[i], 2, 8, 1);
          }
          OLED_Refresh();
          memcpy(last_card, card_id, 4);
          
          // 检查卡片ID是否已注册
          if (CheckNFCIDExists(card_id))
          {
            // 卡片已注册，验证成功
            OLED_ShowString(0, 16, (uint8_t*)"Access Granted", 8, 1);
            OLED_Refresh();
            
            // 控制SG90舵机，从0度转动到90度
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500); // 设置脉冲值为500，对应0度
            HAL_Delay(1000); // 等待舵机到位
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1500); // 设置脉冲值为1500，对应90度
            HAL_Delay(1000); // 等待舵机到位
            
            HAL_Delay(2000);
          }
          else
          {
            // 卡片未注册
            OLED_ShowString(0, 16, (uint8_t*)"Access Denied", 8, 1);
            OLED_Refresh();
            HAL_Delay(2000);
          }
          
          // 返回主页面
          OLED_Clear();
          OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
          OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
          OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
          OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
          OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
          OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
          OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
          OLED_Refresh();
        }
      }
    }
    else
    {
      // 清除last_card
      memset(last_card, 0, 4);
    }
    
    // 指纹检测
    static uint8_t last_finger = 0;
    uint16_t fp_id = 0;
    uint16_t score = 0;
    uint8_t finger_result = AS608_VerifyFinger(&fp_id, &score);
    
    if (finger_result == AS608_ACK_OK)
    {
      // 指纹验证成功
      if (!last_finger)
      {
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Fingerprint", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Verified", 8, 1);
        char fp_info[20];
        sprintf(fp_info, "ID: %d, Score: %d", fp_id, score);
        OLED_ShowString(0, 16, (uint8_t*)fp_info, 8, 1);
        OLED_Refresh();
        
        // 控制SG90舵机，从0度转动到90度
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500); // 设置脉冲值为500，对应0度
        HAL_Delay(1000); // 等待舵机到位
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1500); // 设置脉冲值为1500，对应90度
        HAL_Delay(1000); // 等待舵机到位
        
        HAL_Delay(2000);
        
        // 返回主页面
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
        OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
        OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
        OLED_Refresh();
        last_finger = 1;
      }
    }
    else if (finger_result == AS608_ACK_NO_FOUND)
    {
      // 指纹未找到
      if (!last_finger)
      {
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Fingerprint", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Not Found", 8, 1);
        OLED_Refresh();
        HAL_Delay(1000);
        
        // 返回主页面
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
        OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
        OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
        OLED_Refresh();
        last_finger = 1;
      }
    }
    else if (finger_result == AS608_ACK_NO_FINGER)
    {
      // 没有检测到手指
      last_finger = 0;
    }
    else
    {
      // 其他错误
      if (!last_finger)
      {
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Fingerprint", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Error", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)AS608_GetErrorString(finger_result), 8, 1);
        OLED_Refresh();
        HAL_Delay(1000);
        
        // 返回主页面
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"Wait for NFC card", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"Wait for fingerprint", 8, 1);
        OLED_ShowString(0, 16, (uint8_t*)"Press #16 for PIN", 8, 1);
        OLED_ShowString(0, 24, (uint8_t*)"Press #4 to Reg", 8, 1);
        OLED_ShowString(0, 32, (uint8_t*)"Press #12 to Del", 8, 1);
        OLED_ShowString(0, 40, (uint8_t*)"Press #13 for FP Reg", 8, 1);
        OLED_ShowString(0, 48, (uint8_t*)"Press #15 to Del FP", 8, 1);
        OLED_Refresh();
        last_finger = 1;
      }
    }
    
    HAL_Delay(100); // 消抖
		#endif
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
