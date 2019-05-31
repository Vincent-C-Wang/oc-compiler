#include <bitset>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "astree.h"
#include "lyutils.h"
#include "auxlib.h"
#include "symtable.h"

vector<symbol_table*> symbol_stack;
vector<int> block_stack;
size_t next_block = 1;
int current_indent = 0;
int returned = 0;
int typecheck_helper(astree *node);
int return_attr = ATTR_void;
const string *return_struct;
extern FILE* sym_file;
symbol_table *struct_table;

// could have also been done with a switch statement i realize
const string symbol::get_attributes(const string *struct_name = nullptr) {
  string attrs = "";
  if (attributes.test (ATTR_field)) {
    attrs += "field ";
    if (struct_name != nullptr) {
      attrs += "{";
      attrs += *struct_name;
      attrs += "} ";
    }
  }
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


  return attrs;

}


//stack management
void enter_block() {
  block_stack.push_back(next_block);
  symbol_stack.push_back(nullptr);
  next_block++;
}

//popstack and leave block
void leave_block() {
  block_stack.pop_back();
  symbol_stack.pop_back();
}

void insert_ident(const string *lexinfo, symbol *symbol) {
  if (symbol_stack.back() == nullptr) {
    symbol_stack.back() = new symbol_table;
  }

  symbol_stack.back()->insert(symbol_entry(lexinfo, symbol));

}

// constructor for symbols
symbol *new_symbol(astree *node) {
  symbol *new_symbol = new symbol();

  new_symbol->attributes = node->attributes;
  new_symbol->fields = nullptr;
  new_symbol->lloc = node->lloc;
  new_symbol->block_nr = node->block_nr;
  if (node->type_name != nullptr) {
    new_symbol->type_name = node->type_name;
  }
  return new_symbol;
}

//finds a specific symbol in the table
symbol *find_symbol_in_table(symbol_table *table, astree *node) {

  if (table == nullptr) {
    return nullptr;
  }
  auto find = table->find(node->lexinfo);

  if (find != table->end()) {
    return find->second;
  } else {
    return nullptr;
  }

}

symbol *find_ident(astree *node) {
  for (auto it = symbol_stack.rbegin();
       it != symbol_stack.rend(); ++it) {

    symbol *symbol = find_symbol_in_table(*it, node);

    if (symbol != nullptr) {
      return symbol;
    }
  }
  return nullptr;
}

int check_int(astree *node) {
  if (!node->attributes.test(ATTR_int)) return 0;
  if (node->attributes.test(ATTR_void)) return 0;
  if (node->attributes.test(ATTR_null)) return 0;
  if (node->attributes.test(ATTR_string)) return 0;
  if (node->attributes.test(ATTR_struct)) return 0;
  return 1;
}

int check_valid_type(astree *node) {
  int type_defined = 0;
  for (int i = 0; i < ATTR_array; i++) {
    if (node->attributes.test(i)) {
      if (type_defined == 1) {
        return 0;
      }
      type_defined = 1;
    }
  }
  return type_defined;
}

int check_same(attr_bitset first, attr_bitset second) {
  for (int i = 0; i < ATTR_function; i++) {
    if (first.test(i) !=
        second.test(i)) {
      return 0;
    }
  }
  if (first.test(ATTR_array) !=
      second.test(ATTR_array)) {
    return 0;
  }
  return 1;
}

int check_compatible(attr_bitset first, attr_bitset second) {

  if (!first.test(ATTR_int)) {
    if (second.test(ATTR_null)) {
      return 1;
    }
  }

  if (!second.test(ATTR_int)) {
    if (first.test(ATTR_null)) {
      return 1;
    }
  }

  for (int i = 0; i < ATTR_function; i++) {
    if (first.test(i) !=
        second.test(i)) {
      printf("%i\n", i);
      return 0;
    }
  }
  if (first.test(ATTR_array) !=
      second.test(ATTR_array)) {
    return 0;
  }

  return 1;
}

