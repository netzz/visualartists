cmake_minimum_required (VERSION 2.6)


project(stereo)


find_package(osg)
find_package(osgDB)
find_package(osgViewer)
find_package(OpenCV REQUIRED)

#add_executable(bgfg-segm bgfg-segm.cpp)
#target_link_libraries(bgfg-segm ${OpenCV_LIBS})

add_executable(algorithm-list algorithm-list.cpp)
target_link_libraries(algorithm-list ${OpenCV_LIBS})

add_executable(stereo-match StereoAnaliser.cpp stereo-match.cpp)
target_link_libraries(stereo-match ${OpenCV_LIBS})

add_executable(main Balloon.cpp GestureFinder.cpp main.cpp)
target_link_libraries(main osg osgViewer ${OpenCV_LIBS})



