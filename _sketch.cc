//
// Python 2/3 compatibility: PyInt and PyLong
//

// Must be first.
#include <Python.h>

#if (PY_MAJOR_VERSION >= 3)
#define PyInt_Check(arg) PyLong_Check(arg)
#define PyInt_AsLong(arg) PyLong_AsLong(arg)
#define PyInt_FromLong(arg) PyLong_FromLong(arg)
#endif

//
// Python 2/3 compatibility: PyBytes and PyString
// https://docs.python.org/2/howto/cporting.html#str-unicode-unification
//

#include "bytesobject.h"

//
// Python 2/3 compatibility: Module initialization
// http://python3porting.com/cextensions.html#module-initialization
//

#if PY_MAJOR_VERSION >= 3
#define MOD_ERROR_VAL NULL
#define MOD_SUCCESS_VAL(val) val
#define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#define MOD_DEF(ob, name, doc, methods) \
          static struct PyModuleDef moduledef = { \
            PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
          ob = PyModule_Create(&moduledef);
#else
#define MOD_ERROR_VAL
#define MOD_SUCCESS_VAL(val)
#define MOD_INIT(name) void init##name(void)
#define MOD_DEF(ob, name, doc, methods) \
          ob = Py_InitModule3(name, methods, doc);
#endif

//
// Function necessary for Python loading:
//

extern "C" {
    MOD_INIT(_sketch);
}


#include <string>
#include <set>
#include <map>
#include <exception>
#include <iostream>

#include "third-party/smhasher/MurmurHash3.h"

typedef unsigned long long HashIntoType;
typedef std::set<HashIntoType> CMinHashType;
int _hash_murmur(const std::string& kmer);


////

class KmerMinHash
{
public:
  unsigned int num;
  unsigned int ksize;
  long int prime;
  bool is_protein;
  CMinHashType mins;

  KmerMinHash(unsigned int n, unsigned int k, long int p, bool prot) :
    num(n), ksize(k), prime(p), is_protein(prot) { };

  void add_hash(long int h)
  {
    h = ((h % prime) + prime) % prime;
    mins.insert(h);

    if (mins.size() > num) {
      CMinHashType::iterator mi = mins.end();
      mi--;
      mins.erase(mi);
    }
  }
};

////

typedef struct {
  PyObject_HEAD
  KmerMinHash * mh;
} sketch_MinHash_Object;

static
void
sketch_MinHash_dealloc(sketch_MinHash_Object * obj)
{
  delete obj->mh;
  obj->mh = NULL;
  Py_TYPE(obj)->tp_free((PyObject*)obj);
}

static std::map<std::string, std::string> * _codon_table = NULL;

static std::string _dna_to_aa(const std::string& dna)
{
  if (_codon_table == NULL) {
    _codon_table = new std::map<std::string, std::string>;
    (*_codon_table)["TTT"] = "F";
    (*_codon_table)["TTC"] = "F";
    (*_codon_table)["TTA"] = "L";
    (*_codon_table)["TTG"] = "L";
    
    (*_codon_table)["TCT"] = "S";
    (*_codon_table)["TCC"] = "S";
    (*_codon_table)["TCA"] = "S";
    (*_codon_table)["TCG"] = "S";
    
    (*_codon_table)["TAT"] = "Y";
    (*_codon_table)["TAC"] = "Y";
    (*_codon_table)["TAA"] = "*";
    (*_codon_table)["TAG"] = "*";
    
    (*_codon_table)["TGT"] = "C";
    (*_codon_table)["TGC"] = "C";
    (*_codon_table)["TGA"] = "*";
    (*_codon_table)["TGG"] = "W";
    
    (*_codon_table)["CTT"] = "L";
    (*_codon_table)["CTC"] = "L";
    (*_codon_table)["CTA"] = "L";
    (*_codon_table)["CTG"] = "L";
    
    (*_codon_table)["CCT"] = "P";
    (*_codon_table)["CCC"] = "P";
    (*_codon_table)["CCA"] = "P";
    (*_codon_table)["CCG"] = "P";
    
    (*_codon_table)["CAT"] = "H";
    (*_codon_table)["CAC"] = "H";
    (*_codon_table)["CAA"] = "Q";
    (*_codon_table)["CAG"] = "Q";
    
    (*_codon_table)["CGT"] = "R";
    (*_codon_table)["CGC"] = "R";
    (*_codon_table)["CGA"] = "R";
    (*_codon_table)["CGG"] = "R";
    
    (*_codon_table)["ATT"] = "I";
    (*_codon_table)["ATC"] = "I";
    (*_codon_table)["ATA"] = "I";
    (*_codon_table)["ATG"] = "M";
    
    (*_codon_table)["ACT"] = "T";
    (*_codon_table)["ACC"] = "T";
    (*_codon_table)["ACA"] = "T";
    (*_codon_table)["ACG"] = "T";
    
    (*_codon_table)["AAT"] = "N";
    (*_codon_table)["AAC"] = "N";
    (*_codon_table)["AAA"] = "K";
    (*_codon_table)["AAG"] = "K";
    
    (*_codon_table)["AGT"] = "S";
    (*_codon_table)["AGC"] = "S";
    (*_codon_table)["AGA"] = "R";
    (*_codon_table)["AGG"] = "R";
    
    (*_codon_table)["GTT"] = "V";
    (*_codon_table)["GTC"] = "V";
    (*_codon_table)["GTA"] = "V";
    (*_codon_table)["GTG"] = "V";
    
    (*_codon_table)["GCT"] = "A";
    (*_codon_table)["GCC"] = "A";
    (*_codon_table)["GCA"] = "A";
    (*_codon_table)["GCG"] = "A";
    
    (*_codon_table)["GAT"] = "D";
    (*_codon_table)["GAC"] = "D";
    (*_codon_table)["GAA"] = "E";
    (*_codon_table)["GAG"] = "E";
    
    (*_codon_table)["GGT"] = "G";
    (*_codon_table)["GGC"] = "G";
    (*_codon_table)["GGA"] = "G";
    (*_codon_table)["GGG"] = "G";
  }

  std::string aa;
  for (unsigned int j = 0; j < dna.size(); j += 3) {
    std::string codon = dna.substr(j, 3);
    aa += (*_codon_table)[codon];
  }
  return aa;
}

std::string _revcomp(const std::string& kmer);

static
PyObject *
minhash_add_sequence(sketch_MinHash_Object * me, PyObject * args)
{
  const char * sequence = NULL;
  if (!PyArg_ParseTuple(args, "s", &sequence)) {
    return NULL;
  }
  KmerMinHash * mh = me->mh;
  CMinHashType::iterator mins_end;
  
  long int h = 0;
  unsigned int ksize = mh->ksize;

  if (!mh->is_protein) {
    std::string seq = sequence;
    for (unsigned int i = 0; i < seq.length() - ksize + 1; i++) {
      std::string kmer = seq.substr(i, ksize);

      h = _hash_murmur(kmer);
      mh->add_hash(h);
    }
  } else {                      // protein
    std::string seq = sequence;
    for (unsigned int i = 0; i < seq.length() - ksize + 1; i ++) {
      std::string kmer = seq.substr(i, ksize);
      std::string aa = _dna_to_aa(kmer);

      h = _hash_murmur(aa);
      mh->add_hash(h);
      
      std::string rc = _revcomp(kmer);
      aa = _dna_to_aa(rc);

      h = _hash_murmur(aa);
      mh->add_hash(h);
    }
  }
    
  Py_INCREF(Py_None);
  return Py_None;
}

static
PyObject *
minhash_add_hash(sketch_MinHash_Object * me, PyObject * args)
{
  long int hh;
  if (!PyArg_ParseTuple(args, "l", &hh)) {
    return NULL;
  }

  me->mh->add_hash(hh);

  Py_INCREF(Py_None);
  return Py_None;
}

static
PyObject *
minhash_get_mins(sketch_MinHash_Object * me, PyObject * args)
{
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }

  KmerMinHash * mh = me->mh;
  PyObject * mins_o = PyList_New(mh->mins.size());

  unsigned int j = 0;
  for (CMinHashType::iterator i = mh->mins.begin(); i != mh->mins.end(); ++i) {
    PyList_SET_ITEM(mins_o, j, PyLong_FromUnsignedLongLong(*i));
    j++;
  }
  return(mins_o);
}

