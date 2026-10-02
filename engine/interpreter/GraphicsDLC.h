#pragma once

#include "NativeFunctions.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <unordered_map>
#include <string>
#include <cmath>
#include <vector>
#include <unordered_set>

namespace cuff
{

    struct GfxQuad
    {
        SDL_Rect rect;
    };

    struct GfxTransform
    {
        double x = 0;
        double y = 0;
        double zoom = 1;
    };

    class GraphicsState
    {
    public:
        static GraphicsState &instance()
        {
            static GraphicsState s;
            return s;
        }

        SDL_Window *window = nullptr;
        SDL_Renderer *renderer = nullptr;
        bool running = false;
        bool closeRequested = false;
        Uint64 lastTicks = 0;
        double dt = 0.0;
        std::unordered_map<int, SDL_Texture *> images;
        int nextImageId = 1;
        std::unordered_map<int, TTF_Font *> fonts;
        int nextFontId = 1;
        std::unordered_map<int, GfxQuad> quads;
        int nextQuadId = 1;
        std::unordered_map<int, SDL_Texture *> layers;
        int nextLayerId = 1;
        SDL_Texture *layerTarget = nullptr;
        GfxTransform transform;
        std::unordered_map<int, Mix_Chunk *> sounds;
        int nextSoundId = 1;
        Mix_Music *music = nullptr;
        const Uint8 *keys = nullptr;
        int keyCount = 0;
        std::unordered_set<SDL_Scancode> pressedKeys;
        std::string textInput;
        bool textInputActive = false;
        int mouseX = 0;
        int mouseY = 0;
        Uint32 mouseButtons = 0;

        void shutdown()
        {
            for (auto &p : images)
                SDL_DestroyTexture(p.second);
            images.clear();
            for (auto &p : fonts)
                TTF_CloseFont(p.second);
            fonts.clear();
            for (auto &p : layers)
                SDL_DestroyTexture(p.second);
            layers.clear();
            for (auto &p : sounds)
                Mix_FreeChunk(p.second);
            sounds.clear();
            if (music)
            {
                Mix_FreeMusic(music);
                music = nullptr;
            }
            if (TTF_WasInit())
                TTF_Quit();
            if (renderer)
            {
                SDL_DestroyRenderer(renderer);
                renderer = nullptr;
            }
            if (window)
            {
                SDL_DestroyWindow(window);
                window = nullptr;
            }
            if (running)
            {
                Mix_CloseAudio();
                IMG_Quit();
                SDL_Quit();
                running = false;
            }
        }

        ~GraphicsState() { shutdown(); }
    };

    inline int gfxX(double x)
    {
        auto &g = GraphicsState::instance();
        return static_cast<int>((x - g.transform.x) * g.transform.zoom);
    }

    inline int gfxY(double y)
    {
        auto &g = GraphicsState::instance();
        return static_cast<int>((y - g.transform.y) * g.transform.zoom);
    }

    inline int gfxSize(double value)
    {
        return static_cast<int>(value * GraphicsState::instance().transform.zoom);
    }

    inline SDL_Rect gfxRect(double x, double y, double w, double h)
    {
        return SDL_Rect{gfxX(x), gfxY(y), gfxSize(w), gfxSize(h)};
    }

    inline SDL_Texture *gfxImage(const char *fn, int id, const SourceLocation &loc)
    {
        auto &images = GraphicsState::instance().images;
        auto it = images.find(id);
        if (it == images.end())
            throw ModuleError(ErrorCode::UnknownDLC, std::string(fn) + "() unknown image id", loc, "");
        return it->second;
    }

    inline TTF_Font *gfxFont(const char *fn, int id, const SourceLocation &loc)
    {
        auto &fonts = GraphicsState::instance().fonts;
        auto it = fonts.find(id);
        if (it == fonts.end())
            throw ModuleError(ErrorCode::UnknownDLC, std::string(fn) + "() unknown font id", loc, "");
        return it->second;
    }

