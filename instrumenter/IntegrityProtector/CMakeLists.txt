add_library(IntegrityProtector MODULE
        src/IntegrityProtectorPass.cpp
        src/FunctionFetcherHelper.cpp
        src/ProtectedDataHelper.cpp src/IntegrityCodeInserter.cpp include/IntegrityCodeInserter.h)

include_directories(include)

target_compile_features(IntegrityProtector PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(IntegrityProtector PROPERTIES
        COMPILE_FLAGS "-g"
        )

if(APPLE)
    set_target_properties(IntegrityProtector PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            )
endif(APPLE)
