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

#ifndef POSITION_H
#define POSITION_H

#define INITIAL_PHASE 0
#define FINAL_PHASE 256

typedef enum direction {
	NORTH,
	NORTHEAST,
	EAST,
	SOUTHEAST,
	SOUTH,
	SOUTHWEST,
	WEST,
	NORTHWEST,
} Direction;

typedef enum file {
	FILE_A, FILE_B, FILE_C, FILE_D,
	FILE_E, FILE_F, FILE_G, FILE_H,
} File;

typedef enum rank {
	RANK_1, RANK_2, RANK_3, RANK_4,
	RANK_5, RANK_6, RANK_7, RANK_8,
} Rank;

typedef enum square {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
} Square;

typedef enum square_color {
	SQUARE_COLOR_LIGHT,
	SQUARE_COLOR_DARK,
} SquareColor;

/*
 * It's safe to get the opposite color with the ! operator on the color.
 */
typedef enum color {
	COLOR_WHITE,
	COLOR_BLACK,
} Color;

typedef enum piece_type {
	PIECE_TYPE_PAWN,
	PIECE_TYPE_KNIGHT,
	PIECE_TYPE_BISHOP,
	PIECE_TYPE_ROOK,
	PIECE_TYPE_QUEEN,
	PIECE_TYPE_KING,
} PieceType;

typedef enum piece {
	PIECE_WHITE_PAWN   = COLOR_WHITE | PIECE_TYPE_PAWN   << 1,
	PIECE_WHITE_KNIGHT = COLOR_WHITE | PIECE_TYPE_KNIGHT << 1,
	PIECE_WHITE_ROOK   = COLOR_WHITE | PIECE_TYPE_ROOK   << 1,
	PIECE_WHITE_BISHOP = COLOR_WHITE | PIECE_TYPE_BISHOP << 1,
	PIECE_WHITE_QUEEN  = COLOR_WHITE | PIECE_TYPE_QUEEN  << 1,
	PIECE_WHITE_KING   = COLOR_WHITE | PIECE_TYPE_KING   << 1,

	PIECE_BLACK_PAWN   = COLOR_BLACK | PIECE_TYPE_PAWN   << 1,
	PIECE_BLACK_KNIGHT = COLOR_BLACK | PIECE_TYPE_KNIGHT << 1,
	PIECE_BLACK_ROOK   = COLOR_BLACK | PIECE_TYPE_ROOK   << 1,
	PIECE_BLACK_BISHOP = COLOR_BLACK | PIECE_TYPE_BISHOP << 1,
	PIECE_BLACK_QUEEN  = COLOR_BLACK | PIECE_TYPE_QUEEN  << 1,
	PIECE_BLACK_KING   = COLOR_BLACK | PIECE_TYPE_KING   << 1,

	PIECE_NONE = 0xff /* Only used for the array board. */
} Piece;

typedef enum castling_side {
	CASTLING_SIDE_QUEEN,
	CASTLING_SIDE_KING,
} CastlingSide;

struct position;
typedef struct position Position;

int get_phase(const Position *pos);
bool pos_equal(const Position *pos1, const Position *pos2);
void decrement_fullmove_counter(Position *pos);
void increment_fullmove_counter(Position *pos);
void remove_castling(Position *pos, Color c, CastlingSide side);
void add_castling(Position *pos, Color c, CastlingSide side);
void flip_side_to_move(Position *pos);
void set_captured_piece(Position *pos, Piece piece);
void remove_piece(Position *pos, Square sq);
void place_piece(Position *pos, Square sq, Piece piece);
void reset_halfmove_clock(Position *pos);
void increment_halfmove_clock(Position *pos);
void unset_enpassant(Position *pos);
void set_enpassant(Position *pos, File file);
Piece get_captured_piece(const Position *pos);
int has_castling_right(const Position *pos, Color c, CastlingSide side);
int get_fullmove_counter(const Position *pos);
int get_halfmove_clock(const Position *pos);
int enpassant_possible(const Position *pos);
Square get_enpassant_square(const Position *pos);
Color get_side_to_move(const Position *pos);
Square get_king_square(const Position *pos, Color c);
Piece get_piece_at(const Position *pos, Square sq);
int get_number_of_pieces(const Position *pos, Piece piece);
int get_number_of_pieces_of_color(const Position *pos, Color c);
u64 get_piece_bitboard(const Position *pos, Piece piece);
u64 get_color_bitboard(const Position *pos, Color c);
void get_fen(char *fen, const Position *pos);
void backtrack_irreversible_state(Position *pos);
void start_new_irreversible_state(Position *pos);
Position *copy_pos(const Position *pos);
Position *create_pos(const char *fen);
void destroy_pos(Position *pos);
Square file_rank_to_square(File f, Rank r);
File get_file(Square sq);
Rank get_rank(Square sq);
Color get_piece_color(Piece piece);
PieceType get_piece_type(Piece piece);
Piece create_piece(PieceType pt, Color c);
SquareColor get_square_color(Square sq);

#endif
