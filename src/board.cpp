#pragma warning( disable : 6385)
#pragma warning( disable : 6386)
#include "board.h"
#include "evaluation.h"

Board::Board(const Position& postion) {
	init(postion);
}

void Board::setPosition(const Position& postion) {
	init(postion);
}

const int Board::makeMove(int color, int move) {
	U64 from = (U64)1 << ((move & FROM_MASK) >> 6);
	U64 to = (U64)1 << ((move & TO_MASK) >> 12);

	int piece = (move & PIECE_MASK) >> 18;
	int flag = move & FLAG_MASK;
	int captured_piece = (move & CAPTURED_PIECE_MASK) >> 24;

	U64 fromTo = from ^ to;
	colorBB[color] ^= fromTo;
	piece_list[color][piece] ^= fromTo;
	enpassant_square = 0;    // clear previous enpassant square

	if (flag == NO_FLAG) {
		occupiedBB ^= fromTo;
	} else if (flag == DOUBLE_PUSH) {
		occupiedBB ^= fromTo;
		enpassant_square = color == WHITE ? to >> 8 : to << 8;
	} else if (flag == CAPTURE) {
		occupiedBB ^= from;
		colorBB[color ^ 1] ^= to;
		piece_list[color ^ 1][captured_piece] ^= to;
		material += (-color | 1) * MATERIAL_VALUE[captured_piece];
	} else if (flag == EN_PASSANT) {
		U64 capture_square = color == WHITE ? to >> 8 : to << 8;
		occupiedBB ^= fromTo | capture_square;
		colorBB[color ^ 1] ^= capture_square;
		piece_list[color ^ 1][PAWN] ^= capture_square;
		material += (-color | 1) * MATERIAL_VALUE[PAWN];
	} else if (flag == CASTLING) {
		U64 rook_shift;
		if (from < to) { // castling short			
			rook_shift = (to << 1) ^ (from << 1);
		} else { // castling long
			rook_shift = (from >> 1) ^ (to >> 2);
		}
		occupiedBB ^= fromTo | rook_shift;
		colorBB[color] ^= rook_shift;
		piece_list[color][ROOK] ^= rook_shift;
	} else if (flag == PROMOTION) {
		int promoted_piece = (move & PROMOTION_MASK) >> 27;
		occupiedBB ^= fromTo;
		piece_list[color][PAWN] ^= to;
		piece_list[color][promoted_piece] ^= to;
		material += (-color | 1) * (MATERIAL_VALUE[promoted_piece] - MATERIAL_VALUE[PAWN]);
	} else if (flag == PROMOCAPT) {
		int promoted_piece = (move & PROMOTION_MASK) >> 27;
		occupiedBB ^= from;
		colorBB[color ^ 1] ^= to;
		piece_list[color][PAWN] ^= to;
		piece_list[color ^ 1][captured_piece] ^= to;
		piece_list[color][promoted_piece] ^= to;
		material += (-color | 1) * (MATERIAL_VALUE[promoted_piece] + MATERIAL_VALUE[captured_piece] - MATERIAL_VALUE[PAWN]);
	}
	int unmake_info = captured_piece | (castling_rights << 6);
	setCastlingRights(color, piece, flag, from, to);
	return unmake_info;
}

