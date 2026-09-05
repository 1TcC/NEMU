#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_si(char *args){
	uint32_t n = 1;
	if (args != NULL){
		if (sscanf (args , "%u" , &n) != 1 ){
			return 0;
		}
	}
	cpu_exec(n);
	return 0;
}

static int cmd_w(char *args) {
	WP *wp;

	if(args == NULL) {
		printf("Usage: w EXPR\n");
		return 0;
	}

	wp = add_watchpoint(args);

	if(wp == NULL) {
		printf("Invalid expression.\n");
		return 0;
	}

	printf("Watchpoint %d: %s\n", wp->NO, wp->expr);

	return 0;
}

static int cmd_info(char *args){
	if (args == NULL){
		return 0;
	}

	if (strcmp(args, "r") == 0) {
        printf("eax\t0x%08x\n", cpu.eax);
        printf("ecx\t0x%08x\n", cpu.ecx);
        printf("edx\t0x%08x\n", cpu.edx);
        printf("ebx\t0x%08x\n", cpu.ebx);
        printf("esp\t0x%08x\n", cpu.esp);
        printf("ebp\t0x%08x\n", cpu.ebp);
        printf("esi\t0x%08x\n", cpu.esi);
        printf("edi\t0x%08x\n", cpu.edi);
        printf("eip\t0x%08x\n", cpu.eip);
        printf("eflags\t0x%08x\n", cpu.eflags.val);
	}
	else if(strcmp(args, "w") == 0) {
		print_watchpoints();
	}

	return 0;
}

static int cmd_x(char *args){
	uint32_t n;
	uint32_t i;
	swaddr_t addr;

	if (args == NULL){
		return 0;
	}

	if (sscanf(args, "%u %x", &n, &addr) != 2){
		return 0;
	}

	for (i=0; i<n; i++){
		printf("0x%08x: 0x%08x\n",
                addr, swaddr_read(addr, 4));
        addr += 4;
	}
	return 0;
}

static int cmd_p(char *args){
	bool success;
	uint32_t result;

	if(args == NULL){
		return 0;
	}

	result = expr(args, &success);

	if(success){
		printf("%u\n", result);
	}

	return 0;
}

static int cmd_d(char *args) {
	int no;

	if(args == NULL) {
		printf("Usage: d N\n");
		return 0;
	}

	if(sscanf(args, "%d", &no) != 1) {
		printf("Usage: d N\n");
		return 0;
	}

	if(delete_watchpoint(no)) {
		printf("Watchpoint %d deleted.\n", no);
	}
	else {
		printf("Watchpoint %d not found.\n", no);
	}

	return 0;
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Single step", cmd_si },
	{ "info", "Print register", cmd_info },
	{ "x", "Scan memory", cmd_x },
	{ "p", "Evaluate expression", cmd_p },
	{ "w", "Set a watchpoint", cmd_w },
	{ "d", "Delete a watchpoint", cmd_d },
	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