    inline std::string gfxTabs(const std::string &text, int tabSize)
    {
        std::string result;
        int column = 0;
        for (char ch : text)
        {
            if (ch == '\n')
            {
                result.push_back(ch);
                column = 0;
            }
            else if (ch == '\t')
            {
                int count = tabSize - (column % tabSize);
                result.append(static_cast<size_t>(count), ' ');
                column += count;
            }
            else
            {
                result.push_back(ch);
                ++column;
            }
        }
        return result;
    }

    inline int gfxTextWidth(TTF_Font *font, const std::string &text)
    {
        int width = 0;
        size_t start = 0;
        while (start <= text.size())
        {
            size_t end = text.find('\n', start);
            std::string line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
            int lineWidth = 0;
            int lineHeight = 0;
            TTF_SizeUTF8(font, line.c_str(), &lineWidth, &lineHeight);
            width = std::max(width, lineWidth);
            if (end == std::string::npos)
                break;
            start = end + 1;
        }
        return width;
    }

    inline void gfxRequireWindow(const char *fn, const SourceLocation &loc)
    {
        if (!GraphicsState::instance().window)
            throw ModuleError(ErrorCode::UnknownDLC, std::string(fn) + "() called before window(...)", loc,
                               "call graphics window(w, h, title) first");
    }

    inline void registerGraphicsDLC(std::unordered_map<std::string, NativeFn> &reg)
    {
        reg["window"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("window", args, 3, loc);
            int w = static_cast<int>(expectNumber("window", args, 0, loc));
            int h = static_cast<int>(expectNumber("window", args, 1, loc));
            const std::string &title = expectStr("window", args, 2, loc);

            auto &g = GraphicsState::instance();
            if (!g.running)
            {
                SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
                IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
                Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
                TTF_Init();
                g.running = true;
            }
            g.window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                         w, h, SDL_WINDOW_SHOWN);
            g.renderer = SDL_CreateRenderer(g.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            g.lastTicks = SDL_GetPerformanceCounter();
            g.keys = SDL_GetKeyboardState(&g.keyCount);
            return Value::makeEmpty();
        };

        reg["should_close"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("should_close", args, 0, loc);
            return Value::makeBool(GraphicsState::instance().closeRequested);
        };

        reg["poll"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("poll", args, 0, loc);
            auto &g = GraphicsState::instance();
            gfxRequireWindow("poll", loc);

            SDL_Event e;
            g.pressedKeys.clear();
            g.textInput.clear();
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                    g.closeRequested = true;
                else if (e.type == SDL_KEYDOWN && !e.key.repeat)
                    g.pressedKeys.insert(e.key.keysym.scancode);
                else if (e.type == SDL_TEXTINPUT && g.textInputActive)
                    g.textInput += e.text.text;
            }
            g.mouseButtons = SDL_GetMouseState(&g.mouseX, &g.mouseY);

            Uint64 now = SDL_GetPerformanceCounter();
            g.dt = static_cast<double>(now - g.lastTicks) / static_cast<double>(SDL_GetPerformanceFrequency());
            g.lastTicks = now;
            return Value::makeEmpty();
        };

        reg["dt"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("dt", args, 0, loc);
            return Value::makeNumber(GraphicsState::instance().dt);
        };

        reg["resolution"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("resolution", args, 2, loc);
            gfxRequireWindow("resolution", loc);
            int w = static_cast<int>(expectNumber("resolution", args, 0, loc));
            int h = static_cast<int>(expectNumber("resolution", args, 1, loc));
            SDL_RenderSetLogicalSize(GraphicsState::instance().renderer, w, h);
            return Value::makeEmpty();
        };

        reg["camera"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("camera", args, 3, loc);
            auto &t = GraphicsState::instance().transform;
            t.x = expectNumber("camera", args, 0, loc);
            t.y = expectNumber("camera", args, 1, loc);
            t.zoom = expectNumber("camera", args, 2, loc);
            if (t.zoom <= 0)
                throw ValueError("camera() zoom must be positive", loc);
            return Value::makeEmpty();
        };

