#include "utils.h"

static const uint64_t power[64] =
{
	1, 2, 4, 8, 16, 32, 64, 128,
	256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
	65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608,
	16777216, 33554432, 67108864, 134217728, 268435456, 536870912, 1073741824, 2147483648,
	4294967296, 8589934592, 17179869184, 34359738368, 68719476736, 137438953472, 274877906944, 549755813888,
	1099511627776, 2199023255552, 4398046511104, 8796093022208, 17592186044416, 35184372088832, 70368744177664, 140737488355328,
	281474976710656, 562949953421312, 1125899906842624, 2251799813685248, 4503599627370496, 9007199254740992, 18014398509481984, 36028797018963968,
	72057594037927936, 144115188075855872, 288230376151711744, 576460752303423488, 1152921504606846976, 2305843009213693952, 4611686018427387904, 9223372036854775808
};

static const int index64[64] = {
	0, 47,  1, 56, 48, 27,  2, 60,
	57, 49, 41, 37, 28, 16,  3, 61,
	54, 58, 35, 52, 50, 42, 21, 44,
	38, 32, 29, 23, 17, 11,  4, 62,
	46, 55, 26, 59, 40, 36, 15, 53,
	34, 51, 20, 43, 31, 22, 10, 45,
	25, 39, 14, 33, 19, 30,  9, 24,
	13, 18,  8, 12,  7,  6,  5, 63
};

uint64_t Utils::getPower(int exp) {
	return power[exp];
}

uint64_t Utils::getLSB(uint64_t bb) {
	return bb & (0 - bb);
}

uint64_t Utils::clearLSB(uint64_t bb) {
	return bb & (bb - 1);
}

uint64_t Utils::flipBitVertical(uint64_t bit) {
	return bit ^ 56;
}

uint64_t Utils::flip1BBVertical(uint64_t bb) {
	const uint64_t k1 = uint64_t(0x00FF00FF00FF00FF);
	const uint64_t k2 = uint64_t(0x0000FFFF0000FFFF);
	bb = ((bb >> 8) & k1) | ((bb & k1) << 8);
	bb = ((bb >> 16) & k2) | ((bb & k2) << 16);
	bb = (bb >> 32) | (bb << 32);
	return bb;
}

/**
* getLS1B
* @author Kim Walisch (2012)
* @param squares bitboard to scan
* @precondition bb != 0
* @return index (0..63) of least significant one bit
*/
int Utils::getLS1B(uint64_t bb) {
	const uint64_t debruijn64 = uint64_t(0x03f79d71b4cb0a89);
	return index64[((bb ^ (bb - 1)) * debruijn64) >> 58];
}
