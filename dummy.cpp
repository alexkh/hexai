// X=black (goes first), O=white, columns represented by lower-case letter,
// black connects top and bottom
#include <iostream>
#include <string>
#include <sstream> // reading integer from string
#include <limits> // streamsize
#include <locale> // isalpha
#include <unistd.h> // sleep() - simulates ai's thinking process
#include <chrono>
using namespace std;

int autoplay(char color, unsigned short board_side = 11, size_t iter = 1000) {
	char column; // letter representing board column from a-z
	unsigned short col; // numeric column
	unsigned short row; // numeric row
	char c; // used for input processing
	bool game_over = false; // each player's responsibility to check winner
	// send handshake message color: name of program by author
	// this string should uniquely identify the player
	cout << color << ": dummy player v.0.2 by Alexandre Kharlamov "
			"https://github.com/alexkh/hexai\n" << flush;
	if(color == 'X') {
		// wait for other player's handshake message
		cin >> c; // should be the other player's color
		if(c != 'O') {
			cout << "X. E: expecting handshake message from O\n"
				<< flush;
			return -2;
		}
		cin >> c; // should be ':'
		if(c != ':') {
			cout << "X. E: expecting : after O in "
				"handshake message\n" << flush;
			return -3;
		}
		// ignore the rest of the line
		cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		// start the timer
		auto start = std::chrono::steady_clock::now();
		// make a move
		sleep(1);
		// stop the timer
		auto end = std::chrono::steady_clock::now();
		int tmilli = std::chrono::duration<double, std::milli>
			(end - start).count();
		cout << color << "a2 #1 t=" << tmilli << "ms\n" << flush;
	}
	int counter = 1; // count the moves
	while(true) {
		cin >> c; // other player color
		cin >> column; // lower case letter represenging board column
		if(c != (color=='O'? 'X': 'O') || column == ':') {
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			continue;
		}
		if(column == '.') { // the other player quits, game over
			break;
		}
		col = column - 'a';
		if(col >= board_side) {
			cout << color <<  ". E: " << color <<
			" received illegal column: '" << c << "'\n";
			return -4;
		}
		cin >> row;
		if(row > board_side) {
			cout << color << ". E: " << color <<
			" received illegal row: '" << row << "'\n";
			return -5;
		}
		c = cin.peek();
		if(c == '.') { // dot at the end of the other player's move
			// means that he wins, or maybe he gives up - game over
			break;
		}
		cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		// start the timer
		auto start = std::chrono::steady_clock::now();
		if(color == 'X') {
			++counter;
		}
		// register opponent's move
		// check if game is over
		if(counter > 5) { game_over = true; }; // dummy code exits
							// after 5 moves
		if(game_over) {
			break;
		}
		// make a move. If I won, add a dot. dummy exits after 3 moves.
		sleep(1);
		// check if game is over
		if(counter >= 5) { game_over = true; }; // dummy code simplified
		// stop the timer
		auto end = std::chrono::steady_clock::now();
		int tmilli = std::chrono::duration<double, std::milli>
			(end - start).count();
		cout << color << "b3" << (game_over? '.': ' ') << '#' << counter
			<< " t=" << tmilli << "ms\n" << flush;
		if(game_over) {
			break;
		}
		if(color == 'O') {
			++counter;
		}
	}
	return 0;
}

// usage: <program name> (X|O) [<board side>] [<iterations>]
// example: hex X 11 1000
int main(int argc, char *argv[]) {
	char color = 'X'; // can be X or O
	unsigned short board_side = 11; // side of the board minimum 3
	size_t iter = 1000; // number of iterations should be selectable
	// parse command line parameters
	argc = argc > 4? 4: argc; // forward compatibility measure
	switch(argc) {
	case 4:
	{
		stringstream ss; // used for reading numbers from strings
		ss << argv[3]; // put third argument into the stringstream
		ss >> iter; // read its numeric value
	}
	case 3:
	{
		stringstream ss;
		ss << argv[2];
		ss >> board_side;
		board_side = board_side < 3? 3: board_side; // minimum 3
	}
	case 2:
		color = argv[1][0];
		if(color != 'X' && color != 'O') {
			cerr << "E: first argument must be X or O\n";
			return -1; // there is some error
		}
		autoplay(color, board_side, iter);
		return 0;
	case 1: ; // no command line arguments - continue with interactive play
	}
	// optional interactive play code goes here
	return 0;
}
