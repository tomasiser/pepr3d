if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(ASSIMP_ARCHITECTURE "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(ASSIMP_ARCHITECTURE "32")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	
if(WIN32)
	set(ASSIMP_ROOT_DIR $ENV{ASSIMP_ROOT} CACHE PATH "ASSIMP root directory")

  
	# Find path of each library
	find_path(ASSIMP_INCLUDE_DIR
		NAMES
			assimp/anim.h
		HINTS
			${ASSIMP_ROOT_DIR}/include
	)

	if(MSVC12)
		set(ASSIMP_MSVC_VERSION "vc120")
	elseif(MSVC14)	
		set(ASSIMP_MSVC_VERSION "vc140") #MSVC14 represents 140 and 141 toolsets
	endif(MSVC12)
	
	if(MSVC12 OR MSVC14)
	
		find_path(ASSIMP_LIBRARY_DIR
			NAMES
				assimp-${ASSIMP_MSVC_VERSION}-mt.lib
			HINTS
				${ASSIMP_ROOT_DIR}/lib${ASSIMP_ARCHITECTURE}
        ${ASSIMP_ROOT_DIR}/lib/x${ASSIMP_ARCHITECTURE}
		)
		
		find_library(ASSIMP_LIBRARY_RELEASE				assimp-${ASSIMP_MSVC_VERSION}-mt.lib 			PATHS ${ASSIMP_LIBRARY_DIR})
		find_library(ASSIMP_LIBRARY_DEBUG				assimp-${ASSIMP_MSVC_VERSION}-mtd.lib			PATHS ${ASSIMP_LIBRARY_DIR})
		
		set(ASSIMP_LIBRARY 
			optimized 	${ASSIMP_LIBRARY_RELEASE}
			debug		${ASSIMP_LIBRARY_DEBUG}
		)
    
    find_file(ASSIMP_DLL_FILE assimp-${ASSIMP_MSVC_VERSION}-mt.dll			PATHS ${ASSIMP_ROOT_DIR}/bin/x64 ${ASSIMP_ROOT_DIR}/bin64)
		
		set(ASSIMP_LIBRARIES "ASSIMP_LIBRARY_RELEASE" "ASSIMP_LIBRARY_DEBUG")
	  
    if (ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARY_RELEASE)
      set(ASSIMP_FOUND TRUE)
    endif()
	
	endif()
	
else(WIN32)

	find_path(
	  ASSIMP_INCLUDE_DIR
	  NAMES assimp/anim.h assimp/scene.h assimp/version.h assimp/config.h
	  PATHS /usr/local/include /usr/include
	)

	if(NOT ASSIMP_INCLUDE_DIR)
		message(WARNING "Could not find assimp include dir")
	endif()

	find_library(
	  ASSIMP_LIBRARIES
	  NAMES assimp
	  PATHS /usr/local/lib /usr/lib
	)

	if(NOT ASSIMP_LIBRARIES)
		message(WARNING "Could not find assimp library file")
	endif()

	if (ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARIES)
	  SET(ASSIMP_FOUND TRUE)
	ENDIF (ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARIES)

	if (ASSIMP_FOUND)
	  if (NOT assimp_FIND_QUIETLY)
		message(STATUS "Found asset importer library: ${ASSIMP_LIBRARIES}")
		set(ASSIMP_LIBRARY_RELEASE ${ASSIMP_LIBRARIES})
	  endif (NOT assimp_FIND_QUIETLY)
	else (ASSIMP_FOUND)
	  if (assimp_FIND_REQUIRED)
		message(FATAL_ERROR "Could not find asset importer library")
	  endif (assimp_FIND_REQUIRED)
	endif (ASSIMP_FOUND)
	
endif(WIN32)