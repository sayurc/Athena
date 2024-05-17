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
#include <movegen.h>
#include <eval.h>

struct score {
	int mg;
	int eg;
};

/* clang-format off */
static const int mg_pawn_sq_table[64] = {
	  0,   0,   0,   0,   0,   0,  0,   0,
	 98, 134,  61,  95,  68, 126, 34, -11,
	 -6,   7,  26,  31,  65,  56, 25, -20,
	-14,  13,   6,  21,  23,  12, 17, -23,
	-27,  -2,  -5,  12,  17,   6, 10, -25,
	-26,  -4,  -4, -10,   3,   3, 33, -12,
	-35,  -1, -20, -23, -15,  24, 38, -22,
	  0,   0,   0,   0,   0,   0,  0,   0,
};

static const int eg_pawn_sq_table[64] = {
	  0,   0,   0,   0,   0,   0,   0,   0,
	178, 173, 158, 134, 147, 132, 165, 187,
	 94, 100,  85,  67,  56,  53,  82,  84,
	 32,  24,  13,   5,  -2,   4,  17,  17,
	 13,   9,  -3,  -7,  -7,  -8,   3,  -1,
	  4,   7,  -6,   1,   0,  -5,  -1,  -8,
	 13,   8,   8,  10,  13,   0,   2,  -7,
	  0,   0,   0,   0,   0,   0,   0,   0,
};

static const int mg_knight_sq_table[64] = {
	-167, -89, -34, -49,  61, -97, -15, -107,
	 -73, -41,  72,  36,  23,  62,   7,  -17,
	 -47,  60,  37,  65,  84, 129,  73,   44,
	  -9,  17,  19,  53,  37,  69,  18,   22,
	 -13,   4,  16,  13,  28,  19,  21,   -8,
	 -23,  -9,  12,  10,  19,  17,  25,  -16,
	 -29, -53, -12,  -3,  -1,  18, -14,  -19,
	-105, -21, -58, -33, -17, -28, -19,  -23,
};

static const int eg_knight_sq_table[64] = {
	-58, -38, -13, -28, -31, -27, -63, -99,
	-25,  -8, -25,  -2,  -9, -25, -24, -52,
	-24, -20,  10,   9,  -1,  -9, -19, -41,
	-17,   3,  22,  22,  22,  11,   8, -18,
	-18,  -6,  16,  25,  16,  17,   4, -18,
	-23,  -3,  -1,  15,  10,  -3, -20, -22,
	-42, -20, -10,  -5,  -2, -20, -23, -44,
	-29, -51, -23, -15, -22, -18, -50, -64,
};

static const int mg_bishop_sq_table[64] = {
	-29,   4, -82, -37, -25, -42,   7,  -8,
	-26,  16, -18, -13,  30,  59,  18, -47,
	-16,  37,  43,  40,  35,  50,  37,  -2,
	 -4,   5,  19,  50,  37,  37,   7,  -2,
	 -6,  13,  13,  26,  34,  12,  10,   4,
	  0,  15,  15,  15,  14,  27,  18,  10,
	  4,  15,  16,   0,   7,  21,  33,   1,
	-33,  -3, -14, -21, -13, -12, -39, -21,
};

static const int eg_bishop_sq_table[64] = {
	-14, -21, -11,  -8, -7,  -9, -17, -24,
	 -8,  -4,   7, -12, -3, -13,  -4, -14,
	  2,  -8,   0,  -1, -2,   6,   0,   4,
	 -3,   9,  12,   9, 14,  10,   3,   2,
	 -6,   3,  13,  19,  7,  10,  -3,  -9,
	-12,  -3,   8,  10, 13,   3,  -7, -15,
	-14, -18,  -7,  -1,  4,  -9, -15, -27,
	-23,  -9, -23,  -5, -9, -16,  -5, -17,
};

