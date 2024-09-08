/* File : bridge.i */
%module(directors="1") bridge
%{
#include "bridge.h"
%}
/* turn on director wrapping Callback */
%feature("director") Callback;

%include "bridge.h"
