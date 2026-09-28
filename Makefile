CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -O3 -flto -DNDEBUG

TARGET = cuffc
SOURCES = main.cpp

$(TARGET): $(SOURCES) $(wildcard engine/**/*.h engine/*.h)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

EMCC = em++
WASM_ENTRY = wasm/bindings.cpp
WASM_OUT_DIR = npm/dist
WASM_OUT = $(WASM_OUT_DIR)/cuffscript.mjs

EMFLAGS = -std=c++17 -O3 -flto -fexceptions --bind -DNDEBUG \
	--no-entry \
	-s MODULARIZE=1 \
	-s EXPORT_ES6=1 \
	-s EXPORT_NAME=createCuffScriptModule \
	-s ENVIRONMENT=web,worker \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s INITIAL_MEMORY=33554432 \
	-s MAXIMUM_MEMORY=268435456 \
	-s STACK_SIZE=16777216 \
	-s FORCE_FILESYSTEM=1 \
	-s EXPORTED_RUNTIME_METHODS="['FS']" \
	-s NO_EXIT_RUNTIME=1

wasm: $(WASM_ENTRY) $(wildcard engine/**/*.h engine/*.h)
	mkdir -p $(WASM_OUT_DIR)
	$(EMCC) $(EMFLAGS) $(WASM_ENTRY) -o $(WASM_OUT)

wasm-clean:
	rm -f $(WASM_OUT_DIR)/cuffscript.mjs $(WASM_OUT_DIR)/cuffscript.wasm

.PHONY: clean wasm wasm-clean

WEB_OUT_DIR = web
WEB_ENTRY = wasm/graphics_main.cpp
WEB_SCRIPT = graphics/windows_runtime/demo.cuff

WEBFLAGS = -std=c++17 -O3 -fexceptions -DNDEBUG -DCUFF_ENABLE_GRAPHICS \
	-s USE_SDL=2 \
	-s USE_SDL_IMAGE=2 \
	-s SDL2_IMAGE_FORMATS='["png","jpg"]' \
	-s USE_SDL_MIXER=2 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s INITIAL_MEMORY=33554432 \
	-s STACK_SIZE=16777216 \
	-s EXIT_RUNTIME=0 \
	--preload-file $(WEB_SCRIPT)@/demo.cuff

web: $(WEB_ENTRY) $(wildcard engine/**/*.h engine/*.h)
	mkdir -p $(WEB_OUT_DIR)
	$(EMCC) $(WEBFLAGS) $(WEB_ENTRY) -o $(WEB_OUT_DIR)/c2d.js

web-serve: web
	python3 -m http.server 8000 -d $(WEB_OUT_DIR)

web-test: web
	node tests/web_test.mjs

web-clean:
	rm -f $(WEB_OUT_DIR)/c2d.js $(WEB_OUT_DIR)/c2d.wasm $(WEB_OUT_DIR)/c2d.data

.PHONY: web web-serve web-test web-clean
