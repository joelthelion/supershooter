project(ziziprojects)
cmake_minimum_required(VERSION 2.6)

find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(OpenGL REQUIRED)

add_definitions(-DTIXML_USE_STL)
include_directories(tinyxml zizi)
subdirs(tinyxml zizi tests supershooter zombies blocks truckfight)

install(DIRECTORY data
        DESTINATION share/supershooter)

install(FILES config.xml
        DESTINATION share/supershooter)
install(FILES supershooter.desktop
        DESTINATION share/applications)
install(FILES supershooter.xpm
        DESTINATION share/pixmaps)

install(FILES blocks.desktop
        DESTINATION share/applications)
install(FILES blocks.xpm
        DESTINATION share/pixmaps)