        reg["camera_reset"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("camera_reset", args, 0, loc);
            GraphicsState::instance().transform = GfxTransform();
            return Value::makeEmpty();
        };

        reg["key_pressed"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("key_pressed", args, 1, loc);
            const std::string &name = expectStr("key_pressed", args, 0, loc);
            SDL_Scancode code = SDL_GetScancodeFromName(name.c_str());
            if (code == SDL_SCANCODE_UNKNOWN)
                return Value::makeBool(false);
            return Value::makeBool(GraphicsState::instance().pressedKeys.count(code) != 0);
        };

        reg["text_input_start"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("text_input_start", args, 0, loc);
            gfxRequireWindow("text_input_start", loc);
            auto &g = GraphicsState::instance();
            g.textInputActive = true;
            g.textInput.clear();
            SDL_StartTextInput();
            return Value::makeEmpty();
        };

        reg["text_input_stop"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("text_input_stop", args, 0, loc);
            auto &g = GraphicsState::instance();
            g.textInputActive = false;
            g.textInput.clear();
            SDL_StopTextInput();
            return Value::makeEmpty();
        };

        reg["text_input"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("text_input", args, 0, loc);
            auto &g = GraphicsState::instance();
            std::string input = g.textInput;
            g.textInput.clear();
            return Value::makeStr(input);
        };

