#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256,
	EQ,
	NUM,
	NEG,
	HEX,
	REG,
	NEQ,
	AND,
	OR,
	DEREF
	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},				// spaces
	
	{"\\+", '+'},					// plus
	{"-", '-'},
	{"\\*", '*'},
	{"/", '/'},
	{"\\(", '('},
	{"\\)", ')'},

	{"==", EQ},						// equal
	{"!=", NEQ},
	{"!", '!'},
	{"&&", AND},
	{"\\|\\|", OR},

	{"0x[0-9a-fA-F]+", HEX},
	{"[0-9]+", NUM},
	{"\\$[a-zA-Z][a-zA-Z0-9]*", REG}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
    				case NOTYPE:
        				break;

    				case NUM:
					case HEX:
					case REG:
        				Assert(nr_token < 32, "too many tokens");
        				Assert(substr_len < sizeof(tokens[nr_token].str),
           						"token string is too long");

        				tokens[nr_token].type = rules[i].token_type;
        				
						strncpy(tokens[nr_token].str, substr_start, substr_len);
        				tokens[nr_token].str[substr_len] = '\0';

        				nr_token++;
        				break;

    				default:
        				Assert(nr_token < 32, "too many tokens");

        				tokens[nr_token].type = rules[i].token_type;
        				nr_token++;
        				break;
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

static int check_parentheses(int p, int q) {
	int i;
	int balance = 0;
	bool surrounded = true;

	for(i = p; i <= q; i ++) {
		if(tokens[i].type == '(') {
			balance ++;
		}
		else if(tokens[i].type == ')') {
			balance --;
		}

		if(balance < 0) {
			return -1;
		}

		if(balance == 0 && i < q) {
			surrounded = false;
		}
	}

	if(balance != 0) {
		return -1;
	}

	if(tokens[p].type == '(' &&
	   tokens[q].type == ')' &&
	   surrounded) {
		return 1;
	}

	return 0;
}

static int get_precedence(int type) {
	switch(type) {
		case OR:
			return 1;

		case AND:
			return 2;

		case EQ:
		case NEQ:
			return 3;

		case '+':
		case '-':
			return 4;

		case '*':
		case '/':
			return 5;

		default:
			return -1;
	}
}

static void identify_negative() {
	int i;

	for(i = 0; i < nr_token; i ++) {
		if(tokens[i].type == '-') {
			if(i == 0 ||
			   tokens[i - 1].type == '(' ||
			   get_precedence(tokens[i - 1].type) != -1 ||
			   tokens[i - 1].type == NEG||
			   tokens[i - 1].type == '!') {
				tokens[i].type = NEG;
			}
		}
	}
}

static void identify_dereference() {
	int i;

	for(i = 0; i < nr_token; i ++) {
		if(tokens[i].type == '*') {
			if(i == 0 ||
			   tokens[i - 1].type == '(' ||
			   get_precedence(tokens[i - 1].type) != -1 ||
			   tokens[i - 1].type == NEG ||
			   tokens[i - 1].type == '!' ||
			   tokens[i - 1].type == DEREF) {
				tokens[i].type = DEREF;
			}
		}
	}
}

static int find_dominant_operator(int p, int q) {
	int i;
	int op = -1;
	int min_precedence = 3;
	int parentheses = 0;

	for(i = p; i <= q; i ++) {

		if(tokens[i].type == '(') {
			parentheses ++;
			continue;
		}

		if(tokens[i].type == ')') {
			parentheses --;
			continue;
		}

		if(parentheses != 0) {
			continue;
		}

		int precedence = get_precedence(tokens[i].type);

		if(precedence == -1) {
			continue;
		}

		if(precedence <= min_precedence) {    
			min_precedence = precedence;
			op = i;
		}
	}

	return op;
}

static uint32_t eval(int p, int q, bool *success) {
	int op;
	int paren_ret;
	uint32_t val1, val2;
	uint32_t val;

	if(p > q) {
		*success = false;
		return 0;
	}

	if(p == q) {
		if(tokens[p].type == NUM) {
			if(sscanf(tokens[p].str, "%u", &val) != 1) {
				*success = false;
				return 0;
			}

			return val;
		}

		if(tokens[p].type == HEX) {
			if(sscanf(tokens[p].str, "%x", &val) != 1) {
				*success = false;
				return 0;
			}

			return val;
		}

		if(tokens[p].type == REG) {
    		if(strcmp(tokens[p].str, "$eax") == 0) {
       			return cpu.eax;
    		}
    		else if(strcmp(tokens[p].str, "$ecx") == 0) {
        		return cpu.ecx;
    		}
    		else if(strcmp(tokens[p].str, "$edx") == 0) {
        		return cpu.edx;
    		}
    		else if(strcmp(tokens[p].str, "$ebx") == 0) {
        		return cpu.ebx;
    		}
    		else if(strcmp(tokens[p].str, "$esp") == 0) {
        		return cpu.esp;
    		}
    		else if(strcmp(tokens[p].str, "$ebp") == 0) {
       			return cpu.ebp;
    		}
    		else if(strcmp(tokens[p].str, "$esi") == 0) {
        		return cpu.esi;
    		}
    		else if(strcmp(tokens[p].str, "$edi") == 0) {
        		return cpu.edi;
   			}
    		else if(strcmp(tokens[p].str, "$eip") == 0) {
        		return cpu.eip;
    		}

    		*success = false;
    		return 0;
		}
		*success = false;
		return 0;
	}

	paren_ret = check_parentheses(p, q);

	if(paren_ret == -1) {
		*success = false;
		return 0;
	}

	if(paren_ret == 1) {
		return eval(p + 1, q - 1, success);
	}

	op = find_dominant_operator(p, q);

	if(op == -1) {
		if(tokens[p].type == NEG) {
			val = eval(p + 1, q, success);

			if(!(*success)) {
				return 0;
			}

			return 0 - val;
		}

		if(tokens[p].type == '!') {
			val = eval(p + 1, q, success);

			if(!(*success)) {
				return 0;
			}

			return !val;
		}

		if(tokens[p].type == DEREF) {
			val = eval(p + 1, q, success);

			if(!(*success)) {		
				return 0;
			}

			return swaddr_read(val, 4);
		}		

		*success = false;
		return 0;
	}

	val1 = eval(p, op - 1, success);
	if(!(*success)) {
		return 0;
	}

	val2 = eval(op + 1, q, success);
	if(!(*success)) {
		return 0;
	}

	switch(tokens[op].type) {
		case '+':
			return val1 + val2;

		case '-':
			return val1 - val2;

		case '*':
			return val1 * val2;

		case '/':
			if(val2 == 0) {
				*success = false;
				return 0;
			}
			return val1 / val2;

		case EQ:
			return val1 == val2;

		case NEQ:
			return val1 != val2;

		case AND:
			return val1 && val2;

		case OR:
			return val1 || val2;

		default:
			*success = false;
			return 0;
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	identify_negative();
	identify_dereference();

	*success = true;
	return eval(0, nr_token - 1, success);
}

