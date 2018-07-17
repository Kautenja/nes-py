#include <boost/python.hpp>


char const* greet()
{
   return "hello, world";
}


BOOST_PYTHON_MODULE(hello_ext)
{
    using namespace boost::python;
    def("greet", greet);
}


// #include <Python.h>


// static PyObject * laines_init(PyObject *self, PyObject *args) {
//     const char *command;
//     int sts;

//     if (!PyArg_ParseTuple(args, "s", &command))
//         return NULL;
//     sts = system(command);
//     return Py_BuildValue("i", sts);
// }

// static PyObject * laines_reset(PyObject *self) {

// }

// static PyObject * laines_step(PyObject *self, PyObject *args) {

// }