        reg["layer_create"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("layer_create", args, 2, loc);
            gfxRequireWindow("layer_create", loc);
            int w = static_cast<int>(expectNumber("layer_create", args, 0, loc));
            int h = static_cast<int>(expectNumber("layer_create", args, 1, loc));
            auto &g = GraphicsState::instance();
            SDL_Texture *texture = SDL_CreateTexture(g.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
            if (!texture)
                throw ModuleError(ErrorCode::UnknownDLC, "layer_create() could not create layer", loc, SDL_GetError());
            int id = g.nextLayerId++;
            g.layers[id] = texture;
            return Value::makeNumber(id);
        };

        reg["layer_begin"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("layer_begin", args, 1, loc);
            gfxRequireWindow("layer_begin", loc);
            int id = static_cast<int>(expectNumber("layer_begin", args, 0, loc));
            auto &g = GraphicsState::instance();
            auto it = g.layers.find(id);
            if (it == g.layers.end())
                throw ModuleError(ErrorCode::UnknownDLC, "layer_begin() unknown layer id", loc, "");
            g.layerTarget = it->second;
            SDL_SetRenderTarget(g.renderer, g.layerTarget);
            return Value::makeEmpty();
        };

        reg["layer_end"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("layer_end", args, 0, loc);
            gfxRequireWindow("layer_end", loc);
            auto &g = GraphicsState::instance();
            g.layerTarget = nullptr;
            SDL_SetRenderTarget(g.renderer, nullptr);
            return Value::makeEmpty();
        };

        reg["layer_draw"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("layer_draw", args, 3, loc);
            gfxRequireWindow("layer_draw", loc);
            int id = static_cast<int>(expectNumber("layer_draw", args, 0, loc));
            double x = expectNumber("layer_draw", args, 1, loc);
            double y = expectNumber("layer_draw", args, 2, loc);
            auto &g = GraphicsState::instance();
            auto it = g.layers.find(id);
            if (it == g.layers.end())
                throw ModuleError(ErrorCode::UnknownDLC, "layer_draw() unknown layer id", loc, "");
            int w = 0, h = 0;
            SDL_QueryTexture(it->second, nullptr, nullptr, &w, &h);
            SDL_Rect dst = gfxRect(x, y, w, h);
            SDL_RenderCopy(g.renderer, it->second, nullptr, &dst);
            return Value::makeEmpty();
        };

        reg["clear"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("clear", args, 3, loc);
            gfxRequireWindow("clear", loc);
            int r = static_cast<int>(expectNumber("clear", args, 0, loc));
            int g_ = static_cast<int>(expectNumber("clear", args, 1, loc));
            int b = static_cast<int>(expectNumber("clear", args, 2, loc));
            auto &g = GraphicsState::instance();
            SDL_SetRenderDrawColor(g.renderer, r, g_, b, 255);
            SDL_RenderClear(g.renderer);
            return Value::makeEmpty();
        };

        reg["present"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("present", args, 0, loc);
            gfxRequireWindow("present", loc);
            SDL_RenderPresent(GraphicsState::instance().renderer);
            return Value::makeEmpty();
        };

        reg["rect"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("rect", args, 7, loc);
            gfxRequireWindow("rect", loc);
            double x = expectNumber("rect", args, 0, loc);
            double y = expectNumber("rect", args, 1, loc);
            double w = expectNumber("rect", args, 2, loc);
            double h = expectNumber("rect", args, 3, loc);
            SDL_Rect rc = gfxRect(x, y, w, h);
            int r = static_cast<int>(expectNumber("rect", args, 4, loc));
            int g_ = static_cast<int>(expectNumber("rect", args, 5, loc));
            int b = static_cast<int>(expectNumber("rect", args, 6, loc));
            auto &g = GraphicsState::instance();
            SDL_SetRenderDrawColor(g.renderer, r, g_, b, 255);
            SDL_RenderFillRect(g.renderer, &rc);
            return Value::makeEmpty();
        };

        reg["line"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("line", args, 7, loc);
            gfxRequireWindow("line", loc);
            int x1 = static_cast<int>(expectNumber("line", args, 0, loc));
            int y1 = static_cast<int>(expectNumber("line", args, 1, loc));
            int x2 = static_cast<int>(expectNumber("line", args, 2, loc));
            int y2 = static_cast<int>(expectNumber("line", args, 3, loc));
            int r = static_cast<int>(expectNumber("line", args, 4, loc));
            int g_ = static_cast<int>(expectNumber("line", args, 5, loc));
            int b = static_cast<int>(expectNumber("line", args, 6, loc));
            auto &g = GraphicsState::instance();
            SDL_SetRenderDrawColor(g.renderer, r, g_, b, 255);
            SDL_RenderDrawLine(g.renderer, gfxX(x1), gfxY(y1), gfxX(x2), gfxY(y2));
            return Value::makeEmpty();
        };

        reg["circle"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("circle", args, 6, loc);
            gfxRequireWindow("circle", loc);
            int cx = static_cast<int>(expectNumber("circle", args, 0, loc));
            int cy = static_cast<int>(expectNumber("circle", args, 1, loc));
            int radius = static_cast<int>(expectNumber("circle", args, 2, loc));
            int r = static_cast<int>(expectNumber("circle", args, 3, loc));
            int g_ = static_cast<int>(expectNumber("circle", args, 4, loc));
            int b = static_cast<int>(expectNumber("circle", args, 5, loc));
            auto &g = GraphicsState::instance();
            SDL_SetRenderDrawColor(g.renderer, r, g_, b, 255);
            for (int y = -radius; y <= radius; ++y)
            {
                int span = static_cast<int>(std::sqrt(static_cast<double>(radius * radius - y * y)));
                SDL_RenderDrawLine(g.renderer, gfxX(cx - span), gfxY(cy + y), gfxX(cx + span), gfxY(cy + y));
            }
            return Value::makeEmpty();
        };

        reg["image_load"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("image_load", args, 1, loc);
            gfxRequireWindow("image_load", loc);
            const std::string &path = expectStr("image_load", args, 0, loc);
            auto &g = GraphicsState::instance();
            SDL_Texture *tex = IMG_LoadTexture(g.renderer, path.c_str());
            if (!tex)
                throw ModuleError(ErrorCode::UnknownDLC, "image_load() could not load '" + path + "'", loc, SDL_GetError());
            int id = g.nextImageId++;
            g.images[id] = tex;
            return Value::makeNumber(id);
        };

        reg["image_draw"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("image_draw", args, 3, loc);
            gfxRequireWindow("image_draw", loc);
            int id = static_cast<int>(expectNumber("image_draw", args, 0, loc));
            int x = static_cast<int>(expectNumber("image_draw", args, 1, loc));
            int y = static_cast<int>(expectNumber("image_draw", args, 2, loc));
            auto &g = GraphicsState::instance();
            auto it = g.images.find(id);
            if (it == g.images.end())
                throw ModuleError(ErrorCode::UnknownDLC, "image_draw() unknown image id", loc, "");
            int w = 0, h = 0;
            SDL_QueryTexture(it->second, nullptr, nullptr, &w, &h);
            SDL_Rect dst = gfxRect(x, y, w, h);
            SDL_RenderCopy(g.renderer, it->second, nullptr, &dst);
            return Value::makeEmpty();
        };

        reg["image_width"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("image_width", args, 1, loc);
            int id = static_cast<int>(expectNumber("image_width", args, 0, loc));
            auto &g = GraphicsState::instance();
            auto it = g.images.find(id);
            if (it == g.images.end())
                throw ModuleError(ErrorCode::UnknownDLC, "image_width() unknown image id", loc, "");
            int w = 0, h = 0;
            SDL_QueryTexture(it->second, nullptr, nullptr, &w, &h);
            return Value::makeNumber(w);
        };

        reg["image_height"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("image_height", args, 1, loc);
            int id = static_cast<int>(expectNumber("image_height", args, 0, loc));
            auto &g = GraphicsState::instance();
            auto it = g.images.find(id);
            if (it == g.images.end())
                throw ModuleError(ErrorCode::UnknownDLC, "image_height() unknown image id", loc, "");
            int w = 0, h = 0;
            SDL_QueryTexture(it->second, nullptr, nullptr, &w, &h);
            return Value::makeNumber(h);
        };

        reg["image_draw_ex"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("image_draw_ex", args, 5, loc);
            gfxRequireWindow("image_draw_ex", loc);
            int id = static_cast<int>(expectNumber("image_draw_ex", args, 0, loc));
            int x = static_cast<int>(expectNumber("image_draw_ex", args, 1, loc));
            int y = static_cast<int>(expectNumber("image_draw_ex", args, 2, loc));
            double scale = expectNumber("image_draw_ex", args, 3, loc);
            double rotation = expectNumber("image_draw_ex", args, 4, loc);
            auto &g = GraphicsState::instance();
            auto it = g.images.find(id);
            if (it == g.images.end())
                throw ModuleError(ErrorCode::UnknownDLC, "image_draw_ex() unknown image id", loc, "");
            int w = 0, h = 0;
            SDL_QueryTexture(it->second, nullptr, nullptr, &w, &h);
            SDL_Rect dst = gfxRect(x, y, w * scale, h * scale);
            SDL_RenderCopyEx(g.renderer, it->second, nullptr, &dst, rotation, nullptr, SDL_FLIP_NONE);
            return Value::makeEmpty();
        };

        reg["sprite_draw"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("sprite_draw", args, 9, loc);
            gfxRequireWindow("sprite_draw", loc);
            int id = static_cast<int>(expectNumber("sprite_draw", args, 0, loc));
            SDL_Rect src;
            src.x = static_cast<int>(expectNumber("sprite_draw", args, 1, loc));
            src.y = static_cast<int>(expectNumber("sprite_draw", args, 2, loc));
            src.w = static_cast<int>(expectNumber("sprite_draw", args, 3, loc));
            src.h = static_cast<int>(expectNumber("sprite_draw", args, 4, loc));
            int x = static_cast<int>(expectNumber("sprite_draw", args, 5, loc));
            int y = static_cast<int>(expectNumber("sprite_draw", args, 6, loc));
            double scale = expectNumber("sprite_draw", args, 7, loc);
            double rotation = expectNumber("sprite_draw", args, 8, loc);
            auto &g = GraphicsState::instance();
            auto it = g.images.find(id);
            if (it == g.images.end())
                throw ModuleError(ErrorCode::UnknownDLC, "sprite_draw() unknown image id", loc, "");
            SDL_Rect dst = gfxRect(x, y, src.w * scale, src.h * scale);
            SDL_RenderCopyEx(g.renderer, it->second, &src, &dst, rotation, nullptr, SDL_FLIP_NONE);
            return Value::makeEmpty();
        };

        reg["quad_create"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("quad_create", args, 4, loc);
            GfxQuad quad;
            quad.rect.x = static_cast<int>(expectNumber("quad_create", args, 0, loc));
            quad.rect.y = static_cast<int>(expectNumber("quad_create", args, 1, loc));
            quad.rect.w = static_cast<int>(expectNumber("quad_create", args, 2, loc));
            quad.rect.h = static_cast<int>(expectNumber("quad_create", args, 3, loc));
            auto &g = GraphicsState::instance();
            int id = g.nextQuadId++;
            g.quads[id] = quad;
            return Value::makeNumber(id);
        };

        reg["quad_draw"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("quad_draw", args, 6, loc);
            gfxRequireWindow("quad_draw", loc);
            int imageId = static_cast<int>(expectNumber("quad_draw", args, 0, loc));
            int quadId = static_cast<int>(expectNumber("quad_draw", args, 1, loc));
            double x = expectNumber("quad_draw", args, 2, loc);
            double y = expectNumber("quad_draw", args, 3, loc);
            double scale = expectNumber("quad_draw", args, 4, loc);
            double rotation = expectNumber("quad_draw", args, 5, loc);
            auto &g = GraphicsState::instance();
            SDL_Texture *texture = gfxImage("quad_draw", imageId, loc);
            auto quad = g.quads.find(quadId);
            if (quad == g.quads.end())
                throw ModuleError(ErrorCode::UnknownDLC, "quad_draw() unknown quad id", loc, "");
            SDL_Rect dst = gfxRect(x, y, quad->second.rect.w * scale, quad->second.rect.h * scale);
            SDL_RenderCopyEx(g.renderer, texture, &quad->second.rect, &dst, rotation, nullptr, SDL_FLIP_NONE);
            return Value::makeEmpty();
        };

        reg["image_tint"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("image_tint", args, 5, loc);
            SDL_Texture *texture = gfxImage("image_tint", static_cast<int>(expectNumber("image_tint", args, 0, loc)), loc);
            Uint8 r = static_cast<Uint8>(expectNumber("image_tint", args, 1, loc));
            Uint8 g = static_cast<Uint8>(expectNumber("image_tint", args, 2, loc));
            Uint8 b = static_cast<Uint8>(expectNumber("image_tint", args, 3, loc));
            Uint8 a = static_cast<Uint8>(expectNumber("image_tint", args, 4, loc));
            SDL_SetTextureColorMod(texture, r, g, b);
            SDL_SetTextureAlphaMod(texture, a);
            return Value::makeEmpty();
        };

        reg["font_load"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("font_load", args, 2, loc);
            gfxRequireWindow("font_load", loc);
            const std::string &path = expectStr("font_load", args, 0, loc);
            int size = static_cast<int>(expectNumber("font_load", args, 1, loc));
            TTF_Font *font = TTF_OpenFont(path.c_str(), size);
            if (!font)
                throw ModuleError(ErrorCode::UnknownDLC, "font_load() could not load '" + path + "'", loc, TTF_GetError());
            auto &g = GraphicsState::instance();
            int id = g.nextFontId++;
            g.fonts[id] = font;
            return Value::makeNumber(id);
        };

        reg["font_height"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("font_height", args, 1, loc);
            return Value::makeNumber(TTF_FontHeight(gfxFont("font_height", static_cast<int>(expectNumber("font_height", args, 0, loc)), loc)));
        };

        reg["text_width"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("text_width", args, 3, loc);
            TTF_Font *font = gfxFont("text_width", static_cast<int>(expectNumber("text_width", args, 0, loc)), loc);
            const std::string &text = expectStr("text_width", args, 1, loc);
            int tabSize = static_cast<int>(expectNumber("text_width", args, 2, loc));
            return Value::makeNumber(gfxTextWidth(font, gfxTabs(text, tabSize)));
        };

        reg["text_draw"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("text_draw", args, 9, loc);
            gfxRequireWindow("text_draw", loc);
            int fontId = static_cast<int>(expectNumber("text_draw", args, 0, loc));
            const std::string &source = expectStr("text_draw", args, 1, loc);
            double x = expectNumber("text_draw", args, 2, loc);
            double y = expectNumber("text_draw", args, 3, loc);
            Uint8 r = static_cast<Uint8>(expectNumber("text_draw", args, 4, loc));
            Uint8 g = static_cast<Uint8>(expectNumber("text_draw", args, 5, loc));
            Uint8 b = static_cast<Uint8>(expectNumber("text_draw", args, 6, loc));
            const std::string &align = expectStr("text_draw", args, 7, loc);
            int tabSize = static_cast<int>(expectNumber("text_draw", args, 8, loc));
            TTF_Font *font = gfxFont("text_draw", fontId, loc);
            std::string text = gfxTabs(source, tabSize);
            SDL_Color color{r, g, b, 255};
            size_t start = 0;
            int lineHeight = TTF_FontHeight(font);
            int line = 0;
            while (start <= text.size())
            {
                size_t end = text.find('\n', start);
                std::string value = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
                SDL_Surface *surface = TTF_RenderUTF8_Blended(font, value.c_str(), color);
                if (!surface)
                    throw ModuleError(ErrorCode::UnknownDLC, "text_draw() could not render text", loc, TTF_GetError());
                SDL_Texture *texture = SDL_CreateTextureFromSurface(GraphicsState::instance().renderer, surface);
                int width = surface->w;
                int height = surface->h;
                SDL_FreeSurface(surface);
                if (!texture)
                    throw ModuleError(ErrorCode::UnknownDLC, "text_draw() could not create text texture", loc, SDL_GetError());
                double drawX = x;
                if (align == "right")
                    drawX -= width;
                else if (align != "left")
                {
                    SDL_DestroyTexture(texture);
                    throw ValueError("text_draw() alignment must be left or right", loc);
                }
                SDL_Rect dst = gfxRect(drawX, y + line * lineHeight, width, height);
                SDL_RenderCopy(GraphicsState::instance().renderer, texture, nullptr, &dst);
                SDL_DestroyTexture(texture);
                if (end == std::string::npos)
                    break;
                start = end + 1;
                ++line;
            }
            return Value::makeEmpty();
        };

        reg["rect_overlap"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("rect_overlap", args, 8, loc);
            double x1 = expectNumber("rect_overlap", args, 0, loc);
            double y1 = expectNumber("rect_overlap", args, 1, loc);
            double w1 = expectNumber("rect_overlap", args, 2, loc);
            double h1 = expectNumber("rect_overlap", args, 3, loc);
            double x2 = expectNumber("rect_overlap", args, 4, loc);
            double y2 = expectNumber("rect_overlap", args, 5, loc);
            double w2 = expectNumber("rect_overlap", args, 6, loc);
            double h2 = expectNumber("rect_overlap", args, 7, loc);
            bool hit = x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2;
            return Value::makeBool(hit);
        };

        reg["circle_overlap"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("circle_overlap", args, 6, loc);
            double x1 = expectNumber("circle_overlap", args, 0, loc);
            double y1 = expectNumber("circle_overlap", args, 1, loc);
            double r1 = expectNumber("circle_overlap", args, 2, loc);
            double x2 = expectNumber("circle_overlap", args, 3, loc);
            double y2 = expectNumber("circle_overlap", args, 4, loc);
            double r2 = expectNumber("circle_overlap", args, 5, loc);
            double dx = x1 - x2;
            double dy = y1 - y2;
            double rr = r1 + r2;
            return Value::makeBool(dx * dx + dy * dy <= rr * rr);
        };

        reg["point_in_rect"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("point_in_rect", args, 6, loc);
            double px = expectNumber("point_in_rect", args, 0, loc);
            double py = expectNumber("point_in_rect", args, 1, loc);
            double x = expectNumber("point_in_rect", args, 2, loc);
            double y = expectNumber("point_in_rect", args, 3, loc);
            double w = expectNumber("point_in_rect", args, 4, loc);
            double h = expectNumber("point_in_rect", args, 5, loc);
            bool hit = px >= x && px <= x + w && py >= y && py <= y + h;
            return Value::makeBool(hit);
        };

        reg["key_down"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("key_down", args, 1, loc);
            gfxRequireWindow("key_down", loc);
            const std::string &name = expectStr("key_down", args, 0, loc);
            SDL_Scancode code = SDL_GetScancodeFromName(name.c_str());
            auto &g = GraphicsState::instance();
            if (code == SDL_SCANCODE_UNKNOWN || !g.keys || code >= g.keyCount)
                return Value::makeBool(false);
            return Value::makeBool(g.keys[code] != 0);
        };

        reg["mouse_x"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("mouse_x", args, 0, loc);
            return Value::makeNumber(GraphicsState::instance().mouseX);
        };

        reg["mouse_y"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("mouse_y", args, 0, loc);
            return Value::makeNumber(GraphicsState::instance().mouseY);
        };

        reg["mouse_down"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("mouse_down", args, 1, loc);
            int button = static_cast<int>(expectNumber("mouse_down", args, 0, loc));
            Uint32 mask = SDL_BUTTON(button);
            return Value::makeBool((GraphicsState::instance().mouseButtons & mask) != 0);
        };

        reg["sound_load"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("sound_load", args, 1, loc);
            gfxRequireWindow("sound_load", loc);
            const std::string &path = expectStr("sound_load", args, 0, loc);
            auto &g = GraphicsState::instance();
            Mix_Chunk *chunk = Mix_LoadWAV(path.c_str());
            if (!chunk)
                throw ModuleError(ErrorCode::UnknownDLC, "sound_load() could not load '" + path + "'", loc, Mix_GetError());
            int id = g.nextSoundId++;
            g.sounds[id] = chunk;
            return Value::makeNumber(id);
        };

        reg["sound_play"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("sound_play", args, 1, loc);
            int id = static_cast<int>(expectNumber("sound_play", args, 0, loc));
            auto &g = GraphicsState::instance();
            auto it = g.sounds.find(id);
            if (it == g.sounds.end())
                throw ModuleError(ErrorCode::UnknownDLC, "sound_play() unknown sound id", loc, "");
            Mix_PlayChannel(-1, it->second, 0);
            return Value::makeEmpty();
        };

        reg["music_play"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("music_play", args, 1, loc);
            gfxRequireWindow("music_play", loc);
            const std::string &path = expectStr("music_play", args, 0, loc);
            auto &g = GraphicsState::instance();
            if (g.music)
            {
                Mix_HaltMusic();
                Mix_FreeMusic(g.music);
            }
            g.music = Mix_LoadMUS(path.c_str());
            if (!g.music)
                throw ModuleError(ErrorCode::UnknownDLC, "music_play() could not load '" + path + "'", loc, Mix_GetError());
            Mix_PlayMusic(g.music, -1);
            return Value::makeEmpty();
        };

        reg["music_stop"] = [](std::vector<Value> &args, const SourceLocation &loc) -> Value
        {
            expectArgCount("music_stop", args, 0, loc);
            Mix_HaltMusic();
            return Value::makeEmpty();
        };
    }

} // namespace cuff
