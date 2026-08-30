#include <stdio.h>
#include <math.h>

int round2int_f(float num) {
	if (num >= 0) {
		return (int)ceil(num);  // 正数向上取整
	}
	else {
		return (int)floor(num); // 负数向下取整
	}
}