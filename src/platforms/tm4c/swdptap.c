#include "general.h"
#include "swdptap.h"

static void swdptap_turnaround(uint8_t dir)
{
	static uint8_t olddir = 0;

	DEBUG("%s", dir ? "\n-> ":"\n<- ");

	/* Don't turnaround if direction not changing */
	if(dir == olddir) return;
	olddir = dir;

	if(dir)
		SWDIO_MODE_FLOAT();
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_clear(SWCLK_PORT, SWCLK_PIN);
	if(!dir)
		SWDIO_MODE_DRIVE();
}

static uint8_t swdptap_bit_in(void)
{
	uint16_t ret;

	ret = gpio_get(SWDIO_PORT, SWDIO_PIN);
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_clear(SWCLK_PORT, SWCLK_PIN);

	DEBUG("%d", ret?1:0);

	return ret != 0;
}

static void swdptap_bit_out(uint8_t val)
{
	DEBUG("%d", val);

	gpio_set_val(SWDIO_PORT, SWDIO_PIN, val);
	gpio_set(SWCLK_PORT, SWCLK_PIN);
	gpio_clear(SWCLK_PORT, SWCLK_PIN);
}

int
swdptap_init(void)
{
	swdptap_reset();
	swdptap_seq_out(0xE79E, 16); /* 0b0111100111100111 */ 
	swdptap_reset();
	swdptap_seq_out(0, 16); 

	return 0;
}

void
swdptap_reset(void)
{
	swdptap_turnaround(0);
	/* 50 clocks with TMS high */
	for(int i = 0; i < 50; i++) swdptap_bit_out(1);
}

uint32_t
swdptap_seq_in(int ticks)
{
	uint32_t index = 1;
	uint32_t ret = 0;

	swdptap_turnaround(1);

	while(ticks--) {
		if(swdptap_bit_in()) ret |= index;
		index <<= 1;
	}

	return ret;
}

uint8_t
swdptap_seq_in_parity(uint32_t *ret, int ticks)
{
	uint32_t index = 1;
	uint8_t parity = 0;
	*ret = 0;

	swdptap_turnaround(1);

	while(ticks--) {
		if(swdptap_bit_in()) {
			*ret |= index;
			parity ^= 1;
		}
		index <<= 1;
	}
	if(swdptap_bit_in()) parity ^= 1;

	return parity;
}

void
swdptap_seq_out(uint32_t MS, int ticks)
{
	swdptap_turnaround(0);

	while(ticks--) {
		swdptap_bit_out(MS & 1);
		MS >>= 1; 
	}
}

void
swdptap_seq_out_parity(uint32_t MS, int ticks)
{
	uint8_t parity = 0;

	swdptap_turnaround(0);

	while(ticks--) {
		swdptap_bit_out(MS & 1);
		parity ^= MS;
		MS >>= 1; 
	}
	swdptap_bit_out(parity & 1);
}
