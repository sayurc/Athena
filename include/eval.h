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
	MOVE_PICKER_STAGE_ALL,
};

struct move_with_score {
	Move move;
	i16 score;
};

/*
 * This struct stores data about the moves so we don't have to recompute the
 * scores every time we need to pick a new move.
 */
struct move_picker_context {
	int moves_nb;
	/* Number of moves whose score has been computed. */
	int scored_nb;
	/* The move from the transposition table. It should be 0 if there is no
	 * move. */
	Move tt_move;
	/* This stage is MOVE_PICKER_STAGE_TT only if tt_move is different from
	 * 0 */
	enum move_picker_stage stage;
	int index;
	struct move_with_score moves[256];
};

Move pick_next_move(struct move_picker_context *ctx);
void init_move_picker_context(struct move_picker_context *ctx, Move tt_move,
			      const Move *moves, int moves_nb,
			      const Position *pos);
int evaluate(const Position *pos);

#endif
