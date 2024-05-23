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

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bit.h>
#include <pos.h>

/*
 * The piece placement is stored in two formats, in piece-centric bitboard
 * arrays and in a square-centric array.
 * 
 * In the piece-centric format there are two arrays, one indexed by the color
 * and one indexed by the piece type, both storing bitboards where the set bits
 * represent a piece of that color or type at a square, where the square is
 * counted from the least significant bit to the most significant bit, from 0
 * to 63. The bitboards store pieces using a Little-Endian Rank-File mapping
 * (LERF), which means each byte, from the least significant byte to the most
 * significant byte represents a rank, and each bit of these bytes represents a
 * square on that rank. So, for example, A1 is square 0, H1 is square 7, A2 is
 * square 8, H2 is square 15 and so on.
 * 
 * The square-centric format is just a flat array indexed by the square number
 * in a LERF mapping and each element of the array is a piece, or PIECE_NONE if
 * the square is empty.
 * 
 * The castling rights are stored in a nibble, the 2 least significant bits are
 * for white and the next 2 bits for black, the least significant and most
 * significant bits of each are for the queen and king sides respectively. A
 * set bit means that the king has the right to castle. Notice that having
 * right to castle does not mean that castling really is possible
 * (castling ability), it only means that neither the king nor the rook
 * corresponding to the castling side have moved throughout the game, and the
 * rook hasn't been captured. In order for castling to be possible, the
 * following conditions must be met:
 * - The side must have castling rights;
 * - There are no pieces between the king and rook;
 * - The king is not in check;
 * - The king will not be in check after castling;
 * - The squares between the king's original and final positions are not being
 *   attacked.
 * 
 * The en passant square is not stored, but instead only its file. This is to
 * save space, since it is possible to recover the square by using the color of
 * the side to move. Because there are only 8 files the file is stored in just
 * the 3 least significant bits of a nibble and the most significant bit of the
 * nibble is set when there is an en passant square and unset otherwise. Since
 * both the castling rights and en passant file are stored in a nibble, I
 * stored both together in one byte.
 * 
 * Because changes to some of the position data can't be undone, like the
 * castling ability (if the rook moves back and forth there's no way to know
 * that it happened later on), all this irreversibe state is stored in a stack
 * where the top is the current state and to undo a move one only has to pop
 * the last irreversible state off the stack and undo the changes to the
 * reversibe data.
 */

/*
 * Check if ch is one of the characters in str, where str is a string containing
 * all characters to be checked and not separated by space.
 * For example:
 * is_one_of("ar4h", 'r')
 * returns 1, but
 * is_one_of("r57g", '9')
 * returns 0.
 */
static int is_one_of(const char *str, char ch)
{
	if (!ch || !strchr(str, ch))
		return 0;
	return 1;
}

static size_t parse_pieces(Position *pos, const char *str)
{
	const Piece table[] = {
		['P'] = PIECE_WHITE_PAWN,   ['p'] = PIECE_BLACK_PAWN,
		['N'] = PIECE_WHITE_KNIGHT, ['n'] = PIECE_BLACK_KNIGHT,
		['R'] = PIECE_WHITE_ROOK,   ['r'] = PIECE_BLACK_ROOK,
		['B'] = PIECE_WHITE_BISHOP, ['b'] = PIECE_BLACK_BISHOP,
		['Q'] = PIECE_WHITE_QUEEN,  ['q'] = PIECE_BLACK_QUEEN,
		['K'] = PIECE_WHITE_KING,   ['k'] = PIECE_BLACK_KING,
	};

	size_t rc = 0;
	File file = FILE_A;
	Rank rank = RANK_8;
	for (size_t i = 0; file <= FILE_H || rank > RANK_1; ++i) {
		if (!str[i])
			return 0;
		char ch = str[i];
		++rc;

		if (file > FILE_H) {
			if (str[i] && str[i] != '/')
				return 0;
			--rank;
			file = FILE_A;
			continue;
		}

		if (isdigit(ch)) {
			unsigned digit = (unsigned)(ch - '0');
			if (digit > 8 || digit < 1 || !digit ||
			    file + digit > 8)
				return 0;
			file += digit;
		} else if (is_one_of("PNRBQKpnrbqk", ch)) {
			Piece piece = table[(size_t)ch];
			Square sq = file_rank_to_square(file, rank);
			place_piece(pos, sq, piece);
			++file;
		} else {
			return 0;
		}
	}

	return rc;
}

