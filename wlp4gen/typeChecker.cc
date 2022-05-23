#include "typeChecker.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>

using namespace std;

bool checkRule(TreeNode *root, vector<string> rule, bool isStrict = true) {
  if (isStrict && root->children.size() + 1 != rule.size()) {
    return false;
  }

  if (root->val != rule[0]) {
    return false;
  }

  for (int i = 0; i < rule.size() - 1; i++) {
    if (root->children[i]->val != rule[i + 1]) {
      return false;
    }
  }

  return true;
}

// unordered_map<procedure_name, pair<vector<parameter_type>,
// unordered_map<variable_name, type>>>
TypeChecker::TypeChecker(TreeNode *root) {
  buildSymbolTable(root);
  validateWithType(root, "");
};

TypeChecker::~TypeChecker() {}

void TypeChecker::buildSymbolTable(TreeNode *root) {
  if (!root) {
    return;
  }

  if (root->val == "procedure") {
    string procName =
        root->children[1]->lexeme;  // procedure name, could be wain

    SignatureInnerSymbolTable p;
    buildSigniture(root->children[3], p);
    buildInnerTable(root->children[6], p);
    if (symbolTable.find(procName) == symbolTable.end()) {
      symbolTable[procName] = p;
    } else {
      redefinitionError("redefinition of procedure" + procName);
    }

    validateWithSymbolTable(root->children[7],
                            procName);  // validate statements
    validateWithSymbolTable(root->children[9],
                            procName);  // validate return
  } else if (root->val == "main") {
    string procName =
        root->children[1]->lexeme;  // procedure name, could be wain

    SignatureInnerSymbolTable p;
    buildSigniture(root, p);
    buildInnerTable(root->children[8], p);
    if (symbolTable.find(procName) == symbolTable.end()) {
      symbolTable[procName] = p;
    } else {
      redefinitionError("redefinition of procedure " + procName);
    }

    validateWithSymbolTable(root->children[9],
                            procName);  // validate statements
    validateWithSymbolTable(root->children[11],
                            procName);  // validate return
  }

  for (int i = 0; i < root->children.size(); i++) {
    buildSymbolTable(root->children[i]);
  }
}

void TypeChecker::buildSigniture(TreeNode *root,
                                 SignatureInnerSymbolTable &SISTPair) {
  if (!root) {
    return;
  }

  if (root->val == "dcl") {
    string t = "";
    for (auto s : (root->children[0]->children)) {
      t += s->lexeme;
    }

    SISTPair.first.push_back(t);  // record signiture
    if (SISTPair.second.find(root->children[1]->lexeme) ==
        SISTPair.second.end()) {
      SISTPair.second[root->children[1]->lexeme].first = t;  // record variable
    } else {
      redefinitionError("redefinition of variable " +
                        root->children[1]->lexeme);
    }

    return;
  }

  for (int i = 0; i < root->children.size(); i++) {
    if (root->children[i]->val == "RPAREN") {
      // end of signiture
      return;
    }
    buildSigniture(root->children[i], SISTPair);
  }
}

void TypeChecker::buildInnerTable(TreeNode *root,
                                  SignatureInnerSymbolTable &SISTPair) {
  if (!root) {
    return;
  }

  if (root->val == "dcl") {
    string t = "";
    for (auto s : (root->children[0]->children)) {
      t += s->lexeme;
    }

    if (SISTPair.second.find(root->children[1]->lexeme) ==
        SISTPair.second.end()) {
      SISTPair.second[root->children[1]->lexeme].first = t;  // record variable
    } else {
      redefinitionError("redefinition of variable " +
                        root->children[1]->lexeme);
    }
    return;
  }

  for (int i = 0; i < root->children.size(); i++) {
    buildInnerTable(root->children[i], SISTPair);
  }
}

bool TypeChecker::hasProcedure(string procedure) {
  if (symbolTable.find(procedure) != symbolTable.end()) {
    return true;
  } else {
    return false;
  }
}

bool TypeChecker::hasSymbol(string name, string procedure) {
  if (symbolTable[procedure].second.find(name) !=
      symbolTable[procedure].second.end()) {
    return true;
  } else {
    return false;
  }
}

string TypeChecker::getSymbolType(string name, string procedure) {
  return symbolTable[procedure].second[name].first;
}

const vector<string> &TypeChecker::getSignature(string procedure) {
  return symbolTable[procedure].first;
}

string TypeChecker::getRule(TreeNode *root) {
  string rule = "";
  for (TreeNode *node : root->children) {
    rule += node->val + " ";
  }
  return rule;
}

int TypeChecker::getSymbolOffset(string name, string procedure) {
  if (symbolTable.find(procedure) == symbolTable.end() ||
      symbolTable[procedure].second.find(name) ==
          symbolTable[procedure].second.end()) {
    accessViolationError("procedure " + procedure + " or variable " + name +
                         " does not exist!");
  }
  return symbolTable[procedure].second[name].second;
}

