// Hex playing program by Boris Kaul
// adapted for autoplay by Alexandre Kharlamov
#include <array>
#include <algorithm>
#include <cinttypes>
#include <memory>
#include <iostream>
#include <iomanip>
#include <utility>
#include <random>
#include <cstring>
#include <cstdio>
#include <chrono>
#include <sstream>
#include <limits>
#include <locale>
using namespace std;

template<int Size>
using PlayerState = std::array<uint16_t, Size>;

template<int Size>
bool _isEndGame(const PlayerState<Size>& player_state, uint32_t player) noexcept {
  enum State {
    LinkDown,
    LinkUp,
    Capture
  };

  PlayerState<Size> board(player_state);
  PlayerState<Size> shadow_board;
  shadow_board.fill(0);

  uint32_t shadow_row = board[0];
  uint32_t row = 0;

  shadow_board[0] = board[0];
  board[0] = 0;

  State state = State::LinkDown;
  uint32_t row_num = 1;

  uint32_t connections = 0;
  while (true) {
    switch (state) {
    case State::LinkDown: {
      row = board[row_num];
      connections = (row & shadow_row) | ((shadow_row >> 1) & row);

      if (connections) {
        if (row_num == Size - 1)
          return true;

        state = State::Capture;
      } else {
        shadow_row = shadow_board[row_num];
        if (shadow_row) {
          row_num++;
        } else {
          return false;
        }
      }
      break;
    }
    case State::LinkUp: {
      row = board[row_num];
      connections = (row & shadow_row) | ((shadow_row << 1) & row);
      if (connections) {
        state = State::Capture;
      } else {
        shadow_row = shadow_board[row_num];
        row_num++;
        state = State::LinkDown;
      }
      break;
    }
    case State::Capture: {
      uint32_t capture = 0;

      for (uint32_t col_num = __builtin_ctz(connections);
           connections;
           col_num = __builtin_ctz(connections)) {

        uint32_t xrow = ~row;
        uint32_t rrow = (xrow >> col_num) | (xrow << (32 - col_num)); // ror
        uint32_t left_bits_count = __builtin_ctz(rrow);
        uint32_t right_bits_count = __builtin_clz(rrow) + 1;
        uint32_t left_bits_mask = (1 << left_bits_count) - 1;
        uint32_t right_bits_mask = (1 << (right_bits_count - 1)) - 1;
        capture |= left_bits_mask << col_num;
        capture |= (right_bits_mask << (col_num - right_bits_count + 1));

        connections ^= left_bits_mask << col_num;
      }

      board[row_num] ^= capture;
      shadow_board[row_num] |= capture;
      shadow_row = shadow_board[row_num];

      if (board[row_num-1]) {
        row_num--;
        state = State::LinkUp;
      } else {
        row_num++;
        state = State::LinkDown;
      }
      break;
    }
    }
  }
}


template<int Size>
class AIBitBoard {
public:
  static_assert(Size >= 3 && Size <= 16, "Size should be in the range [3, 16]");

  static const uint32_t size = Size;

  void setState(const PlayerState<size>& s) noexcept {
    _state = s;
  }

  void toggle(uint32_t row, uint32_t col) noexcept {
    _state[row] |= 1 << col;
  }

  bool isEndGame(uint32_t player) const noexcept {
    return _isEndGame<size>(_state, player);
  }

private:
  PlayerState<size> _state;
};


template<int Size>
class BitBoard {
public:
  static_assert(Size >= 3 && Size <= 16, "Size should be in the range [3, 16]");

  static const uint32_t size = Size;

  BitBoard() {
    reset();
  }

  void reset() noexcept {
    _players[0].fill(0);
    _players[1].fill(0);
  }

  void toggle(uint32_t id, uint32_t player) noexcept {
    uint32_t row, col;
    if (player) {
      row = id % size;
      col = id / size;
    } else {
      row = id / size;
      col = id % size;
    }
    _players[player][row] |= 1 << col;
  }

  bool isToggled(uint32_t id) const noexcept {
    uint32_t row = id / size;
    uint32_t col = id % size;

    return (((_players[0][row] >> col) & 1) |
            ((_players[1][col] >> row) & 1));
  }

  bool isEndGame(uint32_t player) const noexcept {
    return _isEndGame<11>(_players[player], player);
  }

  const PlayerState<size>& getPlayerState(uint32_t player) const noexcept {
    return _players[player];
  }

  template<int S>
  friend std::ostream& operator<<(std::ostream& o, const BitBoard<S>& b) noexcept;

private:
  PlayerState<size> _players[2];
};

/*
  Print board to stream
 */
