#include "common.h"
#include <stdlib.h>

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

#define CACHE_BLOCK_SIZE 64
#define CACHE_SIZE       (64 * 1024)
#define CACHE_WAY        8
#define CACHE_SET_NUM    (CACHE_SIZE / CACHE_BLOCK_SIZE / CACHE_WAY)

typedef struct {
	bool valid;
	uint32_t tag;
	uint8_t data[CACHE_BLOCK_SIZE];
} CacheLine;

static CacheLine cache[CACHE_SET_NUM][CACHE_WAY];


/* Initialize L1 cache. */
void init_cache() {
	int i, j;

	for(i = 0; i < CACHE_SET_NUM; i ++) {
		for(j = 0; j < CACHE_WAY; j ++) {
			cache[i][j].valid = false;
		}
	}

}

static CacheLine *cache_fetch(hwaddr_t addr) {
	uint32_t set = (addr >> 6) & 0x7f;
	uint32_t tag = addr >> 13;

	int i;

	for(i = 0; i < CACHE_WAY; i ++) {
		if(cache[set][i].valid &&
				cache[set][i].tag == tag) {

			return &cache[set][i];
		}
	}

	int victim = -1;

	for(i = 0; i < CACHE_WAY; i ++) {
		if(!cache[set][i].valid) {
			victim = i;
			break;
		}
	}

	if(victim == -1) {
		victim = rand() % CACHE_WAY;
	}

	CacheLine *line = &cache[set][victim];

	hwaddr_t block_addr =
		addr & ~(CACHE_BLOCK_SIZE - 1);
	for(i = 0; i < CACHE_BLOCK_SIZE; i += 4) {
		uint32_t data =
			dram_read(block_addr + i, 4);

		memcpy(line->data + i,
				&data,
				4);
	}

	line->tag = tag;
	line->valid = true;

	return line;
}


static uint32_t cache_read(hwaddr_t addr, size_t len) {
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

		CacheLine *line =
			cache_fetch(cur_addr);

		memcpy((uint8_t *)&result + done,
				line->data + offset,
				part_len);

		done += part_len;
	}

	return result;
}



static void cache_write(hwaddr_t addr,
		size_t len, uint32_t data) {

	size_t done = 0;

	while(done < len) {
		hwaddr_t cur_addr = addr + done;

		uint32_t set =
			(cur_addr >> 6) & 0x7f;

		uint32_t tag =
			cur_addr >> 13;

		uint32_t offset =
			cur_addr & (CACHE_BLOCK_SIZE - 1);

		size_t part_len =
			CACHE_BLOCK_SIZE - offset;

		if(part_len > len - done) {
			part_len = len - done;
		}

		int i;

		for(i = 0; i < CACHE_WAY; i ++) {
			if(cache[set][i].valid &&
					cache[set][i].tag == tag) {

				memcpy(cache[set][i].data + offset,
						(uint8_t *)&data + done,
						part_len);

				break;
			}
		}


		done += part_len;
	}
	dram_write(addr, len, data);
}


uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	return cache_read(addr, len)
		& (~0u >> ((4 - len) << 3));
}


void hwaddr_write(hwaddr_t addr,
		size_t len, uint32_t data) {

	cache_write(addr, len, data);
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