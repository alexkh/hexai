// tested with gcc 4.8.0, AMD Phenom II X6 1090T,
// to compile: g++ -O3 -std=c++0x -o hexai hexai.cpp
// above compiler flags give about .5 seconds per AI move on a 11x11 board.
// Total rewrite with Monte-Carlo ai, not reusing code from previous homework.
// This code relies on <cstdint> for uint32_t type, for bitwise scan altorithm.
// Black and white stones, black moves first, black sides are North and South
// human(s) or AI can play either side or both.
// Board display convention: 'X'=black, 'O'=white, ' '=empty tile.
// A 2-row scan algorithm similar to that shown by the teacher is used,
// modified to use bitwise arithmetics: each row is represented by up to 32 bits
// of uint32_t type, which is a 32-bit integer. We can then determine which
// stones of first row touch stones of the second row by doing only bitwise
// operations: let A and B be the two topmost rows.
// ((A and B) or ((A << 1) and B))
// [note that real bitshift direction will be right if we want our bit number
// correspond the column number - mirror image. does not matter as long as we
// are consistent, but visually on the board we want to left shift the top row]
// the result of above formula will be a row that has bits set for those stones
// in B that touch any stones in A. If all bits in this result are 0, no need
// to check further: there is no connection. We can then use this result as A,
// take row 3 as B and repeat the check until we reach bottom row.
// To do a similar check for white stones, we need to treat leftmost column as
// the top row and so on, that's why we have vectors blackrow and whitecol.
// For checking position in the Monte-Carlo simulation we only need one such
// vector for black stones to determine the winner since the board is always
// full.
// index-to-row conversion: row = i / side
// index-to-col conversion: col = i % side
// row, col to index conversion: i = row * side + col
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm> // shuffle
#include <random>
#include <chrono>
#include <cstdint> // uint32_t
using namespace std;

