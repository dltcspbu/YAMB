/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.12
 *
 * This file is not intended to be easily readable and contains a number of
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG
 * interface file instead.
 * ----------------------------------------------------------------------------- */

#ifndef SWIG_CSharpWrapper_WRAP_H_
#define SWIG_CSharpWrapper_WRAP_H_

class SwigDirector_messageHandler : public messageHandler, public Swig::Director {

public:
    SwigDirector_messageHandler();
    virtual void handle(std::string const &m);
    virtual ~SwigDirector_messageHandler();

    typedef void (SWIGSTDCALL* SWIG_Callback0_t)(char *);
    void swig_connect_director(SWIG_Callback0_t callbackhandle);

private:
    SWIG_Callback0_t swig_callbackhandle;
    void swig_init_callbacks();
};


#endif
