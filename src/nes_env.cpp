#include <iostream>
#include <string>
#include "gui.hpp"


class NESEnv{

private:

public:
    NESEnv(wchar_t* path) {
        // convert the wchar_t type to a string
        std::wstring ws(path);
        std::string str(ws.begin(), ws.end());
    }

    // void bar(wchar_t* path){
    //     std::wcout << path << std::endl;
    //     // convert the wchar_t type to a string
    //     std::wstring ws(path);
    //     std::string str(ws.begin(), ws.end());
    //     // initialize the GUI with the given ROM path
    //     GUI::init(str);
    //     // run the GUI
    //     GUI::run();
    // }
};


extern "C" {
    NESEnv* NESEnv_init(wchar_t* path){ return new NESEnv(path); }

    // void Foo_bar(Foo* foo, wchar_t* path){ foo->bar(path); }
}
