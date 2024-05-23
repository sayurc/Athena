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

static struct score evaluate_king(const Position *pos, Square sq);
static struct score evaluate_queen(const Position *pos, Square sq);
static struct score evaluate_rook(const Position *pos, Square sq);
static struct score evaluate_bishop(const Position *pos, Square sq);
static struct score evaluate_knight(const Position *pos, Square sq);
static struct score evaluate_pawn(const Position *pos, Square sq);
static void insertion_sort(struct move_with_score *moves, int nb);
static bool wins_exchange(Move move, int threshold, const Position *pos);
static int evaluate_move(Move move, const Position *pos);
static struct score evaluate_king_move(Move move, const Position *pos);
static struct score evaluate_queen_move(Move move, const Position *pos);
static struct score evaluate_rook_move(Move move, const Position *pos);
static struct score evaluate_bishop_move(Move move, const Position *pos);
static struct score evaluate_knight_move(Move move, const Position *pos);
static struct score evaluate_pawn_move(Move move, const Position *pos);
static int get_square_value(Piece piece, Square sq, bool middle_game);
static int mvv_lva(Move move, const Position *pos);
static bool is_outpost(const Position *pos, Square sq, Color side);
static int get_number_of_adjacent_friendly_pawns(const Position *pos, Square sq,
						 Color side);
static int get_number_of_enemy_pawn_stoppers(const Position *pos, Square sq,
					     Color side);
static int get_number_of_friendly_pawn_blockers(const Position *pos, Square sq,
						Color side);
static u64 fill_front_of_square(Square sq, Color side);
static int round_up_to_multiple_of_eight(int n);
static int round_down_to_multiple_of_eight(int n);

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
			if (wins_exchange(move,
					  -ctx->moves[ctx->index].score / 8,
					  pos)) {
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
	struct score (*const piece_functions[])(const Position *, Square) = {
		[PIECE_TYPE_PAWN] = evaluate_pawn,
		[PIECE_TYPE_KNIGHT] = evaluate_knight,
		[PIECE_TYPE_BISHOP] = evaluate_bishop,
		[PIECE_TYPE_ROOK] = evaluate_rook,
		[PIECE_TYPE_QUEEN] = evaluate_queen,
		[PIECE_TYPE_KING] = evaluate_king,
	};

	const Color color = get_side_to_move(pos);
	const int phase = get_phase(pos);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	for (Color c = COLOR_WHITE; c <= COLOR_BLACK; ++c) {
		for (PieceType pt = PIECE_TYPE_PAWN; pt <= PIECE_TYPE_KING;
		     ++pt) {
			const Piece piece = create_piece(pt, c);
			u64 bb = get_piece_bitboard(pos, piece);
			while (bb) {
				const Square sq = (Square)unset_ls1b(&bb);
				const struct score piece_score =
					piece_functions[pt](pos, sq);
				if (c == color) {
					score.mg += piece_score.mg;
					score.eg += piece_score.eg;
				} else {
					score.mg -= piece_score.mg;
					score.eg -= piece_score.eg;
				}
			}
		}
	}

	/* Linear interpolation of (INITIAL_PHASE, score.mg) and
	 * (FINAL_PHASE, score.eg). */
	return ((score.mg * (FINAL_PHASE - phase)) +
		score.eg * (phase - INITIAL_PHASE)) /
	       FINAL_PHASE;
}

static struct score evaluate_king(const Position *pos, Square sq)
{
	const Piece piece = get_piece_at(pos, sq);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	score.mg += get_square_value(piece, sq, true);
	score.eg += get_square_value(piece, sq, false);

	return score;
}

static struct score evaluate_queen(const Position *pos, Square sq)
{
	const Piece piece = get_piece_at(pos, sq);

	struct score score;
	score.mg = point_value[PIECE_TYPE_QUEEN];
	score.eg = point_value[PIECE_TYPE_QUEEN];

	score.mg += get_square_value(piece, sq, true);
	score.eg += get_square_value(piece, sq, false);

	return score;
}

static struct score evaluate_rook(const Position *pos, Square sq)
{
	const Piece piece = get_piece_at(pos, sq);

	struct score score;
	score.mg = point_value[PIECE_TYPE_ROOK];
	score.eg = point_value[PIECE_TYPE_ROOK];

	score.mg += get_square_value(piece, sq, true);
	score.eg += get_square_value(piece, sq, false);

	return score;
}

static struct score evaluate_bishop(const Position *pos, Square sq)
{
	const Piece piece = get_piece_at(pos, sq);
	const Color side = get_piece_color(piece);

