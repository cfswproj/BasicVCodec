CMAKE_MINIMUM_REQUIRED ( VERSION 2.6 )
###################################################################################################

SET ( PROJ 		                    Compreess_Assignment )
SET ( SRC_DIR	                    "src/" )
SET ( OUTPUT_DIR	                "bin/" )
FILE( GLOB_RECURSE INCL_FILES_SRC  	"${SRC_DIR}*.cpp" 
									"${SRC_DIR}*.hpp"
									"${SRC_DIR}*.h"
	 )
	 
PROJECT ( ${PROJ} )
# Files not immediately part of the current project should be referenced from  
# the main CFLib root.
INCLUDE_DIRECTORIES ( "../" )

ADD_EXECUTABLE ( ${PROJECT_NAME} ${INCL_FILES_SRC} )

TARGET_LINK_LIBRARIES ( ${PROJECT_NAME} 
                       )

SET_TARGET_PROPERTIES ( ${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX )

# Set the compiler warning level
if ( MSVC )
    # This is untested
    SET_PROPERTY( TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS /W4 )
endif ( MSVC )
