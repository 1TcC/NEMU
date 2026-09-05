#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <string.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
	WP *wp;

	if(free_ == NULL) {
		assert(0);
	}

	wp = free_;
	free_ = free_->next;

	wp->next = head;
	head = wp;

	return wp;
}

void free_wp(WP *wp) {
	WP *p;

	assert(wp != NULL);

	if(head == wp) {
		head = head->next;
	}
	else {
		p = head;

		while(p != NULL &&
		      p->next != wp) {
			p = p->next;
		}

		assert(p != NULL);

		p->next = wp->next;
	}

	wp->next = free_;
	free_ = wp;
}

WP* add_watchpoint(char *e) {
	bool success;
	uint32_t value;
	WP *wp;

	assert(e != NULL);
	assert(strlen(e) < 128);

	value = expr(e, &success);

	if(!success) {
		return NULL;
	}

	wp = new_wp();

	strcpy(wp->expr, e);
	wp->value = value;

	return wp;
}

void print_watchpoints() {
	WP *wp = head;

	if(wp == NULL) {
		printf("No watchpoints.\n");
		return;
	}

	printf("Num\tValue\t\tExpression\n");

	while(wp != NULL) {
		printf("%d\t%u\t\t%s\n",
		       wp->NO,
		       wp->value,
		       wp->expr);

		wp = wp->next;
	}
}

bool delete_watchpoint(int no) {
	WP *wp = head;

	while(wp != NULL) {
		if(wp->NO == no) {
			free_wp(wp);
			return true;
		}

		wp = wp->next;
	}

	return false;
}