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

#ifndef EVAL_H
#define EVAL_H

#define INF 32000

/*
 * TODO: replace MOVE_PICKER_STAGE_ALL with more stages such as captures.
 */
enum move_picker_stage {
	/* Transposition table stage: return the TT move. */
	MOVE_PICKER_STAGE_TT,
	MOVE_PICKER_STAGE_CAPTURE_INIT,
	MOVE_PICKER_STAGE_GOOD_CAPTURE,
	MOVE_PICKER_STAGE_REFUTATION,
	MOVE_PICKER_STAGE_QUIET_INIT,
	MOVE_PICKER_STAGE_QUIET,
	MOVE_PICKER_STAGE_BAD_CAPTURE,
};

/*
 * This struct stores data about the moves so we don't have to recompute the
 * scores every time we need to pick a new move.
 */
struct move_picker_context {
	bool skip_quiets;
	int captures_end;
	int quiets_end;
	int bad_captures_end;
	/* The move from the transposition table. It should be 0 if there is no
	 * move. */
	Move tt_move;
	/* This stage is MOVE_PICKER_STAGE_TT only if tt_move is different from
	 * 0 */
	enum move_picker_stage stage;
	int index;
	int refutation_index;
	struct move_with_score moves[256];
	Move refutations[2];
	int refutations_end;
	const int (*butterfly_history)[64][64];
};

Move pick_next_move(struct move_picker_context *ctx, Position *pos);
void init_move_picker_context(struct move_picker_context *ctx, Move tt_move,
			      const Move *refutations, int refutations_nb,
			      const int (*butterfly_history)[64][64],
			      bool skip_quiets);
int evaluate(const Position *pos);
bool wins_exchange(Move move, int threshold, const Position *pos);
#ifdef TEST
void test_eval(void);
#endif

#endif