template<int Size>
std::ostream& operator<<(std::ostream& o, const BitBoard<Size>& b) noexcept {
  o << ' ';
  for (uint32_t i = 0; i < Size; ++i) {
    o << ' ' << static_cast<char>('A' + i) << ' ';
  }
  o << std::endl;

  for (uint32_t i = 0; i < Size; ++i) {
    // indentation
    for (int s = 0; s < i; ++s) {
      o << "  ";
    }
    o << std::setw(2) << i+1;

    for (int j = 0; j < Size; ++j) {
      o << ' ';
      if (b._players[0][i] >> j & 1) {
        o << 'X';
      } else if (b._players[1][j] >> i & 1) {
        o << 'O';
      } else {
        o << '.';
      }
      o << ' ';
    }

    o << std::setw(2) << i+1 << std::endl;
  }

  // indentation
  o << ' ';
  for (int s = 0; s < Size; ++s) {
    o << "  ";
  }
  for (int i = 0; i < Size; ++i) {
    o << ' ' << static_cast<char>('A' + i) << ' ';
  }
  o << std::endl;

  return o;
}

template<int Size>
class GameBoard {
public:
  static const uint32_t size = Size;

  GameBoard() : _board() {
    reset();
  }

  void reset() noexcept {
    std::iota(std::begin(_freeNodes), std::end(_freeNodes), 0);
    _freeNodesIndex = _freeNodes;
  }

  bool toggle(uint32_t id, uint32_t player) noexcept {
    if (_board.isToggled(id)) {
      return false;
    }

    _board.toggle(id, player);
    id = _freeNodesIndex[id];
    std::swap(_freeNodes[id], _freeNodes[--_freeNodesCount]);
    std::swap(_freeNodesIndex[id], _freeNodesIndex[_freeNodesCount]);

    return true;
  }

  bool toggle(uint32_t row, uint32_t col, uint32_t player) noexcept {
    return toggle(row * size + col);
  }

  bool isToggled(uint32_t id) const noexcept {
    return _board.isToggled(id);
  }

  bool isEndGame(uint32_t player) {
    return _board.isEndGame(player);
  }

  const BitBoard<size>& getBoard() const noexcept {
    return _board;
  }

  const std::array<uint16_t, size * size>& getFreeNodes() const noexcept {
    return _freeNodes;
  }

  uint32_t getFreeNodesCount() const noexcept {
    return _freeNodesCount;
  }

  template<int S>
  friend std::ostream& operator<<(std::ostream& os, const GameBoard<S>& b) noexcept;

private:
  BitBoard<size> _board;
  std::array<uint16_t, size * size> _freeNodes;
  std::array<uint16_t, size * size> _freeNodesIndex;
  uint32_t _freeNodesCount = size * size;
};

template<int Size>
std::ostream& operator<<(std::ostream& os, const GameBoard<Size>& b) noexcept {
  os << b._board;
  os << std::endl;
  os << std::endl;

  return os;
}

template<class B>
class MonteCarloAI {
private:
  struct Position {
    union {
      struct {
        uint8_t row;
        uint8_t col;
      };
      uint16_t v;
    };

    bool operator==(const Position& o) const noexcept {
      return v == o.v;
    }
  };

public:
  using gameBoardType = B;
  static const uint32_t boardSize = B::size;

  MonteCarloAI(uint32_t seed) : _rng(seed) {}

  uint32_t getNextMove(gameBoardType& board, uint32_t player, uint32_t iterations) {
    auto start = std::chrono::steady_clock::now();

    uint32_t free_nodes_count = board.getFreeNodesCount();
    Position free_nodes[free_nodes_count];
    Position free_nodes_copy[free_nodes_count];

    auto& nodes = board.getFreeNodes();

    for (int i = 0; i < free_nodes_count; ++i) {
      uint32_t id = nodes[i];
      uint8_t row, col;
      if (player) {
        row = id % boardSize;
        col = id / boardSize;
      } else {
        row = id / boardSize;
        col = id % boardSize;
      }
      free_nodes[i].row = row;
      free_nodes[i].col = col;
    }

    std::memcpy(&free_nodes_copy[0],
                &free_nodes[0],
                free_nodes_count * sizeof(Position));

    AIBitBoard<boardSize> b;
    uint32_t max_wins = 0;
    Position win_pos = free_nodes_copy[0];

    typedef std::uniform_int_distribution<uint16_t> distr_type;
    typedef distr_type::param_type distr_param;

    distr_type distr;
    uint32_t moves = (free_nodes_count - 2) / 2 + player;

    for (uint32_t p = 0; p < free_nodes_count; ++p) {
      uint32_t wins = 0;
      uint32_t possible_wins = iterations;
      Position pos = free_nodes_copy[p];

      for (uint32_t j = 0; j < iterations; ++j) {
        b.setState(board.getBoard().getPlayerState(player));
        b.toggle(pos.row, pos.col);

        for (uint32_t k = 0; k < moves; ++k) {
          using std::swap;

          uint32_t kpos = distr(_rng, distr_param(0, (free_nodes_count - 1) - k));
          Position id = free_nodes[kpos];
          swap(free_nodes[kpos], free_nodes[(free_nodes_count - 1) - k]);

          if (pos == id) {
            k--;
            continue;
          }

          b.toggle(id.row, id.col);

        }
        if (b.isEndGame(player)) {
          wins++;
        } else {
          possible_wins--;
        }

        if (possible_wins < max_wins) {
          goto end_loop;
        }
      }

      if (wins > max_wins) {
        win_pos = pos;
        max_wins = wins;
      }

    end_loop: {
      }
    }

    auto end = std::chrono::steady_clock::now();

    auto diff = end - start;
//    std::cout << "time:" << std::chrono::duration<double, std::milli>(diff).count() << std::endl;

    if (player)
      return win_pos.col * boardSize + win_pos.row;
    else
      return win_pos.row * boardSize + win_pos.col;
  }

private:
  std::mt19937 _rng;
};