static size_t parse_side(Position *pos, const char *str)
{
	switch (str[0]) {
	case 'w':
		pos->side_to_move = COLOR_WHITE;
		break;
	case 'b':
		pos->side_to_move = COLOR_BLACK;
		break;
	default:
		return 0;
	}
	return 1;
}

static size_t parse_castling(Position *pos, const char *str)
{
	if (str[0] == '-')
		return 1;
	int Kcnt = 0, Qcnt = 0, kcnt = 0, qcnt = 0;
	size_t rc = 0;
	for (; str[rc] && str[rc] != ' '; ++rc) {
		switch (str[rc]) {
		case 'K':
			add_castling(pos, COLOR_WHITE, CASTLING_SIDE_KING);
			++Kcnt;
			break;
		case 'Q':
			add_castling(pos, COLOR_WHITE, CASTLING_SIDE_QUEEN);
			++Qcnt;
			break;
		case 'k':
			add_castling(pos, COLOR_BLACK, CASTLING_SIDE_KING);
			++kcnt;
			break;
		case 'q':
			add_castling(pos, COLOR_BLACK, CASTLING_SIDE_QUEEN);
			++qcnt;
			break;
		default:
			return 0;
		}
		if (Kcnt > 1 || Qcnt > 1 || kcnt > 1 || qcnt > 1)
			return 0;
	}
	return rc;
}

static size_t parse_enpassant(Position *pos, const char *str)
{
	if (str[0] == '-')
		return 1;
	if (!str[0] || !str[1])
		return 0;
	else if (str[0] < 'a' || str[0] > 'h' ||
		 (str[1] != '3' && str[1] != '6'))
		return 0;
	File file = (File)(str[0] - 'a');
	set_enpassant(pos, file);
	return 2;
}

static size_t parse_halfmove_clock(Position *pos, const char *str)
{
	char *endptr = NULL;
	errno = 0;
	unsigned long clock = strtoul(str, &endptr, 10);
	if (errno == ERANGE)
		return 0;
	else if (endptr == str)
		return 0;
	else if (clock > SHRT_MAX)
		return 0;
	pos->irr_states[pos->irr_state_idx].halfmove_clock = (u8)clock;
	return (size_t)(endptr - str);
}

/*
 * Both the parse_fullmove_counter and parse_halfmove_clock functions parse the
 * number up to the first invalid character (a character that is not part of a
 * number). So the functions will not fail because of the space character after
 * the number in the FEN string.
 */
static size_t parse_fullmove_counter(Position *pos, const char *str)
{
	char *endptr = NULL;
	errno = 0;
	unsigned long counter = strtoul(str, &endptr, 10);
	if (errno == ERANGE)
		return 0;
	else if (endptr == str)
		return 0;
	else if (counter > SHRT_MAX)
		return 0;
	pos->fullmove_counter = (short)counter;
	return (size_t)(endptr - str);
}

/*
 * Modifies a position by parsing a FEN string and returns the number of
 * characters read, if it's less than the length then an error ocurred. Each of
 * the parse_* functions return the number of characters that were read, and if
 * the sequence of characters is invalid they return 0.
 */
