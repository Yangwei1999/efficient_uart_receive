//
// Created by yangwei on 2026/5/22.
//

#include "ringbuff.h"

void ringbuf_write(ringbuf_t *rb, uint8_t *data, uint32_t len)
{
	for(uint32_t i = 0; i < len; i++)
	{
		rb->buf[rb->head] = data[i];

		rb->head =
			(rb->head + 1) % RINGBUF_SIZE;
	}
}

uint32_t ringbuf_read(ringbuf_t *rb, uint8_t *out, uint32_t len)
{
	uint32_t cnt = 0;
	while((rb->tail != rb->head) && (cnt < len))
	{
		out[cnt++] = rb->buf[rb->tail];

		rb->tail =
			(rb->tail + 1) % RINGBUF_SIZE;
	}

	return cnt;
}