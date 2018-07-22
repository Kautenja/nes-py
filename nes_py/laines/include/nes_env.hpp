#include <string>

class NESEnv {
public:
    NESEnv(wchar_t* path);
    void reset();
    void step(unsigned char action);
};