static size_t parse_fen(Position *pos, const char *fen)
{
	size_t (*const steps[])(Position *, const char *) = {
		parse_pieces,	 parse_side,	       parse_castling,
		parse_enpassant, parse_halfmove_clock, parse_fullmove_counter,
	};
	const size_t num_steps = sizeof(steps) / sizeof(steps[0]);

	size_t rc = 0, ret = 0;
	for (size_t i = 0; i < num_steps; ++i) {
		ret = steps[i](pos, fen);
		if (!ret)
			return 0;
		fen += ret;
		rc += ret;
		if (i < num_steps - 1) {
			if (fen[0] != ' ')
				return 0;
			else
				++rc;
		}
		++fen;
	}

	return rc;
}

/*
 * This function returns a number between 0 and 256 representing the game phase
 * where 0 is the initial phase and 256 is the final phase. This approach of
 * representing the game with several phases prevents evaluation discontinuity.
 */
int get_phase(const Position *pos)
{
	const int weights[] = { [PIECE_TYPE_PAWN] = 0,
				[PIECE_TYPE_KNIGHT] = 1,
				[PIECE_TYPE_BISHOP] = 1,
				[PIECE_TYPE_ROOK] = 2,
				[PIECE_TYPE_QUEEN] = 4 };
	const int neutral =
		16 * weights[PIECE_TYPE_PAWN] + 4 * weights[PIECE_TYPE_KNIGHT] +
		4 * weights[PIECE_TYPE_BISHOP] + 4 * weights[PIECE_TYPE_ROOK] +
		2 * weights[PIECE_TYPE_QUEEN];

	int phase = neutral;

	for (Color c = COLOR_WHITE; c <= COLOR_BLACK; ++c) {
		for (PieceType pt = PIECE_TYPE_PAWN; pt <= PIECE_TYPE_QUEEN;
		     ++pt) {
			Piece piece = create_piece(pt, c);
			int num = get_number_of_pieces(pos, piece);
			phase -= num * weights[pt];
		}
	}
	phase = (256 * phase + (neutral / 2)) / neutral;

	return phase;
}

/*
 * Returns true if two positions are the same and false otherwise. This is not
 * a full comparison of the positions in memory, it's supposed to be used for
 * enforcing the threefold repetition rule so it only compares the elements that
 * are necessary to make two positions the same according to the rules of chess.
 */
bool pos_equal(const Position *pos1, const Position *pos2)
{
	if (get_side_to_move(pos1) != get_side_to_move(pos2))
		return false;
	for (Color c = COLOR_WHITE; c <= COLOR_BLACK; ++c) {
		for (CastlingSide s = CASTLING_SIDE_QUEEN;
		     s <= CASTLING_SIDE_KING; ++s) {
			const bool pos1_has = has_castling_right(pos1, c, s);
			const bool pos2_has = has_castling_right(pos2, c, s);
			if (pos1_has != pos2_has)
				return false;
		}
	}
	if (has_en_passant_square(pos1) != has_en_passant_square(pos2))
		return false;
	else if (has_en_passant_square(pos1) && has_en_passant_square(pos2)) {
		if (get_en_passant_square(pos1) != get_en_passant_square(pos2))
			return false;
	}
	if (pos1->color_bb[COLOR_WHITE] != pos2->color_bb[COLOR_WHITE])
		return false;
	if (pos1->color_bb[COLOR_BLACK] != pos2->color_bb[COLOR_BLACK])
		return false;
	for (PieceType pt = PIECE_TYPE_PAWN; pt <= PIECE_TYPE_KING; ++pt) {
		if (pos1->type_bb[pt] != pos2->type_bb[pt])
			return false;
	}

	return true;
}

void decrement_fullmove_counter(Position *pos)
{
	--pos->fullmove_counter;
}

void increment_fullmove_counter(Position *pos)
{
	++pos->fullmove_counter;
}

void remove_castling(Position *pos, Color c, CastlingSide side)
{
	const size_t idx = pos->irr_state_idx;
	u8 *const ptr = &pos->irr_states[idx].castling_rights_and_enpassant;
	*ptr &= (u8)(~(1 << side << 2 * c));
}

