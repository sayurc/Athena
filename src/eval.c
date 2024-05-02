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

#include <bit.h>
#include <pos.h>
#include <eval.h>

/*
 * These are the intrinsic point value of each piece in the centipawn scale.
 */
static const int point_value[] = {
	[PIECE_TYPE_PAWN] = 100,
	[PIECE_TYPE_KNIGHT] = 325,
	[PIECE_TYPE_BISHOP] = 350,
	[PIECE_TYPE_ROOK] = 500,
	[PIECE_TYPE_QUEEN] = 1000,
	[PIECE_TYPE_KING] = 10000,
};

int evaluate(const Position *pos)
{
	const Color c = get_side_to_move(pos);

	int score = 0;
	for (PieceType pt = PIECE_TYPE_PAWN; pt <= PIECE_TYPE_QUEEN; ++pt) {
		const Piece p1 = create_piece(pt, c), p2 = create_piece(pt, !c);
		const int nb1 = get_number_of_pieces(pos, p1);
      		const int nb2 = get_number_of_pieces(pos, p2);
		score += point_value[pt] * (nb1 - nb2);
	}

	return score;
}
