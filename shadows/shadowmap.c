#define PY_SSIZE_T_CLEAN
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <numpy/arrayobject.h>

/* -------------------------------------------------------------------------- */

static PyObject *shadowmap_raster_f(PyObject*, PyObject*);
static PyObject *shadowmap_raster_d(PyObject*, PyObject*);
static PyObject *shadowmap_indexes_f(PyObject*, PyObject*);
static PyObject *shadowmap_indexes_d(PyObject*, PyObject*);

/* -------------------------------------------------------------------------- */

static int is_floatmatrix(PyArrayObject *mat) {
    if (PyArray_TYPE(mat) != NPY_FLOAT || PyArray_NDIM(mat) != 2)  {
        PyErr_SetString(PyExc_ValueError,
            "Array must be of type Float and 2 dimensional.");
        return 0;
    }
    return 1;
}

/* -------------------------------------------------------------------------- */

static int is_doublematrix(PyArrayObject *mat) {
    if (PyArray_TYPE(mat) != NPY_DOUBLE || PyArray_NDIM(mat) != 2)  {
        PyErr_SetString(PyExc_ValueError,
            "Array must be of type Double and 2 dimensional.");
        return 0;
    }
    return 1;
}

/* -------------------------------------------------------------------------- */

static uint16_t is_uint16matrix(PyArrayObject *mat) {
    if (PyArray_TYPE(mat) != NPY_UINT16 || PyArray_NDIM(mat) != 2)  {
        PyErr_SetString(PyExc_ValueError,
            "Array must be of type UINT16 and 2 dimensional.");
        return 0;
    }
    return 1;
}

/* -------------------------------------------------------------------------- */

static float **ptrfloatvector(long n)  {
    float **v;
    v = malloc((size_t) (n * sizeof(float*)));
    if (!v) {
        printf("Allocation of memory for float array failed.");
        exit(0);
    }
    return v;
}

/* -------------------------------------------------------------------------- */

static double **ptrdoublevector(long n)  {
    double **v;
    v = (double **)malloc((size_t) (n * sizeof(double)));
    if (!v) {
        printf("Allocation of memory for double array failed.");
        exit(0);
    }
    return v;
}

/* -------------------------------------------------------------------------- */

static uint8_t **ptruint8vector(long n)  {
    uint8_t **v;
    v = malloc((size_t) (n * sizeof(uint8_t*)));
    if (!v) {
        printf("Allocation of memory for uint8_t array failed.");
        exit(0);
    }
    return v;
}

/* -------------------------------------------------------------------------- */

static uint16_t **ptruint16vector(long n)  {
    uint16_t **v;
    v = malloc((size_t) (n * sizeof(uint16_t*)));
    if (!v) {
        printf("Allocation of memory for uint16_t array failed.");
        exit(0);
    }
    return v;
}

/* -------------------------------------------------------------------------- */

static void free_floatCarrayptrs(float **v)  {
    free((char*) v);
}

/* -------------------------------------------------------------------------- */

static void free_doubleCarrayptrs(double **v)  {
    free((char*) v);
}

/* -------------------------------------------------------------------------- */

static void free_uint8Carrayptrs(uint8_t **v)  {
    free((char*) v);
}

/* -------------------------------------------------------------------------- */

static void free_uint16Carrayptrs(uint16_t **v)  {
    free((char*) v);
}

/* -------------------------------------------------------------------------- */

static float **pymatrix_to_floatCarrayptrs(PyArrayObject *arrayin) {
    float **c, *a;
    int i, n, m;
    npy_intp *dims = PyArray_DIMS(arrayin);

    n = dims[0];
    m = dims[1];
    c = ptrfloatvector(n);
    a = (float *) PyArray_DATA(arrayin);
    for (i = 0; i < n; i++)  {
        c[i] = a + i * m;
    }
    return c;
}

/* -------------------------------------------------------------------------- */

static double **pymatrix_to_doubleCarrayptrs(PyArrayObject *arrayin) {
    double **c, *a;
    int i, n, m;
    npy_intp *dims = PyArray_DIMS(arrayin);

    n = dims[0];
    m = dims[1];
    c = ptrdoublevector(n);
    a = (double *) PyArray_DATA(arrayin);
    for (i = 0; i < n; i++)  {
        c[i] = a + i * m;
    }
    return c;
}

/* -------------------------------------------------------------------------- */

static uint8_t **pymatrix_to_uint8Carrayptrs(PyArrayObject *arrayin) {
    uint8_t **c, *a;
    int i, n, m;
    npy_intp *dims = PyArray_DIMS(arrayin);

    n = dims[0];
    m = dims[1];
    c = ptruint8vector(n);
    a = (uint8_t *) PyArray_DATA(arrayin);
    for (i = 0; i < n; i++)  {
        c[i] = a + i * m;
    }
    return c;
}

