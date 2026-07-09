#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import serial
import time
import sys
import tty
import termios

# --- 配置参数 ---

SERIAL_PORT = '/dev/serial0'  
BAUD_RATE = 115200

def get_char():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def main():
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("串口连接成功！")
    except Exception as e:
        print("串口连接失败: {}".format(e))
        return

    print("\n操作说明:")
    print("  [W] - 前进~")
    print("  [S] - 后退~")
    print("  [A] - 左转~")
    print("  [D] - 右转~")
    print("  [X] - 停止~")
    print("  [Q] - 退出程序~")

    try:
        while True:
            # 获取键盘输入
            char = get_char().lower()
            
            if char == 'q':
                print("\n退出程序...")
                ser.write(b'x')
                break
                
            elif char in ['w', 'a', 's', 'd', 'x']:
                if char == 'w': print("\r前进", end="")
                elif char == 's': print("\r后退", end="")
                elif char == 'a': print("\r左转", end="")
                elif char == 'd': print("\r右转", end="")
                elif char in ['x']: print("\r停止 [X]", end="")
                
                # 发送指令给STM32
                ser.write(char.encode('utf-8'))
                
    except KeyboardInterrupt:
        print("\n程序中断")
        ser.write(b'x') 
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == '__main__':
    main()