void TypeChecker::setSymbolOffset(string name, string procedure, int offset) {
  if (symbolTable.find(procedure) == symbolTable.end() ||
      symbolTable[procedure].second.find(name) ==
          symbolTable[procedure].second.end()) {
    accessViolationError("procedure: " + procedure + ", or variable: " + name +
                         ", does not exist!");
  }
  symbolTable[procedure].second[name].second = offset;
}

void TypeChecker::validateWithSymbolTable(TreeNode *root, string procedure) {
  if (!root) {
    return;
  }

  if (checkRule(root, {"lvalue", "ID"}, true)) {
    string variableName = root->children[0]->lexeme;
    if (hasSymbol(variableName, procedure)) {
      return;
    } else {
      undeclaredError("variable " + variableName +
                      " is used without being declared");
    }
  } else if (checkRule(root, {"factor", "ID"}, false)) {
    if (root->children.size() == 1) {
      string variableName = root->children[0]->lexeme;
      if (hasSymbol(variableName, procedure)) {
        return;
      } else {
        undeclaredError("variable " + variableName +
                        " is used without being declared");
      }
    } else if (root->children.size() > 2 &&
               root->children[1]->val == "LPAREN") {
      string procName = root->children[0]->lexeme;
      if (hasProcedure(procName)) {
        return;
      } else {
        undeclaredError("procedure " + procName +
                        " is used without being declared");
      }
    } else {
      unknownError(
          "This error is not suppose to happpen when validating with symbol "
          "table!");
    }
  }

  for (int i = 0; i < root->children.size(); i++) {
    validateWithSymbolTable(root->children[i], procedure);
  }
}

void TypeChecker::validateWithType(TreeNode *root, string procedure) {
  if (debug) {
    if (root->children.size() != 0) {
      cout << "==============" << endl;
      cout << root->val << " ";
      for (int i = 0; i < root->children.size(); i++) {
        cout << root->children[i]->val << " ";
      }
      cout << endl;
    } else {
      // cout << root->val << " " << root->lexeme << endl;
    }
  }

  /* Comparisons */
  if (checkRule(root, {"test", "expr", "EQ", "expr"}, true) ||
      checkRule(root, {"test", "expr", "NE", "expr"}, true) ||
      checkRule(root, {"test", "expr", "LT", "expr"}, true) ||
      checkRule(root, {"test", "expr", "LE", "expr"}, true) ||
      checkRule(root, {"test", "expr", "GE", "expr"}, true) ||
      checkRule(root, {"test", "expr", "GT", "expr"}, true)) {
    string l = typeOf(root->children[0], procedure);
    string r = typeOf(root->children[2], procedure);
    if (l != r) {
      typeCorrectnessError("comparison between " + l + " and " + r);
    }
  }

  /* Control flow */
  // Composition of welltyped subelement

  /* Deallocation */
  else if (checkRule(
               root,
               {"statement", "DELETE", "LBRACK", "RBRACK", "expr", "SEMI"},
               true)) {
    string exprType = typeOf(root->children[3], procedure);  // expr
    if (exprType != "int*") {
      typeCorrectnessError("delete [] must use int* parameter, instead " +
                           exprType + " is given");
    }
  }

  /* Printing */
  else if (checkRule(
               root,
               {"statement", "PRINTLN", "LPAREN", "expr", "RPAREN", "SEMI"},
               true)) {
    string exprType = typeOf(root->children[2], procedure);  // expr
    if (exprType != "int") {
      typeCorrectnessError("println must use int parameter, instead " +
                           exprType + " is given");
    }
  }

  /* Assignment */
  else if (checkRule(root, {"statement", "lvalue", "BECOMES", "expr", "SEMI"},
                     true)) {
    string l = typeOf(root->children[0], procedure);
    string r = typeOf(root->children[2], procedure);
    if (l != r) {
      typeCorrectnessError("cannot assign " + r + " to " + l);
    }
  }

  /* Sequencing */
  // Composition of welltyped subelement

  /* Decl’ns */
  else if (checkRule(root, {"dcls", "dcls", "dcl", "BECOMES", "NUM", "SEMI"},
                     true)) {
    string dclType = typeOf(root->children[1], procedure);
    if (dclType != "int") {
      typeCorrectnessError("cannot assign NUM to " + dclType);
    }
  }

  else if (checkRule(root, {"dcls", "dcls", "dcl", "BECOMES", "NULL", "SEMI"},
                     true)) {
    string dclType = typeOf(root->children[1], procedure);
    if (dclType != "int*") {
      typeCorrectnessError("cannot assign NULL to " + dclType);
    }
  }

  /* Procedures */
  else if (checkRule(root,
                     {"main", "INT", "WAIN", "LPAREN", "dcl", "COMMA", "dcl",
                      "RPAREN", "LBRACE", "dcls", "statements", "RETURN",
                      "expr", "SEMI", "RBRACE"},
                     true)) {
    procedure = "wain";

    string second = typeOf(root->children[5],
                           procedure);  // second parameter in wain
    if (second != "int") {
      typeCorrectnessError("wain must have int as second argument");
    }

    validateWithType(root->children[9], procedure);  // statements

    string returnValue = typeOf(root->children[11], procedure);  // return
    if (returnValue != "int") {
      typeCorrectnessError("wain must return int");
    }
  } else if (checkRule(root,
                       {"procedure", "INT", "ID", "LPAREN", "params", "RPAREN",
                        "LBRACE", "dcls", "statements", "RETURN", "expr",
                        "SEMI", "RBRACE"},
                       true)) {
    procedure = root->children[1]->lexeme;

    validateWithType(root->children[7], procedure);  // statements

    string returnValue =
        typeOf(root->children[9], procedure);  // return value (expr)
    if (returnValue != "int") {
      string procedureName = root->children[1]->lexeme;
      typeCorrectnessError("procedure " + procedureName + " must return int");
    }
  }

  for (int i = 0; i < root->children.size(); i++) {
    validateWithType(root->children[i], procedure);
  }
}

