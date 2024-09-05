package bridge

import (
	"fmt"
)

type GoBridge interface {
	Bridge
	deleteBridge()
	IsBridge()
}

type goBridge struct {
	Bridge
}

func (p *goBridge) deleteBridge() {
	DeleteDirectorCallback(p.Bridge)
}

func (p *goBridge) IsBridge() {}

type overwrittenMethodsOnBridge struct {
	p Bridge
}

func NewApp() App {
	om := &overwrittenMethodsOnBridge{}
	p := NewDirectorBridge(om)
	om.p = p

	return &goBridge{Bridge: p}
}

func DeleteBridge(p GoBridge) {
	p.deleteBridge()
}

func (p *overwrittenMethodsOnBridge) Run() {
	fmt.Println("GoBridge.Run")
}