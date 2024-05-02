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
#include <stdlib.h>

#include <bit.h>
#include <rng.h>

/*
 * These functions implement the xoshiro256** and SplitMix64 PRNGs, the latter
 * being used to seed the former.
 */

static u64 sm_next(void);
static void sm_seed(u64 n);
static u64 rotl(u64 x, int k);

static u64 s[4];
static u64 sm_s;

/*
 * Most magic numbers seem to have sparse bits. So considering the RNG generates
 * a uniformly distributed sequence of numbers, this function should generate
 * numbers which have 1/8 of their bits set on average.
 */
u64 next_sparse_rand(void)
{
	return next_rand() & next_rand() & next_rand();
}

u64 next_rand(void)
{
	const u64 result = rotl(s[0] + s[3], 23) + s[0];
	const u64 t = s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 45);

	return result;
}

void seed_rng(u64 n)
{
	sm_seed(n);
	const size_t len = sizeof(s) / sizeof(s[0]);
	for (size_t i = 0; i < len; ++i)
		s[i] = sm_next();
}

static u64 sm_next(void)
{
	u64 z = sm_s;

	z += 0x9e3779b97f4a7c15;
	z  = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z  = (z ^ (z >> 27)) * 0x94d049bb133111eb;

	return z ^ (z >> 31);
}

static void sm_seed(u64 n)
{
	sm_s = n;
}

static u64 rotl(u64 x, int k)
{
	return (x << k) | (x >> (64 - k));
}