// Board does the Monte-Carlo simulations, its field is optimized for
// quick determining of the winner.
class Board {
	unsigned char side; // side of the board defines its size
	size_t size; // always = side * side
	vector<int> stone; // all the stones, all whites on top,
		// those stones that are on the board are on top and bottom of
		// this array, pointers cur0 and cur1 separate the stones on
		// the board from stones off the board (which still are used
		// for Monte-Carlo simulation, but not shown on the board)
	vector<int>::iterator cur0, cur1, middle; // iterators pointing to first
		// one, one after last, and first black stone (aka middle)
		// stones that are not on the board = those
		// that are shuffled during Monte-Carlo tests
	vector<uint32_t> blackrow, whitecol; // bitmaps storing the stones as
		// single bits in a row: each row is 32-
	char winner; // ' '=game is running, 'X' or 'O' means game is over
	bool whites_move; // whose move is it now? 1 if whites
	bool black_ai; // is black player played by AI?
	bool white_ai; // is white player played by AI?
	bool init_success; // initialization successful flag
	default_random_engine *randengine; // used for shuffle
	size_t nshuffles{1000}; // number of shuffles to perform
public:
	Board(unsigned char side, bool aiblack = 1, bool aiwhite = 1) {
		reset(side, aiblack, aiwhite);
	}
	Board() : init_success(false) {}; // we allow uninitialized board to be
		// created,
		// in case the class user wants to specify the board size at
		// a later time, which is done using reset() function
	~Board() {}
	// full reset used for starting a new game, makes sure that the side of
	// the board is at least 3
	void reset(unsigned char side, bool aiblack = 1, bool aiwhite = 1) {
		if(!init_success) { // seed only once
			unsigned seed = chrono::system_clock::now()
				.time_since_epoch().count();
			randengine = new default_random_engine(seed);
		}
		if(side < 3) {
			side = 3;
		}
		if(side > 32) {
			side = 32;
		}
		this->side = side; // side of the board won't change during game
		size = side * side; // number of tiles is set only once here
		stone.resize(size); // resize this vector without initializing
		// initialize the stone vector:
		for(int i = 0; i < size; i++) {
			stone[i] = i;
		}
		cur0 = stone.begin();
		cur1 = stone.end();
		middle = cur0 + (size / 2); // black stones start here
		// each of our stones will always be assigned to a unique tile
		// no matter if the stone is  on the board or not.
		// no matter how many moves and shuffles are done
		// each stone is assigned to a tile, the color of the stone
		// depends on its position in the stone array. Top half will
		// be white, bottom half - black. If the board side is odd
		// then the number of black stones is 1 more than white ones
		blackrow.clear();
		blackrow.resize(side); // initialize to zero the blackrow vector
		whitecol.clear();
		whitecol.resize(side);
		winner = ' '; // game is running
		whites_move = false; // black starts
		black_ai = aiblack; // is black to be played by computer?
		white_ai = aiwhite; // is white to be played by computer (too)?
		init_success = true; // init done, allow calling other functions
	}
	// returns side of the board or 0 if the board is not initialized
	unsigned char get_side() {
		if(!init_success) {
			return 0;
		}
		return side;
	}
	// used for debugging
	void print_stones() {
		if(!init_success) {
			return;
		}
		for(auto cur : stone) {
			cout << cur << ' ';
		}
		cout << endl;
	}
	// print the board on the screen
	void print() {
		if(!init_success) {
			return;
		}
		// output top numbers
		cout << "    ";
		for(int j = 0; j < side; j++) {
			if(j < 9) {
				cout << (j + 1) << "   ";
			} else {
				cout << (j + 1) << "  ";
			}
		}
		cout << endl;
		cout << "   ";
		// print the top /\/\/\/\/\ part
		for(int j = 0; j < side; j++) {
			cout << "/ \\ ";
		}
		cout << endl;
		string pad; // spaces added for each new row
		pad.reserve(side * 2);
		for(int i = 0; i < side; i++) { // i stands for row here
			cout << pad << setfill(' ') << setw(2) << (i + 1);
			for(int j = 0; j < side; j++) {
				cout << "| "; // << tile [i * side + j];
				// i is row, j is col
				if(blackrow[i] & uint32_t(1) << j) {
					cout << 'X';
				} else if(whitecol[j] & uint32_t(1) << i) {
					cout << 'O';
				} else {
					cout << ' ';
				}
				cout << ' ';
			}
			cout << '|' << endl << pad << "   ";
			for(int j = 0; j < side; j++) {
				cout << "\\ / ";
			}
			if(i != (side - 1)) {
				cout << "\\";
			}
			cout << endl;
			pad.append("  ");
		}
	}
	// checks a vector for connectedness whether it's black or white stones
	bool is_connected(vector<uint32_t> &row) {
		uint32_t a, b, c, s; // temporary variables
		a = row[0];
		// iterate row by row
		for(auto r = row.begin() + 1; r != row.end(); ++r) {
			b = (a | (a >> 1)) & (*r); // all this does is checks
				// which of the current row's stones touch
				// the "connected" stones in previous row
				// however, within row connections need
				// to be checked separately by iteratively
				// testing adjacent left stones and then
				// adjacent right stones until no more adja-
				// cent stones left
			if(b == *r) {
				if(!b) {
					return false;
				}
				a = b;
				continue;
			}
			// check left connections
			s = b << 1;
			while(true) {
				c = s & (*r);
				if(c) {
					b |= c;
					s = c << 1;
				} else {
					break;
				}
			};
			// check right connections
			s = b >> 1;
			while(true) {
				c = s & (*r);
				if(c) {
					b |= c;
					s = c >> 1;
				} else {
					break;
				}
			}
			if(!b) {
				return false;
			}
			a = b;
		}
		return true;
	}
	// checks if game is over and sets winner to X=black or O=white
	void check_game_over() {
		if(is_connected(blackrow)) { // check black side
			winner = 'X';
		} else if(is_connected(whitecol)) { // check white side
			winner = 'O';
		}
	}
	// checks if white would win in current stones configuration
	bool is_white_winning() {
		vector<uint32_t> wcol(whitecol); // local array of white columns
		// fill out the wcol vector with stones that are off the board
		for(auto it = cur0; it != middle; ++it) {
			wcol[(*it) % side] |=
				uint32_t(1) << ((*it) / side);
		}
		return is_connected(wcol); // check if connection exists
	}
	// ai move, tricky part here is that since the whites are on top of
	// our stone array and blacks are on bottom, playing for one color is
	// slightly different than the other
	void make_move() {
		if(!init_success) {
			return;
		}
		// if the number of tiles on board is side-1 and more, before
		// doing 1000 Monte-Carlo runs it makes sense to check if adding
		// a single tile can win the game by trying each possible move
		// once and checking if resulting position will be a win.
		// For simplicity we'll just do this check at every move
		shuffle(cur0, cur1, *randengine);
		uint32_t a;
		int i;
		if(whites_move) {
			// do a monte-carlo simulation
			vector<size_t> moves(cur0, cur1); // we must copy
				// avaliable moves or after shuffling
				// we'll lose track of which one have been
				// checked
			vector<int> tile(size); // display values for debugging
			size_t max; // move with maximum value
			size_t win_count; // count number of wins for white
			size_t max_count = 0; // wins on the best move
			for(auto it = moves.begin(); it != moves.end(); ++it) {
				win_count = 0;
				// do the swap so this move is reflected in
				// the stone vector
				auto pickmove = find(cur0, cur1, *it);
				*pickmove = *cur0;
				*cur0 = *it;
				// do the Monte-Carlo based on this move
				for(int j = 0; j < nshuffles; ++j) {
					shuffle(cur0 + 1, cur1, *randengine);
					if(is_white_winning()) {
						++win_count;
					}
				}
				if(win_count > max_count) {
					max = *it;
					max_count = win_count;
				}
				tile[*it] = win_count;
			}
			// draw the table - for debugging
			cout << "White move values:\n";
			for(int j = 0; j < size; j++) {
				if(!(j % side)) {
					cout << '\n';
				}
				cout << '\t' << tile[j];
			}
			cout << '\n';
			// make the best move
			auto it = find(cur0, cur1, max);
			*it = *cur0;
			*cur0 = max;
			// update whitecol
			whitecol[(*cur0) % side] |=
				uint32_t(1) << ((*cur0) / side);
			cur0++;
		} else {
			--cur1;
			// do a monte-carlo simulation
			vector<size_t> moves(cur0, cur1 + 1); // we must copy
				// avaliable moves or after shuffling
				// we'll lose track of which one have been
				// checked
			vector<int> tile(size); // display values for debugging
			size_t max; // move with maximum value
			size_t win_count; // count number of wins for black
			size_t max_count = 0; // wins on the best move
			for(auto it = moves.begin(); it != moves.end(); ++it) {
				win_count = 0;
				// do the swap so this move is reflected in
				// the stone vector
				auto pickmove = find(cur0, cur1, *it);
				*pickmove = *cur1;
				*cur1 = *it;
				// do the Monte-Carlo based on this move
				for(int j = 0; j < nshuffles; ++j) {
					shuffle(cur0, cur1, *randengine);
					if(!is_white_winning()) {
						++win_count;
					}
				}
				if(win_count > max_count) {
					max = *it;
					max_count = win_count;
				}
				tile[*it] = win_count;
			}
			// draw the table - for debugging
			cout << "Black move values:\n";
			for(int j = 0; j < size; j++) {
				if(!(j % side)) {
					cout << '\n';
				}
				cout << '\t' << tile[j];
			}
			cout << '\n';
			// make the best move
			auto it = find(cur0, cur1 + 1, max);
			*it = *cur1;
			*cur1 = max;
			// update blackrow
			blackrow[(*cur1) / side] |=
				uint32_t(1) << ((*cur1) % side);
		}
	}
	// human move
	int try_move(unsigned char row, unsigned char col) {
		if(!init_success) {
			return -100;
		}
		// check bounds
		if(row > side || col > side) {
			return -1; // error: one of the values is out of bounds
		}
		// check if this tile is empty
		size_t ind = row * side + col;
		if((blackrow[row] & (uint32_t(1) << col)) ||
			(whitecol[col] & (uint32_t(1) << row))) {
			return -2; // error: this tile is not empty
		}
		switch(whites_move) {
		case false:
		{
			// update blackrow:
			blackrow[row] |= uint32_t(1) << col;
			// we must also update stone vector
			auto it = find(cur0, cur1, ind);
			// we must swap two stones
			--cur1;
			*it = *cur1;
			*cur1 = ind;
			break;
		}
		case true:
		{
			// update whitecol:
			whitecol[col] |= uint32_t(1) << row;
			auto it = find(cur0, cur1, ind);
			// we must swap two stones
			*it = *cur0;
			*cur0 = ind;
			cur0++;
			break;
		}
		}
		return 0;
	}
	// main loop
	void play() {
		if(!init_success) {
			return;
		}
		while(winner == ' ' && cur0 != cur1) {
			unsigned short row, col;
			switch((black_ai && !whites_move) ||
				(white_ai && whites_move)) {
			case false:
				// prompt human player to make a move
				if(whites_move) {
					cout << "Your move, O: row col ";
				} else {
					cout << "Your move, X: row col ";
				}
				cin >> row >> col;
				if(int e = try_move(row - 1, col - 1)) {
					switch(e) {
					case -1:
						cout << "Error: out of bounds. "
							<< "Try again" << endl;
						continue;
					case -2:
						cout << "Error: tile is filled."
							<< " Try again" << endl;
						continue;
					default:
						cout << "Fatal error" << endl;
						exit(e);
					}
				}
				break;
			case true:
				// make ai move
				make_move();
				break;
			}
			//print_stones();
			print();
			// check if game is over
			check_game_over(); // sets the winner variable if needed
			// toggle current player
			whites_move = whites_move? false: true;
		}
		if(winner == 'X') {
			cout << "Congratulations to the black player!" << endl;
		} else {
			cout << "Congratulations to the white player!" << endl;
		}
	}
};

main() {
	// first, select board size
	while(true) {
		unsigned short side, p1ai, p2ai;
		cout << "Board side: ";
		cin >> side;
		cout << "Black played by AI? 1=yes, 0=no: ";
		cin >> p1ai;
		cout << "White played by AI? 1=yes, 0=no: ";
		cin >> p2ai;
		Board board(side, p1ai, p2ai);
		board.print();
		board.print_stones();
		board.play();
		break;
	}
	return 0;
}
