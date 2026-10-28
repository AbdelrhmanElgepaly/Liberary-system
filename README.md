# Library Manager (C++ / Qt 6 / Microsoft Access)

Desktop application for managing a university library: books, students, borrowing and returning,
overdue reports. It is a C++ rewrite of the original Python/Tkinter project, with a new interface.

* Language: **C++17**, object-oriented, layered (models / database / services / user interface)
* Interface: **Qt 6 Widgets** + Qt Style Sheet (`resources/style.qss`)
* Database: **Microsoft Access** (`.accdb`) through ODBC (Qt SQL `QODBC`)
* Starts directly in a window - **no console / CMD window** (built as a Windows GUI program)

## Features

| Area | What you can do |
|---|---|
| Sign in | Administrator login (3 attempts), passwords stored as salted hashes |
| Dashboard | Statistics (titles, copies, students, borrowed, overdue), recent activity, overdue list |
| Books | Search (all fields / ID / title / author / genre), add (duplicate ID blocked), edit, delete with confirmation, add or remove copies |
| Students | Search, add, edit, delete with confirmation, number of books each student has |
| Borrow & Return | Choose student + book, 7-day loan, blocked when no copy is left or the student already has the book, return with confirmation |
| Reports | Currently borrowed, overdue, full history; export to CSV (Excel / Power BI) |

## What you need on Windows (nothing is assumed to be installed)

1. **Visual Studio 2022** (Community is free) with the workload *Desktop development with C++*
   (includes CMake). Alternative: any MinGW/MSVC kit that Qt Creator supports.
2. **Qt 6.5 or newer** for MSVC 2019/2022 64-bit, from <https://www.qt.io/download-open-source>
   (Qt Online Installer > Qt 6.x > *MSVC 2019 64-bit* or *MSVC 2022 64-bit*). Qt's SQL ODBC plugin is included.
3. **Access ODBC driver**, same bitness as the program (64-bit):
   * installed with Microsoft Office (check that Office is 64-bit), **or**
   * the free *Microsoft Access Database Engine 2016 Redistributable* (`AccessDatabaseEngine_X64.exe`).
   Microsoft Access itself is *not* required to run the program, only the driver.
   If your Office is 32-bit, either install a 32-bit Qt kit and build 32-bit, or replace that Office with the 64-bit one:
   the driver and the program must have the same bitness.

## Getting the .exe quickly

Two ways, pick one:

1. **No installation: let GitHub build it for you (free).** Create a repository on github.com,
   upload this whole folder (including the hidden `.github` folder), open the **Actions** tab >
   **Build Windows exe** > **Run workflow**. After about 10 minutes download the artifact
   `LibraryManager-windows`: a zip with `LibraryManager.exe` and every DLL it needs.
2. **On your PC:** install Visual Studio 2022 (C++) + Qt 6 (MSVC 64-bit), open
   *x64 Native Tools Command Prompt for VS 2022* in this folder and run `build_windows.bat`.
   The result is the folder `dist` (copy the whole folder, not only the exe).

In both cases the PC that runs the program also needs the 64-bit Access ODBC driver (see below).

## Build and run

### Option A - Qt Creator (simplest)
1. *File > Open File or Project...* > select `CMakeLists.txt`.
2. Choose the kit *Desktop Qt 6.x MSVC 64-bit* > **Configure Project**.
3. Press the green **Run** button.

### Option B - Visual Studio 2022
1. *File > Open > Folder...* > the `LibraryManager` folder.
2. Add a `CMakeSettings.json` / preset with `CMAKE_PREFIX_PATH` = your Qt folder, e.g.
   `C:/Qt/6.7.3/msvc2022_64`, then pick the *x64-Debug* configuration.
3. *Build > Build All*, then run `LibraryManager.exe`.

### Option C - command line (x64 Native Tools prompt)
```
cmake -S . -B build -G "Ninja" -DCMAKE_PREFIX_PATH=C:/Qt/6.7.3/msvc2022_64 -DCMAKE_BUILD_TYPE=Release
cmake --build build
build\LibraryManager.exe
```

