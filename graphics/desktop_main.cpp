#include "../engine/CuffEngine.h"
#include "../engine/interpreter/GraphicsDLC.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace cuff;

static std::string readFile(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string dirOf(const std::string &path)
{
    size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? "." : path.substr(0, slash);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: c2d <script.cuff>\n";
        return 1;
    }

    std::string path = argv[1];
    std::string source = readFile(path);
    if (source.empty())
    {
        std::cerr << "could not read '" << path << "'\n";
        return 1;
    }

    CuffEngine::Result parsed = CuffEngine::run(source, false);
    if (!parsed.success)
    {
        std::cerr << "ERROR: " << parsed.error << "\n";
        return 1;
    }

    Interpreter interp;
    try
    {
        interp.run(*parsed.ast, dirOf(path));

        std::vector<Value> noArgs;
        if (interp.hasFunction("on_load"))
            interp.callFunctionByName("on_load", noArgs);

        while (GraphicsState::instance().window && !GraphicsState::instance().closeRequested)
        {
            std::vector<Value> zero;
            interp.callNative("poll", zero);

            std::vector<Value> updateArgs{Value::makeNumber(GraphicsState::instance().dt)};
            if (interp.hasFunction("on_update"))
                interp.callFunctionByName("on_update", updateArgs);
            if (interp.hasFunction("on_draw"))
                interp.callFunctionByName("on_draw", noArgs);

            interp.callNative("present", zero);
        }
    }
    catch (const CuffError &e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        GraphicsState::instance().shutdown();
        return 1;
    }

    GraphicsState::instance().shutdown();
    return 0;
}
