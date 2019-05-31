
#ifndef __STRINGSET__
#define __STRINGSET__

#include <string>
#include <unordered_set>
using namespace std;

#include <stdio.h>

struct stringset {
   stringset();
   static unordered_set<string> set;
   static const string* intern_stringset (const char*);
   static void dump_stringset (FILE*);
};

#endif
