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

#ifndef TT_H
#define TT_H

typedef enum bound {
	BOUND_LOWER,
	BOUND_UPPER,
	BOUND_EXACT,
} Bound;

typedef struct node_data {
	i16 score;
	u8 depth;
	u8 bound;
	u64 hash;
	Move best_move;
} NodeData;

u64 hash_pos(const Position *pos);
bool get_tt_entry(NodeData *data, const Position *pos);
void store_tt_entry(const NodeData *data);
void init_tt_entry(NodeData *node_data, int score, int depth, Bound bound,
		   Move best_move, const Position *pos);
void prefetch_tt(void);
void clear_tt(void);
void resize_tt(size_t size);
void tt_init(size_t size);
void tt_free(void);

#endif
