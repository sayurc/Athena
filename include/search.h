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

#ifndef SEARCH_H
#define SEARCH_H

/*
 * The flags INFO_FLAG_MATE and INFO_FLAG_CP are mutually exclusive and the flag
 * INFO_FLAG_LBOUND should be set only if one of them is set.
 */
enum info_flag {
	INFO_FLAG_DEPTH  = 0x1,
	INFO_FLAG_NODES  = 0x1 << 1,
	INFO_FLAG_NPS    = 0x1 << 2,
	INFO_FLAG_MATE   = 0x1 << 3,
	INFO_FLAG_TIME   = 0x1 << 4,
	INFO_FLAG_CP     = 0x1 << 5,
	INFO_FLAG_LBOUND = 0x1 << 6,
};

struct info {
	enum info_flag flags;
	int depth;
	int cp;
	int mate;
	long long nodes;
	long long nps;
	long long time;
};

struct thread_data;

struct search_argument {
	Position pos;
	int depth;
	int mate;
	int movestogo;
	long long nodes;
	long long time[2];
	long long inc[2];
	long long movetime;
	void (*info_sender)(const struct info *);
	void (*best_move_sender)(Move);
	atomic_bool *stop;
	Move moves[POSITION_STACK_CAPACITY];
	int moves_nb;
#ifdef SEARCH_STATISTICS
	FILE *log_file;
#endif
};

void *search(void *arg);

#endif
