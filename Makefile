.PHONY: build run

build:
	@mkdir -p build
	@cd build && cmake .. && cmake --build .

run:
	@./build/code