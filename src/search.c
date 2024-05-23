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
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>

#include <pthread.h>

#include <bit.h>
#include <str.h>
#include <pos.h>
#include <move.h>
#include <movegen.h>
#include <eval.h>
#include <tt.h>
#include <search.h>

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

#ifdef SEARCH_STATISTICS
struct iteration_statistics {
	long long nodes;
	long long quiescence_nodes;
};
#endif

/*
 * This is the state of the search. The value running points to may be modified
 * by the caller to signal that the search should stop.
 */
struct state {
	Position pos;
	Move best_move;
	int completed_depth;
	long long nodes; /* All nodes, including quiescence nodes. */
#ifdef SEARCH_STATISTICS
	long long quiescence_nodes;
	FILE *log_file;
#endif
	struct timespec start_time;
	atomic_bool *stop;
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
		   struct limits *limits, int alpha, int beta, int depth);
static int qsearch(struct state *state, struct stack_element *stack,
		   struct limits *limits, int alpha, int beta, int depth);
static int tt_score_to_score(int score, int ply);
static int score_to_tt_score(int score, int ply);
static void init_stack(struct stack_element *stack, int capacity);
static void init_limits(struct limits *limits,
			const struct search_argument *arg);
static void init_state(struct state *state, const struct search_argument *arg);
static long long compute_nps(const struct timespec *t1,
			     const struct timespec *t2, long long nodes);
static long long timespec_to_milliseconds(const struct timespec *ts);
static struct timespec compute_elapsed_time(const struct timespec *t1,
					    const struct timespec *t2);
static bool time_is_up(const struct timespec *stop_time);
static bool is_in_check(const Position *pos);
static void add_time(struct timespec *ts, long long time);
static long long compute_search_time(const Position *pos, long long time,
				     int movestogo);
#ifdef SEARCH_STATISTICS
static void log_position(FILE *fp, const Position *pos);
static void log_iteration_statistics(FILE *fp, int depth,
				     const struct iteration_statistics *stats);
#endif

void *search(void *search_arg)
{
	struct search_argument *arg = (struct search_argument *)search_arg;

	struct stack_element stack[MAX_PLY + 1];
	init_stack(stack, sizeof(stack) / sizeof(stack[0]));

	struct state state;
	init_state(&state, arg);

	struct limits limits;
	init_limits(&limits, arg);

#ifdef SEARCH_STATISTICS
	log_position(state.log_file, state.pos);
#endif

	Move best_move = 0;
	for (int depth = 1; depth <= limits.depth; ++depth) {
		struct timespec t1;
		timespec_get(&t1, TIME_UTC);

		const long long old_nodes = state.nodes;
#ifdef SEARCH_STATISTICS
		const long long old_qnodes = state.quiescence_nodes;
#endif

		const int score =
			negamax(&state, stack, &limits, -INF, INF, depth);
		if (*state.stop) {
			/* If the search stops in the first iteration we use
			 * its best move anyway since we have no choice. */
			if (depth == 1)
				best_move = state.best_move;
			break;
		}

		struct timespec t2;
		timespec_get(&t2, TIME_UTC);

#ifdef SEARCH_STATISTICS
		struct iteration_statistics stats;
		stats.nodes = state.nodes - old_nodes;
		stats.quiescence_nodes = state.quiescence_nodes - old_qnodes;
		log_iteration_statistics(state.log_file, depth, &stats);
#endif

		long long nps = compute_nps(&t1, &t2, state.nodes - old_nodes);
		struct timespec time_since_start =
			compute_elapsed_time(&state.start_time, &t2);

		struct info info;
		info.flags = INFO_FLAG_DEPTH;
		info.flags |= INFO_FLAG_NODES;
		info.flags |= INFO_FLAG_NPS;
		info.flags |= INFO_FLAG_TIME;
		info.depth = depth;
		info.nodes = state.nodes;
		info.nps = nps;
		info.time = timespec_to_milliseconds(&time_since_start);
		/* When the score is a mate score we use the mate flag instead
		 * of the cp flag and extract the moves to mate from the score.
		 */
		if (score >= INF - MAX_PLY) {
			info.flags |= INFO_FLAG_MATE;
			info.mate = (INF - score + 1) / 2;
		} else if (score <= -INF + MAX_PLY) {
			info.flags |= INFO_FLAG_MATE;
			info.mate = -(INF + score + 1) / 2;
		} else {
			info.flags |= INFO_FLAG_CP;
			info.cp = score;
		}
		arg->info_sender(&info);

		best_move = state.best_move;
	}

	/* Here state.best_move will always be a valid move because the negamax
	 * function ensures that we search at least depth 1. */
	arg->best_move_sender(best_move);
	*state.stop = true;
	pthread_exit(NULL);
}

