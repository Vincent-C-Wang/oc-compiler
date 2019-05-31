#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "astree.h"
#include "lyutils.h"
#include "auxlib.h"
#include "emit.h"

int counter = 0;
string indent = "        ";


string get_base_type(astree *type_node) {
  string type_string = "";
  if (type_node->attributes[ATTR_int]) {
    type_string = "int";
  } else if (type_node->attributes[ATTR_string]) {
    type_string = "char*";
  } else if (type_node->attributes[ATTR_struct]) {
    type_string = "struct s_" + *(type_node->type_name) + "*";
  } else if (type_node->attributes[ATTR_void]) {
    type_string = "void";
  }
  return type_string;
}


string type(astree *node) {
  int is_array = 0;
  string type_string = "";
  astree *type_node = node;

  if (node->attributes[ATTR_array]) {
    is_array = 1;
    type_node = node->children[0];
  }

  type_string = get_base_type(type_node);

  if (is_array) {
    type_string += "*";
  }

  return type_string;
}

string ident_type(astree *node) {
  int is_array = 0;
  string type_string = "";
  astree *type_node = node;

  if (node->attributes[ATTR_array]) {
    is_array = 1;
  }

  type_string = get_base_type(type_node);

  if (is_array) {
    type_string += "*";
  }

  return type_string;

}

astree *declid_node(astree *node) {
  if (node->symbol== TOK_ARRAY) {
    return node->children[1];
  } else {
    return node->children[0];
  }

}

string declid_name(astree *node) {

  if (node->symbol== TOK_ARRAY) {
    return *(node->children[1]->lexinfo);
  } else {
    return *(node->children[0]->lexinfo);
  }

}

string handle_name(astree *node, size_t block_num) {

  string name = "";

  switch (node->symbol) {

    case TOK_IDENT:
    case TOK_DECLID: {
      string block_string = "";
      if (block_num != 0) {
        block_string = to_string(block_num);
      }
      name = "_" + block_string + "_" + *(node->lexinfo);
      break;
    }

  }


  return name;
}

string loc_string(astree *node) {
  string loc_string = "_"
                      + to_string(node->lloc.filenr)
                      + "_"
                      + to_string(node->lloc.linenr)
                      + "_"
                      + to_string(node->lloc.offset);
  return loc_string;

}

string handle_name(astree *node) {

  string name = "";

  switch (node->symbol) {

    case TOK_DECLID: {
      string block_string = "";

      if (node->block_nr) {
        block_string = to_string(node->block_nr);
      }
      name = "_" + block_string + "_" + *(node->lexinfo);
      break;
    }
    case TOK_IF:
    case TOK_IFELSE:
    case TOK_WHILE: {
      name = *(node->lexinfo) + loc_string(node);
      break;
    }

  }
  return name;
}

string vreg(astree* node) {
  string typechar;
  if (node->attributes[ATTR_int]) {
    typechar = "i";
  } else if (node->attributes[ATTR_string]) {
    typechar = "s";
  } else if (node->attributes[ATTR_struct]) {
    typechar = "p";
  }
  return typechar + to_string(++counter);
}


