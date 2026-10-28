cmake_minimum_required(VERSION 3.21)
project(LibraryManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

# --- layer 1: models + database + business logic (no GUI code) -------------------------------
add_library(LibraryCore STATIC
    src/models/Book.cpp
    src/models/Student.cpp
    src/db/DatabaseManager.cpp
    src/db/BookRepository.cpp
    src/db/StudentRepository.cpp
    src/db/LoanRepository.cpp
    src/db/UserRepository.cpp
    src/services/LibraryService.cpp
    src/services/AuthService.cpp
)
target_include_directories(LibraryCore PUBLIC src)
target_link_libraries(LibraryCore PUBLIC Qt6::Core Qt6::Sql)
if(WIN32)
    target_link_libraries(LibraryCore PUBLIC odbccp32)   # SQLConfigDataSource (creates the .accdb)
endif()

# --- layer 2: user interface ------------------------------------------------------------------
add_library(LibraryUi STATIC
    src/ui/UiUtil.cpp
    src/ui/Toast.cpp
    src/ui/Page.cpp
    src/ui/BookDialog.cpp
    src/ui/StudentDialog.cpp
    src/ui/BooksPage.cpp
    src/ui/StudentsPage.cpp
    src/ui/LoansPage.cpp
    src/ui/ReportsPage.cpp
    src/ui/DashboardPage.cpp
    src/ui/LoginDialog.cpp
    src/ui/MainWindow.cpp
)
target_link_libraries(LibraryUi PUBLIC LibraryCore Qt6::Widgets)

# --- the application ---------------------------------------------------------------------------
set(APP_SOURCES src/main.cpp resources/resources.qrc)
if(WIN32)
    list(APPEND APP_SOURCES app.rc)    # executable icon
endif()

# WIN32 => GUI subsystem: the program starts with NO console window.
qt_add_executable(LibraryManager WIN32 ${APP_SOURCES})
target_link_libraries(LibraryManager PRIVATE LibraryUi)

# Run `cmake --install` (or the windeployqt step below) to get a folder you can copy to another PC.
if(WIN32)
    add_custom_command(TARGET LibraryManager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
                "Tip: run  windeployqt $<TARGET_FILE:LibraryManager>  to copy the Qt DLLs next to the exe")
endif()
