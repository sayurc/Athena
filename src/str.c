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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <str.h>

/*
 * This is an implementation of the GNU vasprintf. We get the final string
 * length by calling vsprintf with size 0 and string pointer NULL which makes
 * it return the number of characters that would have been written, but without
 * writing anything. An extra character is added to the length to make place
 * for the 0 byte.
 */
int my_vasprintf(char **restrict strp, const char *restrict fmt, va_list va)
{
	va_list copy;

	va_copy(copy, va);
	int len = vsnprintf(NULL, 0, fmt, copy);
	va_end(copy);
	if (len < 0) {
		*strp = NULL;
		return len;
	}
	++len;

	*strp = malloc((size_t)len);
	if (!*strp)
		return -1;

	int ret = vsnprintf(*strp, (size_t)len, fmt, va);

	if (ret < 0)
		free(*strp);

	return ret;
}

/*
 * This is an implementation of the GNU asprintf.
 */
int my_asprintf(char **restrict strp, const char *restrict fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	int ret = my_vasprintf(strp, fmt, va);
	va_end(va);

	return ret;
}
