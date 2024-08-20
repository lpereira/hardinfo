message(STATUS "qgears2 module being added...")

find_package(Qt5 REQUIRED Core Gui Widgets OpenGL)
include(FindPkgConfig)

execute_process(COMMAND moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
execute_process(COMMAND moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
execute_process(COMMAND /usr/bin/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
execute_process(COMMAND /usr/bin/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
execute_process(COMMAND /usr/lib/qt5/bin/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
execute_process(COMMAND /usr/lib/qt5/bin/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
execute_process(COMMAND /usr/lib64/qt5/bin/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
execute_process(COMMAND /usr/lib64/qt5/bin/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
execute_process(COMMAND /usr/lib/i386-linux-gnu/qt5/bin/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
execute_process(COMMAND /usr/lib/i386-linux-gnu/qt5/bin/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)

include_directories(
  .
  Qt5::OpenGL
  Qt5::Widgets
  Qt5::Gui
  Qt5::Core
)

add_executable(qgears2
  main.cpp
  qgears.cpp
  commonrenderer.cpp
  qglgears.cpp
  fdclock.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp
)

set_target_properties(qgears2 PROPERTIES COMPILE_FLAGS "-pipe -O2 -Wall -Wextra -D_REENTRANT -fPIC")

target_link_libraries(qgears2
  GL
  pthread
  Qt5::OpenGL
  Qt5::Widgets
  Qt5::Gui
  Qt5::Core
)

install(FILES ${CMAKE_BINARY_DIR}/deps/qgears2/qgears2 DESTINATION ${CMAKE_INSTALL_LIBDIR}/hardinfo2/modules
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
      
message(STATUS "qgears2 module added - DONE")
