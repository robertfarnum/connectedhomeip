package main

import (
	"fmt"
	"github.com/robertfarnum/go-app-bridge/bridge"
)

type Callback interface {
	bridge.Callback
	deleteCallback()
	IsCallback()
}

type callback struct {
	bridge.Callback
}

func (p *callback) deleteCallback() {
	bridge.DeleteDirectorCallback(p.Callback)
}

func (p *callback) IsCallback() {}

type overwrittenMethodsOnCallback struct {
	p bridge.Callback
}

func NewCallback() Callback {
	om := &overwrittenMethodsOnCallback{}
	p := bridge.NewDirectorCallback(om)
	om.p = p

	return &callback{Callback: p}
}

func DeleteCallback(p Callback) {
	p.deleteCallback()
}

func (p *overwrittenMethodsOnCallback) Status() {
	fmt.Println("Callback.Status")
}