package main

import (
	"github.com/robertfarnum/go-app-bridge/bridge"
)

func main() {
    c := NewCallback()
	b := bridge.NewBridge()
	b.SetCallback(c)
	b.Start(0, nil)
}