void Board::unmakeMove(int color, int move, int unmake_info) {
	U64 from = (U64)1 << ((move & FROM_MASK) >> 6);
	U64 to = (U64)1 << ((move & TO_MASK) >> 12);

	int flag = move & FLAG_MASK;
	U64 fromTo = from ^ to;
	colorBB[color] ^= fromTo;
	piece_list[color][(move & PIECE_MASK) >> 18] ^= fromTo;
	castling_rights = (unmake_info & CASTLING_MASK) >> 6;

	if (flag == NO_FLAG || flag == DOUBLE_PUSH) {
		occupiedBB ^= fromTo;
	} else if (flag == CAPTURE) {
		int captured_piece = unmake_info & CAPTURE_MASK;
		occupiedBB ^= from;
		colorBB[color ^ 1] ^= to;
		piece_list[color ^ 1][captured_piece] ^= to;
		material -= (-color | 1) * MATERIAL_VALUE[captured_piece];
	} else if (flag == EN_PASSANT) {
		U64 capture_square = color == WHITE ? to >> 8 : to << 8;
		occupiedBB ^= capture_square | fromTo;
		colorBB[color ^ 1] ^= capture_square;
		piece_list[color ^ 1][PAWN] ^= capture_square;
		material -= (-color | 1) * MATERIAL_VALUE[PAWN];
	} else if (flag == CASTLING) {
		U64 rook_shift;
		if (from < to) { // castling short			
			rook_shift = (to << 1) ^ (from << 1);
		} else { // castling long
			rook_shift = (from >> 1) ^ (to >> 2);
		}
		occupiedBB ^= fromTo | rook_shift;
		colorBB[color] ^= rook_shift;
		piece_list[color][ROOK] ^= rook_shift;
	} else if (flag == PROMOTION) {
		int promoted_piece = (move & PROMOTION_MASK) >> 27;
		occupiedBB ^= fromTo;
		piece_list[color][PAWN] ^= to;
		piece_list[color][promoted_piece] ^= to;
		material -= (-color | 1) * (MATERIAL_VALUE[promoted_piece] - MATERIAL_VALUE[PAWN]);
	} else if (flag == PROMOCAPT) {
		int promoted_piece = (move & PROMOTION_MASK) >> 27;
		int captured_piece = unmake_info & CAPTURE_MASK;
		occupiedBB ^= from;
		colorBB[color ^ 1] ^= to;
		piece_list[color][PAWN] ^= to;
		piece_list[color ^ 1][unmake_info & CAPTURE_MASK] ^= to;
		piece_list[color][promoted_piece] ^= to;
		material -= (-color | 1) * (MATERIAL_VALUE[promoted_piece] + MATERIAL_VALUE[captured_piece] - MATERIAL_VALUE[PAWN]);
	}
}

void Board::init(const Position& fenInfo) {
	for (int i = 0; i < COLORS; i++) {
		for (int j = 0; j < PIECES; j++) {
			piece_list[i][j] = fenInfo.piece_list[i][j];
			colorBB[i] += piece_list[i][j];			
		}
	}
	material = evaluation::popCountValue(piece_list);
	occupiedBB = colorBB[0] | colorBB[1];
	enpassant_square = fenInfo.enpassant_square;
	castling_rights = fenInfo.castling_rights;
}

void Board::setCastlingRights(int color, int piece, int flag, U64 from, U64 to) {
	if (castling_rights) {
		// only update castling rights after a friendly rook or king move or a capture
		if (piece == ROOK || piece == KING) {
			// check own kingside castling rights when move is a king or rook move
			if (castling_rights & KING_SIDE[color]) {
				if (from & (E_SQUARE[color]) || (from & H_SQUARE[color])) {
					castling_rights ^= KING_SIDE[color];
				}
			}
			// check own queenside castling rights when move is a king or rook move
			if (castling_rights & QUEEN_SIDE[color]) {
				if (from & E_SQUARE[color] || from & A_SQUARE[color]) {
					castling_rights ^= QUEEN_SIDE[color];
				}
			}
		}
		if (flag == CAPTURE) {
			// check enemies kingside castling rights when move is a rook capture
			if (castling_rights & KING_SIDE[color ^ 1]) {
				if (to & H_SQUARE[color ^ 1]) {
					castling_rights ^= KING_SIDE[color ^ 1];
				}
			}
			// check enemies queenside castling rights when move is a rook capture
			if (castling_rights & QUEEN_SIDE[color ^ 1]) {
				if (to & A_SQUARE[color ^ 1]) {
					castling_rights ^= QUEEN_SIDE[color ^ 1];
				}
			}
		}
	}
}

Board::~Board() {
}