bool _askYesOrNo(std::string msg) {
  std::string line;

  while (true) {
    std::cout << msg;
    std::cout << " [y/n] ";
    std::getline(std::cin, line);
    std::istringstream iss(line);
    char answer;
    iss >> answer;

    answer |= 0x20;
    if (answer == 'y') {
      return true;
    } else if (answer == 'n') {
      return false;
    }
  }
}

uint32_t _askBotDifficulty() {
  std::cout << "Bot Difficulty:\n";
  std::cout << " 1. Easy\n";
  std::cout << " 2. Normal\n";
  std::cout << " 3. Hard\n";
  std::cout << " 4. Insane\n";

  std::string line;
  while (true) {
    std::cout << "[1/2/3/4]: ";
    std::getline(std::cin, line);
    std::istringstream iss(line);
    int mode;
    iss >> mode;
    if (mode > 0 && mode < 5)
      return mode;
  }
}

/*
  Human Player
 */
template<class Board>
class Player {
public:
  explicit Player(uint32_t player_id, Board& b) : _id(player_id), _board(b) {}

	virtual void set_trials(uint32_t trials) {};

  virtual uint32_t askMove() {
    char colc;
    int32_t rowc;

    std::string line;
    while (true) {
      std::getline(std::cin, line);
      std::istringstream iss(line);
      iss >> colc >> rowc;
      int32_t row, col;
      col = colc - 'A';
      row = rowc - 1;

      if (col >= 0 && col < Board::size && row >= 0 && row < Board::size)
        return (row * Board::size + col);
      else
        std::cout << "Invalid position\n";
    }
  }

protected:
  uint32_t _id;
  Board& _board;
};

/*
  Hex game Bot
 */
template<class Board>
class HexBot : public Player<Board> {
public:
  HexBot(uint32_t player_id, Board& b, uint32_t difficulty, uint32_t seed)
      : Player<Board>(player_id, b), _ai(seed) {
    switch (difficulty) {
    case 1:
      _trials = 100;
      break;
    case 2:
      _trials = 1000;
      break;
    case 3:
      _trials = 5000;
      break;
    case 4:
      _trials = 40000;
      break;
    }
  }

	void set_trials(uint32_t trials) {
		_trials = trials;
	}

  virtual uint32_t askMove() {
    return _ai.getNextMove(Player<Board>::_board, Player<Board>::_id, _trials);
  }

protected:
  MonteCarloAI<Board> _ai;
  uint32_t _trials;
};

/*
  Main Game Object
 */
template<int Size>
class Game {
public:
  typedef GameBoard<Size> gameBoardType;
  typedef MonteCarloAI<gameBoardType> aiType;
  typedef Player<gameBoardType> playerType;
  typedef HexBot<gameBoardType> botType;

  /*
    Game state
   */
  enum class State : uint8_t {
    Menu,
    Game,
    EndGame,
    Shutdown
  };

  /*
    Game Constructor
   */
  Game(uint32_t seed) {}

  /*
    Run Game
   */
  void run() {
    while (_state != State::Shutdown) {
      // clear screen
      //std::cout << "\x1B[2J\x1B[H";

      switch (_state) {
      case State::Menu: {
        std::cout << "Welcome to the HEX game\n";

        uint32_t second = _askYesOrNo("Do you want to play first?");
        uint32_t first = second ^ 1;
        _players[first].reset(new playerType(first, _board));
        auto difficulty = _askBotDifficulty();
        _players[second].reset(new botType(second, _board, difficulty, 0));
        _state = State::Game;

        break;
      }
      case State::Game: {
        _printInfo();
        std::cout << _board;

        std::cout << std::endl;

        std::cout << "Player [" << _currentPlayer + 1 << "] turn\n";
        uint32_t pos;
        while (true) {
          pos = _players[_currentPlayer]->askMove();

          if (_board.isToggled(pos)) {
            std::cout << "This position is already occupied\n";
          } else {
            break;
          }
        }
        _board.toggle(pos, _currentPlayer);
        if (_board.isEndGame(_currentPlayer)) {
          _state = State::EndGame;
        } else {
          _currentPlayer ^= 1;
          _turn += 1;
        }
        break;
      }
      case State::EndGame: {
        _printInfo();
        std::cout << _board;
        std::cout << std::endl;
        std::cout << "Player [" << _currentPlayer + 1 << "] wins\n";
        bool yes = _askYesOrNo("Restart");
        if (yes) {
          _state = State::Menu;
          _reset();
        } else {
          _state = State::Shutdown;
        }
        break;
      }
      case State::Shutdown:
        break;
      }
    }
  }