int check_compatible(astree *node, astree *other) {
  DEBUGF('s', "node: %s\nother: %s\n", node->lexinfo->c_str(),
                                   other->lexinfo->c_str());
  if (!check_valid_type(node) ||
      !check_valid_type(other)) {
    return 0;
  }
  if (!check_int(node)) {
    if (other->attributes.test(ATTR_null)) {
      return 1;
    }
  }

  if (!check_int(other)) {
    if (node->attributes.test(ATTR_null)) {
      return 1;
    }
  }
  for (int i = 0; i < ATTR_function; i++) {
    if (node->attributes.test(i) !=
        other->attributes.test(i)) {
      return 0;
    }
  }
  if (node->attributes.test(ATTR_array) !=
      other->attributes.test(ATTR_array)) {
    return 0;
  }



  return 1;
}


void inherit_type(symbol *symbol, astree *node) {

  for (int i = 0; i < ATTR_function; i++) {
    if (node->attributes.test(i)) {
      symbol->attributes.set(i);

      if (i == ATTR_struct) {
        symbol->type_name = node->type_name;
      }
    }
  }
}
//over loaded cause i can't be bothered to go back and check all the cases
void inherit_type(astree *node, symbol *symbol) {

  for (int i = 0; i < ATTR_function; i++) {
    if (symbol->attributes.test(i)) {
      node->attributes.set(i);

      if (i == ATTR_struct) {
        node->type_name = symbol->type_name;
      }
    }
  }
}

//inherits all attributes
void inherit_attributes(symbol *symbol, astree *node) {

  for (int i = 0; i < ATTR_bitset_size; i++) {
    if (node->attributes.test(i)) {
      symbol->attributes.set(i);

      if (i == ATTR_struct) {
        symbol->type_name = node->type_name;
      }
    }
  }
}

void inherit_attributes(astree *node, symbol *symbol) {

  for (int i = 0; i < ATTR_bitset_size; i++) {
    if (symbol->attributes.test(i)) {
      node->attributes.set(i);

      if (i == ATTR_struct) {
        node->type_name = symbol->type_name;
      }
    }
  }
}




void inherit_attributes(astree *parent, astree *child) {
  for (int i = 0; i < ATTR_bitset_size; i++) {
    if (child->attributes.test(i)) {
      parent->attributes.set(i);

      if (i == ATTR_struct) {
        parent->type_name = child->type_name;
      }
    }
  }
}


void inherit_type(astree *parent, astree *child) {
  for (int i = 0; i < ATTR_function; i++) {
    if (child->attributes.test(i)) {
      parent->attributes.set(i);

      if (i == ATTR_struct) {
        parent->type_name = child->type_name;
      }
    }
  }
}

void print_sym(const string *lexinfo, location lloc,
               const string attributes_str, int block_num = -1) {

  if (block_stack.back() == 0 && next_block > 1) {
    fprintf(sym_file, "\n");
  }
  for (int i = 0; i < current_indent; i++) {
    fprintf(sym_file, "   ");
  }
  fprintf(sym_file, "%s (%zd.%zd.%zd) ",
        lexinfo->c_str(),
        lloc.filenr,
        lloc.linenr,
        lloc.offset);

  if (block_num > -1) {
    fprintf(sym_file, "{%i} ", block_num);
  }

  fprintf(sym_file, "%s\n", attributes_str.c_str());
}