static int negamax(struct state *state, struct stack_element *stack,
		   struct limits *limits, int alpha, int beta, int depth)
{
	/* Only check time each 8192 nodes to avoid making system calls which
	 * slows down the search. */
	if (!(state->nodes % 8192) && limits->limited_time)
		*state->stop = time_is_up(&limits->stop_time);
	/* Only stop when it is not the root node, this ensures we have a best
	 * move to send. */
	if (stack->ply && *state->stop)
		return 0;

	/* Fall into the quiescence search when we reach the bottom. */
	if (!depth)
		return qsearch(state, stack, limits, alpha, beta, depth);

	Position *pos = &state->pos;

	/* We don't count the start position. */
	if (stack->ply)
		++state->nodes;

	/* TT lookup */
	bool found_tt_entry = false;
	NodeData tt_data;
	found_tt_entry = get_tt_entry(&tt_data, pos);
	if (stack->ply && found_tt_entry && tt_data.depth >= depth) {
		const int score = tt_score_to_score(tt_data.score, stack->ply);
		switch (tt_data.bound) {
		case BOUND_EXACT:
			return score;
		case BOUND_LOWER:
			/* If this score is a lower bound and it is greater than
			 * or equal to beta then we are guaranteed a fail-high
			 * in this node, so we prune this branch in advance. */
			if (score >= beta)
				return score;
			break;
		case BOUND_UPPER:
			/* Iff the score is an upper bound and less than or
			 * equal to alpha then we are guaranteed a fail-low and
			 * we can just return this upper bound. */
			if (score <= alpha)
				return score;
			break;
		default:
			abort();
		}
	}

	Move best_move = 0;

	Bound bound = BOUND_UPPER;
	int best_score = -INF;
	int moves_cnt = 0;

	const Move tt_move = found_tt_entry ? tt_data.best_move : 0;
	struct move_picker_context mp_ctx;
	init_move_picker_context(&mp_ctx, tt_move);
	for (Move move = pick_next_move(&mp_ctx, pos); move;
	     move = pick_next_move(&mp_ctx, pos)) {
		if (!move_is_legal(pos, move))
			continue;
		++moves_cnt;

		do_move(pos, move);
		const int score = -negamax(state, stack + 1, limits, -beta,
					   -alpha, depth - 1);
		undo_move(pos, move);

		/* We also need to quit the search here because deeper nodes
		 * will return score 0 and if we don't do the same we might
		 * end up returning the score of other deeper nodes even though
		 * we really don't know if that is the best score since the last
		 * search probably didn't have time to finish. So in this case
		 * we just return without updating the PV. */
		if (stack->ply && *state->stop)
			return 0;

		if (score > best_score) {
			best_score = score;
			if (score > alpha) {
				best_move = move;
				if (score >= beta) {
					bound = BOUND_LOWER;
					break;
				}
				bound = BOUND_EXACT;
				alpha = score;
			}
		}
	}

	if (!moves_cnt)
		best_score = is_in_check(pos) ? -INF + stack->ply : 0;

