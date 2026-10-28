# Database design (Microsoft Access)

File: `library.accdb` (created automatically in `data\` next to the program on first run).
Connection: ODBC, driver `Microsoft Access Driver (*.mdb, *.accdb)`, through Qt SQL (`QODBC`).

## Entity relationship

```
 Books (1) ──────< Loans >────── (1) Students        Users  (administrator accounts, no relation)
 BookID PK         LoanID PK        StudentID PK
                   BookID FK
                   StudentID FK
```
One book can appear in many loans, one student can have many loans. A loan with an empty
`ReturnDate` is "currently borrowed"; a loan whose `DueDate` is before today and has no
`ReturnDate` is "overdue". Returned loans are kept (history) so reports stay possible.

## Tables

**Books**

| Field | Access type | Rules |
|---|---|---|
| BookID | TEXT(50) | **Primary key** (unique, case-insensitive in Access) |
| Title | TEXT(255) | required |
| Author | TEXT(255) | required |
| Genre | TEXT(100) | required |
| Copies | LONG | required, never negative (checked by the program) |
| ShelfLocation | TEXT(100) | required |

**Students**

| Field | Access type | Rules |
|---|---|---|
| StudentID | TEXT(50) | **Primary key** |
| FullName | TEXT(150) | required |
| Department | TEXT(100) | optional |
| Phone | TEXT(30) | optional |
| Email | TEXT(150) | optional |

**Loans**

| Field | Access type | Rules |
|---|---|---|
| LoanID | COUNTER (AutoNumber) | **Primary key** |
| BookID | TEXT(50) | **Foreign key** -> Books.BookID |
| StudentID | TEXT(50) | **Foreign key** -> Students.StudentID |
| IssueDate | DATETIME | required |
| DueDate | DATETIME | required (IssueDate + 7 days) |
| ReturnDate | DATETIME | empty until the book is returned |

**Users** (administrators)

| Field | Access type | Rules |
|---|---|---|
| UserID | COUNTER | **Primary key** |
| Username | TEXT(50) | required, unique |
| PasswordHash | TEXT(64) | SHA-256 of salt + password |
| Salt | TEXT(32) | random per user |
| FullName | TEXT(100) | optional |

Default administrator created on first run: **salah / root** (same as the original Python project).

## SQL to create the tables manually (optional)

The program creates all of this by itself. If you prefer to build the database in Access
yourself: *Create > Query Design > SQL View*, paste **one statement at a time**, run it.

```sql
CREATE TABLE Books (
  BookID TEXT(50) NOT NULL, Title TEXT(255) NOT NULL, Author TEXT(255) NOT NULL,
  Genre TEXT(100) NOT NULL, Copies LONG NOT NULL, ShelfLocation TEXT(100) NOT NULL,
  CONSTRAINT pk_Books PRIMARY KEY (BookID));

CREATE TABLE Students (
  StudentID TEXT(50) NOT NULL, FullName TEXT(150) NOT NULL, Department TEXT(100),
  Phone TEXT(30), Email TEXT(150),
  CONSTRAINT pk_Students PRIMARY KEY (StudentID));

CREATE TABLE Users (
  UserID COUNTER, Username TEXT(50) NOT NULL, PasswordHash TEXT(64) NOT NULL,
  Salt TEXT(32) NOT NULL, FullName TEXT(100),
  CONSTRAINT pk_Users PRIMARY KEY (UserID),
  CONSTRAINT uq_Users_Username UNIQUE (Username));

CREATE TABLE Loans (
  LoanID COUNTER, BookID TEXT(50) NOT NULL, StudentID TEXT(50) NOT NULL,
  IssueDate DATETIME NOT NULL, DueDate DATETIME NOT NULL, ReturnDate DATETIME,
  CONSTRAINT pk_Loans PRIMARY KEY (LoanID),
  CONSTRAINT fk_Loans_Books FOREIGN KEY (BookID) REFERENCES Books (BookID),
  CONSTRAINT fk_Loans_Students FOREIGN KEY (StudentID) REFERENCES Students (StudentID));
```

## Report queries (for Access and Power BI)

The program tries to create these two saved queries itself. If they are missing in Access
(*Navigation pane > Queries*), create them in SQL View (they are Access-only SQL, because of `Date()`):

```sql
-- qryBorrowedBooks
SELECT l.LoanID, l.BookID, b.Title, l.StudentID, s.FullName, l.IssueDate, l.DueDate
FROM (Loans AS l INNER JOIN Books AS b ON l.BookID = b.BookID)
     INNER JOIN Students AS s ON l.StudentID = s.StudentID
WHERE l.ReturnDate IS NULL;

-- qryOverdueBooks
SELECT l.LoanID, l.BookID, b.Title, l.StudentID, s.FullName, l.IssueDate, l.DueDate,
       DateDiff('d', l.DueDate, Date()) AS DaysOverdue
FROM (Loans AS l INNER JOIN Books AS b ON l.BookID = b.BookID)
     INNER JOIN Students AS s ON l.StudentID = s.StudentID
WHERE l.ReturnDate IS NULL AND l.DueDate < Date();
```

## Power BI (only if your instructor requires it - please confirm)

Power BI reads the same Access file; nothing in the program changes.

1. Install the 64-bit *Microsoft Access Database Engine* (Power BI Desktop is 64-bit).
2. Power BI Desktop > **Get data > Access database** > choose `library.accdb`.
3. Tick `Books`, `Students`, `Loans`, `qryBorrowedBooks`, `qryOverdueBooks` > **Load**.
4. Relationships: `Loans[BookID] -> Books[BookID]` and `Loans[StudentID] -> Students[StudentID]`.
5. Suggested visuals: cards (titles, copies, borrowed, overdue), bar chart of loans per genre,
   table of overdue books with `DaysOverdue`, line chart of loans by `IssueDate`.
6. After the library changes data, press **Refresh** in Power BI.

The *Export CSV* button on the Reports page is an alternative: it writes UTF-8 CSV that opens in Excel and Power BI.
