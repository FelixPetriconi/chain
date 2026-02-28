include(cmake/CPM.cmake)

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
function(chain_setup_dependencies)

    # For each dependency, see if it's
    # already been provided to us by a parent project

    if (NOT TARGET stlab::stlab)
        cpmaddpackage(
                NAME stlab
                VERSION 2.2.0
                GITHUB_REPOSITORY "stlab/libraries"
                OPTIONS "BUILD_TESTING OFF")
    endif ()


    if (NOT TARGET doctest::doctest)
        cpmaddpackage("gh:doctest/doctest@2.4.12")
    endif ()

#    if (NOT TARGET tools::tools)
#        cpmaddpackage("gh:lefticus/tools#update_build_system")
#    endif ()

endfunction()