static PyMethodDef sketch_MinHash_methods [] = {
  { "add_sequence",
    (PyCFunction)minhash_add_sequence, METH_VARARGS,
    "Add kmer into MinHash"
  },
  { "add_hash",
    (PyCFunction)minhash_add_hash, METH_VARARGS,
    "Add kmer into MinHash"
  },
  { "get_mins",
    (PyCFunction)minhash_get_mins, METH_VARARGS,
    "Get MinHash signature"
  },
  { NULL, NULL, 0, NULL } // sentinel
};

static
PyObject *
sketch_MinHash_new(PyTypeObject * subtype, PyObject * args, PyObject * kwds)
{
    PyObject * self     = subtype->tp_alloc( subtype, 1 );
    if (self == NULL) {
        return NULL;
    }

    unsigned int _n, _ksize;
    long int _p;
    PyObject * is_protein_o;
    if (!PyArg_ParseTuple(args, "IIlO", &_n, &_ksize, &_p, &is_protein_o)){
      return NULL;
    }
    
    sketch_MinHash_Object * myself = (sketch_MinHash_Object *)self;

    myself->mh = new KmerMinHash(_n, _ksize, _p,\
                                 PyObject_IsTrue(is_protein_o));

    return self;
}

static PyTypeObject sketch_MinHash_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)        /* init & ob_size */
    "_sketch.MinHash",                    /* tp_name */
    sizeof(sketch_MinHash_Object),         /* tp_basicsize */
    0,                                    /* tp_itemsize */
    (destructor)sketch_MinHash_dealloc,    /* tp_dealloc */
    0,                                    /* tp_print */
    0,                                    /* tp_getattr */
    0,                                    /* tp_setattr */
    0,                                    /* tp_compare */
    0,                                    /* tp_repr */
    0,                                    /* tp_as_number */
    0,                                    /* tp_as_sequence */
    0,                                    /* tp_as_mapping */
    0,                                    /* tp_hash */
    0,                                    /* tp_call */
    0,                                    /* tp_str */
    0,                                    /* tp_getattro */
    0,                                    /* tp_setattro */
    0,                                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                   /* tp_flags */
    "A MinHash sketch.",                  /* tp_doc */
    0,                                    /* tp_traverse */
    0,                                    /* tp_clear */
    0,                                    /* tp_richcompare */
    0,                                    /* tp_weaklistoffset */
    0,                                    /* tp_iter */
    0,                                    /* tp_iternext */
    sketch_MinHash_methods,               /* tp_methods */
    0,                                    /* tp_members */
    0,                                    /* tp_getset */
    0,                                         /* tp_base */
    0,                                         /* tp_dict */
    0,                                         /* tp_descr_get */
    0,                                         /* tp_descr_set */
    0,                                         /* tp_dictoffset */
    0,                                         /* tp_init */
    0,                                         /* tp_alloc */
    sketch_MinHash_new,                        /* tp_new */
};

