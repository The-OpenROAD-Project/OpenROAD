find_package(GTest)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    # Specify the commit you depend on and update it regularly.
    URL "https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip"
  )
  FetchContent_MakeAvailable(googletest)
endif()