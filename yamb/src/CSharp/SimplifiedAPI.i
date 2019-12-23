// External ZeroMQ interface. Client Use via CreateContext
// Details on SWIG http://www.swig.org/Doc3.0/SWIGDocumentation.html#Library_std_shared_ptr
// swig.exe -c++ -csharp -namespace LibraryNamespace -I../CPP/Library/ -outdir ./CSharpLibrary/ -o ./CSharpWrapper/SimpleAPIWrapper.cxx SimplifiedAPI.i

%module(directors="1") CSharpWrapper
//%feature("director"); // Manual shared_ptr into needed type conversion is required! Stuff like SWIGTYPE_p_std__shared_ptrT_%TYPE_NAME%_t into TYPE_NAME replacement

%include "std_string.i"
%include "std_shared_ptr.i"
%include "std_vector.i"


%template(stringvector) std::vector<std::string>;
%shared_ptr(messageHandler)

%feature("director") messageHandler;
%feature("director") messageHandler;

%{
#include <messageBroker.hpp>
%}

%include <messageBroker.hpp>
