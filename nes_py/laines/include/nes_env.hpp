#include <string>

class NESEnv {

private:
    std::string path;

public:
    NESEnv(wchar_t* path);
    void reset();
    void step(unsigned char action);

};