static const int mg_rook_sq_table[64] = {
	 32,  42,  32,  51, 63,  9,  31,  43,
	 27,  32,  58,  62, 80, 67,  26,  44,
	 -5,  19,  26,  36, 17, 45,  61,  16,
	-24, -11,   7,  26, 24, 35,  -8, -20,
	-36, -26, -12,  -1,  9, -7,   6, -23,
	-45, -25, -16, -17,  3,  0,  -5, -33,
	-44, -16, -20,  -9, -1, 11,  -6, -71,
	-19, -13,   1,  17, 16,  7, -37, -26,
};

static const int eg_rook_sq_table[64] = {
	13, 10, 18, 15, 12,  12,   8,   5,
	11, 13, 13, 11, -3,   3,   8,   3,
	 7,  7,  7,  5,  4,  -3,  -5,  -3,
	 4,  3, 13,  1,  2,   1,  -1,   2,
	 3,  5,  8,  4, -5,  -6,  -8, -11,
	-4,  0, -5, -1, -7, -12,  -8, -16,
	-6, -6,  0,  2, -9,  -9, -11,  -3,
	-9,  2,  3, -1, -5, -13,   4, -20,
};

static const int mg_queen_sq_table[64] = {
	-28,   0,  29,  12,  59,  44,  43,  45,
	-24, -39,  -5,   1, -16,  57,  28,  54,
	-13, -17,   7,   8,  29,  56,  47,  57,
	-27, -27, -16, -16,  -1,  17,  -2,   1,
	 -9, -26,  -9, -10,  -2,  -4,   3,  -3,
	-14,   2, -11,  -2,  -5,   2,  14,   5,
	-35,  -8,  11,   2,   8,  15,  -3,   1,
	 -1, -18,  -9,  10, -15, -25, -31, -50,
};

static const int eg_queen_sq_table[64] = {
	 -9,  22,  22,  27,  27,  19,  10,  20,
	-17,  20,  32,  41,  58,  25,  30,   0,
	-20,   6,   9,  49,  47,  35,  19,   9,
	  3,  22,  24,  45,  57,  40,  57,  36,
	-18,  28,  19,  47,  31,  34,  39,  23,
	-16, -27,  15,   6,   9,  17,  10,   5,
	-22, -23, -30, -16, -16, -23, -36, -32,
	-33, -28, -22, -43,  -5, -32, -20, -41,
};

static const int mg_king_sq_table[64] = {
	-65,  23,  16, -15, -56, -34,   2,  13,
	 29,  -1, -20,  -7,  -8,  -4, -38, -29,
	 -9,  24,   2, -16, -20,   6,  22, -22,
	-17, -20, -12, -27, -30, -25, -14, -36,
	-49,  -1, -27, -39, -46, -44, -33, -51,
	-14, -14, -22, -46, -44, -30, -15, -27,
	  1,   7,  -8, -64, -43, -16,   9,   8,
	-15,  36,  12, -54,   8, -28,  24,  14,
};

static const int eg_king_sq_table[64] = {
	-74, -35, -18, -18, -11,  15,   4, -17,
	-12,  17,  14,  17,  17,  38,  23,  11,
	 10,  17,  23,  15,  20,  45,  44,  13,
	 -8,  22,  24,  27,  26,  33,  26,   3,
	-18,  -4,  21,  24,  27,  23,   9, -11,
	-19,  -3,  11,  21,  23,  16,   7,  -9,
	-27, -11,   4,  13,  14,   4,  -5, -17,
	-53, -34, -21, -11, -28, -14, -24, -43
};
/* clang-format on */

static void insertion_sort(struct move_with_score *moves, int nb);
static bool wins_exchange(Move move, int threshold, const Position *pos);
static int evaluate_move(Move move, const Position *pos);
static int get_square_value(Piece piece, Square sq, bool middle_game);
static int mvv_lva(Move move, const Position *pos);

/*
 * These are the intrinsic point value of each piece in the centipawn scale.
 */
static const int point_value[] = {
	[PIECE_TYPE_PAWN] = 100,   [PIECE_TYPE_KNIGHT] = 325,
	[PIECE_TYPE_BISHOP] = 350, [PIECE_TYPE_ROOK] = 500,
	[PIECE_TYPE_QUEEN] = 1000, [PIECE_TYPE_KING] = 10000,
};

/*
 * This function returns 0 when there are no more moves.
 */
