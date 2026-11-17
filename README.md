# Library Management System

A console-based library management system written in C++ for my Programming 1 final project.
A librarian can keep track of books, members, and checkouts in one program, and the data
is saved to a text file so it's still there the next time the program runs.

## Features
- Librarian login (3 tries)
- Add, remove, search (by title, author, or ID), list, and sort books
- Register and remove members
- Check books out with a 14-day due date, and return them
- Automatic late fees ($0.25 per day) and an overdue report
- Saves and loads data from library_data.txt
- Input validation and exception handling so it doesn't crash on bad input

## C++ concepts used
Arrays, pointers, control statements and loops, functions, structures, classes and objects,
exceptions, and file input/output.

## How to compile and run
Requires a C++ compiler such as g++ (C++17).

    g++ -std=c++17 -Wall -Wextra -o library main.cpp
    ./library

On Windows, run `library.exe` instead of `./library`.

The librarian password is `books123`.

## Author
Nathaly Rivero, Programming 1
