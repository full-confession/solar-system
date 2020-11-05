#include "App.hpp"


int main(int const argc, char const* const argv[])
{
    auto width = 1600;
    auto height = 900;

    if(argc == 3)
    {
        std::cout << argv[1] << std::endl;
        std::cout << argv[2] << std::endl;
        width = std::atoi(argv[1]);
        height = std::atoi(argv[2]);

        if(width == 0 || height == 0)
        {
            width = 1600;
            height = 900;
        }
    }

    try
    {
        App app{ width, height };
        app.Run();
    }
    catch(std::exception const& e)
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
