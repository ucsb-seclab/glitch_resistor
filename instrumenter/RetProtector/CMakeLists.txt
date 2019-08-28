add_library(RetProtector MODULE src/RetProtectorPass.cpp)

include_directories(include)

target_compile_features(RetProtector PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(RetProtector PROPERTIES
        COMPILE_FLAGS "-g"
        )

if(APPLE)
    set_target_properties(RetProtector PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)
