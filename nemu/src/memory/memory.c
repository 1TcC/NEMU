#include "common.h"
#include <stdlib.h>

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

/*
 * L1 Cache
 *
 * block size:       64 B
 * total size:       64 KB
 * associativity:    8-way
 * replacement:      random
 * write policy:     write through
 * write miss:       not write allocate
 */

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

/*
 * Cache performance statistics.
 *
 * According to the PA3 handout:
 *   cache hit  -> 2 cycles
 *   cache miss -> 200 cycles
 */
static uint64_t cache_hit = 0;
static uint64_t cache_miss = 0;
static uint64_t cache_time = 0;


/* Initialize L1 cache. */
void init_cache() {
	int i, j;

	for(i = 0; i < CACHE_SET_NUM; i ++) {
		for(j = 0; j < CACHE_WAY; j ++) {
			cache[i][j].valid = false;
		}
	}

	cache_hit = 0;
	cache_miss = 0;
	cache_time = 0;
}


/* Print cache statistics. */
void print_cache_stat() {
	printf("Cache hit  = %llu\n",
			(unsigned long long)cache_hit);

	printf("Cache miss = %llu\n",
			(unsigned long long)cache_miss);

	printf("Cache time = %llu cycles\n",
			(unsigned long long)cache_time);
}


/*
 * Read miss handler.
 *
 * Address layout:
 *
 *   31              13 12       6 5        0
 *   +-----------------+----------+----------+
 *   |      tag        |   set    |  offset  |
 *   |     19 bit      |  7 bit   |  6 bit   |
 *   +-----------------+----------+----------+
 */
static CacheLine *cache_fetch(hwaddr_t addr) {
	uint32_t set = (addr >> 6) & 0x7f;
	uint32_t tag = addr >> 13;

	int i;

	/* Search all 8 ways. */
	for(i = 0; i < CACHE_WAY; i ++) {
		if(cache[set][i].valid &&
				cache[set][i].tag == tag) {

			cache_hit ++;
			cache_time += 2;

			return &cache[set][i];
		}
	}

	/* Cache miss. */
	cache_miss ++;
	cache_time += 200;

	/*
	 * Prefer an invalid cache line.
	 * If every way is valid, randomly choose one.
	 */
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

	/*
	 * Align address to the beginning of the 64-byte block.
	 */
	hwaddr_t block_addr =
		addr & ~(CACHE_BLOCK_SIZE - 1);

	/*
	 * Load an entire 64-byte cache block from DRAM.
	 *
	 * dram_read() returns at most 4 bytes each time,
	 * therefore 16 reads are required.
	 */
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


/*
 * Read data from L1 cache.
 *
 * A 1/2/4-byte access may cross a cache block boundary,
 * therefore it may need to be divided into two accesses.
 */
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


/*
 * Write data to L1 cache.
 *
 * Policy:
 *
 *   hit:
 *       update cache
 *       write DRAM
 *
 *   miss:
 *       do NOT allocate cache line
 *       write DRAM directly
 *
 * This implements:
 *   write through
 *   not write allocate
 */
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
		bool hit = false;

		for(i = 0; i < CACHE_WAY; i ++) {
			if(cache[set][i].valid &&
					cache[set][i].tag == tag) {

				hit = true;

				memcpy(cache[set][i].data + offset,
						(uint8_t *)&data + done,
						part_len);

				break;
			}
		}

		if(hit) {
			cache_hit ++;
			cache_time += 2;
		}
		else {
			/*
			 * Not write allocate:
			 * do not load the missing block.
			 */
			cache_miss ++;
			cache_time += 200;
		}

		done += part_len;
	}

	/*
	 * Write through:
	 * every write must also update DRAM.
	 */
	dram_write(addr, len, data);
}


/* Memory accessing interfaces */

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