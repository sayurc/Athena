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

#ifndef MOVEGEN_H
#define MOVEGEN_H

enum move_gen_type {
	MOVE_GEN_TYPE_QUIET,
	MOVE_GEN_TYPE_CAPTURE,
};

bool move_is_pseudo_legal(Move move, const Position *pos);
u64 get_west_ray(Square sq);
u64 get_east_ray(Square sq);
u64 get_southwest_ray(Square sq);
u64 get_southeast_ray(Square sq);
u64 get_northwest_ray(Square sq);
u64 get_northeast_ray(Square sq);
u64 get_south_ray(Square sq);
u64 get_north_ray(Square sq);
u64 shift_bb_southwest(u64 bb, int n);
u64 shift_bb_southeast(u64 bb, int n);
u64 shift_bb_northwest(u64 bb, int n);
u64 shift_bb_northeast(u64 bb, int n);
u64 shift_bb_south(u64 bb, int n);
u64 shift_bb_north(u64 bb, int n);
u64 shift_bb_east(u64 bb, int n);
u64 shift_bb_west(u64 bb, int n);
u64 get_file_bitboard(File file);
u64 movegen_perft(Position *pos, int depth);
u64 get_attackers(Square sq, const Position *pos);
bool square_is_attacked_by_pawn(Square sq, Color by_side, const Position *pos);
bool is_square_attacked(Square sq, Color by_side, const Position *pos);
int get_pseudo_legal_moves(struct move_with_score *moves, enum move_gen_type type,
			   const Position *restrict pos);
void movegen_init(void);

#endif
