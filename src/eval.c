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
#include <move.h>
#include <eval.h>

static int mvv_lva(Move move, const Position *pos);

/*
 * These are the intrinsic point value of each piece in the centipawn scale.
 */
static const int point_value[] = {
	[PIECE_TYPE_PAWN] = 100,   [PIECE_TYPE_KNIGHT] = 325,
	[PIECE_TYPE_BISHOP] = 350, [PIECE_TYPE_ROOK] = 500,
	[PIECE_TYPE_QUEEN] = 1000, [PIECE_TYPE_KING] = 10000,
};

int pick_next_move(const Move *moves, int len, const Position *pos)
{
	int best_score = -INF;
	int best = 0;

	for (int i = 0; i < len; ++i) {
		int score = 0;
		if (move_is_capture(moves[i]))
			score += mvv_lva(moves[i], pos);
		if (score > best_score) {
			best_score = score;
			best = i;
		}
	}

	return best;
}

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

/*
 * Most Valuable Victim - Least Valuable Aggressor.
 *
 * A heuristic that scores a capturing move based on the fact that capturing
 * valuable pieces with less valuable pieces is generally good. Capturing a
 * queen with a knight is probably worth it even if we lose the knight on the
 * next move.
 */
static int mvv_lva(Move move, const Position *pos)
{
	const Piece piece1 = get_piece_at(pos, get_move_origin(move));
	const PieceType attacker = get_piece_type(piece1);
	const Color c = get_piece_color(piece1);
	const MoveType move_type = get_move_type(move);
	Piece piece2;
	if (move_type == MOVE_EP_CAPTURE)
		piece2 = c == COLOR_WHITE ? PIECE_BLACK_PAWN : PIECE_WHITE_PAWN;
	else
		piece2 = get_piece_at(pos, get_move_target(move));
	const PieceType victim = get_piece_type(piece2);

	/* Here we assume that the point_value table is sorted from least
	 * valuable to most valuable */
	const int len = sizeof(point_value) / sizeof(point_value[0]);
	return point_value[len - (int)attacker] + point_value[victim];
}