std::string _revcomp(const std::string& kmer)
{
    std::string out = kmer;
    size_t ksize = out.size();

    for (size_t i=0; i < ksize; ++i) {
        char complement;

        switch(kmer[i]) {
        case 'A':
            complement = 'T';
            break;
        case 'C':
            complement = 'G';
            break;
        case 'G':
            complement = 'C';
            break;
        case 'T':
            complement = 'A';
            break;
        default:
            throw std::exception();
            break;
        }
        out[ksize - i - 1] = complement;
    }
    return out;
}

int _hash_murmur(const std::string& kmer)
{
  int out[2];
  HashIntoType h, r;
  uint32_t seed = 0;
  MurmurHash3_x86_32((void *)kmer.c_str(), kmer.size(), seed, &out);
  return out[0];
}

static PyMethodDef SketchMethods[] = {
    { NULL, NULL, 0, NULL } // sentinel
};

MOD_INIT(_sketch)
{
    if (PyType_Ready( &sketch_MinHash_Type ) < 0) {
        return MOD_ERROR_VAL;
    }

    PyObject * m;

    MOD_DEF(m, "_sketch",
            "interface for the sourmash module low-level extensions",
            SketchMethods);

    if (m == NULL) {
        return MOD_ERROR_VAL;
    }

    Py_INCREF(&sketch_MinHash_Type);
    if (PyModule_AddObject( m, "MinHash",
                            (PyObject *)&sketch_MinHash_Type ) < 0) {
        return MOD_ERROR_VAL;
    }
    return MOD_SUCCESS_VAL(m);
}