	struct score score;
	score.mg = point_value[PIECE_TYPE_BISHOP];
	score.eg = point_value[PIECE_TYPE_BISHOP];

	score.mg += get_square_value(piece, sq, true);
	score.eg += get_square_value(piece, sq, false);

	if (is_outpost(pos, sq, side)) {
		score.mg += 26;
		score.eg += 14;
	}

	return score;
}

static struct score evaluate_knight(const Position *pos, Square sq)
{
	const Piece piece = get_piece_at(pos, sq);
	const Color side = get_piece_color(piece);

	struct score score;
	score.mg = point_value[PIECE_TYPE_KNIGHT];
	score.eg = point_value[PIECE_TYPE_KNIGHT];

	score.mg += get_square_value(piece, sq, true);
	score.eg += get_square_value(piece, sq, false);

	if (is_outpost(pos, sq, side)) {
		score.mg += 30;
		score.eg += 18;
	}

	return score;
}

/*
 * Evaluate the score for a single pawn on the square sq.
 */
static struct score evaluate_pawn(const Position *pos, Square sq)
{
	Color c = get_piece_color(get_piece_at(pos, sq));
	const Piece piece = create_piece(PIECE_TYPE_PAWN, c);

	/* Material score. */
	struct score score;
	score.mg = point_value[PIECE_TYPE_PAWN];
	score.eg = point_value[PIECE_TYPE_PAWN];

	/* Piece-square-table score. */
	score.mg += get_square_value(piece, sq, true);
	score.eg += get_square_value(piece, sq, false);

	/* Penalty for doubled pawns. */
	if (get_number_of_friendly_pawn_blockers(pos, sq, c)) {
		score.mg -= 8;
		score.eg -= 12;
	}

	/* Bonus for passed pawn. */
	if (get_number_of_enemy_pawn_stoppers(pos, sq, c) == 0) {
		score.mg += 10;
		score.eg += 22;
	}

	/* Penalty for isolated pawn. */
	if (get_number_of_adjacent_friendly_pawns(pos, sq, c) == 0) {
		score.mg -= 5;
		score.eg -= 15;
	}

	return score;
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
	struct score (*const piece_functions[])(Move, const Position *) = {
		[PIECE_TYPE_PAWN] = evaluate_pawn_move,
		[PIECE_TYPE_KNIGHT] = evaluate_knight_move,
		[PIECE_TYPE_BISHOP] = evaluate_bishop_move,
		[PIECE_TYPE_ROOK] = evaluate_rook_move,
		[PIECE_TYPE_QUEEN] = evaluate_queen_move,
		[PIECE_TYPE_KING] = evaluate_king_move,
	};

	const int phase = get_phase(pos);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	const Square from = get_move_origin(move);
	const Piece piece = get_piece_at(pos, from);
	const PieceType piece_type = get_piece_type(piece);

	if (move_is_capture(move)) {
		const int tmp = mvv_lva(move, pos);
		score.mg += tmp;
		score.eg += tmp;
	}

	struct score pawn_score = piece_functions[piece_type](move, pos);
	score.mg += pawn_score.mg;
	score.eg += pawn_score.eg;

	return ((score.mg * (FINAL_PHASE - phase)) +
		score.eg * (phase - INITIAL_PHASE)) /
	       FINAL_PHASE;
}

static struct score evaluate_king_move(Move move, const Position *pos)
{
	const Color side = get_side_to_move(pos);
	const Piece king = create_piece(PIECE_TYPE_KING, side);
	const Square from = get_move_origin(move);
	const Square to = get_move_target(move);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	score.mg += get_square_value(king, to, true) -
		    get_square_value(king, from, true);
	score.eg += get_square_value(king, to, false) -
		    get_square_value(king, from, false);

	return score;
}

static struct score evaluate_queen_move(Move move, const Position *pos)
{
	const Color side = get_side_to_move(pos);
	const Piece queen = create_piece(PIECE_TYPE_QUEEN, side);
	const Square from = get_move_origin(move);
	const Square to = get_move_target(move);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	score.mg += get_square_value(queen, to, true) -
		    get_square_value(queen, from, true);
	score.eg += get_square_value(queen, to, false) -
		    get_square_value(queen, from, false);

	return score;
}

