hexai
=====

Hex Game AI implementation C++

Warning: this code is faulty. Don't use it! It was speed-coded to try some
ideas while taking Ira Pohl's C++ online course, but there is a major flaw
in the chosen approach, which is not fixable without a major code rewrite.

Just run ./buildall on a Linux system with bash shell to build all programs.

There are two different hex implementations: hexai.cpp by me and hex.cpp by
Boris Kaul. A little bash script 'arbi' is used to make them play each other
by connecting their output and input directly to each other and copying them
to a log file. Here is how I'd use arbi:

./arbi ./hex ./hexai log3.txt 2000 1000

To play same game 100 times repeatedly I just run this bash one-liner:

for i in {1..100}; do ./arbi ./hex ./hexai log3.txt 2000 1000; done

K, so hex will play black, hexai - white. log3.txt is our log file.
The last two numbers are iteration. Since hex is a bit faster than hexai,
I doubled its number of iterations to make the time more even.

Now, once the log file is produced, I can generate a report file by running
analyze program:

./analyze log3.txt report/report3.html

where log3.txt is the input file produced by arbi and report3.html is the output
file that needs to be placed into same dir as the files it depends on:
* jquery.js
* raphael.js
* hexreport.js
* hexreport.css

All arbi does is records stdout of both programs to a log file without any
checking. Analyze program is a slightly modified hexai which does the deep
testing of each move, and determines who wins in every game saved to the log.

For those who want to join the competition and modify their code so that it's
compatible:

Basically, protocol is simple:
first, you send out a handshake in the form of color (X or O) followed
by : (without space between), a space, name of your program by author,
and maybe also the github address to identify your program in the tournament.
For example:
O: hexai by Alexandre Kharlamov https://github.com/alexkh/hexai

Your program must also recognize the opponent's handshake and happily ignore it.

Each move starts with your color (X or O), column letter and row number without
spaces. After that goes a space, move number (it is possible that moves register
out of order in the log file!) and time according to this scheme:

Xa10 #14 t=358ms
Oa11.#14 t=330ms

Note that a dot after the move means the game is over, and the other party
should quit too.

At any point any player can stop the game by sending:

X.
or
O.

depending on the color. This usually happens when the other program finds an
error. The other party should quit immediately. The color letter and error
message might follow after O. or X. example:

O. E: received illegal move a10

and quit immediately so that next game can start right away. This current game
will be considered unfinished, but this unfortunate event should not block the
tournament.

If you receive a line that does not start with X or O you should ignore that
entire line.

That's all there is for the protocol. It's implemented in the dummy.cpp:
https://github.com/alexkh/hexai/blob/master/dummy.cpp

Your program should recognize 3 command line parameters:
1) color letter (capital X or capital O) = required argument for autoplay
2) board side (11 by default)
3) number of iterations (1000 by default)

We use regular cout and cin for communication because our bash script arbi
connects the pipes of the two programs in a circular way so we don't have to
worry about all that.
