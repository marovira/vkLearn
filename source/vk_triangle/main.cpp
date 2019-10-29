#include "Application.hpp"

#include <fmt/printf.h>
#include <stdexcept>

int main()
{
    Application app;
    try
    {
        app.run();
    }
    catch (std::exception const& e)
    {
        fmt::print("{}\n", e.what());
        return 1;
    }

    return 0;
}
