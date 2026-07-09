# 硬件与文件说明
## 文件分工
- `main.c`：STM32 程序，Keil MDK 编译后烧录至小车板卡
- `picontrol.py`：树莓派控制脚本，在树莓派端运行

## 1. 树莓派 ↔ STM32 串口通信（USART1）
| 树莓派引脚 | STM32引脚 |
| Pin 8 (GPIO14) | PA10 (RX1) |
| Pin 10 (GPIO15) | PA9 (TX1) |
| Pin 6 (GND) | GND |

## 2. STM32 ↔ 驱动模块串口通信（USART2）
| STM32引脚 | 驱动模块引脚 |
| PA2 (TX2) | RX2 |
| PA3 (RX2) | TX2 |
