#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

#include <bitset>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

#include "astree.h"
#include "lyutils.h"
#include "auxlib.h"
// the symbol structure
struct symbol {
  attr_bitset attributes;
  symbol_table* fields;
  location lloc;
  size_t block_nr;
  vector<symbol*> parameters;
  const string* param_name;
  const string* type_name;
  const string get_attributes(const string *struct_name);
};


// the only function that needs t be called by main anyways
int typecheck(astree *tree);

#endif
