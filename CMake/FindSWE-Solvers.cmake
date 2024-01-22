include(FetchContent)

FetchContent_Declare(
    SWE-Solvers
    GIT_REPOSITORY https://github.com/jmorenohj/SWE-Solvers.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(SWE-Solvers)