	int autoplay(char color, unsigned short board_side = 11,
						size_t iter = 1000) {
		uint32_t second = (color == 'O'? 1: 0);
		uint32_t first = second ^ 1;
		_reset();
		_players[first].reset(new playerType(first, _board));
		_players[second].reset(new botType(second, _board, 2, 0));
		_players[second]->set_trials(iter);
		_state = State::Game;
		char column; // letter representing board column from a-z
		unsigned short col; // numeric column
		unsigned short row; // numeric row
		char c; // used for input processing
		size_t move; // result of calling askMove()
		// send handshake message color: name of program by author
		// this string should uniquely identify the player
		cout << color << ": hex by Boris Kaul adapted by AK\n" << flush;
		if(color == 'X') {
			// wait for other player's handshake message
			cin >> c; // should be the other player's color
			if(c != 'O') {
				cout << "X. E: expecting handshake message "
					"from O\n" << flush;
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
			move = _players[_currentPlayer]->askMove();
			_board.toggle(move, _currentPlayer);
			// stop the timer
			auto end = std::chrono::steady_clock::now();
			int tmilli = std::chrono::duration<double, std::milli>
				(end - start).count();
			cout << color << char((move % board_side) + 'a') <<
				(move / board_side + 1) << " #1 t=" <<
				tmilli << "ms\n" << flush;
			_currentPlayer ^= 1;
			_turn += 1;
		}
		int counter = 1; // count the moves
		while(true) {
			cin >> c; // other player color
			cin >> column; // lower case letter represenging
					// board column
			if(c != (color=='O'? 'X': 'O') || column == ':') {
				cin.clear();
				cin.ignore(numeric_limits<streamsize>::max(),
					'\n');
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
			if(c == '.') { // dot at the end of the other player's
				// move means that he wins,
				// or maybe he gives up - game over
				break;
			}
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			// start the timer
			auto start = std::chrono::steady_clock::now();
			// register the opponent's move
			move = (row - 1) * board_side + col;
			if(_board.isToggled(move)) {
				cout << color << ". E: received illegal "
					<< "move " << column << row << '\n';
				return -6;
			}
			_board.toggle(move, _currentPlayer);
			// check if game is over
			if(_board.isEndGame(_currentPlayer)) {
				break;
			}
			_currentPlayer ^= 1;
			_turn += 1;
			if(color == 'X') {
				++counter;
			}
			// make a move. If I won, add a dot.
			move = _players[_currentPlayer]->askMove();
			_board.toggle(move, _currentPlayer);
			bool over = _board.isEndGame(_currentPlayer);
			// stop the timer
			auto end = std::chrono::steady_clock::now();
			int tmilli = std::chrono::duration<double, std::milli>
				(end - start).count();
			cout << color << char((move % board_side) + 'a') <<
				(move / board_side + 1) << (over? '.': ' ') <<
				'#' << counter << " t=" << tmilli <<
				"ms\n" << flush;
			if(over) {
				break;
			}
			_currentPlayer ^= 1;
			_turn += 1;
			if(color == 'O') {
				++counter;
			}
		}
		return 0;

	}

  State getState() const noexcept { return _state; }

private:
  void _reset() {
    _board.reset();
    _currentPlayer = 0;
    _turn = 0;
  }

  void _printInfo() {
    std::cout << "Current turn: " << _turn << std::endl;
    std::cout << "Player 1: X (top-down)\n";
    std::cout << "Player 2: O (left-right)\n";
    std::cout << std::endl;
  }

  gameBoardType _board;
  State _state = State::Menu;
  uint32_t _turn = 0;
  uint32_t _currentPlayer = 0;
  std::unique_ptr<playerType> _players[2];
};

int main(int argc, char* argv[]) {
  std::random_device rd;

  auto g = Game<11>(rd());

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
		if(board_side != 11) {
			cerr << "E: " << color <<
				" can only play on an 11x11 board\n";
			return -100;
		}
		g.autoplay(color, board_side, iter);
		return 0;
	case 1: ; // no command line arguments - continue with interactive play
	}

  g.run();

  return 0;
}
