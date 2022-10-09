#pragma once
#ifdef min
#undef min
#endif   // max
#ifdef max
#undef max
#endif   // max
#ifdef printf
#undef printf
#endif

#include <memory>
#include "page.hpp"

class Menu {
  private:
    std::unique_ptr<AbstractPage> mainPage;

};