add_library(BranchProtector MODULE src/BranchProtectorPass.cpp)

include_directories(include)

target_compile_features(BranchProtector PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(BranchProtector PROPERTIES
        COMPILE_FLAGS "-g"
        )

if(APPLE)
    set_target_properties(BranchProtector PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)
