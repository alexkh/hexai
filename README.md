hexai
=====

Hex Game AI implementation C++

K, right now it's a messy hax-up of stuff, but the basic functinality is there
and you can see an example of report produced by analyze.cpp at:

https://wizstaff.com/hex/report.html

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