### Running outside Qt Creator / giving the program to someone else
```
C:\Qt\6.7.3\msvc2022_64\bin\windeployqt.exe build\LibraryManager.exe
```
This copies the Qt DLLs and the `sqldrivers\qsqlodbc.dll` plugin next to the exe.

## First run

* The database is created automatically: `data\library.accdb` next to the exe (tables included).
  To use another file: `LibraryManager.exe D:\path\mylibrary.accdb`.
* Login: **salah / root** (as in the original project). Change it for real use.
* Want sample books, students and loans (including overdue ones) to see the screens filled?
  Start once with `LibraryManager.exe --demo` (only fills an *empty* database).
* No console window appears: the executable is linked with the Windows GUI subsystem
  (`WIN32` in `CMakeLists.txt`). Error messages are shown in message boxes.

## Project structure

```
src/models/    Book, Student, Loan            data classes + validation rules
src/db/        DatabaseManager                connection to Access, SQL helper, transactions, schema
               Book/Student/Loan/UserRepository   all SQL of one table each
src/services/  LibraryService                 business rules (borrow, return, delete rules, stats)
               AuthService                    login, password hashing
src/ui/        MainWindow + pages, dialogs    the graphical interface (never talks to SQL directly)
resources/     style.qss, images/, icons/     styling and artwork
tools/         make_assets.py                 regenerates every image (Pillow)
docs/          DATABASE.md                    tables, relations, SQL, Power BI notes
```
`LibraryCore` (models + db + services) contains no GUI code; `LibraryUi` depends on it.

## Images

All pictures (login/dashboard artwork, emblem, icons) are generated by `tools/make_assets.py`,
so they are original and free of any logo or copyright. To use your own photo or institute logo,
replace `resources/images/login_art.png`, `hero.png` or `emblem.png` (keep the file names) and rebuild.
The names shown in the interface are in one file: `src/ui/Branding.h`.

## Rules implemented in the program

* A book/student ID must be unique; all required fields are checked before saving.
* Copies can never become negative; removing more copies than available is refused.
* A book can be borrowed only when at least one copy is available, and a student cannot hold two
  copies of the same book at once. Borrowing/returning change the loan and the copy count in
  **one database transaction** (all or nothing).
* A book or student with books still borrowed cannot be deleted. If only returned loans exist,
  the confirmation dialog says their history will be deleted too.

## What was verified, and what still must be checked on Windows

The program was developed in a Linux sandbox that has **no Windows and no Access driver**. Therefore:

| Item | Status |
|---|---|
| Compiles with Qt 6.4 (GCC), no warnings | verified |
| Business rules: 56 automated checks (add/duplicate/validation, search, copies, borrow/return, delete rules, overdue, login, hashing, persistence after re-opening) | verified, using a temporary stand-in database in the sandbox only (not part of the project) |
| Every screen, dialog and notification rendered and inspected visually | verified (offscreen rendering, Linux fonts) |
| Reading/writing the real `.accdb`, creating it, the Access-specific SQL | **not verified - please test on Windows** |
| No console window, `windeployqt` deployment, 32/64-bit driver match | **not verified - please test on Windows** |

Things worth checking first on Windows: (1) program starts and creates `data\library.accdb`;
(2) add a book, close the program, start again - it must still be there; (3) search finds partial
words (uses `LIKE '%text%'`); (4) borrow/return changes the copy count; (5) in Access, open the file
and look at the tables and the two saved queries. If something fails, the error text appears in a
message box - send it and it can be fixed quickly.

## Known limits

* English interface only (names/titles in Arabic are stored and displayed correctly).
* One workstation / one administrator account (no screen for creating more users).
* Loan period is fixed at 7 days (`LibraryService::kLoanPeriodDays`).
* The `qryBorrowedBooks` / `qryOverdueBooks` queries are created on a best-effort basis; if the
  ODBC driver refuses, create them by hand from `docs/DATABASE.md` (the program itself does not need them).
