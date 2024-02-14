set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets Multimedia)

qt_add_executable(twice-qt6
	MANUAL_FINALIZATION
	main.cc
	mainwindow.cc
	mainwindow.h
	display_widget.cc
	display_widget.h
	actions.cc
	actions.h
)

target_link_libraries(twice-qt6 PRIVATE
	Qt6::Core
	Qt6::Widgets
	Qt6::OpenGLWidgets
	Qt6::Multimedia
	twice
)

install(TARGETS twice-qt6)

qt_finalize_executable(twice-qt6)