int emit_helper(FILE *outfile, astree *node) {

  switch (node->symbol) {
    case TOK_BLOCK: {
      for (size_t i = 0; i < node->children.size(); i++) {
        emit_helper(outfile, node->children[i]);
      }
      break;

    }
    case TOK_VARDECL: {
      emit_helper(outfile, node->children[1]);
      string type_name = type(node->children[0]);
      string var_name = handle_name(declid_node(node->children[0]));
      node->oilname = var_name;

      if (node->global) {
        fprintf(outfile, "%s%s = %s;\n",
                       indent.c_str(),
                       var_name.c_str(),
                       node->children[1]->oilname.c_str());

      } else {
        fprintf(outfile, "%s%s %s = %s;\n",
                       indent.c_str(),
                       type_name.c_str(), var_name.c_str(),
                       node->children[1]->oilname.c_str());

      }
      break;
    }
    case TOK_PARAMLIST:{
      astree *param;

      for(size_t i = 0; i < node->children.size(); i++) {
        param = node->children[i];
        string type_string = type(param);
        string declid_name = handle_name(declid_node(param));
        fprintf(outfile, "%s%s %s", indent.c_str(),
                type_string.c_str(), declid_name.c_str());
        if (i != (node->children.size() - 1)) {
          fprintf(outfile, ",\n");
        }
      }
      break;
    }
    case TOK_CALL: {

      for (size_t i = 0; i < node->children.size(); i++) {
        emit_helper(outfile, node->children[i]);
      }

      string type_string = ident_type(node);
      string target = vreg(node);
      node->oilname = target;
      if (node->attributes[ATTR_void]) {
        fprintf(outfile, "%s%s(", indent.c_str(),
                node->children[0]->oilname.c_str());
      } else {
        fprintf(outfile, "%s%s %s = %s(", indent.c_str(),
                         type_string.c_str(),
                         target.c_str(),
                         node->children[0]->oilname.c_str());
      }

      for (size_t j = 1; j < node->children.size(); j++) {
        fprintf(outfile, "%s", node->children[j]->oilname.c_str());
        if (j < node->children.size() - 1) {
          fprintf(outfile, ", ");
        }
      }
      fprintf(outfile, ");\n");

      break;
    }
    case TOK_IFELSE: {
      emit_helper(outfile, node->children[0]);
      string lloc_string = loc_string(node);
      fprintf(outfile, "%sif (!%s) goto else%s;\n", indent.c_str(),
                       node->children[0]->oilname.c_str(),
                       lloc_string.c_str());
      emit_helper(outfile, node->children[1]);
      fprintf(outfile, "%sgoto fi%s;\n", indent.c_str(),
                       lloc_string.c_str());
      fprintf(outfile, "else%s:;\n", lloc_string.c_str());
      emit_helper(outfile, node->children[2]);
      fprintf(outfile, "fi%s:;\n", lloc_string.c_str());
      break;
    }
    case TOK_IF: {
      emit_helper(outfile, node->children[0]);
      string lloc_string = loc_string(node);
      fprintf(outfile, "%sif (!%s) goto fi%s;\n", indent.c_str(),
                       node->children[0]->oilname.c_str(),
                       lloc_string.c_str());
      emit_helper(outfile, node->children[1]);
      fprintf(outfile, "fi%s:;\n", lloc_string.c_str());
      break;
    }
    case TOK_WHILE: {
      string stmnt_string = handle_name(node);
      string break_string = "break" + loc_string(node);

      fprintf(outfile, "%s:;\n",
                       stmnt_string.c_str());
      emit_helper(outfile, node->children[0]);
      fprintf(outfile, "%sif (!%s) goto %s;\n", indent.c_str(),
                       node->children[0]->oilname.c_str(),
                       break_string.c_str());
      emit_helper(outfile, node->children[1]);
      fprintf(outfile, "%sgoto %s;\n", indent.c_str(),
                       stmnt_string.c_str());
      fprintf(outfile, "%s:;\n", break_string.c_str());
      break;
    }
    case TOK_RETURN: {
      emit_helper(outfile, node->children[0]);
      fprintf(outfile, "%sreturn %s;\n", indent.c_str(),
                        node->children[0]->oilname.c_str());
      break;
    }
    case TOK_RETURNVOID: {
      fprintf(outfile, "%sreturn;\n", indent.c_str());
      break;
    }
    case TOK_IDENT: {
      node->oilname = handle_name(node, node->declared_block);
      break;
    }
    case TOK_NEWARRAY: {
      emit_helper(outfile, node->children[0]);
      emit_helper(outfile, node->children[1]);
      string type_string = type(node);
      string target_string = "p" + to_string(++counter);
      node->oilname = target_string;
      fprintf(outfile, "%s%s %s = xcalloc(%s, sizeof(%s));\n",
                        indent.c_str(), type_string.c_str(),
                        target_string.c_str(),
                        node->children[1]->oilname.c_str(),
                        get_base_type(node).c_str());
      break;
    }
    case TOK_NEWSTRING: {
      emit_helper(outfile, node->children[0]);
      string type_string = type(node);
      string target_string = "p" + to_string(++counter);
      node->oilname = target_string;
      fprintf(outfile, "%s%s %s = xcalloc(%s, sizeof(char));\n",
                        indent.c_str(), type_string.c_str(),
                        target_string.c_str(),
                        node->children[0]->oilname.c_str());
      break;
    }
    case TOK_NEW: {
      string type_string = type(node);
      string target_string = "p" + to_string(++counter);
      node->oilname = target_string;
      fprintf(outfile, "%s%s %s = xcalloc(1, sizeof (struct s_%s));\n",
                                    indent.c_str(),
                                    type_string.c_str(),
                                    target_string.c_str(),
                                    node->type_name->c_str());
      break;
    }
    case '.': {
      string field_string = "f_" + *(node->children[0]->type_name)
                            + "_" + *(node->children[1]->lexinfo);
      emit_helper(outfile, node->children[0]);
      string type_string = ident_type(node) + "*";
      string target = "a" + to_string(++counter);
      node->oilname = "*" + target;
      if (node->children[0]->symbol == '.') {
        fprintf(outfile, "%s%s %s = &(%s)->%s;\n", indent.c_str(),
                       type_string.c_str(), target.c_str(),
                       node->children[0]->oilname.c_str(),
                       field_string.c_str());
      } else {
        fprintf(outfile, "%s%s %s = &%s->%s;\n", indent.c_str(),
                       type_string.c_str(), target.c_str(),
                       node->children[0]->oilname.c_str(),
                       field_string.c_str());
      }
      break;
    }
    case TOK_INDEX: {
      emit_helper(outfile, node->children[0]);
      emit_helper(outfile, node->children[1]);
      string type_string = ident_type(node->children[0]);
      string target = "a" + to_string(++counter);
      node->oilname = "*" + target;
      fprintf(outfile, "%s%s %s = &%s[%s];\n", indent.c_str(),
                       type_string.c_str(),
                       target.c_str(),
                       node->children[0]->oilname.c_str(),
                       node->children[1]->oilname.c_str());

      break;
    }
    case '=': {
      emit_helper(outfile, node->children[0]);
      emit_helper(outfile, node->children[1]);
      string type_string = type(node);
      node->oilname = node->children[0]->oilname;
      fprintf(outfile, "%s%s = %s;\n", indent.c_str(),
                       node->children[0]->oilname.c_str(),
                       node->children[1]->oilname.c_str());

      break;
    }
    case '!':
    case TOK_POS:
    case TOK_NEG: {
      emit_helper(outfile, node->children[0]);

      string target = vreg(node);
      string type_string = type(node);
      node->oilname = target;

      fprintf(outfile, "%s%s %s = %s %s;\n",
                       indent.c_str(),
                       type_string.c_str(), target.c_str(),
                       node->lexinfo->c_str(),
                       node->children[0]->oilname.c_str());
      break;

    }
    case TOK_EQ:
    case TOK_NE:
    case TOK_LT:
    case TOK_LE:
    case TOK_GT:
    case TOK_GE:
    case '-':
    case '*':
    case '/':
    case '%':
    case '+': {
      emit_helper(outfile, node->children[0]);
      emit_helper(outfile, node->children[1]);

      string target = vreg(node);
      string type_string = type(node);
      node->oilname = target;

      fprintf(outfile, "%s%s %s = %s %s %s;\n",
                       indent.c_str(),
                       type_string.c_str(), target.c_str(),
                       node->children[0]->oilname.c_str(),
                       node->lexinfo->c_str(),
                       node->children[1]->oilname.c_str());
      break;

    }

    case TOK_INTCON: {
      string oil_string = *(node->lexinfo);
      oil_string.erase(0, min(oil_string.find_first_not_of('0'),
                      oil_string.size()-1));
      node->oilname = oil_string;
      break;
    }
    case TOK_CHARCON: {
      node->oilname = *(node->lexinfo);
      break;
    }
    case TOK_NULL: {
      node->oilname = "0";
      break;
    }


  }
  return 0;
}


