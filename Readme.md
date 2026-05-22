## 实现一个带环形缓冲区的串口

### 串口接收方案
采用DMA+空闲中断的方式，需要实现的部分有

- DMA 空闲中断
  - 串口可以接收不定长的数据，通过DMA搬运到DMA_BUFFER 缓冲区中
  - 通过DMA 搬运完成后通知cpu
- 环形缓存区实现
  - 申请一片缓存区，具有首尾index、从首写入、尾巴读出
- DMA搬运完成后，将数据写入环形缓存区

### 环形缓冲区实现
环形缓存区本质上是一个数组、当往里面写入数据时，head指针会自增，当head指针达到环形缓冲区长度时，将head指针指向0，这样就实现了逻辑上的环形缓冲区


当读取环形缓冲区数据时，从尾部tail 读取一个数据、然后tail自增，
存在下面情况， tail = head、 说明此时缓存区无数据可以读
```C
void ringbuf_write(ringbuf_t *rb, uint8_t *data, uint32_t len)
{
	for(uint32_t i = 0; i < len; i++)
	{
		rb->buf[rb->head] = data[i];

		rb->head = (rb->head + 1) % RINGBUF_SIZE;
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
```

### DMA搬运流程
1. 当一帧数据发送到DR寄存器时，DMA自动将其搬运到DMA_BUFFER，如果超过了buffer的大小，会从开头开始覆盖（DMA循环模式）
2. 当一帧数据发送完毕时，出发IDLE中断，在中断中调用用户处理函数，将这一帧数据写入ring缓冲区
3. 业务层通过流的方式读取ringuffer，上层协议解析

注意：需要考虑回绕问题，
假设DMAbuffer 为5， 第一帧数据 为3，第二帧数据为3
那么第二帧的有效数据应该是Buff[3] buff[4] buff[0]出现了回绕

回绕处理方法：
1. 记录上一次pos_old ，
2. 如果 newpos < oldsize 说明回绕了，需要处理从bufeer[oldsize] 到buffer[len - oldsize] + buff[0] + size的数据
3. 如果new>，也不一定不产生回绕、但是此时应该合理设计大小

```C
static uint16_t old_pos = 0;
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  uint16_t new_pos = Size;
  if(new_pos > old_pos)
  {
    ringbuf_write(&ring_buff, &dma_buffer[old_pos], new_pos - old_pos);
  }
  else
  {
    ringbuf_write(&ring_buff, &dma_buffer[old_pos], 256 - old_pos);
    ringbuf_write(&ring_buff, &dma_buffer[0], new_pos);
  }
}

```
