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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>

#include <bit.h>
#include <pos.h>
#include <move.h>
#include <movegen.h>
#include <eval.h>
#include <search.h>

#define INF 32000
#define MAX_DEPTH 256
#define MAX_PLY MAX_DEPTH

/*
 * The search keeps track of information per ply in the search tree. As a
 * micro-optimization we put the ply in the stack to avoid incrementing each on
 * every recursive call.
 */
struct stack_element {
	int ply;
};

/*
 * This is the state of the search. The value running points to may be modified
 * by the caller to signal that the search should stop.
 *
 * completed_depth is the maximum depth for which we were able to search so far
 * in iterative deepening.
 */
struct state {
	Position *pos;
	Move best_move;
	int completed_depth;
	long long nodes;
	atomic_bool *running;
};

/*
 * All searches need a depth, for infinite searches we just set the depth to a
 * very high number. It is a mate search only if mate > 0. And if there is no
 * time limit limited_time is set to false.
 */
struct limits {
	int depth;
	int mate;
	struct timespec stop_time;
	bool limited_time;
};

static int negamax(struct state *state, struct stack_element *stack,
		   struct limits *limits, int depth);
static bool time_is_up(const struct timespec *stop_time);
static bool is_in_check(const Position *pos);
static void add_time(struct timespec *ts, long long time);
static long long compute_search_time(const Position *pos, long long time,
				     int movestogo);

void *search(void *search_arg)
{
	struct search_argument *arg = (struct search_argument *)search_arg;

	struct stack_element stack[MAX_PLY + 1];
	for (int i = 0; i < (int)(sizeof(stack) / sizeof(stack[0])); ++i)
		stack[i].ply = i;

	struct state state;
	state.pos = ((struct search_argument *)arg)->pos;
	state.best_move = 0;
	state.running = ((struct search_argument *)arg)->running;
	*state.running = true;

	Color c = get_side_to_move(state.pos);

	struct limits limits;
	limits.depth = arg->depth < MAX_DEPTH ? arg->depth : MAX_DEPTH;
	limits.mate = arg->mate;
	if (arg->time[c]) {
		limits.limited_time = true;
		timespec_get(&limits.stop_time, TIME_UTC);
		long long time = compute_search_time(state.pos, arg->time[c],
						     arg->movestogo);
		add_time(&limits.stop_time, time);
	} else {
		limits.limited_time = false;
	}

	Move best_move = 0;
	for (int depth = 1; depth <= limits.depth; ++depth) {
		negamax(&state, stack, &limits, depth);
		if (!*state.running) {
			/* If the search stops in the first iteration we use
			 * its best move anyway since we have no choice. */
			if (depth == 1)
				best_move = state.best_move;
			break;
		}
		best_move = state.best_move;
	}

	/* Here state.best_move will always be a valid move because the negamax
	 * function ensures that we search at least depth 1. */
	((struct search_argument *)arg)->best_move_sender(best_move);
	return NULL;
}

static int negamax(struct state *state, struct stack_element *stack,
		   struct limits *limits, int depth)
{
	/* Only check time each 8192 nodes to avoid making system calls which
	 * slows down the search. */
	if (state->nodes % 8192 && limits->limited_time)
		*state->running = !time_is_up(&limits->stop_time);
	/* Only stop when it is not the root node, this ensures we have at least
	 * the first PV move (which is the best move that will be sent). We
	 * return a draw score because we don't know the actual evaluation. */
	if (stack->ply && !*state->running)
		return 0;

	Position *pos = state->pos;

	if (!depth)
		return evaluate(pos);

	++state->nodes;

	int best_score = -INF;
	Move best_move = 0;

	Move moves[256];
	int moves_nb = get_pseudo_legal_moves(moves, pos);
	int j = 0;
	for (int i = 0; i < moves_nb; ++i) {
		if (move_is_legal(pos, moves[i])) {
			moves[j] = moves[i];
			++j;
		}
	}
	moves_nb = j;
	if (!moves_nb)
		return is_in_check(pos) ? -INF : 0;

