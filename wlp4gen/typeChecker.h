#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>

using namespace std;

extern bool debug;

struct TreeNode {
  string val;
  string lexeme;  // TreeNode should have either lexeme or children
  vector<TreeNode *> children;
  TreeNode() : val(0), children({}) {}
  TreeNode(string value) : val(value), children({}) {}
  TreeNode(string value, int length) : val(value), children(length, nullptr) {}
  TreeNode(string value, string lexeme) : val(value), lexeme(lexeme) {}
};

class TypeError {};

// map<procedure_name, pair<vector<parameter_type>, map<variable_name,
// pair<type, offset>>>>
typedef unordered_map<string, pair<string, int>> InnerSymbolTable;
typedef pair<vector<string>, InnerSymbolTable> SignatureInnerSymbolTable;
typedef unordered_map<string, SignatureInnerSymbolTable> ProcedureTable;

class TypeChecker {
 public:
  ProcedureTable symbolTable;

  TypeChecker(TreeNode *root);
  virtual ~TypeChecker();

  string typeOf(TreeNode *root, string procedure);
  string getSymbolType(string name, string procedure);
  int getSymbolOffset(string name, string procedure);
  void setSymbolOffset(string name, string procedure, int offset);
  const vector<string> &getSignature(string procedure);
  void print();

 private:
  bool hasProcedure(string procedure);
  bool hasSymbol(string name, string procedure);

  void getArgTypeList(TreeNode *root, string procedure,
                      vector<string> &argTypeList);
  string getRule(TreeNode *root);

  void buildSymbolTable(TreeNode *root);
  void buildSigniture(TreeNode *root, SignatureInnerSymbolTable &SISTPair);
  void buildInnerTable(TreeNode *root, SignatureInnerSymbolTable &SISTPair);
  void validateWithType(TreeNode *root, string procedure);
  void validateWithSymbolTable(TreeNode *root, string procedure);

  /* Error */
  void typeCorrectnessError(string message);
  void typeDerivationError(string message);
  void redefinitionError(string message);
  void undeclaredError(string message);
  void unknownError(string message);

  void accessViolationError(string message);

  void variableOvershadowProcedureError(string message);
};

/* Global Functions */
bool checkRule(TreeNode *root, vector<string> rule, bool isStrict);

#endif