	const int tt_score = score_to_tt_score(best_score, stack->ply);
	init_tt_entry(&tt_data, tt_score, depth, bound, best_move, pos);
	/* Add this node to the TT when it's not the root node since there is
	 * no point in saving the root node. */
	if (stack->ply)
		store_tt_entry(&tt_data);

	/* Update best move from the root. */
	if (!stack->ply)
		state->best_move = best_move;

	return best_score;
}

/*
 * The quiescence search searches all the captures until there are no more
 * captures.
 */
static int qsearch(struct state *state, struct stack_element *stack,
		   struct limits *limits, int alpha, int beta, int depth)
{
	/* Only check time each 8192 nodes to avoid making system calls which
	 * slows down the search. */
	if (!(state->nodes % 8192) && limits->limited_time)
		*state->stop = time_is_up(&limits->stop_time);
	if (*state->stop)
		return 0;

	Position *pos = &state->pos;

	++state->nodes;
#ifdef SEARCH_STATISTICS
	++state->quiescence_nodes;
#endif

	bool found_tt_entry = false;
	NodeData tt_data;
	found_tt_entry = get_tt_entry(&tt_data, pos);
	if (stack->ply && found_tt_entry && tt_data.depth >= depth) {
		const int score = tt_score_to_score(tt_data.score, stack->ply);
		switch (tt_data.bound) {
		case BOUND_EXACT:
			return score;
		case BOUND_LOWER:
			/* If this score is a lower bound and it is greater than
			 * or equal to beta then we are guaranteed a fail-high
			 * in this node, so we prune this branch in advance. */
			if (score >= beta)
				return score;
			break;
		case BOUND_UPPER:
			/* If the score is an upper bound and less than or equal
			 * to alpha then we are guaranteed a fail-low and we can
			 * just return this upper bound. */
			if (score <= alpha)
				return score;
			break;
		default:
			abort();
		}
	}

	int best_score = evaluate(pos);
	if (!is_in_check(pos) && best_score >= beta)
		return best_score;
	if (best_score > alpha)
		alpha = best_score;

	Bound bound = BOUND_UPPER;
	Move best_move = 0;

	const Move tt_move = found_tt_entry ? tt_data.best_move : 0;
	struct move_picker_context mp_ctx;
	init_move_picker_context(&mp_ctx, tt_move);
	for (Move move = pick_next_move(&mp_ctx, pos); move;
	     move = pick_next_move(&mp_ctx, pos)) {
		/* We test if the move is a capture before testing if it is
		 * legal to avoid testing illegal capturing moves for legality
		 * since the legality test is way more expensive. */
		if (!move_is_capture(move))
			continue;
		if (!move_is_legal(pos, move))
			continue;

		do_move(pos, move);
		const int score = -qsearch(state, stack + 1, limits, -beta,
					   -alpha, depth);
		undo_move(pos, move);

		if (stack->ply && *state->stop)
			return 0;

		if (score > best_score) {
			best_score = score;
			if (score > alpha) {
				best_move = move;
				if (score >= beta) {
					bound = BOUND_LOWER;
					break;
				}
				bound = BOUND_EXACT;
				alpha = score;
			}
		}
	}

	const int tt_score = score_to_tt_score(best_score, stack->ply);
	init_tt_entry(&tt_data, tt_score, depth, bound, best_move, pos);
	if (stack->ply)
		store_tt_entry(&tt_data);

	return best_score;
}

/*
 * This function does the inverse of score_to_tt_score. In the latter we removed
 * the distance from the root to the node that stored the entry, here we add the
 * distance of the node retrieving the entry (which may be a different
 * distance.)
 */
static int tt_score_to_score(int score, int ply)
{
	if (score >= INF - MAX_PLY)
		return score - ply;
	else if (score <= -(INF - MAX_PLY))
		return score + ply;
	else
		return score;
}

