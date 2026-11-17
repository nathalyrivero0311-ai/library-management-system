/*
 * Library Management System
 * Programming 1 - Final Project (solo)
 * Author: Nathaly Rivero
 *
 * A console-based program a librarian can use to keep track of books,
 * members, and checkouts. Data is saved to a text file so it is still
 * there the next time the program runs.
 *
 * Topics covered: arrays, pointers, control statements and loops,
 * functions, structures, classes and objects, exceptions, file I/O.
 *
 * Compile:  g++ -std=c++17 -Wall -Wextra -o library main.cpp
 * Run:      ./library
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <ctime>
#include <cstdlib>

using namespace std;

// ============================================================
//  Constants
// ============================================================
const int MAX_LOANS = 3;                  // most books one member can hold
const int LOAN_DAYS = 14;                 // how long a loan lasts
const double LATE_FEE_PER_DAY = 0.25;     // dollars per day overdue
const string DATA_FILE = "library_data.txt";
const string LIBRARIAN_PASSWORD = "books123";
const int MAX_LOGIN_ATTEMPTS = 3;

// ============================================================
//  Custom exception (used whenever a library rule is broken)
// ============================================================
class LibraryException : public runtime_error {
public:
    explicit LibraryException(const string& message)
        : runtime_error(message) {}
};

// ============================================================
//  Structure: Date
// ============================================================
struct Date {
    int month;
    int day;
    int year;
};

bool isLeapYear(int year) {
    if (year % 400 == 0) return true;
    if (year % 100 == 0) return false;
    return year % 4 == 0;
}

int daysInMonth(int month, int year) {
    switch (month) {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12:
            return 31;
        case 4: case 6: case 9: case 11:
            return 30;
        case 2:
            return isLeapYear(year) ? 29 : 28;
        default:
            return 0;
    }
}

bool isValidDate(const Date& d) {
    if (d.year < 1970 || d.year > 2200) return false;
    if (d.month < 1 || d.month > 12) return false;
    if (d.day < 1 || d.day > daysInMonth(d.month, d.year)) return false;
    return true;
}

// Turns a date into "number of days since 01/01/1970" so that we can
// add days and subtract dates with plain integer math.
long toDayNumber(const Date& d) {
    int y = d.year;
    int m = d.month;
    if (m <= 2) y--;
    long era = (y >= 0 ? y : y - 399) / 400;
    long yearOfEra = y - era * 400;
    long monthPart = (m > 2) ? m - 3 : m + 9;
    long dayOfYear = (153 * monthPart + 2) / 5 + d.day - 1;
    long dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + dayOfEra - 719468;
}

// The reverse of toDayNumber()
Date fromDayNumber(long z) {
    z += 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    long dayOfEra = z - era * 146097;
    long yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524
                      - dayOfEra / 146096) / 365;
    long y = yearOfEra + era * 400;
    long dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    long monthPart = (5 * dayOfYear + 2) / 153;
    long d = dayOfYear - (153 * monthPart + 2) / 5 + 1;
    long m = (monthPart < 10) ? monthPart + 3 : monthPart - 9;
    if (m <= 2) y++;

    Date result;
    result.year = (int)y;
    result.month = (int)m;
    result.day = (int)d;
    return result;
}

Date addDays(const Date& d, int days) {
    return fromDayNumber(toDayNumber(d) + days);
}

// How many days from "from" until "to" (negative if "to" is earlier)
int daysBetween(const Date& from, const Date& to) {
    return (int)(toDayNumber(to) - toDayNumber(from));
}

string dateToString(const Date& d) {
    ostringstream out;
    out << setfill('0') << setw(2) << d.month << "/"
        << setw(2) << d.day << "/" << setw(4) << d.year;
    return out.str();
}

Date getSystemDate() {
    time_t now = time(nullptr);
    tm* local = localtime(&now);   // pointer to a tm structure
    Date d;
    d.month = local->tm_mon + 1;
    d.day = local->tm_mday;
    d.year = local->tm_year + 1900;
    return d;
}

// ============================================================
//  String and input helper functions
// ============================================================
string trim(const string& text) {
    size_t start = 0;
    while (start < text.size() && isspace((unsigned char)text[start])) start++;
    size_t end = text.size();
    while (end > start && isspace((unsigned char)text[end - 1])) end--;
    return text.substr(start, end - start);
}

string toLower(string text) {
    for (size_t i = 0; i < text.size(); i++) {
        text[i] = (char)tolower((unsigned char)text[i]);
    }
    return text;
}

bool containsIgnoreCase(const string& text, const string& key) {
    return toLower(text).find(toLower(key)) != string::npos;
}

vector<string> split(const string& text, char delimiter) {
    vector<string> parts;
    string current;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current += text[i];
        }
    }
    parts.push_back(current);
    return parts;
}

// Cuts a string down so it fits in a table column
string fit(const string& text, size_t width) {
    if (text.size() <= width) return text;
    return text.substr(0, width - 3) + "...";
}

string money(double amount) {
    ostringstream out;
    out << "$" << fixed << setprecision(2) << amount;
    return out.str();
}

void printLine(char symbol = '-', int count = 74) {
    cout << string(count, symbol) << "\n";
}

string readLine(const string& prompt) {
    cout << prompt;
    string line;
    if (!getline(cin, line)) {
        cout << "\nInput ended. Goodbye!\n";
        exit(0);
    }
    return trim(line);
}

string readNonEmpty(const string& prompt) {
    while (true) {
        string text = readLine(prompt);
        if (!text.empty()) return text;
        cout << "  This can't be empty. Please try again.\n";
    }
}

int readInt(const string& prompt, int minValue, int maxValue) {
    while (true) {
        string text = readLine(prompt);
        try {
            size_t used = 0;
            int value = stoi(text, &used);
            if (used != text.size()) {
                throw invalid_argument("extra characters");
            }
            if (value < minValue || value > maxValue) {
                cout << "  Please enter a number from " << minValue
                     << " to " << maxValue << ".\n";
                continue;
            }
            return value;
        } catch (const exception&) {
            cout << "  That isn't a valid number. Please try again.\n";
        }
    }
}

// Turns "MM/DD/YYYY" into a Date. Returns false if it isn't valid.
bool parseDate(const string& text, Date& result) {
    vector<string> parts = split(text, '/');
    if (parts.size() != 3) return false;
    try {
        Date d;
        d.month = stoi(parts[0]);
        d.day = stoi(parts[1]);
        d.year = stoi(parts[2]);
        if (!isValidDate(d)) return false;
        result = d;
        return true;
    } catch (const exception&) {
        return false;
    }
}

Date readDate(const string& prompt) {
    while (true) {
        string text = readLine(prompt);
        Date d;
        if (parseDate(text, d)) return d;
        cout << "  Please use the format MM/DD/YYYY (for example 12/01/2026).\n";
    }
}

// ============================================================
//  Class: Book
// ============================================================
class Book {
private:
    int id;
    string title;
    string author;
    bool available;
    Date dueDate;
    int borrowerId;      // -1 when nobody has the book

public:
    Book(int bookId, const string& bookTitle, const string& bookAuthor)
        : id(bookId), title(bookTitle), author(bookAuthor),
          available(true), borrowerId(-1) {
        dueDate.month = 0;
        dueDate.day = 0;
        dueDate.year = 0;
    }

    int getId() const { return id; }
    string getTitle() const { return title; }
    string getAuthor() const { return author; }
    bool isAvailable() const { return available; }
    Date getDueDate() const { return dueDate; }
    int getBorrowerId() const { return borrowerId; }

    void checkOut(int memberId, const Date& due) {
        available = false;
        borrowerId = memberId;
        dueDate = due;
    }

    void giveBack() {
        available = true;
        borrowerId = -1;
        dueDate.month = 0;
        dueDate.day = 0;
        dueDate.year = 0;
    }

    // One line of the data file: id|title|author|available|due|borrower
    string toFileString() const {
        ostringstream out;
        out << id << "|" << title << "|" << author << "|"
            << (available ? 1 : 0) << "|"
            << dueDate.month << "/" << dueDate.day << "/" << dueDate.year
            << "|" << borrowerId;
        return out.str();
    }

    static Book fromFileString(const string& line) {
        vector<string> f = split(line, '|');
        if (f.size() != 6) {
            throw LibraryException("Bad book record in data file.");
        }
        Book b(stoi(f[0]), f[1], f[2]);
        if (f[3] == "0") {
            vector<string> d = split(f[4], '/');
            if (d.size() != 3) {
                throw LibraryException("Bad due date in data file.");
            }
            Date due;
            due.month = stoi(d[0]);
            due.day = stoi(d[1]);
            due.year = stoi(d[2]);
            b.checkOut(stoi(f[5]), due);
        }
        return b;
    }
};

// ============================================================
//  Class: Member (uses a fixed-size array to hold current loans)
// ============================================================
class Member {
private:
    int id;
    string name;
    int loans[MAX_LOANS];    // IDs of books this member has out
    int loanCount;

public:
    Member(int memberId, const string& memberName)
        : id(memberId), name(memberName), loanCount(0) {
        for (int i = 0; i < MAX_LOANS; i++) {
            loans[i] = -1;
        }
    }

    int getId() const { return id; }
    string getName() const { return name; }
    int getLoanCount() const { return loanCount; }
    const int* getLoans() const { return loans; }   // returns a pointer
    bool canBorrow() const { return loanCount < MAX_LOANS; }

    // Walks through the array with a pointer instead of an index
    bool hasBook(int bookId) const {
        const int* current = loans;
        const int* end = loans + loanCount;
        while (current != end) {
            if (*current == bookId) return true;
            current++;
        }
        return false;
    }

    void addLoan(int bookId) {
        if (!canBorrow()) {
            throw LibraryException("This member already has the maximum number of books.");
        }
        loans[loanCount] = bookId;
        loanCount++;
    }

    void removeLoan(int bookId) {
        int position = -1;
        for (int i = 0; i < loanCount; i++) {
            if (loans[i] == bookId) {
                position = i;
                break;
            }
        }
        if (position == -1) {
            throw LibraryException("This member doesn't have that book.");
        }
        // shift everything after it one spot to the left
        for (int i = position; i < loanCount - 1; i++) {
            loans[i] = loans[i + 1];
        }
        loanCount--;
        loans[loanCount] = -1;
    }

    // One line of the data file: id|name|count|bookId,bookId,...
    string toFileString() const {
        ostringstream out;
        out << id << "|" << name << "|" << loanCount << "|";
        for (int i = 0; i < loanCount; i++) {
            if (i > 0) out << ",";
            out << loans[i];
        }
        return out.str();
    }

    static Member fromFileString(const string& line) {
        vector<string> f = split(line, '|');
        if (f.size() != 4) {
            throw LibraryException("Bad member record in data file.");
        }
        Member m(stoi(f[0]), f[1]);
        int count = stoi(f[2]);
        if (count > 0) {
            vector<string> ids = split(f[3], ',');
            for (size_t i = 0; i < ids.size(); i++) {
                m.addLoan(stoi(ids[i]));
            }
        }
        return m;
    }
};

// Used by sort() to put books in alphabetical order by title
bool titleComesFirst(const Book& a, const Book& b) {
    return toLower(a.getTitle()) < toLower(b.getTitle());
}

// ============================================================
//  Class: Library (owns all the books and members)
// ============================================================
class Library {
private:
    vector<Book> books;
    vector<Member> members;
    Date today;
    int nextBookId;
    int nextMemberId;

    // These return a pointer, or nullptr if nothing was found.
    Book* findBook(int bookId) {
        for (size_t i = 0; i < books.size(); i++) {
            if (books[i].getId() == bookId) return &books[i];
        }
        return nullptr;
    }

    Member* findMember(int memberId) {
        for (size_t i = 0; i < members.size(); i++) {
            if (members[i].getId() == memberId) return &members[i];
        }
        return nullptr;
    }

    bool isOverdue(const Book& b) const {
        return !b.isAvailable() && daysBetween(b.getDueDate(), today) > 0;
    }

    void printBookHeader() const {
        printLine();
        cout << left << setw(6) << "ID" << setw(30) << "Title"
             << setw(20) << "Author" << "Status\n";
        printLine();
    }

    void printBookRow(const Book& b) const {
        cout << left << setw(6) << b.getId()
             << setw(30) << fit(b.getTitle(), 28)
             << setw(20) << fit(b.getAuthor(), 18);
        if (b.isAvailable()) {
            cout << "Available\n";
        } else {
            cout << "Out, due " << dateToString(b.getDueDate());
            if (isOverdue(b)) cout << " (OVERDUE)";
            cout << "\n";
        }
    }

public:
    Library() : today(getSystemDate()), nextBookId(101), nextMemberId(1001) {}

    Date getToday() const { return today; }

    void setToday(const Date& d) {
        today = d;
        cout << "\n  Today's date is now " << dateToString(today) << ".\n";
    }

    // ---------------- books ----------------
    void addBook(const string& title, const string& author) {
        string cleanTitle = title;
        string cleanAuthor = author;
        // '|' is used as a separator in the data file, so swap it out
        replace(cleanTitle.begin(), cleanTitle.end(), '|', '/');
        replace(cleanAuthor.begin(), cleanAuthor.end(), '|', '/');

        for (size_t i = 0; i < books.size(); i++) {
            if (toLower(books[i].getTitle()) == toLower(cleanTitle) &&
                toLower(books[i].getAuthor()) == toLower(cleanAuthor)) {
                throw LibraryException("That book is already in the library (ID "
                                       + to_string(books[i].getId()) + ").");
            }
        }
        books.push_back(Book(nextBookId, cleanTitle, cleanAuthor));
        cout << "\n  Added \"" << cleanTitle << "\" with ID " << nextBookId << ".\n";
        nextBookId++;
    }

    void removeBook(int bookId) {
        for (size_t i = 0; i < books.size(); i++) {
            if (books[i].getId() == bookId) {
                if (!books[i].isAvailable()) {
                    throw LibraryException("That book is checked out right now, so it can't be removed.");
                }
                cout << "\n  Removed \"" << books[i].getTitle() << "\".\n";
                books.erase(books.begin() + i);
                return;
            }
        }
        throw LibraryException("No book with ID " + to_string(bookId) + ".");
    }

    // mode 1 = title, 2 = author, 3 = ID
    void searchBooks(int mode, const string& key) {
        int found = 0;
        bool headerShown = false;
        for (size_t i = 0; i < books.size(); i++) {
            bool match = false;
            if (mode == 1) {
                match = containsIgnoreCase(books[i].getTitle(), key);
            } else if (mode == 2) {
                match = containsIgnoreCase(books[i].getAuthor(), key);
            } else {
                match = (to_string(books[i].getId()) == key);
            }
            if (match) {
                if (!headerShown) {
                    cout << "\n";
                    printBookHeader();
                    headerShown = true;
                }
                printBookRow(books[i]);
                found++;
            }
        }
        if (found == 0) {
            cout << "\n  No books matched \"" << key << "\".\n";
        } else {
            printLine();
            cout << "  " << found << " book(s) found.\n";
        }
    }

    void listBooks() const {
        if (books.empty()) {
            cout << "\n  There are no books in the library yet.\n";
            return;
        }
        int availableCount = 0;
        cout << "\n";
        printBookHeader();
        for (size_t i = 0; i < books.size(); i++) {
            printBookRow(books[i]);
            if (books[i].isAvailable()) availableCount++;
        }
        printLine();
        cout << "  Total: " << books.size() << " | Available: " << availableCount
             << " | Checked out: " << books.size() - availableCount << "\n";
    }

    void sortBooksByTitle() {
        sort(books.begin(), books.end(), titleComesFirst);
        cout << "\n  Books are now sorted by title.\n";
    }

    // ---------------- members ----------------
    void addMember(const string& name) {
        string cleanName = name;
        replace(cleanName.begin(), cleanName.end(), '|', '/');
        members.push_back(Member(nextMemberId, cleanName));
        cout << "\n  Registered " << cleanName << " with member ID "
             << nextMemberId << ".\n";
        nextMemberId++;
    }

    void removeMember(int memberId) {
        for (size_t i = 0; i < members.size(); i++) {
            if (members[i].getId() == memberId) {
                if (members[i].getLoanCount() > 0) {
                    throw LibraryException("This member still has books checked out.");
                }
                cout << "\n  Removed member " << members[i].getName() << ".\n";
                members.erase(members.begin() + i);
                return;
            }
        }
        throw LibraryException("No member with ID " + to_string(memberId) + ".");
    }

    void listMembers() const {
        if (members.empty()) {
            cout << "\n  There are no members yet.\n";
            return;
        }
        cout << "\n";
        printLine();
        cout << left << setw(8) << "ID" << setw(28) << "Name" << "Books out\n";
        printLine();
        for (size_t i = 0; i < members.size(); i++) {
            cout << left << setw(8) << members[i].getId()
                 << setw(28) << fit(members[i].getName(), 26)
                 << members[i].getLoanCount() << " of " << MAX_LOANS << "\n";
        }
        printLine();
        cout << "  " << members.size() << " member(s).\n";
    }

    void showMemberLoans(int memberId) {
        Member* m = findMember(memberId);
        if (m == nullptr) {
            throw LibraryException("No member with ID " + to_string(memberId) + ".");
        }
        cout << "\n  " << m->getName() << " (ID " << m->getId() << ") has "
             << m->getLoanCount() << " book(s) checked out.\n";
        if (m->getLoanCount() == 0) return;

        printBookHeader();
        const int* loanList = m->getLoans();
        for (int i = 0; i < m->getLoanCount(); i++) {
            Book* b = findBook(*(loanList + i));    // pointer arithmetic
            if (b != nullptr) printBookRow(*b);
        }
        printLine();
    }

    // ---------------- checkout and return ----------------
    void checkOutBook(int memberId, int bookId) {
        Member* m = findMember(memberId);
        if (m == nullptr) {
            throw LibraryException("No member with ID " + to_string(memberId) + ".");
        }
        Book* b = findBook(bookId);
        if (b == nullptr) {
            throw LibraryException("No book with ID " + to_string(bookId) + ".");
        }
        if (!b->isAvailable()) {
            throw LibraryException("\"" + b->getTitle() + "\" is already checked out.");
        }
        if (!m->canBorrow()) {
            throw LibraryException(m->getName() + " already has " + to_string(MAX_LOANS)
                                   + " books, which is the limit.");
        }
        // members with an overdue book can't borrow more
        const int* loanList = m->getLoans();
        for (int i = 0; i < m->getLoanCount(); i++) {
            Book* held = findBook(loanList[i]);
            if (held != nullptr && isOverdue(*held)) {
                throw LibraryException(m->getName() + " has an overdue book (\""
                                       + held->getTitle() + "\") and must return it first.");
            }
        }

        Date due = addDays(today, LOAN_DAYS);
        b->checkOut(memberId, due);
        m->addLoan(bookId);
        cout << "\n  " << m->getName() << " checked out \"" << b->getTitle() << "\".\n";
        cout << "  Due date: " << dateToString(due) << " (" << LOAN_DAYS << " days).\n";
    }

    void returnBook(int bookId) {
        Book* b = findBook(bookId);
        if (b == nullptr) {
            throw LibraryException("No book with ID " + to_string(bookId) + ".");
        }
        if (b->isAvailable()) {
            throw LibraryException("\"" + b->getTitle() + "\" isn't checked out.");
        }
        Member* m = findMember(b->getBorrowerId());
        int daysLate = daysBetween(b->getDueDate(), today);

        cout << "\n  \"" << b->getTitle() << "\" returned";
        if (m != nullptr) {
            cout << " by " << m->getName();
            m->removeLoan(bookId);
        }
        cout << ".\n";

        if (daysLate > 0) {
            double fee = daysLate * LATE_FEE_PER_DAY;
            cout << "  It was " << daysLate << " day(s) late. Late fee: "
                 << money(fee) << "\n";
        } else {
            cout << "  Returned on time. No fee.\n";
        }
        b->giveBack();
    }

    void overdueReport() const {
        int count = 0;
        double totalFees = 0.0;
        cout << "\n  Overdue report for " << dateToString(today) << "\n";
        printLine();
        for (size_t i = 0; i < books.size(); i++) {
            if (!isOverdue(books[i])) continue;
            int daysLate = daysBetween(books[i].getDueDate(), today);
            double fee = daysLate * LATE_FEE_PER_DAY;
            string borrower = "unknown";
            for (size_t j = 0; j < members.size(); j++) {
                if (members[j].getId() == books[i].getBorrowerId()) {
                    borrower = members[j].getName();
                }
            }
            cout << "  " << fit(books[i].getTitle(), 28) << " | " << borrower
                 << " | " << daysLate << " day(s) late | " << money(fee) << "\n";
            totalFees += fee;
            count++;
        }
        if (count == 0) {
            cout << "  Nothing is overdue.\n";
        } else {
            printLine();
            cout << "  " << count << " overdue book(s). Total fees: "
                 << money(totalFees) << "\n";
        }
    }

    // ---------------- file saving and loading ----------------
    void save(const string& filename) const {
        ofstream out(filename);
        if (!out) {
            throw LibraryException("Could not open " + filename + " for saving.");
        }
        out << "NEXT|" << nextBookId << "|" << nextMemberId << "\n";
        for (size_t i = 0; i < books.size(); i++) {
            out << "B|" << books[i].toFileString() << "\n";
        }
        for (size_t i = 0; i < members.size(); i++) {
            out << "M|" << members[i].toFileString() << "\n";
        }
        cout << "\n  Saved " << books.size() << " book(s) and " << members.size()
             << " member(s) to " << filename << ".\n";
    }

    // Returns false if there is no data file yet
    bool load(const string& filename) {
        ifstream in(filename);
        if (!in) return false;

        books.clear();
        members.clear();
        string line;
        int skipped = 0;
        while (getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            try {
                if (line.compare(0, 5, "NEXT|") == 0) {
                    vector<string> f = split(line, '|');
                    nextBookId = stoi(f.at(1));
                    nextMemberId = stoi(f.at(2));
                } else if (line.compare(0, 2, "B|") == 0) {
                    books.push_back(Book::fromFileString(line.substr(2)));
                } else if (line.compare(0, 2, "M|") == 0) {
                    members.push_back(Member::fromFileString(line.substr(2)));
                } else {
                    skipped++;
                }
            } catch (const exception&) {
                skipped++;
            }
        }
        cout << "  Loaded " << books.size() << " book(s) and " << members.size()
             << " member(s) from " << filename << ".\n";
        if (skipped > 0) {
            cout << "  (" << skipped << " damaged line(s) were skipped.)\n";
        }
        return true;
    }

    // Starter data so the program isn't empty the very first time
    void loadSampleData() {
        addBookQuietly("The Hobbit", "J.R.R. Tolkien");
        addBookQuietly("Charlotte's Web", "E.B. White");
        addBookQuietly("To Kill a Mockingbird", "Harper Lee");
        addBookQuietly("Harry Potter and the Sorcerer's Stone", "J.K. Rowling");
        addBookQuietly("The Great Gatsby", "F. Scott Fitzgerald");
        addBookQuietly("Charlie and the Chocolate Factory", "Roald Dahl");
        members.push_back(Member(nextMemberId++, "Maria Lopez"));
        members.push_back(Member(nextMemberId++, "James Carter"));
        cout << "  No saved data found, so sample data was loaded.\n";
    }

    void addBookQuietly(const string& title, const string& author) {
        books.push_back(Book(nextBookId, title, author));
        nextBookId++;
    }
};

// ============================================================
//  Menu and program flow
// ============================================================
bool login() {
    cout << "  Librarian login\n";
    for (int attempt = 1; attempt <= MAX_LOGIN_ATTEMPTS; attempt++) {
        string typed = readLine("  Password: ");
        if (typed == LIBRARIAN_PASSWORD) {
            cout << "  Welcome!\n";
            return true;
        }
        int left = MAX_LOGIN_ATTEMPTS - attempt;
        cout << "  Wrong password. " << left << " attempt(s) left.\n";
    }
    return false;
}

void showMenu(const Date& today) {
    cout << "\n";
    printLine('=');
    cout << "  LIBRARY MENU                              Today: "
         << dateToString(today) << "\n";
    printLine('=');
    cout << "   1. Add a book              8. Check out a book\n";
    cout << "   2. Remove a book           9. Return a book\n";
    cout << "   3. Search for a book      10. Show a member's books\n";
    cout << "   4. List all books         11. Overdue report\n";
    cout << "   5. Register a member      12. Set today's date (for demo)\n";
    cout << "   6. Remove a member        13. Sort books by title\n";
    cout << "   7. List members           14. Save data\n";
    cout << "   0. Save and exit\n";
    printLine('=');
}

// Runs the option the librarian picked. Returns false when it's time to quit.
bool handleChoice(Library& library, int choice) {
    switch (choice) {
        case 1: {
            string title = readNonEmpty("  Title: ");
            string author = readNonEmpty("  Author: ");
            library.addBook(title, author);
            break;
        }
        case 2:
            library.removeBook(readInt("  Book ID to remove: ", 1, 999999));
            break;
        case 3: {
            cout << "  Search by: 1) Title  2) Author  3) Book ID\n";
            int mode = readInt("  Choose 1-3: ", 1, 3);
            string key = readNonEmpty("  Search for: ");
            library.searchBooks(mode, key);
            break;
        }
        case 4:
            library.listBooks();
            break;
        case 5:
            library.addMember(readNonEmpty("  Member name: "));
            break;
        case 6:
            library.removeMember(readInt("  Member ID to remove: ", 1, 999999));
            break;
        case 7:
            library.listMembers();
            break;
        case 8: {
            int memberId = readInt("  Member ID: ", 1, 999999);
            int bookId = readInt("  Book ID: ", 1, 999999);
            library.checkOutBook(memberId, bookId);
            break;
        }
        case 9:
            library.returnBook(readInt("  Book ID being returned: ", 1, 999999));
            break;
        case 10:
            library.showMemberLoans(readInt("  Member ID: ", 1, 999999));
            break;
        case 11:
            library.overdueReport();
            break;
        case 12:
            library.setToday(readDate("  New date (MM/DD/YYYY): "));
            break;
        case 13:
            library.sortBooksByTitle();
            break;
        case 14:
            library.save(DATA_FILE);
            break;
        case 0:
            library.save(DATA_FILE);
            cout << "\n  Goodbye!\n";
            return false;
        default:
            break;
    }
    return true;
}

int main() {
    printLine('=');
    cout << "      LIBRARY MANAGEMENT SYSTEM  (C++ Programming 1 Project)\n";
    printLine('=');

    if (!login()) {
        cout << "\n  Too many wrong passwords. The program will close.\n";
        return 1;
    }

    Library library;
    if (!library.load(DATA_FILE)) {
        library.loadSampleData();
    }

    bool running = true;
    while (running) {
        showMenu(library.getToday());
        int choice = readInt("  Enter your choice: ", 0, 14);
        try {
            running = handleChoice(library, choice);
        } catch (const LibraryException& error) {
            cout << "\n  [Error] " << error.what() << "\n";
        } catch (const exception& error) {
            cout << "\n  [Unexpected problem] " << error.what() << "\n";
        }
    }
    return 0;
}
