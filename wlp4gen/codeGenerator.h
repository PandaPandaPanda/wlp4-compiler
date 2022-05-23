#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>

#include "typeChecker.h"

using namespace std;

class CodeGenerator {
 public:
  CodeGenerator(TreeNode *root, TypeChecker *TC);
  virtual ~CodeGenerator();

 private:
  TypeChecker *typeChecker;
  int offset;  // reset to 0 at every procedure or wain, decrement for every dcl
               // by 4
  int labelCounter;
  void genPrologue();
  void genCode(TreeNode *root, string procedure);
  void push(int reg);
  void pop(int reg);
};

#endif