/*
 * We can't store mate scores directly in the TT because the same position can
 * be found in different plies, which means that if we use the mate score in a
 * transposition with larger ply than the node that stored the entry we will get
 * a larger score even though the mate takes longer. This would make the engine
 * choose longer mates and if it keeps choosing longer mates it might never
 * deliver mate.
 *
 * This function adjusts the mate score for the TT and if the score is not mate
 * it is left unchanged. Instead of storing the mate score based on the distance
 * from the root node, which can vary depending on the variation, we store the
 * distance from the node that is storing the entry. So whenever we find the
 * same node again in a different line we can add/subtract the new ply to/from
 * the TT score and we will get a score based on the ply of the new variation.
 */
static int score_to_tt_score(int score, int ply)
{
	if (score >= INF - MAX_PLY)
		return score + ply;
	else if (score <= -(INF - MAX_PLY))
		return score - ply;
	else
		return score;
}

static void init_stack(struct stack_element *stack, int capacity)
{
	for (int i = 0; i < capacity; ++i)
		stack[i].ply = i;
}

static void init_limits(struct limits *limits,
			const struct search_argument *arg)
{
	Color c = get_side_to_move(&arg->pos);

	limits->depth = arg->depth < MAX_DEPTH ? arg->depth : MAX_DEPTH;
	limits->mate = arg->mate;
	if (arg->time[c]) {
		limits->limited_time = true;
		timespec_get(&limits->stop_time, TIME_UTC);
		long long time = compute_search_time(&arg->pos, arg->time[c],
						     arg->movestogo);
		add_time(&limits->stop_time, time);
	} else {
		limits->limited_time = false;
	}
}

static void init_state(struct state *state, const struct search_argument *arg)
{
	copy_position(&state->pos, &arg->pos);
	state->best_move = 0;
	state->completed_depth = 0;
	state->nodes = 0;
#ifdef SEARCH_STATISTICS
	state->quiescence_nodes = 0;
	state->log_file = ((struct search_argument *)arg)->log_file;
#endif
	timespec_get(&state->start_time, TIME_UTC);
	state->stop = ((struct search_argument *)arg)->stop;
}

static bool is_in_check(const Position *pos)
{
	const Color c = get_side_to_move(pos);
	const Square king_sq = get_king_square(pos, c);
	return is_square_attacked(king_sq, !c, pos);
}

static long long compute_nps(const struct timespec *t1,
			     const struct timespec *t2, long long nodes)
{
	const struct timespec et = compute_elapsed_time(t1, t2);
	return nodes * 1000 / timespec_to_milliseconds(&et);
}

static long long timespec_to_milliseconds(const struct timespec *ts)
{
	const long long t = ts->tv_sec * 1000 + ts->tv_nsec / 1000000L;
	/* Round up. */
	if (!t)
		return 1;
	return t;
}

/*
 * Computes the difference t2 - t1.
 */
static struct timespec compute_elapsed_time(const struct timespec *t1,
					    const struct timespec *t2)
{
	time_t sec = t2->tv_sec - t1->tv_sec;
	long nsec = t2->tv_nsec - t1->tv_nsec;
	if (nsec < 0) {
		--sec;
		nsec += 1000000000L;
	}

	const struct timespec diff = { .tv_sec = sec, .tv_nsec = nsec };
	return diff;
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

#ifdef SEARCH_STATISTICS
static void log_position(FILE *fp, const Position *pos)
{
	char fen[512];
	get_fen(fen, pos);
	fprintf(fp, "Position %s\n", fen);
}

static void log_iteration_statistics(FILE *fp, int depth,
				     const struct iteration_statistics *stats)
{
	fprintf(fp, "Depth %d\n", depth);
	fprintf(fp, "Total nodes: %lld\nQuiescence nodes: %lld\n", stats->nodes,
		stats->quiescence_nodes);
	fflush(fp);
}
#endif