Move pick_next_move(struct move_picker_context *ctx, Position *pos)
{
top:
	switch (ctx->stage) {
	case MOVE_PICKER_STAGE_TT:
		++ctx->stage;
		return ctx->tt_move;
	case MOVE_PICKER_STAGE_CAPTURE_INIT: {
		int added = get_pseudo_legal_moves(ctx->moves,
						   MOVE_GEN_TYPE_CAPTURE, pos);
		for (int i = 0; i < added; ++i) {
			const Move move = ctx->moves[i].move;
			ctx->moves[i].score = (i16)evaluate_move(move, pos);
		}
		/* In MOVE_PICKER_STAGE_GOOD_CAPTURE and
		 * MOVE_PICKER_STAGE_BAD_CAPTURE we want to simply return the
		 * next winning captures or losing captures, respectively. But
		 * we also have to return them sorted. If we don't sort the
		 * captures here those two stages will have to look for the
		 * best winning or losing capture, which would take longer
		 * since they would have to not only find a winning/losing
		 * capture but also compare the scores. */
		insertion_sort(ctx->moves, added);
		ctx->captures_end = added;
		++ctx->stage;
		/* Fall into MOVE_PICKER_STAGE_GOOD_CAPTURE. */
	}
	case MOVE_PICKER_STAGE_GOOD_CAPTURE:
		/* If we've run out of good captures. */
		if (ctx->index == ctx->captures_end) {
			++ctx->stage;
			goto top;
		}
		/* Skip TT move which was already returned in the TT stage. */
		if (ctx->moves[ctx->index].move == ctx->tt_move)
			++ctx->index;
		if (ctx->index == ctx->captures_end) {
			++ctx->stage;
			goto top;
		}

		for (; ctx->index < ctx->captures_end; ++ctx->index) {
			const Move move = ctx->moves[ctx->index].move;
			if (wins_exchange(move, -ctx->moves[ctx->index].score / 8, pos)) {
				++ctx->index;
				return move;
			}
			/* We move the bad captures to the start of the array
			 * so we can use them later. This does not undo the
			 * sorting we did in MOVE_PICKER_STAGE_CAPTURE_INIT. */
			ctx->moves[ctx->bad_captures_end] =
				ctx->moves[ctx->index];
			++ctx->bad_captures_end;
		}

		/* If this code is executed that means we ran out of good
		 * captures, all the bad captures (if any) were moved to the
		 * start of the array and we can fall into
		 * MOVE_PICKER_STAGE_QUIET_INIT */
		++ctx->stage;
	case MOVE_PICKER_STAGE_QUIET_INIT: {
		/* The bad captures were moved to the start of the array so we
		 * put the quiet moves after them, overwriting the good captures
		 * that have already been returned. */
		ctx->index = ctx->bad_captures_end;
		ctx->quiets_end = ctx->bad_captures_end;
		int added = get_pseudo_legal_moves(&ctx->moves[ctx->index],
						   MOVE_GEN_TYPE_QUIET, pos);
		ctx->quiets_end += added;
		for (int i = ctx->index; i < ctx->quiets_end; ++i) {
			const Move move = ctx->moves[i].move;
			ctx->moves[i].score = (i16)evaluate_move(move, pos);
		}
		insertion_sort(&ctx->moves[ctx->index], added);

		++ctx->stage;
		/* Fall into MOVE_PICKER_STAGE_QUIET. */
	}
	case MOVE_PICKER_STAGE_QUIET:
		/* If we've run out of quiet moves. */
		if (ctx->index == ctx->quiets_end) {
			ctx->index = 0; /* For MOVE_PICKER_STAGE_BAD_CAPUTRE. */
			++ctx->stage;
			goto top;
		}
		/* Skip TT move which was already returned in the TT stage. */
		if (ctx->moves[ctx->index].move == ctx->tt_move)
			++ctx->index;
		if (ctx->index == ctx->quiets_end) {
			ctx->index = 0;
			++ctx->stage;
			goto top;
		}

		++ctx->index;
		return ctx->moves[ctx->index - 1].move;
	case MOVE_PICKER_STAGE_BAD_CAPTURE:
		/* If we've run out of moves. */
		if (ctx->index == ctx->bad_captures_end) {
			++ctx->stage;
			return 0;
		}
		/* Skip TT move which was already returned in the TT stage. */
		if (ctx->moves[ctx->index].move == ctx->tt_move)
			++ctx->index;
		if (ctx->index == ctx->bad_captures_end) {
			++ctx->stage;
			return 0;
		}

		++ctx->index;
		return ctx->moves[ctx->index - 1].move;
	}
}

