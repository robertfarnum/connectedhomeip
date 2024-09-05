/* File : Bridge.i */
%module(directors="1") Bridge
%{
#include "Bridge.h"
%}
/* turn on director wrapping Bridge */
%feature("director") Bridge;

%include "Bridge.h"
