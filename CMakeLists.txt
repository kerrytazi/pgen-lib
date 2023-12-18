cmake_minimum_required(VERSION 3.25)

MACRO(ADD_MSVC_PRECOMPILED_HEADER Prefix PrecompiledHeader PrecompiledSource SourcesVar)
	IF(MSVC)
		GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
		SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
		SET(Sources ${${SourcesVar}})

		SET_SOURCE_FILES_PROPERTIES(${Prefix}/${PrecompiledSource}
			PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
			OBJECT_OUTPUTS "${PrecompiledBinary}"
		)
		SET_SOURCE_FILES_PROPERTIES(${Sources}
			PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
			OBJECT_DEPENDS "${PrecompiledBinary}"
		)

		LIST(APPEND ${SourcesVar} ${Prefix}/${PrecompiledSource})
	ENDIF(MSVC)

	LIST(APPEND ${SourcesVar} ${Prefix}/${PrecompiledHeader})
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

function(assign_source_group)
	foreach(_source IN ITEMS ${ARGN})
		if (IS_ABSOLUTE "${_source}")
			file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
		else()
			set(_source_rel "${_source}")
		endif()
		get_filename_component(_source_path "${_source_rel}" PATH)
		string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
		source_group("${_source_path_msvc}" FILES "${_source}")
	endforeach()
endfunction(assign_source_group)


project(pgen-lib VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
	src/pgen.hpp
	src/pgen.cpp
)

ADD_MSVC_PRECOMPILED_HEADER("src" "pch.hpp" "pch.cpp" PROJECT_SOURCES)

add_library(pgen-lib ${PROJECT_SOURCES})

target_include_directories(pgen-lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

assign_source_group(${PROJECT_SOURCES})

