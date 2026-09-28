#include <emscripten.h>
#include "../engine/CuffEngine.h"
#include "../engine/interpreter/GraphicsDLC.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace cuff;

static CuffEngine::Result *parsed = nullptr;
static Interpreter *interp = nullptr;

static void frame()
{
    auto &g = GraphicsState::instance();
    if (g.closeRequested)
    {
        emscripten_cancel_main_loop();
        return;
    }

    try
    {
        std::vector<Value> zero;
        std::vector<Value> noArgs;
        interp->callNative("poll", zero);

        std::vector<Value> updateArgs{Value::makeNumber(g.dt)};
        if (interp->hasFunction("on_update"))
            interp->callFunctionByName("on_update", updateArgs);
        if (interp->hasFunction("on_draw"))
            interp->callFunctionByName("on_draw", noArgs);

        interp->callNative("present", zero);
    }
    catch (const CuffError &e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        emscripten_cancel_main_loop();
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return 1;

    std::ifstream f(argv[1], std::ios::binary);
    if (!f)
        return 1;
    std::ostringstream ss;
    ss << f.rdbuf();

    parsed = new CuffEngine::Result(CuffEngine::run(ss.str(), false));
    if (!parsed->success)
    {
        std::cerr << "ERROR: " << parsed->error << "\n";
        return 1;
    }

    interp = new Interpreter();
    try
    {
        interp->run(*parsed->ast, "/");

        std::vector<Value> noArgs;
        if (interp->hasFunction("on_load"))
            interp->callFunctionByName("on_load", noArgs);
    }
    catch (const CuffError &e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    if (!GraphicsState::instance().window)
        return 1;

    emscripten_set_main_loop(frame, 0, 1);
    return 0;
}
