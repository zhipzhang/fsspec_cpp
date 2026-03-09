#include <stdio.h>
#include <Python.h>

int main() {
    printf("Before Py_Initialize\n");
    fflush(stdout);
    
    Py_Initialize();
    printf("After Py_Initialize\n");
    fflush(stdout);
    
    PyObject* module = PyImport_ImportModule("fsspec");
    if (!module) {
        printf("Failed to import fsspec\n");
        PyErr_Print();
        return 1;
    }
    printf("Imported fsspec\n");
    fflush(stdout);
    
    PyObject* core = PyImport_ImportModule("fsspec.core");
    if (!core) {
        printf("Failed to import fsspec.core\n");
        PyErr_Print();
        return 1;
    }
    printf("Imported fsspec.core\n");
    fflush(stdout);
    
    Py_DECREF(core);
    Py_DECREF(module);
    
    printf("Done\n");
    return 0;
}