void add_castling(Position *pos, Color c, CastlingSide side)
{
	const size_t idx = pos->irr_state_idx;
	u8 *const ptr = &pos->irr_states[idx].castling_rights_and_enpassant;
	*ptr |= (u8)(1 << side << 2 * c);
}

void flip_side_to_move(Position *pos)
{
	if (pos->side_to_move == COLOR_WHITE)
		pos->side_to_move = COLOR_BLACK;
	else
		pos->side_to_move = COLOR_WHITE;
}

void set_captured_piece(Position *pos, Piece piece)
{
	const size_t idx = pos->irr_state_idx;
	pos->irr_states[idx].captured_piece = (u8)piece;
}

/*
 * Remove a piece from a square.
 */
void remove_piece(Position *pos, Square sq)
{
	const Piece piece = get_piece_at(pos, sq);
	const u64 bb = U64(0x1) << sq;
	pos->color_bb[get_piece_color(piece)] &= ~bb;
	pos->type_bb[get_piece_type(piece)] &= ~bb;
	pos->board[sq] = PIECE_NONE;
}

/*
 * Place a piece at a square, if another piece is at this square, it will be
 * removed first.
 * 
 * There's an important detail about this function. The bitboards are stored in
 * a piece centric format, so we can't just overwrite the old piece with the
 * new one if the pieces are of different types, the bitboard of the piece
 * being replaced would still store the piece as if it's still on the board,
 * because only the new piece's board would be modified. Because of that, the
 * old piece must be removed first with the remove_piece function by the
 * caller. The reason why this is not done here is to avoid slowing down code
 * that places a piece in an empty square.
 */
void place_piece(Position *pos, Square sq, Piece piece)
{
	const u64 bb = U64(0x1) << sq;
	if (pos->board[sq] != PIECE_NONE)
		remove_piece(pos, sq);
	pos->color_bb[get_piece_color(piece)] |= bb;
	pos->type_bb[get_piece_type(piece)] |= bb;
	pos->board[sq] = piece;
}

void reset_halfmove_clock(Position *pos)
{
	pos->irr_states[pos->irr_state_idx].halfmove_clock = 0;
}

void increment_halfmove_clock(Position *pos)
{
	++pos->irr_states[pos->irr_state_idx].halfmove_clock;
}

void unset_enpassant(Position *pos)
{
	const size_t idx = pos->irr_state_idx;
	pos->irr_states[idx].castling_rights_and_enpassant &= 0xf;
}

/*
 * Set the possibility of en passant and store the file.
 */
void set_enpassant(Position *pos, File file)
{
	const size_t idx = pos->irr_state_idx;
	pos->irr_states[idx].castling_rights_and_enpassant &= 0x8f;
	pos->irr_states[idx].castling_rights_and_enpassant |= 0x80;
	pos->irr_states[idx].castling_rights_and_enpassant |= (file & 0x7) << 4;
}

Piece get_captured_piece(const Position *pos)
{
	return pos->irr_states[pos->irr_state_idx].captured_piece;
}

int has_castling_right(const Position *pos, Color c, CastlingSide side)
{
	const size_t idx = pos->irr_state_idx;
	const u8 *const ptr =
		&pos->irr_states[idx].castling_rights_and_enpassant;
	return (*ptr & 0x1 << side << 2 * c) != 0;
}

int get_fullmove_counter(const Position *pos)
{
	return pos->fullmove_counter;
}

int get_halfmove_clock(const Position *pos)
{
	return pos->irr_states[pos->irr_state_idx].halfmove_clock;
}

int has_en_passant_square(const Position *pos)
{
	const size_t idx = pos->irr_state_idx;
	return pos->irr_states[idx].castling_rights_and_enpassant & 0x80;
}

Square get_en_passant_square(const Position *pos)
{
	const size_t idx = pos->irr_state_idx;
	const u8 *const ptr =
		&pos->irr_states[idx].castling_rights_and_enpassant;

	const File f = (*ptr & 0x70) >> 4;
	const Rank r = pos->side_to_move == COLOR_WHITE ? RANK_6 : RANK_3;

	return file_rank_to_square(f, r);
}