static struct score evaluate_rook_move(Move move, const Position *pos)
{
	const Color side = get_side_to_move(pos);
	const Piece rook = create_piece(PIECE_TYPE_ROOK, side);
	const Square from = get_move_origin(move);
	const Square to = get_move_target(move);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	score.mg += get_square_value(rook, to, true) -
		    get_square_value(rook, from, true);
	score.eg += get_square_value(rook, to, false) -
		    get_square_value(rook, from, false);

	return score;
}

static struct score evaluate_bishop_move(Move move, const Position *pos)
{
	const Color side = get_side_to_move(pos);
	const Piece bishop = create_piece(PIECE_TYPE_BISHOP, side);
	const Square from = get_move_origin(move);
	const Square to = get_move_target(move);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	score.mg += get_square_value(bishop, to, true) -
		    get_square_value(bishop, from, true);
	score.eg += get_square_value(bishop, to, false) -
		    get_square_value(bishop, from, false);

	if (is_outpost(pos, to, side)) {
		score.mg += 26;
		score.eg += 14;
	}
	if (is_outpost(pos, from, side)) {
		score.mg -= 26;
		score.eg -= 14;
	}

	return score;
}

static struct score evaluate_knight_move(Move move, const Position *pos)
{
	const Color side = get_side_to_move(pos);
	const Piece knight = create_piece(PIECE_TYPE_KNIGHT, side);
	const Square from = get_move_origin(move);
	const Square to = get_move_target(move);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	score.mg += get_square_value(knight, to, true) -
		    get_square_value(knight, from, true);
	score.eg += get_square_value(knight, to, false) -
		    get_square_value(knight, from, false);

	if (is_outpost(pos, to, side)) {
		score.mg += 30;
		score.eg += 18;
	}
	if (is_outpost(pos, from, side)) {
		score.mg -= 30;
		score.eg -= 18;
	}

	return score;
}

static struct score evaluate_pawn_move(Move move, const Position *pos)
{
	const Color side = get_side_to_move(pos);
	const Piece pawn = create_piece(PIECE_TYPE_PAWN, side);
	const Square from = get_move_origin(move);
	const Square to = get_move_target(move);

	struct score score;
	score.mg = 0;
	score.eg = 0;

	score.mg += get_square_value(pawn, to, true) -
		    get_square_value(pawn, from, true);
	score.eg += get_square_value(pawn, to, false) -
		    get_square_value(pawn, from, false);

	if (move_is_promotion(move)) {
		score.mg += point_value[PIECE_TYPE_QUEEN] -
			    point_value[PIECE_TYPE_PAWN];
		/* Promotions are more promising in the endgame. */
		score.eg += point_value[PIECE_TYPE_QUEEN];
	}

	if (get_number_of_enemy_pawn_stoppers(pos, from, side) == 0) {
		/* Bonus for moving a passed pawn. */
		score.mg += 4;
		score.eg += 7;
	} else {
		if (get_number_of_enemy_pawn_stoppers(pos, to, side) == 0) {
			/* Bonus for creating a passed pawn. */
			score.mg += 10;
			score.eg += 22;
		}
	}

	if (move_is_capture(move)) {
		if (!move_is_promotion(move)) {
			/* Penalty for doubling the pawn. */
			if (get_number_of_friendly_pawn_blockers(pos, to,
								 side)) {
				score.mg -= 8;
				score.eg -= 12;
			}
			/* Penalty for isolating the pawn. */
			if (get_number_of_adjacent_friendly_pawns(pos, to,
								  side) == 0) {
				score.mg -= 5;
				score.eg -= 15;
			}
		}
	}

