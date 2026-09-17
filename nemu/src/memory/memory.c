#include "common.h"
#include <stdlib.h>

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

#define CACHE_BLOCK_SIZE 64
#define BLOCK_OFFSET_BITS 6

/* L1: 64KB, 8-way */
#define L1_CACHE_SIZE (64 * 1024)
#define L1_WAY        8
#define L1_SET_NUM    (L1_CACHE_SIZE / CACHE_BLOCK_SIZE / L1_WAY)
#define L1_SET_BITS   7

/* L2: 4MB, 16-way */
#define L2_CACHE_SIZE (4 * 1024 * 1024)
#define L2_WAY        16
#define L2_SET_NUM    (L2_CACHE_SIZE / CACHE_BLOCK_SIZE / L2_WAY)
#define L2_SET_BITS   12


typedef struct {
	bool valid;
	uint32_t tag;
	uint8_t data[CACHE_BLOCK_SIZE];
} L1CacheLine;


typedef struct {
	bool valid;
	bool dirty;
	uint32_t tag;
	uint8_t data[CACHE_BLOCK_SIZE];
} L2CacheLine;


static L1CacheLine l1_cache[L1_SET_NUM][L1_WAY];
static L2CacheLine l2_cache[L2_SET_NUM][L2_WAY];


void init_cache(void) {
	int i, j;

	for(i = 0; i < L1_SET_NUM; i ++) {
		for(j = 0; j < L1_WAY; j ++) {
			l1_cache[i][j].valid = false;
		}
	}

	for(i = 0; i < L2_SET_NUM; i ++) {
		for(j = 0; j < L2_WAY; j ++) {
			l2_cache[i][j].valid = false;
			l2_cache[i][j].dirty = false;
		}
	}
}



static void l2_writeback(uint32_t set, L2CacheLine *line) {
	int i;

	if(!(line->valid && line->dirty)) {
		return;
	}

	hwaddr_t block_addr =
		(line->tag << (L2_SET_BITS + BLOCK_OFFSET_BITS))
		| (set << BLOCK_OFFSET_BITS);

	for(i = 0; i < CACHE_BLOCK_SIZE; i += 4) {
		uint32_t data;

		memcpy(&data, line->data + i, 4);
		dram_write(block_addr + i, 4, data);
	}

	line->dirty = false;
}



static L2CacheLine *l2_fetch(hwaddr_t addr) {
	uint32_t set =
		(addr >> BLOCK_OFFSET_BITS) & (L2_SET_NUM - 1);

	uint32_t tag =
		addr >> (BLOCK_OFFSET_BITS + L2_SET_BITS);

	int i;

	/* hit */
	for(i = 0; i < L2_WAY; i ++) {
		if(l2_cache[set][i].valid &&
				l2_cache[set][i].tag == tag) {

			return &l2_cache[set][i];
		}
	}

	int victim = -1;

	for(i = 0; i < L2_WAY; i ++) {
		if(!l2_cache[set][i].valid) {
			victim = i;
			break;
		}
	}

	if(victim == -1) {
		victim = rand() % L2_WAY;
	}

	L2CacheLine *line = &l2_cache[set][victim];


	l2_writeback(set, line);

	hwaddr_t block_addr =
		addr & ~(CACHE_BLOCK_SIZE - 1);


		for(i = 0; i < CACHE_BLOCK_SIZE; i += 4) {
		uint32_t data =
			dram_read(block_addr + i, 4);

		memcpy(line->data + i, &data, 4);
	}

	line->tag = tag;
	line->valid = true;
	line->dirty = false;

	return line;
}



static void l2_write(hwaddr_t addr,
		size_t len, uint32_t data) {

	size_t done = 0;

	while(done < len) {
		hwaddr_t cur_addr = addr + done;

		uint32_t offset =
			cur_addr & (CACHE_BLOCK_SIZE - 1);

		size_t part_len =
			CACHE_BLOCK_SIZE - offset;

		if(part_len > len - done) {
			part_len = len - done;
		}


		L2CacheLine *line =
			l2_fetch(cur_addr);

		memcpy(line->data + offset,
				(uint8_t *)&data + done,
				part_len);

		line->dirty = true;

		done += part_len;
	}
}