Color get_side_to_move(const Position *pos)
{
	return pos->side_to_move;
}

Square get_king_square(const Position *pos, Color c)
{
	const Piece piece = create_piece(PIECE_TYPE_KING, c);
	const u64 bb = get_piece_bitboard(pos, piece);
	return get_ls1b(bb);
}

Piece get_piece_at(const Position *pos, Square sq)
{
	return pos->board[sq];
}

int get_number_of_pieces(const Position *pos, Piece piece)
{
	const u64 bb = get_piece_bitboard(pos, piece);

	return popcnt(bb);
}

int get_number_of_pieces_of_color(const Position *pos, Color c)
{
	const u64 bb = get_color_bitboard(pos, c);

	return popcnt(bb);
}

u64 get_piece_bitboard(const Position *pos, Piece piece)
{
	const PieceType type = get_piece_type(piece);
	const Color color = get_piece_color(piece);
	const u64 bb = pos->type_bb[type] & pos->color_bb[color];
	return bb;
}

u64 get_color_bitboard(const Position *pos, Color c)
{
	return pos->color_bb[c];
}

/*
 * The fen array must have space for at least 512 characters including the
 * terminating null byte, just to be safe.
 */
void get_fen(char *fen, const Position *pos)
{
	char piece_table[] = {
		[PIECE_WHITE_PAWN] = 'P',   [PIECE_WHITE_KNIGHT] = 'N',
		[PIECE_WHITE_BISHOP] = 'B', [PIECE_WHITE_ROOK] = 'R',
		[PIECE_WHITE_QUEEN] = 'Q',  [PIECE_WHITE_KING] = 'K',
		[PIECE_BLACK_PAWN] = 'p',   [PIECE_BLACK_KNIGHT] = 'n',
		[PIECE_BLACK_BISHOP] = 'b', [PIECE_BLACK_ROOK] = 'r',
		[PIECE_BLACK_QUEEN] = 'q',  [PIECE_BLACK_KING] = 'k',
	};

	for (int r = RANK_8; r >= RANK_1; --r) {
		int empty_cnt = 0;
		for (int f = FILE_A; f <= FILE_H; ++f) {
			const enum square sq =
				file_rank_to_square((enum file)f, (enum rank)r);
			const enum piece p = get_piece_at(pos, sq);
			if (p == PIECE_NONE) {
				++empty_cnt;
			} else {
				if (empty_cnt) {
					/* empty_cnt is guaranteed to be just
					 * one digit, so this is safe. */
					fen += sprintf(fen, "%d", empty_cnt);
				}
				empty_cnt = 0;
				const char c = piece_table[p];
				*fen = c;
				++fen;
			}
		}
		if (empty_cnt)
			fen += sprintf(fen, "%d", empty_cnt);
		if (r > RANK_1) {
			*fen = '/';
			++fen;
		}
	}

	*fen = ' ';
	++fen;

	*fen = get_side_to_move(pos) == COLOR_WHITE ? 'w' : 'b';
	++fen;

	*fen = ' ';
	++fen;

	bool has_castling = false;
	for (int c = COLOR_WHITE; c <= COLOR_BLACK; ++c) {
		for (int s = CASTLING_SIDE_KING; s >= CASTLING_SIDE_QUEEN;
		     --s) {
			if (has_castling_right(pos, (enum color)c,
					       (enum castling_side)s)) {
				has_castling = true;
				char ch = s == CASTLING_SIDE_QUEEN ? 'q' : 'k';
				ch = c == COLOR_WHITE ? (char)toupper((int)ch) :
							ch;
				*fen = ch;
				++fen;
			}
		}
	}
	if (!has_castling) {
		*fen = '-';
		++fen;
	}

	*fen = ' ';
	++fen;

	if (has_en_passant_square(pos)) {
		const enum square sq = get_en_passant_square(pos);
		const enum file f = get_file(sq);
		const enum rank r = get_rank(sq);
		*fen = 'a' + (char)f;
		++fen;
		fen += sprintf(fen, "%d", r + 1);
	} else {
		*fen = '-';
		++fen;
	}

	*fen = ' ';
	++fen;

	/* According to FIDE rules it is a draw when the halfmove clock reaches
	 * 75, so we only need two digits. */
	fen += snprintf(fen, 2, "%d", get_halfmove_clock(pos));

	*fen = ' ';
	++fen;

	/* Using the FIDE rules we can calculate that the longest possible chess
	 * game is 8848.5 fullmoves long, so we only need four digits. */
	fen += snprintf(fen, 4, "%d", get_fullmove_counter(pos));

	*fen = ' ';
	++fen;

	*fen = 0;
}