// function to handle structures
int handle_struct(astree *node) {
  int errors = 0;
  if (block_stack.back() != 0) {
    errllocprintf (node->lloc, "error: structs must be declared in "
                    "global scope\n" ,
                     node->lexinfo->c_str());
    return 1;
  }

  astree *type_id = node->children[0];
  type_id->attributes.set(ATTR_struct);
  type_id->type_name = type_id->lexinfo;

  auto find = struct_table->find(type_id->lexinfo);
  symbol *struct_symbol;

  if (find != struct_table->end()) {
    struct_symbol = find->second;
    if (struct_symbol != nullptr) {
      if (struct_symbol->fields != nullptr) {
        errllocprintf (node->lloc, "error: \'%s\' already declared"
                       "\n" ,
                       type_id->lexinfo->c_str());
        return errors + 1;
      }
    }
  }

  struct_symbol = new_symbol(node);
  struct_symbol->attributes = type_id->attributes;


  struct_table->insert(symbol_entry(type_id->lexinfo, struct_symbol));
  struct_symbol->fields = new symbol_table;
  struct_symbol->type_name = type_id->type_name;

  print_sym(type_id->lexinfo,
            type_id->lloc,
            type_id->get_attributes(),
            type_id->block_nr);

  current_indent++;
  astree *declarer;
  astree *basetype;
  astree *declid;
  symbol *symbol_to_insert;
  for (size_t i = 1; i < node->children.size(); i++) {
    declarer = node->children[i];
    if (declarer->symbol == TOK_ARRAY) {
      declarer->attributes.set(ATTR_array);
      basetype = declarer->children[0];
      symbol_to_insert = new_symbol(basetype);
      symbol_to_insert->attributes.set(ATTR_array);
      declid = declarer->children[1];
    } else {
      basetype = declarer;
      symbol_to_insert = new_symbol(basetype);
      declid = declarer->children[0];
    }

    declid->attributes.set(ATTR_field);

    DEBUGF('s', "declarer %s\n", declarer->lexinfo->c_str());
    DEBUGF('s', "basetype %s\n", basetype->lexinfo->c_str());
    DEBUGF('s', "declid %s\n", declid->lexinfo->c_str());

    if (errors > 0) {
      return errors;
    }

    switch (basetype->symbol) {
      case TOK_VOID: {
        errllocprintf (node->lloc, "error: \'%s\' cannot"
                       " have type void'\n",
                       declid->lexinfo->c_str());
        basetype->attributes.set(ATTR_void);
        return errors + 1;
        break;
      }
      case TOK_INT: {
        basetype->attributes.set(ATTR_int);
        inherit_type(symbol_to_insert, basetype);
        inherit_attributes(symbol_to_insert, declid);
        struct_symbol->fields->insert(symbol_entry(declid->lexinfo,
                                            symbol_to_insert));
        print_sym(declid->lexinfo,
                symbol_to_insert->lloc,
                symbol_to_insert->get_attributes(type_id->lexinfo));
        break;
      }
      case TOK_STRING: {
        basetype->attributes.set(ATTR_string);
        inherit_type(symbol_to_insert, basetype);
        inherit_attributes(symbol_to_insert, declid);
        struct_symbol->fields->insert(symbol_entry(declid->lexinfo,
                                            symbol_to_insert));
        print_sym(declid->lexinfo,
                symbol_to_insert->lloc,
                symbol_to_insert->get_attributes(type_id->lexinfo));
        break;
      }
      case TOK_TYPEID: {
        basetype->attributes.set(ATTR_struct);
        basetype->type_name = basetype->lexinfo;
        symbol *find_struct =
                find_symbol_in_table(struct_table, basetype);

        if (find_struct == nullptr) {
          struct_table->insert(symbol_entry(basetype->lexinfo,
                                            nullptr));
        }
        inherit_type(symbol_to_insert, basetype);
        inherit_attributes(symbol_to_insert, declid);
        struct_symbol->fields->insert(symbol_entry(declid->lexinfo,
                                            symbol_to_insert));
        print_sym(declid->lexinfo,
                symbol_to_insert->lloc,
                symbol_to_insert->get_attributes(type_id->lexinfo));

        break;
      }
    }

  }
  current_indent--;
  return errors;
}