static L1CacheLine *l1_fetch(hwaddr_t addr) {
	uint32_t set =
		(addr >> BLOCK_OFFSET_BITS) & (L1_SET_NUM - 1);

	uint32_t tag =
		addr >> (BLOCK_OFFSET_BITS + L1_SET_BITS);

	int i;

	for(i = 0; i < L1_WAY; i ++) {
		if(l1_cache[set][i].valid &&
				l1_cache[set][i].tag == tag) {

			return &l1_cache[set][i];
		}
	}

	int victim = -1;

	for(i = 0; i < L1_WAY; i ++) {
		if(!l1_cache[set][i].valid) {
			victim = i;
			break;
		}
	}

	if(victim == -1) {
		victim = rand() % L1_WAY;
	}

	L1CacheLine *line =
		&l1_cache[set][victim];

	L2CacheLine *l2_line =
		l2_fetch(addr);

	memcpy(line->data,
			l2_line->data,
			CACHE_BLOCK_SIZE);

	line->tag = tag;
	line->valid = true;

	return line;
}


static uint32_t l1_read(hwaddr_t addr, size_t len) {
	uint32_t result = 0;
	size_t done = 0;

	while(done < len) {
		hwaddr_t cur_addr = addr + done;

		uint32_t offset =
			cur_addr & (CACHE_BLOCK_SIZE - 1);

		size_t part_len =
			CACHE_BLOCK_SIZE - offset;

		if(part_len > len - done) {
			part_len = len - done;
		}

		L1CacheLine *line =
			l1_fetch(cur_addr);

		memcpy((uint8_t *)&result + done,
				line->data + offset,
				part_len);

		done += part_len;
	}

	return result;
}


static void l1_write(hwaddr_t addr,
		size_t len, uint32_t data) {

	size_t done = 0;

	while(done < len) {
		hwaddr_t cur_addr = addr + done;

		uint32_t set =
			(cur_addr >> BLOCK_OFFSET_BITS)
			& (L1_SET_NUM - 1);

		uint32_t tag =
			cur_addr >>
			(BLOCK_OFFSET_BITS + L1_SET_BITS);

		uint32_t offset =
			cur_addr & (CACHE_BLOCK_SIZE - 1);

		size_t part_len =
			CACHE_BLOCK_SIZE - offset;

		if(part_len > len - done) {
			part_len = len - done;
		}

		int i;

		for(i = 0; i < L1_WAY; i ++) {
			if(l1_cache[set][i].valid &&
					l1_cache[set][i].tag == tag) {

				memcpy(
					l1_cache[set][i].data + offset,
					(uint8_t *)&data + done,
					part_len);

				break;
			}
		}

		done += part_len;
	}

	l2_write(addr, len, data);
}



uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	return l1_read(addr, len)
		& (~0u >> ((4 - len) << 3));
}


void hwaddr_write(hwaddr_t addr,
		size_t len, uint32_t data) {

	l1_write(addr, len, data);
}


uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	return hwaddr_read(addr, len);
}


void lnaddr_write(lnaddr_t addr,
		size_t len, uint32_t data) {

	hwaddr_write(addr, len, data);
}


uint32_t swaddr_read(swaddr_t addr, size_t len) {
#ifdef DEBUG
	assert(len == 1 ||
			len == 2 ||
			len == 4);
#endif

	return lnaddr_read(addr, len);
}


void swaddr_write(swaddr_t addr,
		size_t len, uint32_t data) {

#ifdef DEBUG
	assert(len == 1 ||
			len == 2 ||
			len == 4);
#endif

	lnaddr_write(addr, len, data);
}