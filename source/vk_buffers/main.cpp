#include "Application.hpp"

#include <fmt/printf.h>
#include <stdexcept>

int main()
{
    Application app;
    try
    {
        app.init();
        app.run();
        app.exit();
    }
    catch (std::exception const& e)
    {
        fmt::print("{}\n", e.what());
    }
    return 0;
}
