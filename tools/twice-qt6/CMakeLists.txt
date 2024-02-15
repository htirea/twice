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
	emulator_thread.cc
	emulator_thread.h
	audio_out.cc
	audio_out.h
)

target_include_directories(twice-qt6 PRIVATE
	${SDL_INCLUDE_DIR}
)

target_link_libraries(twice-qt6 PRIVATE
	Qt6::Core
	Qt6::Widgets
	Qt6::OpenGLWidgets
	Qt6::Multimedia
	${TWICE_SDL_LIBRARIES}
	twice
)

install(TARGETS twice-qt6)

qt_finalize_executable(twice-qt6)