void TypeChecker::getArgTypeList(TreeNode *root, string procedure,
                                 vector<string> &argTypeList) {
  if (checkRule(root, {"arglist", "expr"}, true)) {
    string exprType = typeOf(root->children[0], procedure);
    argTypeList.push_back(exprType);
  } else if (checkRule(root, {"arglist", "expr", "COMMA", "arglist"}, true)) {
    string exprType = typeOf(root->children[0], procedure);
    argTypeList.push_back(exprType);
    getArgTypeList(root->children[2], procedure, argTypeList);
  } else {
    typeCorrectnessError("cannot parse arguments: " + getRule(root));
  }
}

string TypeChecker::typeOf(TreeNode *root, string procedure) {
  if (debug) {
    cout << "Typeof at ";
    if (root->children.size() != 0) {
      cout << root->val << " ";
      for (int i = 0; i < root->children.size(); i++) {
        cout << root->children[i]->val << " ";
      }
      cout << endl;
    } else {
      cout << root->val << " " << root->lexeme << endl;
    }
  }
  string type = "";

  /* Literals and identifiers */
  if (checkRule(root, {"ID"}, false)) {
    type = getSymbolType(root->lexeme, procedure);
  }

  else if (checkRule(root, {"NUM"}, false)) {
    type = "int";
  }

  else if (checkRule(root, {"NULL"}, false)) {
    type = "int*";
  }

  // singleton
  else if (checkRule(root, {"expr", "term"}, true) ||
           checkRule(root, {"term", "factor"}, true) ||
           checkRule(root, {"lvalue", "ID"}, true) ||
           checkRule(root, {"factor", "ID"}, true) ||
           checkRule(root, {"factor", "NUM"}, true) ||
           checkRule(root, {"factor", "NULL"}, true)) {
    type = typeOf(root->children[0], procedure);
  }

  // declaration
  else if (checkRule(root, {"dcl", "type", "ID"}, true)) {
    type = typeOf(root->children[1], procedure);
  }

  /* Parenthesized expressions */
  else if (checkRule(root, {"factor", "LPAREN", "expr", "RPAREN"}, true) ||
           checkRule(root, {"lvalue", "LPAREN", "lvalue", "RPAREN"}, true)) {
    type = typeOf(root->children[1], procedure);
  }

  /* Pointers */
  else if (checkRule(root, {"factor", "AMP", "lvalue"}, true)) {
    string LvalueType = typeOf(root->children[1], procedure);
    if (LvalueType != "int")
      typeDerivationError("& must be used with int");
    else
      type = "int*";
  }

  else if (checkRule(root, {"lvalue", "STAR", "factor"}, true) ||
           checkRule(root, {"factor", "STAR", "factor"}, true)) {
    string factorType = typeOf(root->children[1], procedure);

    if (factorType == "int") {
      typeDerivationError("cannot deference an int");
    } else {
      type = "int";  // deferenced from int*
    }
  }

  else if (checkRule(root, {"factor", "NEW", "INT", "LBRACK", "expr", "RBRACK"},
                     true)) {
    string exprType = typeOf(root->children[3], procedure);

    if (exprType != "int") {
      typeDerivationError("new [] must use int parameter, instead " + exprType +
                          " is given");
    }

    type = "int*";
  }

  /* Addition */
  else if (checkRule(root, {"expr", "expr", "PLUS", "term"}, true)) {
    string l = typeOf(root->children[0], procedure);  // expr
    string r = typeOf(root->children[2], procedure);  // term
    if (l == "int" && r == "int") {
      type = "int";
    } else if (l == "int*" && r == "int") {
      type = "int*";
    } else if (l == "int" && r == "int*") {
      type = "int*";
    } else {  // (l == "int*" && r == "int*") {
      typeDerivationError("cannot add two int*");
    }
  }

  /* Subtraction */
  else if (checkRule(root, {"expr", "expr", "MINUS", "term"}, true)) {
    string l = typeOf(root->children[0], procedure);  // expr
    string r = typeOf(root->children[2], procedure);  // term
    if (l == "int" && r == "int") {
      type = "int";
    } else if (l == "int*" && r == "int") {
      type = "int*";
    } else if (l == "int" && r == "int*") {
      typeDerivationError("cannot subtract int* from int");
    } else {  // (l == "int*" && r == "int*") {
      type = "int";
    }
  }

  /* Multiplication and division and mod */
  else if (checkRule(root, {"term", "term", "STAR", "factor"}, true) ||
           checkRule(root, {"term", "term", "SLASH", "factor"}, true) ||
           checkRule(root, {"term", "term", "PCT", "factor"}, true)) {
    if (typeOf(root->children[0], procedure) != "int" ||
        typeOf(root->children[2], procedure) != "int") {
      string op = root->children[1]->lexeme;
      typeDerivationError("invalid operation: " + op +
                          " cannot be used with int* " + " in " + procedure);
    }

    type = "int";
  }

  /* Procedure call */
  else if (checkRule(root, {"factor", "ID", "LPAREN", "RPAREN"}, true)) {
    string procName = root->children[0]->lexeme;

    if (hasSymbol(procName, procedure)) {
      variableOvershadowProcedureError(
          "procedure " + procName +
          " is overshadowed by variable with the same name, therefore cannot "
          "be called.");
    }
    if (getSignature(procName).size() != 0) {
      typeDerivationError("procedure takes in 0 parameters");
    }

    type = "int";
  }

  else if (checkRule(root, {"factor", "ID", "LPAREN", "arglist", "RPAREN"},
                     true)) {
    string procName = root->children[0]->lexeme;

    if (hasSymbol(procName, procedure)) {
      variableOvershadowProcedureError(
          "procedure " + procName +
          " is overshadowed by variable with the same name, therefore cannot "
          "be called.");
    }

    vector<string> argTypeList;
    getArgTypeList(root->children[2], procedure, argTypeList);
    vector<string> functionSignature = getSignature(procName);

    if (argTypeList.size() != functionSignature.size()) {
      typeDerivationError("invalid number of arguments");
    }

    for (int i = 0; i < argTypeList.size(); i++) {
      if (argTypeList[i] != functionSignature[i]) {
        typeDerivationError("procedure expected " + functionSignature[i] +
                            ", got " + argTypeList[i]);
      }
    }

    type = "int";
  }

  if (type == "") {
    typeDerivationError("cannot be typed at " + getRule(root));
  }

  if (debug) {
    cout << "Found type of " << root->val << " to be " << type << endl;
  }
  return type;
}

