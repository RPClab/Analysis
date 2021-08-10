include_guard(GLOBAL)

include(CPM)
cpm()

if(NOT DEFINED RAPIDCSV_REPOSITORY)
    set(RAPIDCSV_REPOSITORY "https://github.com/d99kris/rapidcsv.git")
endif()

if(NOT DEFINED RAPIDCSV_TAG)
    set(RAPIDCSV_TAG "v8.51")
endif()

declare_option(REPOSITORY rapidcsv OPTION RAPIDCSV_BUILD_TESTS VALUE OFF)
print_options(REPOSITORY rapidcsv)

CPMAddPackage(NAME rapidcsv
        GIT_REPOSITORY ${RAPIDCSV_REPOSITORY}
        GIT_TAG ${RAPIDCSV_TAG}
        FETCHCONTENT_UPDATES_DISCONNECTED ${IS_OFFLINE}
        OPTIONS "${rapidcsv_OPTIONS}"
        )
