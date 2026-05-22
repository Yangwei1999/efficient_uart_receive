//
// Created by yangwei on 2026/5/22.
//

#ifndef RING_UART_RINGBUFF_H
#define RING_UART_RINGBUFF_H

#include "main.h"

#define RINGBUF_SIZE 2048

typedef struct
{
	uint8_t buf[RINGBUF_SIZE];

	volatile uint32_t head;

	volatile uint32_t tail;

}ringbuf_t;

uint32_t ringbuf_read(ringbuf_t *rb,
					  uint8_t *out,
					  uint32_t len);

void ringbuf_write(ringbuf_t *rb,
				   uint8_t *data,
				   uint32_t len);;

#endif //RING_UART_RINGBUFF_H