void TypeChecker::typeCorrectnessError(string message) {
  cerr << "ERROR: TypeCorrectnessError, " << message << endl;
  throw TypeError();
}
void TypeChecker::typeDerivationError(string message) {
  cerr << "ERROR: TypeDerivationError, " << message << endl;
  throw TypeError();
}
void TypeChecker::redefinitionError(string message) {
  cerr << "ERROR: RedefinitionError, " << message << endl;
  throw TypeError();
}
void TypeChecker::undeclaredError(string message) {
  cerr << "ERROR: UndeclaredError, " << message << endl;
  throw TypeError();
}
void TypeChecker::unknownError(string message) {
  cerr << "ERROR: UnknownError, " << message << endl;
  throw TypeError();
}

void TypeChecker::accessViolationError(string message) {
  cerr << "ERROR: AccessViolationError, " << message << endl;
  throw TypeError();
}

void TypeChecker::variableOvershadowProcedureError(string message) {
  cerr << "ERROR: variableOvershadowProcedureError, " << message << endl;
  throw TypeError();
}

void TypeChecker::print() {
  for (ProcedureTable::iterator it1 = symbolTable.begin();
       it1 != symbolTable.end(); it1++) {
    // Print the procedure name
    cerr << it1->first << ": ";
    // Print the procedure's signature
    for (vector<string>::iterator vec = it1->second.first.begin();
         vec != it1->second.first.end(); vec++) {
      cerr << *vec << " ";
    }
    cerr << endl;
    // Print the procedure's symbol table
    for (InnerSymbolTable::iterator it2 = it1->second.second.begin();
         it2 != it1->second.second.end(); it2++) {
      cerr << it2->first << " " << it2->second.first << endl;
    }
  }
}