/*
 * tt_move should be 0 if there is no transposition table move.
 */
void init_move_picker_context(struct move_picker_context *ctx, Move tt_move)
{
	ctx->captures_end = 0;
	ctx->quiets_end = 0;
	ctx->bad_captures_end = 0;
	ctx->tt_move = tt_move;
	ctx->stage = ctx->tt_move ? MOVE_PICKER_STAGE_TT :
				    MOVE_PICKER_STAGE_CAPTURE_INIT;
	ctx->index = 0;
}

int evaluate(const Position *pos)
{
	const Color color = get_side_to_move(pos);
	const int phase = get_phase(pos);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	/* Material */
	for (PieceType pt = PIECE_TYPE_PAWN; pt <= PIECE_TYPE_QUEEN; ++pt) {
		const Piece p1 = create_piece(pt, color),
			    p2 = create_piece(pt, !color);
		const int nb1 = get_number_of_pieces(pos, p1);
		const int nb2 = get_number_of_pieces(pos, p2);
		const int tmp = point_value[pt] * (nb1 - nb2);
		/* The material score for the middle-game and end-game are the
		 * same. */
		score.mg += tmp;
		score.eg += tmp;
	}

	/* PST */
	for (Square sq = A1; sq <= H8; ++sq) {
		const Piece piece = get_piece_at(pos, sq);
		if (piece == PIECE_NONE)
			continue;
		if (get_piece_color(piece) == color) {
			score.mg += get_square_value(piece, sq, true);
			score.eg += get_square_value(piece, sq, false);
		} else {
			score.mg -= get_square_value(piece, sq, true);
			score.eg += get_square_value(piece, sq, false);
		}
	}

	/* Linear interpolation of (INITIAL_PHASE, score.mg) and
	 * (FINAL_PHASE, score.eg). */
	return ((score.mg * (FINAL_PHASE - phase)) +
		score.eg * (phase - INITIAL_PHASE)) /
	       FINAL_PHASE;
}

static void insertion_sort(struct move_with_score *moves, int nb)
{
	for (int i = 1; i < nb; ++i) {
		const struct move_with_score m = moves[i];
		int j = i - 1;
		for (; j >= 0 && moves[j].score < m.score; --j)
			moves[j + 1] = moves[j];
		moves[j + 1] = m;
	}
}

/*
 * SEE (Static Exchange Evaluation). Returns true if the side to move wins the
 * exchange by a piece value greater than the threshold, and returns false
 * otherwise.
 *
 * It works by simulating a sequence of captures until there are no more
 * attackers or we are sure that one of the sides won the exchange with a high
 * enough score.
 *
 * The exchange is won in these case:
 * - If after a sequence of captures we get a score above the threshold even
 *   if we assume the opponent will capture the last piece we used;
 * - If our score is above the threshold and our opponent runs out of pieces;
 * - If our score is above the threshold and our opponent only has the king
 *   attacking the square but the square is also being attacked by one of our
 *   pieces, meaning our opponent can't capture.
 */
