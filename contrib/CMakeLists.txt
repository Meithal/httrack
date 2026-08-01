cmake_minimum_required(VERSION 3.12)

# if(APPLE AND NOT CMAKE_OSX_SYSROOT)
#     execute_process(
#         COMMAND xcrun --sdk macosx --show-sdk-path
#         OUTPUT_VARIABLE _sdk_path
#         OUTPUT_STRIP_TRAILING_WHITESPACE
#     )
#     set(CMAKE_OSX_SYSROOT "${_sdk_path}" CACHE PATH "macOS SDK" FORCE)
# endif()

### set the lowest xcode 26 handles
set(MAX_SDK "10.8")
if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET ${MAX_SDK})
	set(MACOSX_DEPLOYMENT_TARGET ${MAX_SDK})
endif()

project(Httrack C OBJC)

set(HTTRACK_SOURCES 
	htscore.c htsparse.c htsback.c htscache.c 
	htscatchurl.c htsfilters.c htsftp.c htshash.c coucal/coucal.c
	htshelp.c htslib.c htscoremain.c 
	htsname.c htsrobots.c htstools.c htswizard.c 
	htsalias.c htsthread.c htsindex.c htsbauth.c 
	htsmd5.c htszlib.c htswrap.c htsconcat.c 
	htsmodules.c htscharset.c punycode.c htsencoding.c 
	md5.c 
	minizip/ioapi.c minizip/mztools.c minizip/unzip.c minizip/zip.c
)

set(HTTRACK_HEADERS
	hts-indextmpl.h htsalias.h htsback.h htsbase.h htssafe.h
	htsbasenet.h htsbauth.h htscache.h htscatchurl.h
	htsconfig.h htscore.h htsparse.h htscoremain.h htsdefines.h
	htsfilters.h htsftp.h htsglobal.h htshash.h coucal/coucal.h
	htshelp.h htsindex.h htslib.h htsmd5.h
	htsmodules.h htsname.h htsnet.h
	htsopt.h htsrobots.h htsthread.h
	htstools.h htswizard.h htswrap.h htszlib.h
	htsstrings.h htsarrays.h httrack-library.h
	htscharset.h punycode.h htsencoding.h
	htsentities.h htsentities.sh htsbasiccharsets.sh htscodepages.h 
	md5.h coucal/murmurhash3.h 
	minizip/crypt.h minizip/ioapi.h minizip/mztools.h minizip/unzip.h minizip/zip.h
)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../src)

list(TRANSFORM HTTRACK_SOURCES PREPEND "${SRC_ROOT}/")
list(TRANSFORM HTTRACK_HEADERS PREPEND "${SRC_ROOT}/")

add_library(httr_lib STATIC ${HTTRACK_SOURCES})
target_sources(httr_lib PRIVATE ${HTTRACK_HEADERS})
target_compile_definitions(httr_lib PRIVATE DEBUG=1 HDEBUG=1)

target_include_directories(httr_lib PUBLIC 
	"${SRC_ROOT}/coucal"
	"${SRC_ROOT}"
)


set(LIBRESSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../portable")
add_subdirectory("${LIBRESSL_ROOT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/libssl")

# list(APPEND CMAKE_MODULE_PATH "${LIBRESSL_ROOT_DIR}")
# find_package(LibreSSL REQUIRED)

target_link_libraries(httr_lib PUBLIC 
	# LibreSSL::Crypto 
	# LibreSSL::SSL # doesnt provide SSL_library_init & alii for some reason
	# LibreSSL::TLS
	crypto
	ssl
	tls
)

find_package(ZLIB REQUIRED)
find_package(Iconv REQUIRED)  # macOS only, but safe

target_link_libraries(httr_lib PUBLIC 
	ZLIB::ZLIB 
	Iconv::Iconv 
)

target_compile_definitions(httr_lib PUBLIC 
	# HTTRACK_SKIP_GNU_BUILDTOOLS=1
	HTS_DO_NOT_REDEFINE_in_addr_t=1
	HAVE_STRNLEN=1
	#HTS_USEOPENSSL=0 // with find_package LibreSSL::SSL, SSL_library_init isnt available, so this can be used instead to download only http sites (not https)
)

add_executable(httr
	"${SRC_ROOT}/httrack.c"
	"${SRC_ROOT}/httrack.h"
)
target_link_libraries(httr PUBLIC httr_lib)


if(APPLE)
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.meithal")

	add_executable(
		HttrackMac
		MACOSX_BUNDLE
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/AppDelegate.h
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/AppDelegate.m
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/main.m
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/Base.lproj/MainMenu.xib
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/Controllers/ControllerMainMenu.h
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/Controllers/ControllerMainMenu.m
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/Models/ModelsApp.h
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/Models/ModelsApp.m
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/CoreLogic.h
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/CoreLogic.m
	)

	target_sources(HttrackMac PRIVATE "${CMAKE_CURRENT_LIST_DIR}/TODO.md")

	target_link_libraries(HttrackMac PRIVATE httr_lib "-framework AppKit -framework WebKit")
	target_include_directories(HttrackMac PRIVATE "${SRC_ROOT}")

	set_source_files_properties(
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/Base.lproj/MainMenu.xib
		  PROPERTIES
		    MACOSX_PACKAGE_LOCATION Resources
	)

	set_target_properties(
		HttrackMac
		  PROPERTIES
		    MACOSX_BUNDLE 1
		    MACOSX_BUNDLE_GUI_IDENTIFIER ${MACOSX_BUNDLE_GUI_IDENTIFIER}
		    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/plist.in
	)

	#enable_testing()
	# cf https://stackoverflow.com/a/78355693/13577241
	find_package(XCTest REQUIRED)

	xctest_add_bundle(UnitTests HttrackMac
    	${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMacTests/HttrackMacTests.m
	)
	set_target_properties(UnitTests 
		PROPERTIES 
		XCODE_ATTRIBUTE_TEST_HOST ""
		# XCODE_ATTRIBUTE_TEST_TARGET_NAME HttrackMac
		XCODE_PRODUCT_TYPE com.apple.product-type.bundle.unit-test
	)

	xctest_add_bundle(UITests HttrackMac
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMacUITests/HttrackMacUITests.m
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMacUITests/HttrackMacUITestsLaunchTests.m
	)
	set_target_properties(
		UITests 
		 PROPERTIES 
	      XCODE_ATTRIBUTE_TEST_HOST ""
		  # XCODE_ATTRIBUTE_BUNDLE_LOADER ""
		  # XCODE_ATTRIBUTE_TEST_TARGET_NAME HttrackMac
		  XCODE_PRODUCT_TYPE com.apple.product-type.bundle.ui-testing
	)

endif()

add_executable(
	objctest "test.m"
)
