#include <iostream>
#include <string>
#include "gui.hpp"


class Foo{
    public:
        void bar(wchar_t* path){
            std::wcout << path << std::endl;
            // convert the wchar_t type to a string
            std::wstring ws(path);
            std::string str(ws.begin(), ws.end());
            // initialize the GUI with the given ROM path
            GUI::init(str);
            // run the GUI
            GUI::run();
        }
};


extern "C" {
    Foo* Foo_new(){ return new Foo(); }
    void Foo_bar(Foo* foo, wchar_t* path){ foo->bar(path); }
}