int handle_vardecl(astree *node) {
  int errors = 0;
  astree *left_child = nullptr;
  astree *right_child = nullptr;
  if (node->children.size() == 2) {
    left_child = node->children[0];
    right_child = node->children[1];
  }

  astree *declarer = left_child;
  astree *basetype;
  astree *declid;


  if (declarer->symbol == TOK_ARRAY) {
    declarer->attributes.set(ATTR_array);
    basetype = left_child->children[0];
    declid = left_child->children[1];
  } else {
    basetype = left_child;
    declid = left_child->children[0];
  }

  node->block_nr = block_stack.back();
  declarer->block_nr = block_stack.back();
  basetype->block_nr = block_stack.back();
  declid->block_nr = block_stack.back();


  DEBUGF('s', "declarer %s\n", declarer->lexinfo->c_str());
  DEBUGF('s', "basetype %s\n", basetype->lexinfo->c_str());
  DEBUGF('s', "declid %s\n", declid->lexinfo->c_str());

  switch (basetype->symbol) {
    case TOK_VOID: {
      errllocprintf (node->lloc, "error: \'%s\' cannot"
                     " have type void'\n",
                     declid->lexinfo->c_str());
      errors++;
      break;
    }
    case TOK_INT: {
      basetype->attributes.set(ATTR_int);
      break;
    }
    case TOK_STRING: {
      basetype->attributes.set(ATTR_string);
      break;
    }
    case TOK_TYPEID: {
      auto find_struct = struct_table->find(basetype->lexinfo);
      if (find_struct == struct_table->end()) {
        errllocprintf (basetype->lloc, "error: struct \'%s\' was not "
                       "declared in this scope\n",
                       basetype->lexinfo->c_str());
        errors++;
      } else if (find_struct->second == nullptr) {
        errllocprintf (basetype->lloc, "error: incomplete "
                      "data type \'%s\'\n",
                     basetype->lexinfo->c_str());
        errors++;
      }
      basetype->attributes.set(ATTR_struct);
      basetype->type_name = basetype->lexinfo;
      declarer->attributes.set(ATTR_struct);
      declarer->type_name = left_child->lexinfo;
      break;
    }
  }
  errors += typecheck_helper(right_child);
  inherit_type(declarer, basetype);

  int compatible = check_compatible(declarer, right_child);

  if (!compatible) {
    errllocprintf2(node->lloc, "error: type %sis not "
                   "compatible with type %s\n",
                   declarer->type_string().c_str(),
                   right_child->type_string().c_str());
    errors++;
  }
  symbol *symbol_to_insert = find_ident(declid);
  if (symbol_to_insert != nullptr) {
    errllocprintf (declid->lloc, "error: variable \'%s\'"
                   " already defined \n",
                   declid->lexinfo->c_str());
    errors++;
  }
  if (errors == 0) {
    symbol_to_insert = new_symbol(declarer);
    symbol_to_insert->attributes.set(ATTR_variable);
    symbol_to_insert->attributes.set(ATTR_lval);
    insert_ident(declid->lexinfo, symbol_to_insert);
    print_sym(declid->lexinfo,
                    symbol_to_insert->lloc,
                    symbol_to_insert->get_attributes(),
                    symbol_to_insert->block_nr);
  }
  return errors;
}
// traverse the astree by checking all possible token types