void backtrack_irreversible_state(Position *pos)
{
	--pos->irr_state_idx;
}

/*
 * This function must be called before externally calling any function that
 * modifies the irreversible state of the position.
 *
 * It creates a new copy of the old irreversible state and pushes
 * it onto the stack, making it the current one.
 */
void start_new_irreversible_state(Position *pos)
{
	pos->irr_state_idx += 1;
	size_t idx = pos->irr_state_idx;
	pos->irr_states[idx] = pos->irr_states[idx - 1];
}

void copy_position(Position *copy, const Position *pos)
{
	memcpy(copy, pos, sizeof(Position));
	memcpy(copy->irr_states, pos->irr_states,
	       copy->irr_state_cap * sizeof(struct irreversible_state));
}

/*
 * Initializes a new position using FEN. It returns 0 on success and 1
 * otherwise. It is assumed that the string it not empty and that there are no
 * leading or trailing white spaces. Keep in mind that whether the position is
 * actually valid according to the rules of chess is not checked, so even if the
 * FEN string is a valid FEN string according to the grammar, the position might
 * be illegal. For example, the number of pawns on the board is not checked, so
 * it is possible to set up a position using a FEN string that describes a board
 * with 9 pawns. This is intentional, as the user might want to set up a
 * non-standard board.
 */
int init_position(Position *pos, const char *fen)
{
	if (!pos) {
		fprintf(stderr, "Could not allocate memory.\n");
		exit(1);
	}

	pos->irr_state_cap = 256;
	pos->irr_state_idx = 0;

	pos->fullmove_counter = 0;
	pos->irr_states[pos->irr_state_idx].captured_piece = PIECE_NONE;
	reset_halfmove_clock(pos);
	unset_enpassant(pos);
	remove_castling(pos, COLOR_WHITE, CASTLING_SIDE_KING);
	remove_castling(pos, COLOR_WHITE, CASTLING_SIDE_QUEEN);
	remove_castling(pos, COLOR_BLACK, CASTLING_SIDE_KING);
	remove_castling(pos, COLOR_BLACK, CASTLING_SIDE_QUEEN);
	for (Square sq = A1; sq <= H8; ++sq)
		pos->board[sq] = PIECE_NONE;
	for (size_t i = 0; i < 6; ++i)
		pos->type_bb[i] = 0;
	for (size_t i = 0; i < 2; ++i)
		pos->color_bb[i] = 0;

	size_t rc = parse_fen(pos, fen);
	if (rc != strlen(fen))
		return 1;
	return 0;
}

Square file_rank_to_square(File f, Rank r)
{
	return 8 * r + f;
}

File get_file(Square sq)
{
	return sq % 8;
}

Rank get_rank(Square sq)
{
	return sq / 8;
}

Color get_piece_color(Piece piece)
{
	return piece & 0x1;
}

PieceType get_piece_type(Piece piece)
{
	return piece >> 1;
}

Piece create_piece(PieceType pt, Color c)
{
	return pt << 1 | c;
}

SquareColor get_square_color(Square sq)
{
	u64 dark = U64(0xaa55aa55aa55aa55);
	return !((dark >> sq) & 1);
}
