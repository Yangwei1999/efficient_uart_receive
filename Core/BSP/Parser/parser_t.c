//
// Created by yangwei on 2026/6/2.
//

#include "parser_t.h"


#define MAX_DATA_LEN 64

typedef enum
{
	WAIT_HEADER1,
	WAIT_HEADER2,
	WAIT_LEN,
	WAIT_DATA,
	WAIT_CRC

  }parser_state_t;

typedef struct
{
	parser_state_t state;

	uint8_t len;

	uint8_t data[MAX_DATA_LEN];

	uint8_t data_pos;

	uint8_t crc;

}parser_t;

void parser_init(parser_t *p)
{
	p->state = WAIT_HEADER1;

	p->len = 0;

	p->data_pos = 0;

	p->crc = 0;
}

uint8_t calc_crc(uint8_t len,
				 uint8_t *data)
{
	uint8_t crc = 0;

	crc += 0xAA;
	crc += 0x55;
	crc += len;

	for(uint8_t i = 0; i < len; i++)
	{
		crc += data[i];
	}

	return crc;
}

void packet_ok(parser_t *p)
{
	printf("recv packet: ");

	for(int i = 0; i < p->len; i++)
	{
		printf("%02X ", p->data[i]);
	}

	printf("\n");
}

void packet_error(void)
{
	printf("crc error\n");
}

void parser_input(parser_t *p,
				  uint8_t ch)
{
	switch(p->state)
	{
		case WAIT_HEADER1:

			if(ch == 0xAA)
			{
				p->state = WAIT_HEADER2;
			}

			break;

		case WAIT_HEADER2:

			if(ch == 0x55)
			{
				p->state = WAIT_LEN;
			}
			else
			{
				p->state = WAIT_HEADER1;
			}

			break;

		case WAIT_LEN:

			if(ch > MAX_DATA_LEN)
			{
				parser_init(p);
			}
			else
			{
				p->len = ch;

				p->data_pos = 0;

				p->state = WAIT_DATA;
			}

			break;

		case WAIT_DATA:

			p->data[p->data_pos++] = ch;

			if(p->data_pos >= p->len)
			{
				p->state = WAIT_CRC;
			}

			break;

		case WAIT_CRC:
		{
			uint8_t crc;

			crc = calc_crc(
					p->len,
					p->data);

			if(crc == ch)
			{
				packet_ok(p);
			}
			else
			{
				packet_error();
			}

			parser_init(p);

			break;
		}
	}
}
