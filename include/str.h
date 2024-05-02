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

#ifndef STR_H
#define STR_H

/*
 * Version of asprintf that doesn't return anything and aborts in case of error.
 */
#define SAFE_ASPRINTF(...)\
	if (asprintf(__VA_ARGS__) == -1) {\
		fprintf(stderr, "Out of memory or some other error\n");\
		abort();\
	}

int vasprintf(char **strp, const char *fmt, va_list va);
int asprintf(char **strp, const char *fmt, ...);

#endif