	return score;
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

/*
 * An outpost is a square that cannot be easily attacked by an enemy pawn. The
 * conditions are the following:
 *
 * (a) The square must be on rank 4, 5 or 6 for white and 5, 4, 3 for black.
 * (b) The square must not be currently attacked by an enemy pawn;
 * (c) If there is a pawn on the adjacent files that can push and attack the
 *     square we need a blocker that can stop it.
 *
 * TODO: make the conditions stronger when we start using attack and defend
 * maps. For example, condition (c) can be improved by allowing such an enemy
 * pawn not only when there is a blocker but also when there is a pawn
 * defending.
 */
static bool is_outpost(const Position *pos, Square sq, Color side)
{
	const Rank rank = get_rank(sq);
	if (side == COLOR_WHITE) {
		if (rank < RANK_4 || rank > RANK_6)
			return false;
	} else {
		if (rank > RANK_5 || rank < RANK_3)
			return false;
	}

	const Piece friendly_pawn = create_piece(PIECE_TYPE_PAWN, side);
	const u64 friendly_pawns_bb = get_piece_bitboard(pos, friendly_pawn);
	const Piece enemy_pawn = create_piece(PIECE_TYPE_PAWN, !side);
	const u64 enemy_pawns_bb = get_piece_bitboard(pos, enemy_pawn);

	const u64 front_mask = fill_front_of_square(sq, side);
	const u64 adjacent_front_mask1 = shift_bb_east(front_mask, 1);
	const u64 adjacent_front_mask2 = shift_bb_west(front_mask, 1);
	const u64 threats_bb[2] = { enemy_pawns_bb & adjacent_front_mask1,
				    enemy_pawns_bb & adjacent_front_mask2 };
	for (int i = 0; i < 2; ++i) {
		/* If there are no enemy pawns on the adjacent files, in front
		 * of the square. */
		if (!threats_bb[i])
			continue;
		/* This is the square of the closest pawn. There are no other
		 * pawns in front of it. */
		const Square threat_sq =
			side == COLOR_WHITE ? (Square)get_ls1b(threats_bb[i]) :
					      (Square)get_ms1b(threats_bb[i]);
		const u64 threat_sq_front_mask =
			fill_front_of_square(threat_sq, !side);
		/* The blocker has to be between the threat and the diagonal
		 * square. This implicitly tests if the square is attacked since
		 * if the threatening pawn is on an attacking square the blocker
		 * mask will be 0 because threat_sq_front_mask only covers
		 * squares in front of our square. */
		const u64 blocker_mask = adjacent_front_mask1 &
					 threat_sq_front_mask;
		/* If there are no pawns to stop the threat. */
		if (!(friendly_pawns_bb & blocker_mask))
			return false;
	}

	return true;
}

/*
 * Returns the number of pawns on files adjacent to the file of sq.
 */
static int get_number_of_adjacent_friendly_pawns(const Position *pos, Square sq,
						 Color side)
{
	const u64 file_bb = get_file_bitboard(get_file(sq));
	const Piece pawn = create_piece(PIECE_TYPE_PAWN, side);
	const u64 friendly_pawns_bb = get_piece_bitboard(pos, pawn);
	const u64 adjacent_files_bb = shift_bb_east(file_bb, 1) |
				      shift_bb_west(file_bb, 1);
	return popcnt(friendly_pawns_bb & adjacent_files_bb);
}

/*
 * Returns the number of enemy pawns that can stop a pawn on square sq. This
 * function is used to detect passed pawns.
 */
static int get_number_of_enemy_pawn_stoppers(const Position *pos, Square sq,
					     Color side)
{
	const Piece enemy_pawn = create_piece(PIECE_TYPE_PAWN, !side);
	const u64 enemy_pawns_bb = get_piece_bitboard(pos, enemy_pawn);
	const u64 front_mask = fill_front_of_square(sq, side);
	const u64 mask = front_mask | shift_bb_east(front_mask, 1) |
			 shift_bb_west(front_mask, 1);
	return popcnt(enemy_pawns_bb & mask);
}

/*
 * Returns the number of friendly pawns in front of a pawn on square sq. This
 * function is used to detect doubled pawns.
 */
static int get_number_of_friendly_pawn_blockers(const Position *pos, Square sq,
						Color side)
{
	const Piece pawn = create_piece(PIECE_TYPE_PAWN, side);
	const u64 friendly_pawns_bb = get_piece_bitboard(pos, pawn);
	return popcnt(friendly_pawns_bb & fill_front_of_square(sq, side));
}

/*
 * Returns a bitboard of all the squares in front of square (from the point of
 * view of side.)
 *
 * In order to to do this we need to shift the file bitboard, removing all the
 * squares from the ranks behind and including the square's rank. When it in
 * white, the shift should remove all the bits starting from the square on the
 * greatest file of the rank down to square 0 (A1). When it is black, the shift
 * should remove all the bits starting from the square on the smallest file of
 * the rank up to square 63 (H8). So we shift right by the greatest square plus
 * one, and left by 63 minus the smallest square plus one.
 *
 * Finding the greatest square plus one on a rank is equivalent to finding the
 * multiple of 8 greater than or equal to any square on the rank. Finding the
 * smallest square is equivalent to finding the multiple of 8 less than or equal
 * to any square on the rank.
 */
static u64 fill_front_of_square(Square sq, Color side)
{
	const u64 file_bb = get_file_bitboard(get_file(sq));
	const int shift =
		side == COLOR_WHITE ?
			round_up_to_multiple_of_eight((int)sq) :
			63 - round_down_to_multiple_of_eight((int)sq) + 1;
	const u64 bb = side == COLOR_WHITE ? (file_bb >> shift) << shift :
					     (file_bb << shift) >> shift;
	return bb;
}

static int round_up_to_multiple_of_eight(int n)
{
	return (((int)n - 1) | 7) + 1;
}

static int round_down_to_multiple_of_eight(int n)
{
	return ((int)n & -8);
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

struct outpost_data {
	const char *fen;
	Square sq;
	bool expected_result;
};

static void test_is_outpost(void)
{
	/* clang-format off */
	const struct outpost_data data[] = {
		{"r1b1k1nr/pp2ppbp/2n3p1/2p5/2P5/2N1PNP1/PP3PBP/R1BR2K1 b kq - 2 9", G7, false},
		{"B3k1nr/p3pp1p/1p4p1/2p5/b1n5/2P1P1P1/P4P1P/R1B1R1K1 b k - 3 15", D4, false},
		{"r1b1k1nr/1p1pppbp/p1n3p1/3Nq3/2BNP3/4B3/PPP2PPP/R2QK2R w KQkq - 6 9", F6, false},
		{"1rb1k1nr/1p2ppbp/pNnp2p1/8/2B1P3/2P1BN2/PP3PPP/3RK2R w Kk - 0 13", C4, false},
		{"r1bq1rk1/ppp2ppp/2n1pn2/3p4/Q1PP4/P1P2N2/4PPPP/R1B1KB1R w KQ - 1 8", G5, false},
		{"r1bq1rk1/ppp2p2/2n1p2p/3p2p1/Q1PPn3/P1P2NB1/4PPPP/R3KB1R w KQ - 2 11", G3, false},
		{"r2qr1k1/p4pb1/2p1b1p1/2Bnp2p/2B1N2P/5P2/PPPQ2P1/2KR3R b - - 1 16", F4, false},
		{"3rr1k1/B5b1/2p1n3/4pp1p/4N2P/5P2/PPP5/2KR3R w - - 0 22", G3, false},
		{"rn1q1rk1/pb2bppp/1p3n2/2pp4/3P4/BPNBPN2/P4PPP/R2Q1RK1 b - - 1 10", E4, false},
		{"2rq2k1/pb1nrpbp/1p1N1np1/3p4/3P4/5NPB/PB3P1P/2RQ1RK1 b - - 4 17", E5, false},

		{"r3r1k1/1p1qbppp/p2p1n2/4pPB1/4P3/2NQ4/PPP3PP/R4RK1 w - - 2 16", D5, true},
		{"3r4/p4pkp/4p1p1/3n4/1pRP4/1P6/P3BPPP/6K1 b - - 1 30", C3, true},
		{"3q1r1k/p1p3pp/1p1p4/4p2n/4P3/PBP2P2/1P1Q1P1P/4R2K b - - 2 3", F4, true},
		{"2r1k2r/1q1b1pp1/p3p2p/np2P3/4N1P1/P1P1Q2P/2PB1P2/R3R1K1 b k - 2 2", C4, true},
		{"2r1brk1/pp2n1pp/4p2q/2PpNp2/1P1Pn3/P1NB2P1/5P1P/2RQR1K1 w - - 0 1", E5, true},
		{"r2q2k1/p1p2p2/1pn4p/3p4/3Pr1pP/P1P1P1B1/2Q2PP1/R4K1R w - - 0 18", F4, true},
		{"1r1qr1k1/p4pb1/2p1b1p1/2Bnp2p/2B1N1PP/5P2/PPPQ4/2KR3R b - - 0 17", F4, true},
	};
	/* clang-format on */

	for (size_t i = 0; i < sizeof(data) / sizeof(data[i]); ++i) {
		Position pos;
		init_position(&pos, data[i].fen);
		const bool result =
			is_outpost(&pos, data[i].sq, get_side_to_move(&pos));
		TEST_ASSERT_MESSAGE(result == data[i].expected_result,
				    data[i].fen);
	}
}

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
		Position pos;
		init_position(&pos, data[i].fen);
		bool s;
		const Move move = lan_to_move(data[i].move, &pos, &s);
		if (!s)
			abort();
		const bool result = wins_exchange(move, 0, &pos);
		TEST_ASSERT_MESSAGE(result == data[i].expected_result,
				    data[i].fen);
	}
}

int main(void)
{
	UNITY_BEGIN();

	RUN_TEST(test_wins_exchange);
	RUN_TEST(test_is_outpost);

	return UNITY_END();
}
#endif
