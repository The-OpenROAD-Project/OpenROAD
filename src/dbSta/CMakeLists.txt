# Find this section:
set(DBSTA_SOURCES
    # ... existing files ...
)

# Add this line:
set(DBSTA_SOURCES
    ${DBSTA_SOURCES}
    pin_checker.cc
)

# Ensure it's included in the library:
add_library(dbSta ${DBSTA_SOURCES})
