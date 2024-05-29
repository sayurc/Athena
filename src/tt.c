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
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARCH_x64
#include <immintrin.h>
#endif

#include <bit.h>
#include <rng.h>
#include <pos.h>
#include <move.h>
#include <movegen.h>
#include <tt.h>

struct transposition_table {
	NodeData *ptr;
	size_t capacity;
};

static void init_hash(void);
static size_t compute_capacity(size_t max_size);
static size_t find_prime(size_t n);
static bool is_prime(size_t n);

static struct transposition_table transposition_table = { .ptr = NULL,
							  .capacity = 0 };

/*
 * Returns true if the node data is in the transposition table table and false
 * otherwise.
 */
bool get_tt_entry(NodeData *restrict data, const Position *restrict pos)
{
	const u64 node_hash = get_position_hash(pos);
	const size_t key = node_hash % transposition_table.capacity;
	struct node_data tt_data = transposition_table.ptr[key];
	if (node_hash == tt_data.hash) {
		*data = tt_data;
		return true;
	}
	return false;
}

void store_tt_entry(const NodeData *data)
{
	const size_t key = data->hash % transposition_table.capacity;
	transposition_table.ptr[key] = *data;
}

void init_tt_entry(NodeData *data, int score, int depth, Bound bound,
		   Move best_move, const Position *pos)
{
	data->score = (i16)score;
	data->depth = (u8)depth;
	data->bound = (u8)bound;
	data->best_move = best_move;
	data->hash = get_position_hash(pos);
}

void prefetch_tt(void)
{
#ifdef ARCH_x64
	_mm_prefetch(transposition_table.ptr, _MM_HINT_T0);
#else
	return;
#endif
}

/*
 * This function does nothing if the transposition table has not been
 * initialized.
 */
void clear_tt(void)
{
	NodeData *const ptr = transposition_table.ptr;
	if (!ptr)
		return;
	const size_t capacity = transposition_table.capacity;
	memset(ptr, 0, capacity * sizeof(NodeData));
}

/*
 * This function does nothing if the transposition table has not been
 * initialized.
 */
void resize_tt(size_t size)
{
	if (!transposition_table.ptr)
		return;
	const size_t old_capacity = transposition_table.capacity;
	const size_t new_capacity = compute_capacity(size);
	transposition_table.capacity = new_capacity;
	transposition_table.ptr = realloc(transposition_table.ptr,
					  new_capacity * sizeof(NodeData));
	if (!transposition_table.ptr) {
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}

	if (new_capacity > old_capacity) {
		memset(transposition_table.ptr + old_capacity, 0,
		       (new_capacity - old_capacity) * sizeof(NodeData));
	}
}

/*
 * The capacity of the transposition table is calculated with the size given in
 * mebibytes. For performance reasons the capacity is a prime number less than
 * the originally calculated capacity. If the size is too big then a default
 * capacity is used instead.
 */
void tt_init(size_t size)
{
	init_hash();

	transposition_table.capacity = compute_capacity(size);
	transposition_table.ptr =
		calloc(transposition_table.capacity, sizeof(NodeData));
	if (!transposition_table.ptr) {
		fprintf(stderr, "Out of memory.");
		exit(1);
	}
}

void tt_free(void)
{
	transposition_table.capacity = 0;
	free(transposition_table.ptr);
	transposition_table.ptr = NULL;
}

/*
 * Generate a set of unique random numbers for Zobrist hashing.
 */
static void init_hash(void)
{
}

/*
 * To lower the chances of collisions the capacity of the transposition table
 * should be a prime number, so we return the prime number that results in a
 * table size that is closest to, but not exceeding, max_size.
 */
static size_t compute_capacity(size_t max_size)
{
	const size_t mib_in_byte = 1048576;

	size_t tmp = max_size;
	/* Check for overflow when converting to bytes. */
	if (tmp > SIZE_MAX / mib_in_byte)
		tmp = SIZE_MAX / sizeof(NodeData);
	else
		tmp = (tmp * mib_in_byte) / (long)sizeof(NodeData);
	return find_prime(tmp);
}

/*
 * Finds the greatest prime number less than or equal to n. Notice that n must
 * be at least 2.
 */
static size_t find_prime(size_t n)
{
	for (size_t p = n; p > 0; --p) {
		if (is_prime(p))
			return p;
	}

	fprintf(stderr, "Internal error.\n");
	exit(1);
}

static bool is_prime(size_t n)
{
	for (size_t m = n - 1; m * m >= n; --m) {
		if (n % m == 0)
			return false;
	}
	return true;
}
