cmake_minimum_required (VERSION 3.8)

set(PROJECT_NAME MayaVectorDisplacementDeformer)
project(${PROJECT_NAME})

# Preprocessor directives
add_compile_definitions(REQUIRE_IOSTREAM _BOOL)

# Include and library paths
set(MAYA_DEVKIT_PATH "C:/Maya2020-Devkit/devkitBase")
set(MAYA_INCLUDE_DIR ${MAYA_DEVKIT_PATH}/include)
set(MAYA_LIB_DIR ${MAYA_DEVKIT_PATH}/lib)

include_directories(${MAYA_INCLUDE_DIR})
link_directories(${MAYA_LIB_DIR})

# Compile as DLL
set(SOURCE_FILES
	"src/VectorDisplacementDeformerNode.cpp" "src/VectorDisplacementDeformerNode.h"
	"src/VectorDisplacementUtilities.h" "src/VectorDisplacementUtilities.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# Set flags
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/export:initializePlugin /export:uninitializePlugin")

# Link necessary libraries
target_link_libraries(${PROJECT_NAME} Foundation OpenMaya OpenMayaAnim OpenMayaFx)

# Duplicate and rename DLL to MLL for loading in Maya
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dll ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.mll)