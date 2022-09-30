#pragma once

class ILIDrawer {
  public:
    void drawrect(void);
    void drawdot(void);
    //...

  private:
    // configs
};

class ClampMeasData { };

template<typename DrawingDriver>
class ClampMeterDrawer {
  public:
    void drawMainMenu();
    void drawData(ClampMeasData &);

  private:
    // configs
};