int emit_structs(FILE *outfile, astree *root) {
  astree *node;
  for(size_t i = 0; i < root->children.size(); i++) {
    node = root->children[i];

    if (node->symbol == TOK_STRUCT) {
      const string *struct_name = node->children[0]->type_name;
      fprintf(outfile, "struct s_%s {\n", struct_name->c_str());

      for (size_t j = 1; j < node->children.size(); j++) {
        string type_name = type(node->children[j]);
        string field_name = declid_name(node->children[j]);
        fprintf(outfile, "%s%s f_%s_%s;\n", indent.c_str(),
                         type_name.c_str(),
                         struct_name->c_str(), field_name.c_str());
      }
      fprintf(outfile, "};\n");

    }

  }

  return 0;
}

int emit_stringcons(FILE *outfile) {
  astree *string_node;
  for (size_t i = 0; i < string_queue.size(); i++) {
    string_node = string_queue[i];
    string target = vreg(string_node);
    string_node->oilname = target;
    fprintf(outfile, "char* %s = %s;\n", target.c_str(),
            string_node->lexinfo->c_str());
  }

  return 0;
}

int emit_globals(FILE *outfile, astree *root) {
  astree *node;
  for(size_t i = 0; i < root->children.size(); i++) {
    node = root->children[i];

    if (node->symbol == TOK_VARDECL) {
      string type_name = type(node->children[0]);
      string field_name = declid_name(node->children[0]);

      fprintf(outfile, "%s __%s;\n", type_name.c_str(),
              field_name.c_str());
      node->global = 1;

    }

  }

  return 0;
}


int emit_functions(FILE *outfile, astree *root) {
  astree *node;
  for(size_t i = 0; i < root->children.size(); i++) {
    node = root->children[i];

    if (node->symbol == TOK_FUNCTION) {
      string func_return_type = type(node->children[0]);
      string func_name = declid_name(node->children[0]);

      fprintf(outfile, "%s __%s (\n", func_return_type.c_str(),
              func_name.c_str());
      emit_helper(outfile, node->children[1]);
      fprintf(outfile, ")\n{\n");

      emit_helper(outfile, node->children[2]);
      fprintf(outfile, "}\n");
    }

  }

  return 0;
}


int emit_ocmain(FILE *outfile, astree *root) {
  fprintf(outfile, "void __ocmain (void) {\n");
  astree *node;
  for (size_t i = 0; i < root->children.size(); i++) {
    node = root->children[i];

    switch (node->symbol) {
      case TOK_FUNCTION:
      case TOK_PROTOTYPE:
      case TOK_STRUCT:
        break;

      default:
        emit_helper(outfile, node);

    }
  }

  fprintf(outfile, "}\n");

  return 0;
}

int emit(FILE *outfile, astree* root) {
  fprintf(outfile, "#define __OCLIB_C__\n#include \"oclib.oh\"\n");
  emit_structs(outfile, root);
  emit_stringcons(outfile);
  emit_globals(outfile, root);
  emit_functions(outfile, root);
  emit_ocmain(outfile, root);
  return 0;
}
