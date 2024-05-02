/*
 * Copyright (C) 2023 Sayu <mail@sayurc.moe>
 *
 * This file is part of Athena.
 *
 * Athena is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * Athena is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>. 
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef ARCH_x64
#include <immintrin.h>
#endif

#include "bit.h"

u64 pext(u64 n, u64 mask)
{
#ifdef USE_BMI2
	return _pext_u64(n, mask);
#else
	u64 ret = 0;
	for (u64 bits = 1; mask; bits += bits) {
		if (n & mask & -mask)
			ret |= bits;
		mask &= mask - 1;
	}
	return ret;
#endif
}

int popcnt(u64 n)
{
#ifdef USE_POPCNT
	return _mm_popcnt_u64(n);
#else
	const u64 k1 = U64(0x5555555555555555);
	const u64 k2 = U64(0x3333333333333333);
	const u64 k4 = U64(0x0f0f0f0f0f0f0f0f);
	const u64 kf = U64(0x0101010101010101);
	n =  n       -  ((n >> 1)  & k1);
	n = (n & k2) +  ((n >> 2)  & k2);
	n = (n       +  (n >> 4)) & k4 ;
	n = (n * kf) >> 56;
	return (int)n;
#endif
}

/*
 * Sets the least significant 1 bit to 0 and returns its index.
 */
int unset_ls1b(u64 *n)
{
	const int i = get_ls1b(*n);
	*n &= *n - 1;
	return i;
}

/*
 * Returns the index of the least significant 1 bit.
 */
int get_ls1b(u64 n)
{
#ifdef USE_BMI
	return _tzcnt_u64(n);
#else
	const int index[64] = {
		0,  47,  1, 56, 48, 27,  2, 60,
		57, 49, 41, 37, 28, 16,  3, 61,
		54, 58, 35, 52, 50, 42, 21, 44,
		38, 32, 29, 23, 17, 11,  4, 62,
		46, 55, 26, 59, 40, 36, 15, 53,
		34, 51, 20, 43, 31, 22, 10, 45,
		25, 39, 14, 33, 19, 30,  9, 24,
		13, 18,  8, 12,  7,  6,  5, 63,
	};
	const u64 debruijn = U64(0x03f79d71b4cb0a89);

	return index[((n ^ (n - 1)) * debruijn) >> 58];
#endif
}

/*
 * Returns the index of the most significant 1 bit.
 */
int get_ms1b(u64 n) {
#ifdef USE_BMI
	return 63 ^ _lzcnt_u64(n);
#else
	const int index64[64] = {
		 0, 47,  1, 56, 48, 27,  2, 60,
		57, 49, 41, 37, 28, 16,  3, 61,
		54, 58, 35, 52, 50, 42, 21, 44,
		38, 32, 29, 23, 17, 11,  4, 62,
		46, 55, 26, 59, 40, 36, 15, 53,
		34, 51, 20, 43, 31, 22, 10, 45,
		25, 39, 14, 33, 19, 30,  9, 24,
		13, 18,  8, 12,  7,  6,  5, 63,
	};
	const u64 debruijn64 = U64(0x03f79d71b4cb0a89);
	n |= n >> 1; 
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	return index64[(n * debruijn64) >> 58];
#endif
}
