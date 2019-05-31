
#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "stringset.h"
#include "lyutils.h"

astree::astree (int symbol_, const location& lloc_, const char* info) {
  symbol = symbol_;
  lloc = lloc_;
  lexinfo = stringset::intern_stringset(info);
  block_nr = -1;

  // vector defaults to empty -- no children
}

astree::~astree() {
  while (not children.empty()) {
    astree* child = children.back();
    children.pop_back();
    delete child;
  }
  if (yydebug) {
    fprintf (stderr, "Deleting astree (");
    astree::dump (stderr, this);
    fprintf (stderr, ")\n");
  }
}



astree* astree::adopt_children (astree* child1) {

  if (child1 != nullptr) {

    for (astree* child: child1->children) {
      children.push_back(child);
    }
  }

  return this;

}

astree* astree::adopt (astree* child1, astree* child2) {

  if (child1 != nullptr) {
    DEBUGF('a', "tree with symbol %s\n "
            "adopting tree with symbol: %s\n\n",
            parser::get_tname (symbol),
            parser::get_tname (child1->symbol));
    children.push_back (child1);
    //astree::print(stdout, child1);
  }
  if (child2 != nullptr) {
    DEBUGF('a', "tree with symbol %s\n "
            "adopting tree with symbol: %s\n\n",
            parser::get_tname (symbol),
            parser::get_tname (child1->symbol));
    children.push_back (child2);
    //astree::print(stdout, child2);
  }

  return this;
}

astree* astree::adopt_front (astree* child) {

  if (child != nullptr) {
    DEBUGF('a', "tree with symbol %s\n "
            "adopting tree with symbol: %s\n\n",
            parser::get_tname (symbol),
            parser::get_tname (child->symbol));
    vector<astree*>::iterator it;
    it = children.begin();
    children.insert(it, child);

    //astree::print(stdout, child1);
  }

   return this;
}

astree* astree::swap_sym (astree* tree, int new_symbol) {

   tree->symbol = new_symbol;
   return tree;
}

astree* astree::adopt_sym (astree* child, int symbol_) {
  DEBUGF('a', "changing symbol to: %s\n ", parser::get_tname(symbol));

  symbol = symbol_;
  //astree::print(stdout, child);
  return adopt (child);
}


void astree::dump_node (FILE* outfile) {
  fprintf (outfile, "%p->{%s %zd.%zd.%zd \"%s\":",
           this, parser::get_tname (symbol),
           lloc.filenr, lloc.linenr, lloc.offset,
           lexinfo->c_str());
  for (size_t child = 0; child < children.size(); ++child) {
    fprintf (outfile, " %p", children.at(child));
  }
}

void astree::dump_tree (FILE* outfile, int depth) {
  fprintf (outfile, "%*s", depth * 3, "");
  dump_node (outfile);
  fprintf (outfile, "\n");
  for (astree* child: children) child->dump_tree (outfile, depth + 1);
  fflush (NULL);
}

void astree::dump (FILE* outfile, astree* tree) {
  if (tree == nullptr) fprintf (outfile, "nullptr");
                  else tree->dump_node (outfile);
}

void astree::print (FILE* outfile, astree* tree, int depth) {

  for(int i = 0; i < depth; i++) {
    fprintf(outfile, "|  ");
  }

  char* tname = strdup(parser::get_tname(tree->symbol));
  char* short_name = tname;

  if (strstr(tname, "TOK_") == tname) short_name = tname + 4;

  fprintf (outfile, "%s %s (%zd.%zd.%zd) {%lu} %s\n",
           short_name,
           tree->lexinfo->c_str(),
           tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset,
           tree->block_nr, tree->get_attributes().c_str());

  free((char*) tname);

  for (astree* child: tree->children) {
    astree::print (outfile, child, depth + 1);
  }


}


void astree::printtok (FILE* outfile, astree* tree) {

  fprintf (outfile, "%*zd %zd.%.3zd %*i  %-13s  (%s)\n",
           4, tree->lloc.filenr,
           tree->lloc.linenr, tree->lloc.offset,
           4, tree->symbol,
           parser::get_tname (tree->symbol), tree->lexinfo->c_str());
}

string astree::get_attributes() {
  string attrs = "";

  if (attributes.test (ATTR_field)) attrs += "field ";
  if (attributes.test (ATTR_void)) attrs += "void ";
  if (attributes.test (ATTR_int)) attrs += "int ";
  if (attributes.test (ATTR_null)) attrs += "null ";
  if (attributes.test (ATTR_string)) attrs += "string ";
  if (attributes.test (ATTR_struct)) {
    attrs += "struct ";
    attrs += "\"";
    attrs += *type_name;
    attrs += "\" ";
  }
  if (attributes.test (ATTR_array)) attrs += "array ";
  if (attributes.test (ATTR_function)) attrs += "function ";
  if (attributes.test (ATTR_prototype)) attrs += "prototype ";
  if (attributes.test (ATTR_variable)) attrs += "variable ";
  if (attributes.test (ATTR_typeid)) attrs += "typeid ";
  if (attributes.test (ATTR_param)) attrs += "param ";
  if (attributes.test (ATTR_lval)) attrs += "lval ";
  if (attributes.test (ATTR_const)) attrs += "const ";
  if (attributes.test (ATTR_vreg)) attrs += "vreg ";
  if (attributes.test (ATTR_vaddr)) attrs += "vaddr ";

  if (symbol == TOK_IDENT) {
    attrs += "(";
    attrs += to_string(declared_loc.filenr);
    attrs += ".";
    attrs += to_string(declared_loc.linenr);
    attrs += ".";
    attrs += to_string(declared_loc.offset);
    attrs += ")";
  }

  return attrs;

}

string astree::type_string() {
  string type_string = "";
  if (attributes.test (ATTR_void)) type_string += "void ";
  if (attributes.test (ATTR_int)) type_string += "int ";
  if (attributes.test (ATTR_null)) type_string += "null ";
  if (attributes.test (ATTR_string)) type_string += "string ";
  if (attributes.test (ATTR_struct)) {
    type_string += "struct ";
    type_string += "\"";
    type_string += *type_name;
    type_string += "\" ";
  }
  if (attributes.test (ATTR_array)) type_string += "array";
  return type_string;

}


void destroy (astree* tree1, astree* tree2) {
  if (tree1 != nullptr) delete tree1;
  if (tree2 != nullptr) delete tree2;
}


void errllocprintf (const location& lloc, const char* format,
                    const char* arg) {
  static char buffer[0x1000];
  assert (sizeof buffer > strlen (format) + strlen (arg));
  snprintf (buffer, sizeof buffer, format, arg);
  errprintf ("%s:%zd.%zd: %s",
             lexer::filename (lloc.filenr)->c_str(),
             lloc.linenr, lloc.offset,
             buffer);
}

void errllocprintf2 (const location& lloc, const char* format,
                    const char* arg, const char* arg2) {
  static char buffer[0x1000];
  assert (sizeof buffer > strlen (format) + strlen (arg));
  snprintf (buffer, sizeof buffer, format, arg, arg2);


  errprintf ("%s:%zd.%zd: %s",
             lexer::filename (lloc.filenr)->c_str(),
             lloc.linenr, lloc.offset,
             buffer);
}
