target_include_directories( chain INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../>
    $<INSTALL_INTERFACE:include>
)

set(CHAIN_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/chain.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/on.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/segment.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tuple.hpp
)

target_sources( chain INTERFACE ${CHAIN_HEADERS})