int handle_function(astree *node) {
  int errors = 0;
  int needs_matching = 0;
  if (block_stack.back() != 0) {
    errllocprintf (node->lloc, "error: functions must be declared in "
                    "global scope\n" ,
                     node->lexinfo->c_str());
    return 1;
  }


  astree *return_type = node->children[0];
  astree *basetype;
  astree *declid;


  if (return_type->symbol == TOK_ARRAY) {
    return_type->attributes.set(ATTR_array);
    basetype = return_type->children[0];
    declid = return_type->children[1];
  } else {
    basetype = return_type;
    declid = return_type->children[0];
  }

  node->block_nr = block_stack.back();
  return_type->block_nr = block_stack.back();
  basetype->block_nr = block_stack.back();
  declid->block_nr = block_stack.back();

  astree *paramlist = node->children[1];
  astree *block = nullptr;

  node->block_nr = block_stack.back();

  if (node->symbol == TOK_FUNCTION) {
    block = node->children[2];
  }

  symbol *function_symbol = find_ident(declid);

  if (function_symbol != nullptr) {
    if (node->symbol == TOK_FUNCTION) {
      if (function_symbol->attributes.test(ATTR_function)) {
        errllocprintf (node->lloc, "error: redeclaration of "
                       "\'%s\'\n",
                       declid->lexinfo->c_str());
          return errors + 1;
      } else {
        needs_matching = 1;
      }

    } else {

        errllocprintf (node->lloc, "error: redeclaration of "
                       "\'%s\'\n",
                       declid->lexinfo->c_str());
        return errors + 1;
    }
  }


  switch (basetype->symbol) {
    case TOK_VOID: {
      basetype->block_nr = block_stack.back();
      declid->block_nr = block_stack.back();
      basetype->attributes.set(ATTR_void);
      return_attr = ATTR_void;
      break;
    }
   case TOK_STRING: {
      basetype->block_nr = block_stack.back();
      declid->block_nr = block_stack.back();
      basetype->attributes.set(ATTR_string);
      return_attr = ATTR_string;
      break;
    }
    case TOK_INT: {
      basetype->block_nr = block_stack.back();
      declid->block_nr = block_stack.back();
      basetype->attributes.set(ATTR_int);
      return_attr = ATTR_int;
      break;
    }
    case TOK_TYPEID: {
      basetype->block_nr = block_stack.back();
      declid->block_nr = block_stack.back();
      basetype->attributes.set(ATTR_struct);
      basetype->type_name = basetype->lexinfo;
      return_attr = ATTR_struct;
      return_struct = declid->lexinfo;
      break;
    }
  }

  inherit_type(return_type, basetype);
  if (function_symbol == nullptr) {
    function_symbol = new_symbol(return_type);
    if (node->symbol == TOK_PROTOTYPE) {
      function_symbol->attributes.set(ATTR_prototype);
    } else {
      function_symbol->attributes.set(ATTR_function);
    }


    insert_ident(declid->lexinfo, function_symbol);
    print_sym(declid->lexinfo,
              function_symbol->lloc,
              function_symbol->get_attributes(),
              function_symbol->block_nr);

    enter_block();
    current_indent++;
    paramlist->block_nr = block_stack.back();

    astree *param_declarer;
    astree *param_basetype;
    astree *param_declid;
    symbol *param_symbol;

    for (size_t i = 0; i < paramlist->children.size(); i++) {
      param_declarer = paramlist->children[i];

      if (param_declarer->symbol == TOK_ARRAY) {
        param_declarer->attributes.set(ATTR_array);
        param_basetype = param_declarer->children[0];
        param_declid = param_declarer->children[1];
      } else {
        param_basetype = param_declarer;
        param_declid = param_declarer->children[0];
      }


      errors += typecheck_helper(param_declarer);
      errors += typecheck_helper(param_basetype);
      inherit_type(param_declarer, param_basetype);

      param_declarer ->block_nr = block_stack.back();
      param_declid ->block_nr = block_stack.back();
      param_basetype ->block_nr = block_stack.back();

      param_declarer->attributes.set(ATTR_param);
      param_declarer->attributes.set(ATTR_variable);
      param_declarer->attributes.set(ATTR_lval);
      param_symbol = new_symbol(param_declarer);
      param_symbol->param_name = param_declid->lexinfo;

      if (errors == 0) {
        insert_ident(param_declid->lexinfo, param_symbol);
        function_symbol->parameters.push_back(param_symbol);
        print_sym(param_declid->lexinfo,
                  param_symbol->lloc,
                  param_symbol->get_attributes(),
                  param_symbol->block_nr);
      }

    }
  }

  if (node->symbol == TOK_FUNCTION) {

    function_symbol->attributes.reset(ATTR_prototype);
    function_symbol->attributes.set(ATTR_function);

    if (needs_matching) {
      if (paramlist->children.size() !=
          function_symbol->parameters.size()) {

        errllocprintf (node->lloc, "error: wrong number of "
                       "arguments\n",
                       declid->lexinfo->c_str());
        return errors + 1;
      }
      print_sym(declid->lexinfo,
                declid->lloc,
                function_symbol->get_attributes(),
                declid->block_nr);

      enter_block();
      current_indent++;
      paramlist->block_nr = block_stack.back();

      astree *param_declarer;
      astree *param_basetype;
      astree *param_declid;
      symbol *param_symbol;

      symbol *param_check;

      for (size_t i = 0; i < paramlist->children.size(); i++) {
        param_declarer = paramlist->children[i];

        if (param_declarer->symbol == TOK_ARRAY) {
          param_declarer->attributes.set(ATTR_array);
          param_basetype = param_declarer->children[0];
          param_declid = param_declarer->children[1];
        } else {
          param_basetype = param_declarer;
          param_declid = param_declarer->children[0];
        }


        errors += typecheck_helper(param_declarer);
        errors += typecheck_helper(param_basetype);
        inherit_type(param_declarer, param_basetype);

        param_declarer ->block_nr = block_stack.back();
        param_declid ->block_nr = block_stack.back();
        param_basetype ->block_nr = block_stack.back();

        param_declarer->attributes.set(ATTR_param);
        param_declarer->attributes.set(ATTR_variable);
        param_declarer->attributes.set(ATTR_lval);
        param_symbol = new_symbol(param_declarer);

        const string *param_name = param_declid->lexinfo;
        param_check = function_symbol->parameters[i];


        if (param_name != param_check->param_name) {
          errllocprintf (basetype->lloc,
                         "error: parameter \'%s\' doesn't match \n",
                         param_name->c_str());
          return errors + 1;
        }

        if (!check_same(param_declarer->attributes,
                        param_check->attributes)) {
          errllocprintf (param_declarer->children[0]->lloc,
                         "error: parameter \'%s\' doesn't match \n",
                         param_name->c_str());
          return errors + 1;
        }

        if (errors == 0) {
          insert_ident(param_declid->lexinfo, param_symbol);
          function_symbol->parameters.push_back(param_symbol);
          print_sym(param_declid->lexinfo,
                    param_symbol->lloc,
                    param_symbol->get_attributes(),
                    param_symbol->block_nr);
        }

      }

    }
    errors += typecheck_helper(block);
  }


  return_attr = ATTR_void;
  leave_block();
  current_indent--;
  return errors;
}
//
int typecheck_helper(astree *node) {
  int errors = 0;
  astree *left_child = nullptr;
  astree *right_child = nullptr;
  if (node->children.size() == 2) {
    left_child = node->children[0];
    right_child = node->children[1];
  } else if (node->children.size() == 1) {
    left_child = node->children[0];
  }


  switch (node->symbol) {

    case TOK_STRUCT: {
      errors += handle_struct(node);
      break;
    }
    case TOK_CALL: {

      node->block_nr = block_stack.back();
      symbol *function_symbol = find_ident(node->children[0]);

      errors += typecheck_helper(node->children[0]);

      if (function_symbol == nullptr) {
        errllocprintf (node->lloc, "error: \'%s\' was not "
                       "declared in this scope\n",
                       node->children[0]->lexinfo->c_str());
        return errors + 1;
      }

      if (!function_symbol->attributes.test(ATTR_function) &&
          !function_symbol->attributes.test(ATTR_prototype)) {
        errllocprintf (node->lloc, "error: \'%s\' is not "
                       "a function\n",
                       node->children[0]->lexinfo->c_str());
        return errors + 1;
      }

      astree *paramlist = node;

      if (paramlist->children.size() - 1 !=
          function_symbol->parameters.size()) {
        errllocprintf (node->lloc, "error: wrong number of "
                       "arguments\n", "");
        return errors + 1;
      }

      astree *param;
      for (size_t i = 1; i < paramlist->children.size(); i++) {
        param = paramlist->children[i];
        errors += typecheck_helper(param);
        if (!check_compatible(param->attributes,
            function_symbol->parameters[i - 1]->attributes)) {

          errllocprintf (node->lloc, "error: arguments dont "
                        "match\n", "");
          return errors + 1;
        }
      }


      inherit_type(node, function_symbol);
      node->attributes.set(ATTR_vreg);
      break;
    }
    case TOK_PROTOTYPE:
    case TOK_FUNCTION: {
      errors += handle_function(node);
      break;
    }
    case TOK_RETURNVOID: {
      node->block_nr = block_stack.back();
      if (return_attr != ATTR_void) {
        errllocprintf (node->lloc, "error: invalid return type"
                       " \'%s\'\n", node->lexinfo->c_str());
        errors++;
      }
      break;
    }
    case TOK_RETURN: {
      node->block_nr = block_stack.back();
      errors += typecheck_helper(left_child);
      if (!left_child->attributes.test(return_attr)) {
        errllocprintf (node->lloc, "error: invalid return type"
                       " \'%s\'\n", node->lexinfo->c_str());
        errors++;
      } else if (return_attr == ATTR_typeid &&
                 left_child->type_name != return_struct) {
        errllocprintf (node->lloc, "error: invalid return type"
                       " \'%s\'\n", node->lexinfo->c_str());
      } else {
        returned = 1;
      }
      break;
    }
    case TOK_WHILE:
    case TOK_IF:
    case TOK_IFELSE: {
      node->block_nr = block_stack.back();
      for (size_t i = 0; i < node->children.size(); i++) {
        errors += typecheck_helper(node->children[i]);
      }
      break;
    }
    case TOK_BLOCK: {
      enter_block();
      current_indent++;
      node->block_nr = block_stack.back();
      for (size_t i = 0; i < node->children.size(); i++) {
        errors += typecheck_helper(node->children[i]);
      }
      leave_block();
      current_indent--;
      break;
    }
    case TOK_VARDECL: {
      errors += handle_vardecl(node);
      break;
    }
    case TOK_IDENT: {
      node->block_nr = block_stack.back();
      symbol *ident_symbol = find_ident(node);

      if (ident_symbol == nullptr) {
        errllocprintf (node->lloc, "error: \'%s\' was not "
                       "declared in this scope\n",
                       node->lexinfo->c_str());
        return errors + 1;
      }
      inherit_attributes(node,ident_symbol);
      node->declared_loc = ident_symbol->lloc;
      break;
    }
    case TOK_NEWARRAY: {
      node->block_nr = block_stack.back();
      errors += typecheck_helper(left_child);
      errors += typecheck_helper(right_child);

      if (check_int(right_child)) {
        inherit_type(node, left_child);
        node->attributes.set(ATTR_vreg);
        node->attributes.set(ATTR_array);
      } else {
        errllocprintf (left_child->lloc, "error: size "
                       "is not type int\n",
                       left_child->lexinfo->c_str());
        return errors + 1;
      }
      break;
    }
    case TOK_NEWSTRING: {
      node->block_nr = block_stack.back();
      errors += typecheck_helper(left_child);
      if (check_int(left_child)) {
        node->attributes.set(ATTR_string);
        node->attributes.set(ATTR_vreg);
      } else {
        errllocprintf (left_child->lloc, "error: size "
                       "is not type int\n",
                       left_child->lexinfo->c_str());
        return errors + 1;
      }
      break;
    }
    case TOK_NEW: {
      node->block_nr = block_stack.back();
      errors += typecheck_helper(left_child);
      inherit_type(node, left_child);
      node->attributes.set(ATTR_vreg);
      break;
    }

    case TOK_INDEX: {
      node->block_nr = block_stack.back();
      errors += typecheck_helper(left_child);
      errors += typecheck_helper(right_child);

      if (!right_child->attributes.test(ATTR_int)) {
        errllocprintf (right_child->lloc, "error: invalid type for "
                       "array subscript: %s\n",
                       right_child->get_attributes().c_str());
        errors++;
      }

      if (left_child->attributes.test(ATTR_array)) {
          inherit_type(node, left_child);
          node->attributes.reset(ATTR_array);
          node->attributes.set(ATTR_vaddr);
          node->attributes.set(ATTR_lval);
      } else if (left_child->attributes.test(ATTR_string)){
        node->attributes.set(ATTR_int);
        node->attributes.set(ATTR_vaddr);
        node->attributes.set(ATTR_lval);
      } else {
        errllocprintf (right_child->lloc, "error: type %sis not"
                       " subscriptable\n",
                       left_child->type_string().c_str());
        errors++;
      }
      break;

    }
    case '=': {
      node->block_nr = block_stack.back();
      errors += typecheck_helper(left_child);
      errors += typecheck_helper(right_child);

      if (!left_child->attributes.test(ATTR_lval)){
        errllocprintf (left_child->lloc, "error: type %sis not"
                       " assignable\n",
                       left_child->type_string().c_str());
      }

      if (check_compatible(left_child, right_child)) {
        inherit_type(node, left_child);
        node->attributes.set(ATTR_vreg);
      } else {
        errllocprintf2(node->lloc, "error: type %scannot be "
                   "assigned to type %s\n",
                   right_child->type_string().c_str(),
                   left_child->type_string().c_str());
        errors += 1;
      }
      break;

    }
    case TOK_EQ:
    case TOK_NE:
    case TOK_LT:
    case TOK_LE:
    case TOK_GT:
    case TOK_GE: {
      DEBUGF('s', "case tok_ge\n");
      node->block_nr = block_stack.back();
      errors += typecheck_helper(left_child);
      errors += typecheck_helper(right_child);
      if (check_compatible(left_child, right_child)) {
        node->attributes.set(ATTR_int);
        node->attributes.set(ATTR_vreg);
      } else {
        errllocprintf (node->lloc, "error: \'%s\' has invalid "
                       "type comparison\n" ,
                       node->lexinfo->c_str());
        errors++;
      }
      break;
    }

    case TOK_POS:
    case TOK_NEG:
    case '!': {
      DEBUGF('s', "case %s\n", node->lexinfo->c_str());
      node->block_nr = block_stack.back();
      errors += typecheck_helper(left_child);
      if (check_int(left_child)) {
        node->attributes.set(ATTR_int);
        node->attributes.set(ATTR_vreg);
      } else {
        errllocprintf (node->lloc, "error: \'%s\' has invalid "
                       "type comparison\n" ,
                       node->lexinfo->c_str());
        errors++;
      }
      break;
    }

    case '+':
    case '-':
    case '*':
    case '/':
    case '%': {
      DEBUGF('s', "case %s\n", node->lexinfo);
      node->block_nr = block_stack.back();
      errors += typecheck_helper(left_child);
      errors += typecheck_helper(right_child);
      if (check_int(left_child) && check_int(right_child)) {
        node->attributes.set(ATTR_int);
        node->attributes.set(ATTR_vreg);
      } else {
        errllocprintf (node->lloc, "error: \'%s\' has invalid "
                       "type comparison\n" ,
                       node->lexinfo->c_str());
        errors++;
      }
      break;
    }
    case TOK_VOID: {
      node->block_nr = block_stack.back();
      errllocprintf (node->lloc, "error: type void"
                     " not allowed\n",
                     node->lexinfo->c_str());
      errors++;
      node->attributes.set(ATTR_void);
      break;
    }
    case TOK_INT: {
      node->block_nr = block_stack.back();
      node->attributes.set(ATTR_int);
      break;
    }
    case TOK_STRING: {
      node->block_nr = block_stack.back();
      node->attributes.set(ATTR_string);
      break;
    }
    case TOK_TYPEID: {
      node->block_nr = block_stack.back();
      auto find_struct = struct_table->find(node->lexinfo);
      if (find_struct == struct_table->end()) {
        errllocprintf (node->lloc, "error: struct \'%s\' was not "
                       "declared in this scope\n",
                       node->lexinfo->c_str());
        errors++;
      } else if (find_struct->second == nullptr) {
        errllocprintf (node->lloc, "error: incomplete "
                      "data type \'%s\'\n",
                     node->lexinfo->c_str());
        errors++;
      }
      node->attributes.set(ATTR_struct);
      node->type_name = node->lexinfo;
      break;
    }

    case TOK_FIELD: {
      DEBUGF('s', "case field\n");
      node->block_nr = block_stack.back();
      node->attributes.set(ATTR_field);
      break;
    }

    case TOK_NULL: {
      DEBUGF('s', "case null\n");
      node->block_nr = block_stack.back();
      node->attributes.set(ATTR_null);
      node->attributes.set(ATTR_const);
      break;
   }

    case TOK_STRINGCON: {
      DEBUGF('s', "case stringcon\n");
      node->block_nr = block_stack.back();
      node->attributes.set(ATTR_string);
      node->attributes.set(ATTR_const);
      break;
    }

    case TOK_CHARCON: {
      DEBUGF('s', "case charcon\n");
      node->block_nr = block_stack.back();
      node->attributes.set(ATTR_int);
      node->attributes.set(ATTR_const);
      break;
    }

    case TOK_INTCON: {
      DEBUGF('s', "case intcon\n");
      node->block_nr = block_stack.back();
      node->attributes.set(ATTR_int);
      node->attributes.set(ATTR_const);
      break;
    }

    case TOK_ROOT: {
      DEBUGF('s', "case root\n");
      for (astree* child: node->children) {
          typecheck_helper(child);
      }
      node->block_nr = 0;
      break;
    }

  }
  return errors;

}


int typecheck(astree *tree) {

  symbol_stack.push_back(new symbol_table);
  block_stack.push_back(0);
  struct_table = new symbol_table;

  return typecheck_helper(tree);

}