/* -------------------------------------------------------------------------- */

static uint16_t **pymatrix_to_uint16Carrayptrs(PyArrayObject *arrayin) {
    uint16_t **c, *a;
    int i, n, m;
    npy_intp *dims = PyArray_DIMS(arrayin);

    n = dims[0];
    m = dims[1];
    c = ptruint16vector(n);
    a = (uint16_t *) PyArray_DATA(arrayin);
    for (i = 0; i < n; i++)  {
        c[i] = a + i * m;
    }
    return c;
}

/* -------------------------------------------------------------------------- */

static PyObject *shadowmap_raster_f(PyObject *self, PyObject *args) {
    PyArrayObject *heightmap_arr, *shadowmap_arr;
    float sun_x, sun_y, sun_z, view_alt, z_max, x, y, z;
    uint8_t **shadowmap;
    float **heightmap;
    npy_intp *dims;
    int i, j, lit;

    Py_Initialize();
    import_array();

    if (!PyArg_ParseTuple(args, "O!fffff", &PyArray_Type, &heightmap_arr,
        &sun_x, &sun_y, &sun_z, &view_alt, &z_max)) {
        return NULL;
    }

    if (heightmap_arr == NULL || !is_floatmatrix(heightmap_arr)) {
        return NULL;
    }

    dims = PyArray_DIMS(heightmap_arr);
    shadowmap_arr = (PyArrayObject*) PyArray_ZEROS(2, dims, NPY_UINT8, 0);

    heightmap = pymatrix_to_floatCarrayptrs(heightmap_arr);
    shadowmap = pymatrix_to_uint8Carrayptrs(shadowmap_arr);

    for (i = 0; i < dims[0]; i++) {
        for (j = 0; j < dims[1]; j++) {
            x = (float)j;
            y = (float)i;
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

    free_floatCarrayptrs(heightmap);
    free_uint8Carrayptrs(shadowmap);

    return PyArray_Return(shadowmap_arr);
}

/* -------------------------------------------------------------------------- */

static PyObject *shadowmap_raster_d(PyObject *self, PyObject *args) {
    PyArrayObject *heightmap_arr, *shadowmap_arr;
    double sun_x, sun_y, sun_z, view_alt, z_max, x, y, z;
    uint8_t **shadowmap;
    double **heightmap;
    npy_intp *dims;
    int i, j, lit;

    Py_Initialize();
    import_array();

    if (!PyArg_ParseTuple(args, "O!ddddd", &PyArray_Type, &heightmap_arr,
        &sun_x, &sun_y, &sun_z, &view_alt, &z_max)) {
        return NULL;
    }

    if (heightmap_arr == NULL || !is_doublematrix(heightmap_arr)) {
        return NULL;
    }

    dims = PyArray_DIMS(heightmap_arr);
    shadowmap_arr = (PyArrayObject*) PyArray_ZEROS(2, dims, NPY_UINT8, 0);

    heightmap = pymatrix_to_doubleCarrayptrs(heightmap_arr);
    shadowmap = pymatrix_to_uint8Carrayptrs(shadowmap_arr);

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

    free_doubleCarrayptrs(heightmap);
    free_uint8Carrayptrs(shadowmap);

    return PyArray_Return(shadowmap_arr);
}

/* -------------------------------------------------------------------------- */

static PyObject *shadowmap_indexes_f(PyObject *self, PyObject *args) {
    PyArrayObject *heightmap_arr, *row_idxs_arr, *col_idxs_arr, *shadowmap_arr;
    float z_max, x, y, z;
    float sun_x, sun_y, sun_z;
    float **heightmap;
    uint8_t **shadowmap;
    uint16_t **row_idxs, **col_idxs;
    npy_intp *dims, *idxs_dims;
    uint8_t lit, shade_value, lit_value;
    uint16_t i, j;
    uint32_t k;

    Py_Initialize();
    import_array();

    if (!PyArg_ParseTuple(args, "O!O!O!ffffBB",
        &PyArray_Type, &heightmap_arr, &PyArray_Type, &row_idxs_arr,
        &PyArray_Type, &col_idxs_arr,
        &sun_x, &sun_y, &sun_z, &z_max, &shade_value, &lit_value)) {
        return NULL;
    }

    if (heightmap_arr == NULL || !is_floatmatrix(heightmap_arr) ||
       row_idxs_arr == NULL || !is_uint16matrix(row_idxs_arr) ||
       col_idxs_arr == NULL || !is_uint16matrix(col_idxs_arr)) {
        return NULL;
    }

    dims = PyArray_DIMS(heightmap_arr);
    idxs_dims = PyArray_DIMS(row_idxs_arr);
    shadowmap_arr = (PyArrayObject*) PyArray_ZEROS(2, idxs_dims, NPY_UINT8, 0);

    heightmap = pymatrix_to_floatCarrayptrs(heightmap_arr);
    row_idxs = pymatrix_to_uint16Carrayptrs(row_idxs_arr);
    col_idxs = pymatrix_to_uint16Carrayptrs(col_idxs_arr);
    shadowmap = pymatrix_to_uint8Carrayptrs(shadowmap_arr);

    for (k = 0; k < idxs_dims[0]; k++) {
        i = *row_idxs[k];
        j = *col_idxs[k];
        lit = lit_value;
        x = (float)j;
        y = (float)i;
        z = heightmap[i][j];

        while (x >= 0 && x < dims[1] && y >= 0 && y < dims[0] && z <= z_max) {
            if (z < heightmap[(int)y][(int)x]) {
                lit = shade_value;
                break;
            }
            x += sun_x;
            y += sun_y;
            z += sun_z;

        }
        *shadowmap[k] = lit;

    }

    free_floatCarrayptrs(heightmap);
    free_uint8Carrayptrs(shadowmap);
    free_uint16Carrayptrs(row_idxs);
    free_uint16Carrayptrs(col_idxs);

    return PyArray_Return(shadowmap_arr);
}

/* -------------------------------------------------------------------------- */

static PyObject *shadowmap_indexes_d(PyObject *self, PyObject *args) {
    PyArrayObject *heightmap_arr, *row_idxs_arr, *col_idxs_arr, *shadowmap_arr;
    double z_max, x, y, z;
    double sun_x, sun_y, sun_z;
    double **heightmap;
    uint8_t **shadowmap;
    uint16_t **row_idxs, **col_idxs;
    npy_intp *dims, *idxs_dims;
    uint8_t lit, shade_value, lit_value;
    uint16_t i, j;
    uint32_t k;

    Py_Initialize();
    import_array();

    if (!PyArg_ParseTuple(args, "O!O!O!ddddBB",
        &PyArray_Type, &heightmap_arr, &PyArray_Type, &row_idxs_arr,
        &PyArray_Type, &col_idxs_arr,
        &sun_x, &sun_y, &sun_z, &z_max, &shade_value, &lit_value)) {
        return NULL;
    }

    if (heightmap_arr == NULL || !is_doublematrix(heightmap_arr) ||
       row_idxs_arr == NULL || !is_uint16matrix(row_idxs_arr) ||
       col_idxs_arr == NULL || !is_uint16matrix(col_idxs_arr)) {
        return NULL;
    }

    dims = PyArray_DIMS(heightmap_arr);
    idxs_dims = PyArray_DIMS(row_idxs_arr);
    shadowmap_arr = (PyArrayObject*) PyArray_ZEROS(2, idxs_dims, NPY_UINT8, 0);

    heightmap = pymatrix_to_doubleCarrayptrs(heightmap_arr);
    row_idxs = pymatrix_to_uint16Carrayptrs(row_idxs_arr);
    col_idxs = pymatrix_to_uint16Carrayptrs(col_idxs_arr);
    shadowmap = pymatrix_to_uint8Carrayptrs(shadowmap_arr);

    for (k = 0; k < idxs_dims[0]; k++) {
        i = *row_idxs[k];
        j = *col_idxs[k];
        lit = lit_value;
        x = (double)j;
        y = (double)i;
        z = heightmap[i][j];

        while (x >= 0 && x < dims[1] && y >= 0 && y < dims[0] && z <= z_max) {
            if (z < heightmap[(int)y][(int)x]) {
                lit = shade_value;
                break;
            }
            x += sun_x;
            y += sun_y;
            z += sun_z;

        }
        *shadowmap[k] = lit;

    }

    free_doubleCarrayptrs(heightmap);
    free_uint8Carrayptrs(shadowmap);
    free_uint16Carrayptrs(row_idxs);
    free_uint16Carrayptrs(col_idxs);

    return PyArray_Return(shadowmap_arr);
}

/* -------------------------------------------------------------------------- */

static PyMethodDef ShadowMapMethods[] = {
    {"shadowmap_raster_f",  shadowmap_raster_f, METH_VARARGS,
     "Compute shadows for all the points within an elevation map (single precision)."},
    {"shadowmap_raster_d",  shadowmap_raster_d, METH_VARARGS,
     "Compute shadows for all the points within an elevation map (double precision)."},
    {"shadowmap_indexes_f",  shadowmap_indexes_f, METH_VARARGS,
     "Compute shadows for a selection of points within an elevation map (single precision)."},
    {"shadowmap_indexes_d",  shadowmap_indexes_d, METH_VARARGS,
     "Compute shadows for a selection of points within an elevation map (double precision)."},
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
