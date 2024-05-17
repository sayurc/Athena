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

#ifndef MOVE_H
#define MOVE_H

#define MAX_LAN_LEN 5

/*
 * The moves are encoded in 16 bits in the following form:
 *
 *  0000 000000 000000
 * |____|______|______|
 *   |    |      |
 *   type to   from
 * 
 * In en passant captures the "to" square is the square the attacking piece
 * will move to, and in castling moves it's the square the king will move to.
 */

typedef enum move_type {
	MOVE_OTHER,
	MOVE_DOUBLE_PAWN_PUSH,
	MOVE_KING_CASTLE,
	MOVE_QUEEN_CASTLE,
	MOVE_CAPTURE,
	MOVE_EP_CAPTURE,
	MOVE_KNIGHT_PROMOTION,
	MOVE_ROOK_PROMOTION,
	MOVE_BISHOP_PROMOTION,
	MOVE_QUEEN_PROMOTION,
	MOVE_KNIGHT_PROMOTION_CAPTURE,
	MOVE_ROOK_PROMOTION_CAPTURE,
	MOVE_BISHOP_PROMOTION_CAPTURE,
	MOVE_QUEEN_PROMOTION_CAPTURE,
} MoveType;

typedef u16 Move;
struct move_with_score {
	Move move;
	i16 score;
};

Move lan_to_move(const char *lan, const Position *pos, bool *success);
void move_to_lan(char *lan, Move move);
bool move_is_legal(Position *pos, Move move);
void undo_move(Position *pos, Move move);
void do_move(Position *pos, Move move);
void undo_null_move(Position *pos);
void do_null_move(Position *pos);
Move create_move(Square from, Square to, MoveType type);
bool move_is_quiet(Move move);
bool move_is_capture(Move move);
bool move_is_promotion(Move move);
bool move_is_castling(Move move);
PieceType get_promotion_piece_type(Move move);
Square get_move_origin(Move move);
Square get_move_target(Move move);
MoveType get_move_type(Move move);

#endif
