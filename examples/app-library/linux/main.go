package main

// #cgo LDFLAGS: -L. -lAppLibrary
// #include "include/AppLibrary.h"
import "C"

func StartMatterApp(int argc, string[] argv) {
	LIB_StartMatterApp(C.int(argc), argv)
}

func main() {
	StartMatterApp(0, nil)
}
