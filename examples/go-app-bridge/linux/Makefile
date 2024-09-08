swig: bridge/bridge.i
	swig -go -c++ -gccgo bridge/bridge.i
	ninja -C out

app: main.go
	CGO_LDFLAGS="-Lout -lapp-bridge"; \
	export CGO_LDFLAGS; \
	go build -v -compiler=gccgo -o app main.go callback.go

all: swig app

run:
	export LD_LIBRARY_PATH=out; \
	./app