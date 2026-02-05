cmake_minimum_required(VERSION 3.12)

project(Httrack C)

set(HTTRACK_SOURCES 
httrack.c
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

# set the lowest xcode 26 handles
if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

add_executable(httr ${HTTRACK_SOURCES} ${HTTRACK_HEADERS} )

target_compile_definitions(httr PRIVATE 
	HTTRACK_SKIP_GNU_BUILDTOOLS=1
	HTS_DO_NOT_REDEFINE_in_addr_t=1
	HAVE_STRNLEN=1
	#HTS_USEOPENSSL=0 // with find_package LibreSSL::SSL, SSL_library_init isnt available, so this can be used instead to download only http sites (not https)
)

target_include_directories(httr PRIVATE 
	"${SRC_ROOT}/coucal"
	"${SRC_ROOT}"
)


set(LIBRESSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../portable")
add_subdirectory("${LIBRESSL_ROOT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/libssl")


# list(APPEND CMAKE_MODULE_PATH "${LIBRESSL_ROOT_DIR}")
# find_package(LibreSSL REQUIRED)

target_link_libraries(httr PRIVATE 
	# LibreSSL::Crypto 
	# LibreSSL::SSL # doesnt provide SSL_library_init & alii for some reason
	# LibreSSL::TLS
	crypto
	ssl
	tls
)

find_package(ZLIB REQUIRED)
find_package(Iconv REQUIRED)  # macOS only, but safe

target_link_libraries(httr PRIVATE 
	ZLIB::ZLIB 
	Iconv::Iconv 
)

if(APPLE)
	add_executable(
		HttrackMac
		MACOSX_BUNDLE
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/AppDelegate.h
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/AppDelegate.m
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/main.m
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/Base.lproj/MainMenu.xib
	)

	set_source_files_properties(
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/HttrackMac/Base.lproj/MainMenu.xib
		PROPERTIES
		MACOSX_PACKAGE_LOCATION
		Resources
	)

	set_target_properties(
		HttrackMac
		PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST
		${CMAKE_CURRENT_LIST_DIR}/MacOS_GUI/plist.in
	)

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework AppKit")
endif()