static bool wins_exchange(Move move, int threshold, const Position *pos)
{
	const Square from = get_move_origin(move);
	const Square to = get_move_target(move);
	const Color initial_side = get_side_to_move(pos);

	bool first_capture = true;
	u64 attackers = get_attackers(to, pos);
	Piece piece_to_be_captured;
	if (get_move_type(move) == MOVE_EP_CAPTURE) {
		piece_to_be_captured = initial_side == COLOR_WHITE ?
					       PIECE_BLACK_PAWN :
					       PIECE_WHITE_PAWN;
	} else {
		piece_to_be_captured = get_piece_at(pos, to);
	}
	Color side = initial_side;
	int score = 0;
	/* This loop stops when one of the sides run out of attackers. */
	while (attackers & get_color_bitboard(pos, side)) {
		score += point_value[get_piece_type(piece_to_be_captured)];

		PieceType attacker_type;
		if (first_capture) {
			const Piece piece = get_piece_at(pos, from);
			attacker_type = get_piece_type(piece);

			if (attacker_type == PIECE_TYPE_KING &&
			    attackers & get_color_bitboard(pos, !side))
				return side != initial_side;

			attackers &= ~(U64(0x1) << from);
			piece_to_be_captured = piece;
			first_capture = false;
		} else {
			/* Here we find the least valuable piece to capture the
			 * enemy piece. */
			u64 bb = 0;
			for (attacker_type = PIECE_TYPE_PAWN;
			     attacker_type <= PIECE_TYPE_KING;
			     ++attacker_type) {
				const Piece piece =
					create_piece(attacker_type, side);
				bb = attackers & get_piece_bitboard(pos, piece);
				if (bb)
					break;
			}

			/* If we haven't won the exchange so far and the only
			 * attacker we have is the king we lose if the enemy has
			 * a piece attacking the square. */
			if (attacker_type == PIECE_TYPE_KING &&
			    attackers & get_color_bitboard(pos, !side))
				return side != initial_side;

			/* Remove the piece from the attackers bitboard
			 * as if the piece in the least significant bit
			 * of bb was used. It could actually be any
			 * piece, but we choose this one. */
			const int sq = get_ls1b(bb);
			attackers &= ~(U64(0x1) << sq);
			piece_to_be_captured =
				create_piece(attacker_type, side);
		}

		/* If the score is above the threshold even if we suppose that
		 * we will lose the attacker after the capture, we win the
		 * exchange since we can just stop. */
		if (score - point_value[attacker_type] > threshold)
			return side == initial_side;

		/* Change the score to the other side's score. */
		score = -score;
		side = !side;
	}

	/* If we reach this code then the current side ran out of attackers. */
	if (side == initial_side)
		return score > threshold;
	else
		return -score > threshold;
}

/*
 * This function tries to guess how good a move is without actually searching
 * the position, the better the guess the more nodes will be pruned in the
 * alpha-beta pruning search. Of course, since it is the position evaluation
 * function that decides how good a move actually is during the search, this
 * function has to be adjusted accordingly to it.
 */
static int evaluate_move(Move move, const Position *pos)
{
	const int phase = get_phase(pos);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	const Square from = get_move_origin(move);
	const Square to = get_move_target(move);
	const Piece piece = get_piece_at(pos, from);

	if (move_is_capture(move)) {
		const int tmp = mvv_lva(move, pos);
		score.mg += tmp;
		score.eg += tmp;
	}

	score.mg += get_square_value(piece, to, true);
	score.eg += get_square_value(piece, to, false);
	score.mg -= get_square_value(piece, from, true);
	score.eg -= get_square_value(piece, from, false);

	return ((score.mg * (FINAL_PHASE - phase)) +
		score.eg * (phase - INITIAL_PHASE)) /
	       FINAL_PHASE;
}

static int get_square_value(Piece piece, Square sq, bool middle_game)
{
	const int *mg_table[] = {
		[PIECE_TYPE_PAWN] = mg_pawn_sq_table,
		[PIECE_TYPE_KNIGHT] = mg_knight_sq_table,
		[PIECE_TYPE_BISHOP] = mg_bishop_sq_table,
		[PIECE_TYPE_ROOK] = mg_rook_sq_table,
		[PIECE_TYPE_QUEEN] = mg_queen_sq_table,
		[PIECE_TYPE_KING] = mg_king_sq_table,
	};
	const int *eg_table[] = {
		[PIECE_TYPE_PAWN] = eg_pawn_sq_table,
		[PIECE_TYPE_KNIGHT] = eg_knight_sq_table,
		[PIECE_TYPE_BISHOP] = eg_bishop_sq_table,
		[PIECE_TYPE_ROOK] = eg_rook_sq_table,
		[PIECE_TYPE_QUEEN] = eg_queen_sq_table,
		[PIECE_TYPE_KING] = eg_king_sq_table,
	};

	const PieceType pt = get_piece_type(piece);
	/* The tables are from the point of view of black so we flip the square
	 * with sq ^ 56 when it is white. */
	if (get_piece_color(piece) == COLOR_WHITE) {
		return middle_game ? mg_table[pt][sq ^ 56] :
				     eg_table[pt][sq ^ 56];
	} else {
		return middle_game ? mg_table[pt][sq] : eg_table[pt][sq];
	}
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
	return point_value[len - 1 - (int)attacker] + point_value[victim];
}

