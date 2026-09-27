#pragma once

#include "NativeFunctions.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <unordered_map>
#include <string>
#include <cmath>

namespace cuff
{

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
        std::unordered_map<int, Mix_Chunk *> sounds;
        int nextSoundId = 1;
        Mix_Music *music = nullptr;
        const Uint8 *keys = nullptr;
        int keyCount = 0;
        int mouseX = 0;
        int mouseY = 0;
        Uint32 mouseButtons = 0;

        void shutdown()
        {
            for (auto &p : images)
                SDL_DestroyTexture(p.second);
            images.clear();
            for (auto &p : sounds)
                Mix_FreeChunk(p.second);
            sounds.clear();
            if (music)
            {
                Mix_FreeMusic(music);
                music = nullptr;
            }
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
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                    g.closeRequested = true;
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
            SDL_Rect rc;
            rc.x = static_cast<int>(expectNumber("rect", args, 0, loc));
            rc.y = static_cast<int>(expectNumber("rect", args, 1, loc));
            rc.w = static_cast<int>(expectNumber("rect", args, 2, loc));
            rc.h = static_cast<int>(expectNumber("rect", args, 3, loc));
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
            SDL_RenderDrawLine(g.renderer, x1, y1, x2, y2);
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
                SDL_RenderDrawLine(g.renderer, cx - span, cy + y, cx + span, cy + y);
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
            SDL_Rect dst{x, y, w, h};
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
            SDL_Rect dst{x, y, static_cast<int>(w * scale), static_cast<int>(h * scale)};
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
            SDL_Rect dst{x, y, static_cast<int>(src.w * scale), static_cast<int>(src.h * scale)};
            SDL_RenderCopyEx(g.renderer, it->second, &src, &dst, rotation, nullptr, SDL_FLIP_NONE);
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
