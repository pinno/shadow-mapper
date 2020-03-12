#define PY_SSIZE_T_CLEAN
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <numpy/arrayobject.h>

/* -------------------------------------------------------------------------- */

static PyObject *shadowmap_calculate(PyObject*, PyObject*);

/* -------------------------------------------------------------------------- */

static int is_floatmatrix(PyArrayObject *mat) {
    if (PyArray_TYPE(mat) != NPY_DOUBLE || PyArray_NDIM(mat) != 2)  {
        PyErr_SetString(PyExc_ValueError,
            "Array must be of type Float and 2 dimensional.");
        return 0;
    }
    return 1;
}

/* -------------------------------------------------------------------------- */

static double **ptrvector(long n)  {
    double **v;
    v = (double **)malloc((size_t) (n * sizeof(double)));
    if (!v) {
        printf("Allocation of memory for double array failed.");
        exit(0);
    }
    return v;
}

/* -------------------------------------------------------------------------- */

static void free_Carrayptrs(double **v)  {
    free((char*) v);
}

/* -------------------------------------------------------------------------- */

static double **pymatrix_to_Carrayptrs(PyArrayObject *arrayin) {
    double **c, *a;
    int i,n,m;
    npy_intp *dims = PyArray_DIMS(arrayin);

    n = dims[0];
    m = dims[1];
    c = ptrvector(n);
    a = (double *) PyArray_DATA(arrayin);  /* pointer to arrayin data as double */
    for (i = 0; i < n; i++)  {
        c[i] = a + i * m;
    }
    return c;
}

/* -------------------------------------------------------------------------- */

static PyObject *shadowmap_calculate(PyObject *self, PyObject *args) {
    PyArrayObject *heightmap_arr, *shadowmap_arr;
    double sun_x, sun_y, sun_z, view_alt, z_max, x, y, z;
    double **shadowmap, **heightmap;
    npy_intp *dims;
    int i, j, lit;
    
    Py_Initialize();     
    import_array(); 

    if (!PyArg_ParseTuple(args, "O!ddddd", &PyArray_Type, &heightmap_arr,
        &sun_x, &sun_y, &sun_z, &view_alt, &z_max)) {
        return NULL;
    } 

    if (heightmap_arr == NULL || !is_floatmatrix(heightmap_arr)) {
        return NULL;
    }

    dims = PyArray_DIMS(heightmap_arr);
    shadowmap_arr = (PyArrayObject*) PyArray_ZEROS(2, dims, NPY_DOUBLE, 0);

    heightmap = pymatrix_to_Carrayptrs(heightmap_arr);
    shadowmap = pymatrix_to_Carrayptrs(shadowmap_arr);

    for (i = 0; i < dims[0]; i++) {
        for (j = 0; j < dims[1]; j++) {
            x = (double)j;
            y = (double)i;
            z = heightmap[i][j] + view_alt;
            lit = 1;

            while (x >= 0 && x < dims[1] && y >= 0 && y < dims[0] && z <= z_max) {
                if (z < heightmap[(int)y][(int)x]) {
                    lit = 0;
                    break;
                }
                x += sun_x;
                y += sun_y;
                z += sun_z;
            }

            shadowmap[i][j] = lit;
        }
    }

    free_Carrayptrs(heightmap);
    free_Carrayptrs(shadowmap);

    return PyArray_Return(shadowmap_arr);
}

/* -------------------------------------------------------------------------- */

static PyMethodDef ShadowMapMethods[] = {
    {"calculate",  shadowmap_calculate, METH_VARARGS,
     "Calculate a shadowmap."},
     {NULL}
};

/* -------------------------------------------------------------------------- */

static struct PyModuleDef c_shadowmap =
{
  PyModuleDef_HEAD_INIT,
  "c_shadowmap",
  NULL,
  -1,
  ShadowMapMethods
  };

/* -------------------------------------------------------------------------- */

PyMODINIT_FUNC PyInit_c_shadowmap(void){ 
  import_array();
  return PyModule_Create(&c_shadowmap);
}

/* -------------------------------------------------------------------------- */
