cmake_minimum_required(VERSION 3.16)

project(time-tracker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 🔍 Находим нужные модули
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    event.h
    eventdialog.cpp
    eventdialog.h
    eventdialog.ui
    analysisdialog.cpp
    analysisdialog.h
    analysisdialog.ui
)

# 🔨 Создаём исполняемый файл
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(time-tracker
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(time-tracker SHARED ${PROJECT_SOURCES})
    else()
        add_executable(time-tracker ${PROJECT_SOURCES})
    endif()
endif()

# 🔗 Линкуем библиотеки Qt
target_link_libraries(time-tracker
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Charts
)

# ⚙️ Свойства сборки
set_target_properties(time-tracker PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 📦 Установка
include(GNUInstallDirs)
install(TARGETS time-tracker
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ✅ Финализация Qt6
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(time-tracker)
endif()
