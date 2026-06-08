.PHONY: build run replay clean

BUILD_DIR := build

build:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) --parallel

run: build
	./$(BUILD_DIR)/llm-tracer

replay: build
	@if [ -z "$(FILE)" ]; then echo "Usage: make replay FILE=<path>"; exit 1; fi
	./$(BUILD_DIR)/llm-tracer --replay $(FILE)

clean:
	rm -rf $(BUILD_DIR)
