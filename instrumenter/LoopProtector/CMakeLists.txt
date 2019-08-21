add_library(LoopProtector MODULE src/LoopProtectorPass.cpp)

include_directories(include)

target_compile_features(LoopProtector PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(LoopProtector PROPERTIES
        COMPILE_FLAGS "-g"
        )

if(APPLE)
    set_target_properties(LoopProtector PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)
