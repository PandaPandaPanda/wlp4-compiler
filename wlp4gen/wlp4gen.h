#ifndef WLP4GEN_H
#define WLP4GEN_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>

#include "typeChecker.h"

class WLP4gen {
 public:
  static unordered_map<string, bool> terminal;
  TreeNode *root;

  WLP4gen();
  virtual ~WLP4gen();

  void printToPreOrder();
  void deleteTree();

 private:
  void printToPreOrderHelper(TreeNode *root);
  void deleteTreeHelper(TreeNode *root);
  TreeNode *buildFromPreOrder();
};

#endif
