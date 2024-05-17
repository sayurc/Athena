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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bit.h>
#include <pos.h>
#include <move.h>
#include <movegen.h>

static const Piece promotion_table[][4] = {
	[COLOR_WHITE][MOVE_KNIGHT_PROMOTION - 6] = PIECE_WHITE_KNIGHT,
	[COLOR_WHITE][MOVE_ROOK_PROMOTION   - 6] = PIECE_WHITE_ROOK,
	[COLOR_WHITE][MOVE_BISHOP_PROMOTION - 6] = PIECE_WHITE_BISHOP,
	[COLOR_WHITE][MOVE_QUEEN_PROMOTION  - 6] = PIECE_WHITE_QUEEN,
	[COLOR_BLACK][MOVE_KNIGHT_PROMOTION - 6] = PIECE_BLACK_KNIGHT,
	[COLOR_BLACK][MOVE_ROOK_PROMOTION   - 6] = PIECE_BLACK_ROOK,
	[COLOR_BLACK][MOVE_BISHOP_PROMOTION - 6] = PIECE_BLACK_BISHOP,
	[COLOR_BLACK][MOVE_QUEEN_PROMOTION  - 6] = PIECE_BLACK_QUEEN,
};
static const CastlingSide castling_table[] = {
	[MOVE_KING_CASTLE]  = CASTLING_SIDE_KING,
	[MOVE_QUEEN_CASTLE] = CASTLING_SIDE_QUEEN,
};

/*
 * Since the functions to do and undo moves do basically the same thing, I
 * created this macro. The conditionals that check the choice of doing or
 * undoing are optimized out.
 */
#define ACTION_FOR_MOVE(do_or_undo)\
const int choice_do = 0;\
const int choice_undo = !choice_do;\
const MoveType type = get_move_type(move);\
const Square from = get_move_origin(move);\
const Square to = get_move_target(move);\
const Piece piece = choice_##do_or_undo == choice_do ?\
get_piece_at(pos, from) : get_piece_at(pos, to);\
const Color color = get_piece_color(piece);\
\
if (choice_##do_or_undo == choice_do)\
	start_new_irreversible_state(pos);\
\
if (type == MOVE_OTHER)\
	do_or_undo##_other(pos, from, to, piece);\
else if (type == MOVE_DOUBLE_PAWN_PUSH)\
	do_or_undo##_double_push(pos, from, to, piece);\
else if (type == MOVE_QUEEN_CASTLE || type == MOVE_KING_CASTLE)\
	do_or_undo##_castling(pos, from, to, piece, castling_table[type]);\
else if (type == MOVE_CAPTURE)\
	do_or_undo##_capture(pos, from, to, piece);\
else if (type == MOVE_EP_CAPTURE)\
	do_or_undo##_ep_capture(pos, from, to, piece);\
else if (type >= MOVE_KNIGHT_PROMOTION && type <= MOVE_QUEEN_PROMOTION)\
	do_or_undo##_promotion(pos, from, to, promotion_table[color][type - 6],\
	                       0);\
else if (type >= MOVE_KNIGHT_PROMOTION_CAPTURE)\
	do_or_undo##_promotion(pos, from, to,\
	                       promotion_table[color][type - 10], 1);\
