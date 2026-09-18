# STM32F103C8T6 多功能交互控制系统

## 项目简介

本项目基于 **STM32F103C8T6**，使用 **Keil5 + STM32 标准外设库（SPL）** 开发，实现一个带 OLED 菜单的人机交互控制系统。

通过旋转编码器和按键进行菜单操作，并集成 ADC、MPU6050、PWM 和 USART 等功能，用于练习 STM32 外设驱动、菜单设计、状态管理和多模块程序组织。

## 主要功能

- OLED 多级菜单显示
- 旋转编码器菜单选择
- ENTER / BACK 按键控制
- 菜单滚动显示
- ADC 电压采集与显示
- MPU6050 六轴数据读取
- PWM 占空比调节
- USART 串口日志输出
- SWD 下载与调试

## 硬件平台

- MCU：STM32F103C8T6
- OLED 显示屏
- 旋转编码器
- MPU6050
- 按键 ×2
- USB-TTL 串口模块
- ST-Link

## 主要引脚

| 功能 | 引脚 |
| --- | --- |
| Encoder A | PB0 |
| Encoder B | PB1 |
| BACK | PA4 |
| ENTER | PA5 |
| OLED SCL | PB8 |
| OLED SDA | PB9 |
| PWM | PA1 / TIM2_CH2 |
| USART1 TX | PA9 |
| USART1 RX | PA10 |
| SWDIO | PA13 |
| SWCLK | PA14 |
| ADC | 以 AD.c 配置为准 |
| MPU6050 | 以 MPU6050.c 配置为准 |

## 软件结构

```text
Hardware/
├── OLED
├── Encoder
├── Key
├── Input
├── AD
├── MPU6050
├── PWM
└── Serial

System/
├── Delay
└── Menu

User/
└── main.c
