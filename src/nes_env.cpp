#include <iostream>
#include <string>
#include "gui.hpp"


class Foo{
    public:
        void bar(wchar_t* path){
            std::wcout << path << std::endl;
                // initialize the GUI with the given ROM path
            std::wstring ws(path);
            // your new String
            std::string str(ws.begin(), ws.end());
            GUI::init(str);
            // run the GUI
            GUI::run();
        }
};


extern "C" {
    Foo* Foo_new(){ return new Foo(); }
    void Foo_bar(Foo* foo, wchar_t* path){ foo->bar(path); }
}
