#include "FLOAT.h"

FLOAT F_mul_F(FLOAT a, FLOAT b) {
	long long result = (long long)a * b;
	return result >> 16;
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
	int quotient, remainder;

	asm volatile (
		"idivl %2"
		: "=a"(quotient), "=d"(remainder)
		: "r"(b),
		  "a"((unsigned int)a << 16),
		  "d"(a >> 16)
	);

	return quotient;
}

FLOAT f2F(float a) {
	union {
		float f;
		unsigned int u;
	} value;

	value.f = a;

	unsigned int bits = value.u;
	int sign = bits >> 31;
	int exp = ((bits >> 23) & 0xff) - 127;

	unsigned int frac =
		(bits & 0x7fffff) | 0x800000;

	unsigned int result;

	if(exp < -16) {
		result = 0;
	}
	else if(exp >= 7) {
		result = frac << (exp - 7);
	}
	else {
		result = frac >> (7 - exp);
	}

	return sign ? -(FLOAT)result : (FLOAT)result;
}

FLOAT Fabs(FLOAT a) {
	int mask = a >> 31;
	return (a ^ mask) - mask;
}

/* Functions below are already implemented */

FLOAT sqrt(FLOAT x) {
	FLOAT dt, t = int2F(2);

	do {
		dt = F_div_int((F_div_F(x, t) - t), 2);
		t += dt;
	} while(Fabs(dt) > f2F(1e-4));

	return t;
}

FLOAT pow(FLOAT x, FLOAT y) {
	/* we only compute x^0.333 */
	FLOAT t2, dt, t = int2F(2);

	do {
		t2 = F_mul_F(t, t);
		dt = (F_div_F(x, t2) - t) / 3;
		t += dt;
	} while(Fabs(dt) > f2F(1e-4));

	return t;
}

