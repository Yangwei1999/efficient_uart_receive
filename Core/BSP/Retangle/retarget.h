//
// Created by yangwei on 2026/5/22.
//

#ifndef RING_UART_RETARGET_H
#define RING_UART_RETARGET_H
#include "stm32f4xx_hal.h"
#include <sys/stat.h>
#include <stdio.h>

void RetargetInit(UART_HandleTypeDef *huart);

int _isatty(int fd);

int _write(int fd, char *ptr, int len);

int _close(int fd);

int _lseek(int fd, int ptr, int dir);

int _read(int fd, char *ptr, int len);

int _fstat(int fd, struct stat *st);
#endif //RING_UART_RETARGET_H