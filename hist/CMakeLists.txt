cmake_minimum_required (VERSION 2.6)


project(stereo)


find_package(OpenCV REQUIRED)


#add_executable(bgfg-segm bgfg-segm.cpp)
#target_link_libraries(bgfg-segm ${OpenCV_LIBS})

add_executable(algorithm-list algorithm-list.cpp)
target_link_libraries(algorithm-list ${OpenCV_LIBS})

add_executable(wait-key wait-key.cpp)
target_link_libraries(wait-key ${OpenCV_LIBS})

add_executable(main StereoAnaliser.cpp Balloon.cpp GestureFinder.cpp main.cpp)
target_link_libraries(main freenect freenect_sync freenect_cv ${OpenCV_LIBS})
