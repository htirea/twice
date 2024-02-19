set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets Multimedia)

qt_add_executable(twice-qt
	MANUAL_FINALIZATION
	main.cc
	mainwindow.cc
	mainwindow.h
	mainwindow_actions.cc
	mainwindow_actions.h
	display_widget.cc
	display_widget.h
	emulator_thread.cc
	emulator_thread.h
	audio_io.cc
	audio_io.h
	settings_dialog.cc
	settings_dialog.h
	config_manager.cc
	config_manager.h
	input_manager.cc
	input_manager.h
	settings/input_settings.cc
	settings/input_settings.h
)

target_include_directories(twice-qt PRIVATE
	${SDL_INCLUDE_DIR}
)

target_link_libraries(twice-qt PRIVATE
	Qt6::Core
	Qt6::Widgets
	Qt6::OpenGLWidgets
	Qt6::Multimedia
	${TWICE_SDL_LIBRARIES}
	twice
)

install(TARGETS twice-qt)

qt_finalize_executable(twice-qt)