#ifdef TEST_EVAL

#include <unity/unity.h>

void setUp(void)
{
	movegen_init();
}

void tearDown(void)
{
}

struct exchange_data {
	const char *fen;
	const char *move;
	int threshold;
	bool expected_result;
};

static void test_wins_exchange(void)
{
	/* clang-format off */
	const struct exchange_data data[] = {
		/* These are test exchanges involving the king. */
		{"8/1B6/8/8/4Pk2/2n5/8/7K b - - 0 1", "c3e4", 0, true},
		{"8/8/8/3nK3/8/8/4k3/8 w - - 0 1", "e5d5", point_value[PIECE_TYPE_PAWN], true},
		{"8/8/8/8/8/4qBK1/4k3/8 b - - 0 1", "e3f3", point_value[PIECE_TYPE_PAWN], true},
		{"7k/8/2B1q3/6n1/4p3/4K1N1/8/8 w - - 0 1", "g3e4", point_value[PIECE_TYPE_BISHOP], true},
		{"8/1B6/8/8/4Pk2/2n5/8/4R2K b - - 0 1", "c3e4", 0, false},
		{"8/8/8/4K1p1/5P2/5k2/7B/8 b - - 0 1", "g5f4", 0, false},
		{"8/8/8/4k3/5pR1/5K2/7q/8 w - - 0 1", "g4f4", 0, false},
		{"5r2/8/8/4k3/5pR1/5K2/3Q3q/8 w - - 0 1", "g4f4", point_value[PIECE_TYPE_KNIGHT], false},

		/* These are the cases where a side is winning the exchange
		 * before one of the sides runs out of attackers. */
		{"r1bq1rk1/n1p1pp1p/p2p2p1/3P4/PN2n3/3BBN1P/1bP2PP1/R2Q1RK1 b - - 1 13", "b2a1", 0, true},
		{"7k/8/4q3/8/4p2r/3P1B2/8/K7 w - - 0 1", "d3e4", point_value[PIECE_TYPE_PAWN], true},
		{"7k/1B6/8/6n1/4b3/8/4Q3/K7 w - - 0 1", "e2e4", point_value[PIECE_TYPE_ROOK], false},

		/* These are the cases where a side runs out of attackers before
		 * it can win the exchange. */
		{"6k1/pp3ppp/2p2n2/4Q1P1/2K2P2/1B6/PPPq3P/8 b - - 10 32", "d2c2", point_value[PIECE_TYPE_PAWN], false},
		{"r1bq1rk1/n1p1pp1p/3p1np1/p2P4/PN1B4/3B1N1P/2P2PP1/Q4RK1 w - - 0 16", "d4f6", 0, false},
		{"rnbqkb1r/ppp2ppp/5n2/3p4/2PPp3/1P2P3/PB3PPP/RN1QKBNR w KQkq - 0 6", "c4d5", -point_value[PIECE_TYPE_PAWN], false},
	};
	/* clang-format on */

	for (size_t i = 0; i < sizeof(data) / sizeof(data[i]); ++i) {
		Position *pos = create_pos(data[i].fen);
		bool s;
		const Move move = lan_to_move(data[i].move, pos, &s);
		if (!s)
			abort();
		const bool result = wins_exchange(move, 0, pos);
		TEST_ASSERT_MESSAGE(result == data[i].expected_result,
				    data[i].fen);
		destroy_pos(pos);
	}
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_wins_exchange);

	return UNITY_END();
}
#endif
