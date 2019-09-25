add_library(RecheckLoopProtector MODULE src/LoopProtectorPass.cpp)

include_directories(include ../CommonInstrumentationUtils/include)

target_link_libraries(RecheckLoopProtector CommonInstrumentationUtils)

target_compile_features(RecheckLoopProtector PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(RecheckLoopProtector PROPERTIES
        COMPILE_FLAGS "-g"
        )

if(APPLE)
    set_target_properties(RecheckLoopProtector PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)
