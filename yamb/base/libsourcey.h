// Name of library
#define SCY_NAME "LibSourcey"

// Define the library version
#define SCY_VERSION "1.1.4"
#define SCY_VERSION_MAJOR 1
#define SCY_VERSION_MINOR 1
#define SCY_VERSION_PATCH 4

// Define the source path
#define SCY_SOURCE_DIR "/home/zharkov/git/nsfw/webrtcMessageBroker/libsourcey/src"

// Define the build path
#define SCY_BUILD_DIR "/home/zharkov/git/nsfw/webrtcMessageBroker/cmake-build-debug"

// Define the installation path
#define SCY_INSTALL_DIR "/usr/local"

// Building as shared library (.so or .dll)
/* #undef SCY_SHARED_LIBRARY */

// Disable logging
#define SCY_ENABLE_LOGGING

// Attempt to recover from internal exceptions
// Exceptions thrown inside thread and stream context will be caught, logged and
// handled via the event loop in an attempt to prevent crashes.
/* #undef SCY_EXCEPTION_RECOVERY */

// LibSourcey modules
// cmakedefine HAVE_SCY_base
// cmakedefine HAVE_SCY_http
// cmakedefine HAVE_SCY_json
// cmakedefine HAVE_SCY_av
// cmakedefine HAVE_SCY_net
// cmakedefine HAVE_SCY_rtp
// cmakedefine HAVE_SCY_sdp
// cmakedefine HAVE_SCY_socketio
// cmakedefine HAVE_SCY_stun
// cmakedefine HAVE_SCY_symple
// cmakedefine HAVE_SCY_turn
// cmakedefine HAVE_SCY_xml
// cmakedefine HAVE_SCY_xmpp
// cmakedefine HAVE_SCY_uv
// cmakedefine HAVE_SCY_rtsp
// cmakedefine HAVE_SCY_ice
// cmakedefine HAVE_SCY_pacm
// cmakedefine HAVE_SCY_webrtc
// cmakedefine HAVE_SCY_anionu
// cmakedefine HAVE_SCY_spotapi

// LibUV library
#define HAVE_LIBUV

// OpenSSL library
#define HAVE_OPENSSL
#define OPENSSL_IS_BORINGSSL

// FFmpeg video library
/* #undef HAVE_FFMPEG */
/* #undef HAVE_FFMPEG_AVCODEC */
/* #undef HAVE_FFMPEG_AVFORMAT */
/* #undef HAVE_FFMPEG_AVUTIL */
/* #undef HAVE_FFMPEG_AVFILTER */
/* #undef HAVE_FFMPEG_AVDEVICE */
/* #undef HAVE_FFMPEG_AVRESAMPLE */
/* #undef HAVE_FFMPEG_SWRESAMPLE */
/* #undef HAVE_FFMPEG_SWSCALE */
/* #undef HAVE_FFMPEG_POSTPROC */

// OpenCV library
/* #undef HAVE_OPENCV */

// RtAudio library
/* #undef HAVE_RTAUDIO */

// Define to 1 if you have the <inttypes.h> header file.
/* #undef HAVE_INTTYPES_H */

// Enable macros for format specifiers in <inttypes.h>
#define __STDC_FORMAT_MACROS

// Define to 1 if your processor stores words with the most significant byte
// first (like Motorola and SPARC, unlike Intel and VAX).
/* #undef WORDS_BIGENDIAN */


//
/// Platform and compiler definitions
//

#ifdef _WIN32 // Windows (x64 and x86)
#define SCY_WIN
#endif
#if __unix__ // Unix
#define SCY_UNIX
#endif
#if __posix__ // POSIX
#define SCY_POSIX
#endif
#if __linux__ // Linux
#define SCY_LINUX
#endif
#if __APPLE__ // Mac OS
#define SCY_APPLE
#endif
#if __GNUC__ // GCC compiler
#define SCY_GNUC
#endif
#if defined(__MINGW32__) || defined(__MINGW64__) // MinGW
#define SCY_MINGW
#endif


//
/// Windows specific
//

#ifdef SCY_WIN

// Verify that we're building with the multithreaded
// versions of the runtime libraries
#if defined(_MSC_VER) && !defined(_MT)
#error Must compile with /MD, /MDd, /MT or /MTd
#endif

// Check debug/release settings consistency
#if defined(NDEBUG) && defined(_DEBUG)
#error Inconsistent build settings (check for /MD[d])
#endif

// Unicode Support
#if defined(UNICODE)
#define SCY_UNICODE
#endif

// Disable unnecessary warnings
#if defined(_MSC_VER)
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable : 4251) // ... needs to have dll-interface warning
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
#pragma warning(disable : 4996) // VC++ 8.0 deprecation warnings
#pragma warning(disable : 4351) // new behavior: elements of array '...' will be default initialized
#pragma warning(disable : 4675) // resolved overload was found by argument-dependent lookup
#pragma warning(disable : 4100) // MSVS 'unreferenced formal parameter' warnings showing false positives
#pragma warning(disable : 4706) // assignment within conditional expression
#endif

#endif // SCY_WIN