	for (int i = 0; i < moves_nb; ++i) {
		do_move(pos, moves[i]);
		const int score = -negamax(state, stack + 1, limits, depth - 1);
		undo_move(pos, moves[i]);

		/* We also need to quit the search here because deeper nodes
		 * will return score 0 and if we don't do the same we might
		 * end up returning the score of deeper nodes even though we
		 * really don't know if that is the best score since the last
		 * search probably didn't have time to finish. So in this case
		 * we just return without updating the PV. */
		if (stack->ply && !*state->running)
			return 0;

		if (score > best_score) {
			best_score = score;
			best_move = moves[i];
		}
	}

	/* Update best move from the root. */
	if (!stack->ply)
		state->best_move = best_move;

	return best_score;
}

static bool is_in_check(const Position *pos)
{
	const Color c = get_side_to_move(pos);
	const Square king_sq = get_king_square(pos, c);
	return is_square_attacked(king_sq, !c, pos);
}

static bool time_is_up(const struct timespec *stop_time)
{
	struct timespec now;
	timespec_get(&now, TIME_UTC);
	if (now.tv_sec > stop_time->tv_sec ||
	    (now.tv_sec == stop_time->tv_sec &&
	     now.tv_nsec > stop_time->tv_nsec)) {
		return true;
	}
	return false;
}

/*
 * Adds time in milliseconds to ts.
 */
static void add_time(struct timespec *ts, long long time)
{
	time_t sec = (time_t)floor((double)time / 1e3);
	long nsec = (long)floor(((double)time / 1e3 - (double)sec) * 1e9);
	ts->tv_sec += sec;
	ts->tv_nsec += nsec;
	if (ts->tv_nsec >= 1000000000L) {
		++ts->tv_sec;
		ts->tv_nsec %= 1000000000L;
	}
}

/*
 * Receives a position and the total time left in milliseconds and returns the
 * amount of time the search can use, also in milliseconds.
 * 
 * We need to divide the time we have available among the moves that will be
 * played throughout the game, but the number of future moves depends on how
 * many moves have already been played. We can construct a function that predicts
 * the number of moves that will still be played based on the game pahse by using
 * a linear interpolation between the average number of moves for a full chess
 * game and a safe minimum number of moves we always want to have available,
 * then we can divide the time we have left by the interpolation value at the
 * current phase to obtain the time we should spend on searching the next move.
 *
 * If movestogo is set and is less than the average number of moves, then we 
 * use it instead. In particular, if movestogo is 1 then we can use all the
 * remaining time since the next move will start the next time control. Note,
 * however, that it is not safe to actually use all the remaining time since the
 * engine may take longer to finish calculating, therefore we can only use a
 * portion of it.
 * 
 * It is clear that the more time we have the more time it is safe to use. For
 * example, if we only have 1 second left using 99% of it is not safe since the
 * engine would only have a buffer of less than 1 millisecond between detecting
 * time is up and sending the move. The following mathematical function maps a
 * time in milliseconds to a value between 0 and 1.
 * 
 * f(x) = (x / 1000)^1.1 / (x / 1000 + 1)^1.1
 */
static long long compute_search_time(const Position *pos, long long time,
				     int movestogo)
{
	const int average_game_length = 40;

	if (movestogo == 1) {
		double factor = pow((double)time / 1000., 1.1);
		factor /= pow((double)time / 1000. + 1., 1.1);
		return (long long)((double)time * factor);
	}
	const int phase = get_phase(pos);
	const double max = movestogo && movestogo < average_game_length ?
				   movestogo :
				   average_game_length;
	const double divisor = (max * (256 - phase) + 8 * phase) / 256;
	const double search_time = (double)time / divisor;
	return (long long)search_time;
}
