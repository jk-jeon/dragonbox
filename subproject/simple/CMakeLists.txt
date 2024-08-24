add_library(simple_dragonbox INTERFACE simple_dragonbox.h)
target_compile_features(simple_dragonbox INTERFACE cxx_std_17)

add_executable(simple_dragonbox_test simple_dragonbox_test.cpp)
target_link_libraries(simple_dragonbox_test PRIVATE
    simple_dragonbox
    dragonbox::common
    ryu::ryu
)