\
if (choice_##do_or_undo == choice_undo)\
	backtrack_irreversible_state(pos);\
flip_side_to_move(pos);

Move lan_to_move(const char *lan, const Position *pos, bool *success)
{
	char test_lan[MAX_LAN_LEN + 1];
	struct move_with_score moves[256];
	int len = get_pseudo_legal_moves(moves, MOVE_GEN_TYPE_CAPTURE, pos);
	len += get_pseudo_legal_moves(moves + len, MOVE_GEN_TYPE_QUIET, pos);
	for (int i = 0; i < len; ++i) {
		Move move = moves[i].move;
		move_to_lan(test_lan, move);
		if (!strcmp(test_lan, lan)) {
			*success = true;
			return move;
		}
	}

	*success = false;
	return 0;
}

/*
 * Converts a move to Long Algebraic Notation. The lan array must be at least
 * MAX_LAN_LEN + 1 bytes long.
 */
void move_to_lan(char *lan, Move move)
{
	const char file_to_char[] = {
		[FILE_A] = 'a', [FILE_B] = 'b', [FILE_C] = 'c', [FILE_D] = 'd',
		[FILE_E] = 'e', [FILE_F] = 'f', [FILE_G] = 'g', [FILE_H] = 'h',
	};
	const char rank_to_char[] = {
		[RANK_1] = '1', [RANK_2] = '2', [RANK_3] = '3', [RANK_4] = '4',
		[RANK_5] = '5', [RANK_6] = '6', [RANK_7] = '7', [RANK_8] = '8',
	};
	const char promo_to_char[] = {
		[MOVE_KNIGHT_PROMOTION] = 'n',
		[MOVE_ROOK_PROMOTION] = 'r',
		[MOVE_BISHOP_PROMOTION] = 'b',
		[MOVE_QUEEN_PROMOTION] = 'q',
	};
	const char promo_cap_to_char[] = {
		[MOVE_KNIGHT_PROMOTION_CAPTURE] = 'n',
		[MOVE_BISHOP_PROMOTION_CAPTURE] = 'b',
		[MOVE_ROOK_PROMOTION_CAPTURE] = 'r',
		[MOVE_QUEEN_PROMOTION_CAPTURE] = 'q',
	};

	if (!move) {
		lan[0] = '\0';
		return;
	}

	const Square sq1 = get_move_origin(move);
	const Square sq2 = get_move_target(move);
	const MoveType type = get_move_type(move);
	const File file1 = get_file(sq1);
	const File file2 = get_file(sq2);
	const Rank rank1 = get_rank(sq1);
	const Rank rank2 = get_rank(sq2);

	lan[0] = file_to_char[file1];
	lan[1] = rank_to_char[rank1];
	lan[2] = file_to_char[file2];
	lan[3] = rank_to_char[rank2];
	if (type >= MOVE_KNIGHT_PROMOTION && type <= MOVE_QUEEN_PROMOTION) {
		lan[4] = promo_to_char[type];
	} else if (type >= MOVE_KNIGHT_PROMOTION_CAPTURE &&
		   type <= MOVE_QUEEN_PROMOTION_CAPTURE) {
		lan[4] = promo_cap_to_char[type];
	} else {
		lan[4] = '\0';
	}
	lan[5] = '\0';
}

void do_null_move(Position *pos)
{
	const Color c = get_side_to_move(pos);

	start_new_irreversible_state(pos);
	flip_side_to_move(pos);

	unset_enpassant(pos);
	increment_halfmove_clock(pos);

	if (c == COLOR_BLACK)
		increment_fullmove_counter(pos);
}

void undo_null_move(Position *pos)
{
	flip_side_to_move(pos);
	backtrack_irreversible_state(pos);
}

static void do_promotion(Position *pos, Square from, Square to,
			 Piece promoted_to, int is_capture)
{
	const Color c = get_piece_color(promoted_to);

	if (is_capture) {
		const Piece captured_piece = get_piece_at(pos, to);
		if (captured_piece == PIECE_WHITE_ROOK && to == A1)
			remove_castling(pos, COLOR_WHITE, CASTLING_SIDE_QUEEN);
		else if (captured_piece == PIECE_WHITE_ROOK && to == H1)
			remove_castling(pos, COLOR_WHITE, CASTLING_SIDE_KING);
		else if (captured_piece == PIECE_BLACK_ROOK && to == A8)
			remove_castling(pos, COLOR_BLACK, CASTLING_SIDE_QUEEN);
		else if (captured_piece == PIECE_BLACK_ROOK && to == H8)
			remove_castling(pos, COLOR_BLACK, CASTLING_SIDE_KING);
		reset_halfmove_clock(pos);
		set_captured_piece(pos, get_piece_at(pos, to));
		remove_piece(pos, to);
	}
	unset_enpassant(pos);
	increment_halfmove_clock(pos);
	remove_piece(pos, from);
	place_piece(pos, to, promoted_to);

	if (c == COLOR_BLACK)
		increment_fullmove_counter(pos);
}

static void undo_promotion(Position *pos, Square from, Square to,
			   Piece promoted_to, int is_capture)
{
	const Color c = get_piece_color(promoted_to);
	const Piece pawn = c == COLOR_WHITE ? PIECE_WHITE_PAWN :
	                                      PIECE_BLACK_PAWN;

	remove_piece(pos, to);
	place_piece(pos, from, pawn);
	if (is_capture)
		place_piece(pos, to, get_captured_piece(pos));
}

static void do_castling(Position *pos, Square from, Square to, Piece piece,
			CastlingSide side)
{
	const Color c = get_piece_color(piece);
	const Piece rook = c == COLOR_WHITE ? PIECE_WHITE_ROOK :
	                                      PIECE_BLACK_ROOK;
	Square rook_from, rook_to;

	if (side == CASTLING_SIDE_KING) {
		rook_from = c == COLOR_WHITE ? H1 : H8;
		rook_to = c == COLOR_WHITE ? F1 : F8;
	} else {
		rook_from = c == COLOR_WHITE ? A1 : A8;
		rook_to = c == COLOR_WHITE ? D1 : D8;
	}

	unset_enpassant(pos);
	increment_halfmove_clock(pos);
	remove_piece(pos, rook_from);
	place_piece(pos, rook_to, rook);
	remove_piece(pos, from);
	place_piece(pos, to, piece);
	remove_castling(pos, c, CASTLING_SIDE_KING);
	remove_castling(pos, c, CASTLING_SIDE_QUEEN);

	if (c == COLOR_BLACK)
		increment_fullmove_counter(pos);
}

static void undo_castling(Position *pos, Square from, Square to, Piece piece,
			  CastlingSide side)
{
	const Color c = get_piece_color(piece);
	const Piece rook = c == COLOR_WHITE ? PIECE_WHITE_ROOK :
	                                      PIECE_BLACK_ROOK;
	Square rook_from, rook_to;

	if (side == CASTLING_SIDE_KING) {
		rook_from = c == COLOR_WHITE ? H1 : H8;
		rook_to = c == COLOR_WHITE ? F1 : F8;
	} else {
		rook_from = c == COLOR_WHITE ? A1 : A8;
		rook_to = c == COLOR_WHITE ? D1 : D8;
	}

	remove_piece(pos, rook_to);
	place_piece(pos, rook_from, rook);
	remove_piece(pos, to);
	place_piece(pos, from, piece);

	if (c == COLOR_BLACK)
		decrement_fullmove_counter(pos);
}

static void do_ep_capture(Position *pos, Square from, Square to, Piece piece)
{
	const Color c = get_piece_color(piece);
	const Square pawn_sq = c == COLOR_WHITE ? to - 8 : to + 8;
	const Piece pawn = c == COLOR_WHITE ? PIECE_BLACK_PAWN :
	                                      PIECE_WHITE_PAWN;

	unset_enpassant(pos);
	set_captured_piece(pos, pawn);
	remove_piece(pos, pawn_sq);
	remove_piece(pos, from);
	place_piece(pos, to, piece);
	reset_halfmove_clock(pos);

	if (c == COLOR_BLACK)
		increment_fullmove_counter(pos);
}

static void undo_ep_capture(Position *pos, Square from, Square to, Piece piece)
{
	const Color c = get_piece_color(piece);
	const Square pawn_sq = c == COLOR_WHITE ? to - 8 : to + 8;
	const Piece pawn = c == COLOR_WHITE ? PIECE_BLACK_PAWN :
	                                      PIECE_WHITE_PAWN;

	remove_piece(pos, to);
	place_piece(pos, from, piece);
	place_piece(pos, pawn_sq, pawn);

	if (c == COLOR_BLACK)
		decrement_fullmove_counter(pos);
}

static void do_capture(Position *pos, Square from, Square to, Piece piece)
{
	const PieceType piece_type = get_piece_type(piece);
	const Color piece_color = get_piece_color(piece);
	const Piece captured_piece = get_piece_at(pos, to);

	unset_enpassant(pos);
	set_captured_piece(pos, captured_piece);
	remove_piece(pos, to);
	remove_piece(pos, from);
	place_piece(pos, to, piece);
	reset_halfmove_clock(pos);

	switch (piece_type) {
	case PIECE_TYPE_KING:
		remove_castling(pos, piece_color, CASTLING_SIDE_KING);
		remove_castling(pos, piece_color, CASTLING_SIDE_QUEEN);
		break;
	case PIECE_TYPE_ROOK:
		if (piece_color == COLOR_WHITE && from == A1)
			remove_castling(pos, piece_color, CASTLING_SIDE_QUEEN);
		else if (piece_color == COLOR_WHITE && from == H1)
			remove_castling(pos, piece_color, CASTLING_SIDE_KING);
		else if (piece_color == COLOR_BLACK && from == A8)
			remove_castling(pos, piece_color, CASTLING_SIDE_QUEEN);
		else if (piece_color == COLOR_BLACK && from == H8)
			remove_castling(pos, piece_color, CASTLING_SIDE_KING);
		break;
	default:
		break;
	}

	if (get_piece_type(captured_piece) == PIECE_TYPE_ROOK) {
		if (captured_piece == PIECE_WHITE_ROOK && to == A1)
			remove_castling(pos, COLOR_WHITE, CASTLING_SIDE_QUEEN);
		else if (captured_piece == PIECE_WHITE_ROOK && to == H1)
			remove_castling(pos, COLOR_WHITE, CASTLING_SIDE_KING);
		else if (captured_piece == PIECE_BLACK_ROOK && to == A8)
			remove_castling(pos, COLOR_BLACK, CASTLING_SIDE_QUEEN);
		else if (captured_piece == PIECE_BLACK_ROOK && to == H8)
			remove_castling(pos, COLOR_BLACK, CASTLING_SIDE_KING);
	}

	if (piece_color == COLOR_BLACK)
		increment_fullmove_counter(pos);
}

static void undo_capture(Position *pos, Square from, Square to, Piece piece)
{
	const Color piece_color = get_piece_color(piece);
	const Piece captured_piece = get_captured_piece(pos);

	remove_piece(pos, to);
	place_piece(pos, from, piece);
	place_piece(pos, to, captured_piece);

	if (piece_color == COLOR_BLACK)
		decrement_fullmove_counter(pos);
}

static void do_double_push(Position *pos, Square from, Square to, Piece piece)
{
	const Color c = get_piece_color(piece);

	increment_halfmove_clock(pos);
	remove_piece(pos, from);
	place_piece(pos, to, piece);
	set_enpassant(pos, get_file(from));
	reset_halfmove_clock(pos);

	if (c == COLOR_BLACK)
		increment_fullmove_counter(pos);
}

static void undo_double_push(Position *pos, Square from, Square to, Piece piece)
{
	const Color c = get_piece_color(piece);

	remove_piece(pos, to);
	place_piece(pos, from, piece);
	unset_enpassant(pos);

	if (c == COLOR_BLACK)
		decrement_fullmove_counter(pos);
}

static void do_other(Position *pos, Square from, Square to, Piece piece)
{
	const PieceType pt = get_piece_type(piece);
	const Color c = get_piece_color(piece);

	unset_enpassant(pos);
	increment_halfmove_clock(pos);
	remove_piece(pos, from);
	place_piece(pos, to, piece);

	switch (pt) {
	case PIECE_TYPE_PAWN:
		reset_halfmove_clock(pos);
		break;
	case PIECE_TYPE_KING:
		remove_castling(pos, c, CASTLING_SIDE_KING);
		remove_castling(pos, c, CASTLING_SIDE_QUEEN);
		break;
	case PIECE_TYPE_ROOK:
		if (c == COLOR_WHITE && from == A1)
			remove_castling(pos, c, CASTLING_SIDE_QUEEN);
		else if (c == COLOR_WHITE && from == H1)
			remove_castling(pos, c, CASTLING_SIDE_KING);
		else if (c == COLOR_BLACK && from == A8)
			remove_castling(pos, c, CASTLING_SIDE_QUEEN);
		else if (c == COLOR_BLACK && from == H8)
			remove_castling(pos, c, CASTLING_SIDE_KING);
		break;
	default:
		break;
	}

	if (c == COLOR_BLACK)
		increment_fullmove_counter(pos);
}

static void undo_other(Position *pos, Square from, Square to, Piece piece)
{
	const Color c = get_piece_color(piece);

	remove_piece(pos, to);
	place_piece(pos, from, piece);

	if (c == COLOR_BLACK)
		decrement_fullmove_counter(pos);
}

/*
 * This function returns true is a move is legal and false otherwise. Although
 * the pos argument is non-const the original position is restored so it is safe
 * to call it. A const argument would require copying the position to make
 * changes in the copy and that would be slower.
 */
bool move_is_legal(Position *pos, Move move)
{
	const Color color = get_side_to_move(pos);
	do_move(pos, move);
	const Square sq = get_king_square(pos, color);
	if (is_square_attacked(sq, !color, pos)) {
		undo_move(pos, move);
		return false;
	}
	undo_move(pos, move);
	return true;
}

void undo_move(Position *pos, Move move)
{
	ACTION_FOR_MOVE(undo);
}

void do_move(Position *pos, Move move)
{
	ACTION_FOR_MOVE(do);
}

Move create_move(Square from, Square to, MoveType type)
{
	return (type & 0xf) << 12 | (to & 0x3f) << 6 | (from & 0x3f);
}

bool move_is_quiet(Move move)
{
	return !move_is_capture(move) && !move_is_promotion(move);
}

bool move_is_capture(Move move)
{
	const MoveType type = get_move_type(move);

	return type == MOVE_CAPTURE ||
	       type == MOVE_EP_CAPTURE ||
	       type == MOVE_KNIGHT_PROMOTION_CAPTURE ||
	       type == MOVE_ROOK_PROMOTION_CAPTURE ||
	       type == MOVE_BISHOP_PROMOTION_CAPTURE ||
	       type == MOVE_QUEEN_PROMOTION_CAPTURE;
}

bool move_is_promotion(Move move)
{
	const MoveType type = get_move_type(move);

	return type == MOVE_KNIGHT_PROMOTION ||
	       type == MOVE_BISHOP_PROMOTION ||
	       type == MOVE_ROOK_PROMOTION   ||
	       type == MOVE_QUEEN_PROMOTION  ||
	       type == MOVE_KNIGHT_PROMOTION_CAPTURE ||
	       type == MOVE_BISHOP_PROMOTION_CAPTURE ||
	       type == MOVE_ROOK_PROMOTION_CAPTURE   ||
	       type == MOVE_QUEEN_PROMOTION_CAPTURE;
}

bool move_is_castling(Move move)
{
	const MoveType type = get_move_type(move);

	return type == MOVE_QUEEN_CASTLE || type == MOVE_KING_CASTLE;
}

PieceType get_promotion_piece_type(Move move)
{
	const MoveType type = get_move_type(move);

	switch (type) {
	case MOVE_KNIGHT_PROMOTION:
		return PIECE_TYPE_KNIGHT;
	case MOVE_BISHOP_PROMOTION:
		return PIECE_TYPE_BISHOP;
	case MOVE_ROOK_PROMOTION:
		return PIECE_TYPE_ROOK;
	case MOVE_QUEEN_PROMOTION:
		return PIECE_TYPE_QUEEN;
	case MOVE_KNIGHT_PROMOTION_CAPTURE:
		return PIECE_TYPE_KNIGHT;
	case MOVE_BISHOP_PROMOTION_CAPTURE:
		return PIECE_TYPE_BISHOP;
	case MOVE_ROOK_PROMOTION_CAPTURE:
		return PIECE_TYPE_ROOK;
	case MOVE_QUEEN_PROMOTION_CAPTURE:
		return PIECE_TYPE_QUEEN;
	default:
		abort();
	}
}

Square get_move_origin(Move move)
{
	return move & 0x3f;
}

Square get_move_target(Move move)
{
	return move >> 6 & 0x3f;
}

MoveType get_move_type(Move move)
{
	return move >> 12 & 0xf;
}
