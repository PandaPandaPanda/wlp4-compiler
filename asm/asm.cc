#include <algorithm>
#include <cctype>  // toupper()
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "scanner.h"
using namespace std;

class Assembler {
 public:
  virtual ~Assembler();
  virtual void assemble();
  Assembler();

  // Useful constants
  static const int alignedAccessMultiplier;

 protected:
 private:
  /* Pass 1 */
  // scan and tokenize the program
  bool scanning();
  // parse label table and check for syntax and valid label usage
  bool parsing();

  /* Pass 2 */
  // replace label with address reference
  bool semanticAnalysis();
  bool synthesis();

  /* Pass 1 Helper */
  bool recordLabel(Token tokenLine, int lineNumber);

  bool checkRegCommaRegCommaReg(int line);  // add, sub, slt, sltu
  bool checkRegCommaReg(int line);          // mult, multu, div, divu
  bool checkReg(int line);                  // mfhi, mflo, lis   , jr, jalr
  bool checkRegCommaIntLparenRegRparen(int line);  // lw, sw
  bool checkRegCommaRegCommaTop(int line);         // beq, bne

  bool checkWord(int line);

  bool isValidReg(Token token);
  bool isValid16sSigned(Token token);

  /* Output */
  void outInstruction(
      int i);  // output one line of instruction in unsigned char

  void mips_add(int d, int s, int t);
  void mips_sub(int d, int s, int t);
  void mips_slt(int d, int s, int t);
  void mips_sltu(int d, int s, int t);

  void mips_beq(int s, int t, uint16_t i);
  void mips_bne(int s, int t, uint16_t i);

  void mips_lis(int d);
  void mips_mflo(int d);
  void mips_mfhi(int d);
  void mips_mult(int s, int t);
  void mips_multu(int s, int t);
  void mips_div(int s, int t);
  void mips_divu(int s, int t);

  void mips_lw(int t, uint16_t i, int s);
  void mips_sw(int t, uint16_t i, int s);

  void mips_jr(int s);
  void mips_jalr(int s);

  void mips_dotword(int s);

  /* Error */
  void scanningError(string message);
  void parseError(int lineNumber, string message);
  void semanticError(int lineNumber, string message);

  /* Util */
  bool moreInputRemains() const;  // not virtual
  vector<Token> tokenizeNextLine();
  bool compilationSucceeded;
  vector<vector<Token>> programList;
  unordered_map<string, int> labelTable;  // map label name to line index
};

const int Assembler::alignedAccessMultiplier = 4;

Assembler::Assembler() : compilationSucceeded(true) {}

Assembler::~Assembler() {}

void Assembler::assemble() {
  if (scanning()) {
    if (parsing()) {
      if (semanticAnalysis()) {
        if (synthesis()) {
        }
      }
    }
  }
}

void Assembler::scanningError(string message) {
  cerr << "ERROR: ScanningFailure" << endl;
  cerr << message << endl;
  compilationSucceeded = false;
}

void Assembler::parseError(int lineNumber, string message) {
  cerr << "ERROR: Parse Error in line: ";
  for (auto &token : programList[lineNumber]) {
    cerr << token << ' ';
  }
  cerr << endl;
  cerr << message << endl;
  compilationSucceeded = false;
}

void Assembler::semanticError(int lineNumber, string message) {
  cerr << "ERROR: Semantic Error in line: ";
  for (auto &token : programList[lineNumber]) {
    cerr << token << ' ';
  }
  cerr << endl;
  cerr << message << endl;
  compilationSucceeded = false;
}

bool Assembler::moreInputRemains() const { return !cin.eof(); }

void Assembler::outInstruction(int i) {
  putchar(i >> 24);
  putchar(i >> 16);
  putchar(i >> 8);
  putchar(i);
}

void Assembler::mips_add(int d, int s, int t) {
  outInstruction((s << 21) | (t << 16) | (d << 11) | 32);
}
void Assembler::mips_sub(int d, int s, int t) {
  outInstruction((s << 21) | (t << 16) | (d << 11) | 34);
}
void Assembler::mips_slt(int d, int s, int t) {
  outInstruction((s << 21) | (t << 16) | (d << 11) | 42);
}
void Assembler::mips_sltu(int d, int s, int t) {
  outInstruction((s << 21) | (t << 16) | (d << 11) | 43);
}

void Assembler::mips_beq(int s, int t, uint16_t i) {
  outInstruction(0x10000000 | (s << 21) | (t << 16) | i);
}  // use uint16_t truncate the int
void Assembler::mips_bne(int s, int t, uint16_t i) {
  outInstruction(0x14000000 | (s << 21) | (t << 16) | i);
}

void Assembler::mips_lis(int d) { outInstruction((d << 11) | 20); }
void Assembler::mips_mflo(int d) { outInstruction((d << 11) | 18); }
void Assembler::mips_mfhi(int d) { outInstruction((d << 11) | 16); }

void Assembler::mips_mult(int s, int t) {
  outInstruction((s << 21) | (t << 16) | 24);
}
void Assembler::mips_multu(int s, int t) {
  outInstruction((s << 21) | (t << 16) | 25);
}
void Assembler::mips_div(int s, int t) {
  outInstruction((s << 21) | (t << 16) | 26);
}
void Assembler::mips_divu(int s, int t) {
  outInstruction((s << 21) | (t << 16) | 27);
}

void Assembler::mips_lw(int t, uint16_t i, int s) {
  outInstruction(0x8c000000 | (s << 21) | (t << 16) | i);
}
void Assembler::mips_sw(int t, uint16_t i, int s) {
  outInstruction(0xac000000 | (s << 21) | (t << 16) | i);
}

void Assembler::mips_jr(int s) { outInstruction((s << 21) | 8); }
void Assembler::mips_jalr(int s) { outInstruction((s << 21) | 9); }

void Assembler::mips_dotword(int s) { outInstruction(s); }

bool Assembler::scanning() {
  while (moreInputRemains()) {
    vector<Token> tokenLine{};

    vector<Token> temp = tokenizeNextLine();
    while (moreInputRemains() && temp.size() &&
           temp.back().getKind() == Token::LABEL) {
      tokenLine.insert(tokenLine.end(), temp.begin(), temp.end());
      temp = tokenizeNextLine();
    }
    tokenLine.insert(tokenLine.end(), temp.begin(), temp.end());

    if (tokenLine.size()) {
      programList.push_back(tokenLine);
    }
  }

  return true;
}

bool Assembler::parsing() {
  // handle label
  for (int i = 0; i < programList.size(); i++) {
    int labelCount = 0;
    while (programList[i].size() > labelCount &&
           programList[i][labelCount].getKind() == Token::LABEL) {
      if (!recordLabel(programList[i][labelCount], i)) {
        parseError(i, "Duplicate symbol: " + programList[i][0].getLexeme());
        return false;
      }
      labelCount++;
    }

    if (labelCount > 0) {
      programList[i].erase(programList[i].begin(),
                           programList[i].begin() + labelCount);
      if (programList.back().size() == 0) {  // handle a line full of label
        programList.pop_back();
      }
    }
  }

  // handle syntax
  for (int i = 0; i < programList.size(); i++) {
    Token frontToken = programList[i][0];
    if (frontToken.getKind() == Token::ID) {
      if (frontToken.getLexeme() == "add" || frontToken.getLexeme() == "sub" ||
          frontToken.getLexeme() == "slt" || frontToken.getLexeme() == "sltu") {
        if (!checkRegCommaRegCommaReg(i)) {
          parseError(
              i,
              "Expecting a valid add, sub, slt, or sltu with valid register");
          return false;
        }
      } else if (frontToken.getLexeme() == "mult" ||
                 frontToken.getLexeme() == "multu" ||
                 frontToken.getLexeme() == "div" ||
                 frontToken.getLexeme() == "divu") {
        if (!checkRegCommaReg(i)) {
          parseError(i,
                     "Expecting a valid mult, multu, div, or divu with valid "
                     "register");
          return false;
        }
      } else if (frontToken.getLexeme() == "mfhi" ||
                 frontToken.getLexeme() == "mflo" ||
                 frontToken.getLexeme() == "lis") {
        if (!checkReg(i)) {
          parseError(
              i, "Expecting a valid mfhi, mflo, or lis with valid register");
          return false;
        }
      } else if (frontToken.getLexeme() == "lw" ||
                 frontToken.getLexeme() == "sw") {
        if (!checkRegCommaIntLparenRegRparen(i)) {
          parseError(i,
                     "Expecting a valid lw, or sw with valid register and "
                     "address reference");
          return false;
        }
      } else if (frontToken.getLexeme() == "beq" ||
                 frontToken.getLexeme() == "bne") {
        if (!checkRegCommaRegCommaTop(i)) {
          parseError(i,
                     "Expecting a valid beq, or bne with valid register and "
                     "address reference");
          return false;
        }
      } else if (frontToken.getLexeme() == "jr" ||
                 frontToken.getLexeme() == "jalr") {
        if (!checkReg(i)) {
          parseError(i, "Expecting a valid jr, or jalr with valid register");
          return false;
        }
      } else {
        parseError(i, "Expecting opcode, label, or directive");
        return false;
      }
    } else if (frontToken.getKind() == Token::WORD) {
      if (!checkWord(i)) {
        parseError(i,
                   "Expecting valid .word with a valid int, hexint, label, or "
                   "address");
        return false;
      }
    } else {
      parseError(i, "Expecting opcode, label, or directive");
      return false;
    }
  }

  return true;
}

bool Assembler::semanticAnalysis() {
  for (int i = 0; i < programList.size(); i++) {
    Token frontToken = programList[i][0];
    // check beq, bne, .word and replace their id label with address
    if (frontToken.getLexeme() == "beq" || frontToken.getLexeme() == "bne") {
      Token token5 = programList[i][5];
      if (token5.getKind() == Token::ID) {
        if (labelTable.find(token5.getLexeme()) != labelTable.end()) {
          int move = labelTable[programList[i][5].getLexeme()] - i - 1;
          if (move >= -32768 && move <= 32767) {
            programList[i][5] = Token(Token::INT, to_string(move));
          } else {
            semanticError(i, "Bne, Beq label address out of bounds");
            return false;
          }
        } else {
          semanticError(i, "Bne or Beq referenced a nonexisting label");
          return false;
        }
      }
    } else if (frontToken.getKind() == Token::WORD) {
      Token token1 = programList[i][1];
      if (token1.getKind() == Token::ID) {
        if (labelTable.find(token1.getLexeme()) != labelTable.end()) {
          // If a label is used for i, its value is encoded as an unsigned
          // 32-bit integer. Although this technically imposes a limit on the
          // maximum value of a label operand for .word, MIPS assemblers are not
          // required to enforce this limit, since a program several gigabytes
          // in size would be needed to reach it. we replace with the actual
          // (index*4)
          programList[i][1] = Token(
              Token::INT, to_string(labelTable[programList[i][1].getLexeme()] *
                                    alignedAccessMultiplier));
        } else {
          semanticError(i, ".word refereced a nonexisting label");
          return false;
        }
      }
    }
  }

  return true;
}

bool Assembler::synthesis() {
  for (int i = 0; i < programList.size(); i++) {
    Token frontToken = programList[i][0];
    if (frontToken.getKind() == Token::ID) {
      if (frontToken.getLexeme() == "add") {
        mips_add(programList[i][1].toNumber(), programList[i][3].toNumber(),
                 programList[i][5].toNumber());
      } else if (frontToken.getLexeme() == "sub") {
        mips_sub(programList[i][1].toNumber(), programList[i][3].toNumber(),
                 programList[i][5].toNumber());
      } else if (frontToken.getLexeme() == "slt") {
        mips_slt(programList[i][1].toNumber(), programList[i][3].toNumber(),
                 programList[i][5].toNumber());
      } else if (frontToken.getLexeme() == "sltu") {
        mips_sltu(programList[i][1].toNumber(), programList[i][3].toNumber(),
                  programList[i][5].toNumber());
      } else if (frontToken.getLexeme() == "mult") {
        mips_mult(programList[i][1].toNumber(), programList[i][3].toNumber());
      } else if (frontToken.getLexeme() == "multu") {
        mips_multu(programList[i][1].toNumber(), programList[i][3].toNumber());
      } else if (frontToken.getLexeme() == "div") {
        mips_div(programList[i][1].toNumber(), programList[i][3].toNumber());
      } else if (frontToken.getLexeme() == "divu") {
        mips_divu(programList[i][1].toNumber(), programList[i][3].toNumber());
      } else if (frontToken.getLexeme() == "mfhi") {
        mips_mfhi(programList[i][1].toNumber());
      } else if (frontToken.getLexeme() == "mflo") {
        mips_mflo(programList[i][1].toNumber());
      } else if (frontToken.getLexeme() == "lis") {
        mips_lis(programList[i][1].toNumber());
      } else if (frontToken.getLexeme() == "lw") {
        mips_lw(programList[i][1].toNumber(), programList[i][3].toNumber(),
                programList[i][5].toNumber());
      } else if (frontToken.getLexeme() == "sw") {
        mips_sw(programList[i][1].toNumber(), programList[i][3].toNumber(),
                programList[i][5].toNumber());
      } else if (frontToken.getLexeme() == "beq") {
        mips_beq(programList[i][1].toNumber(), programList[i][3].toNumber(),
                 programList[i][5].toNumber());
      } else if (frontToken.getLexeme() == "bne") {
        mips_bne(programList[i][1].toNumber(), programList[i][3].toNumber(),
                 programList[i][5].toNumber());
      } else if (frontToken.getLexeme() == "jr") {
        mips_jr(programList[i][1].toNumber());
      } else if (frontToken.getLexeme() == "jalr") {
        mips_jalr(programList[i][1].toNumber());
      } else {
        return false;
      }
    } else if (frontToken.getKind() == Token::WORD) {
      mips_dotword(programList[i][1].toNumber());
    }
  }
  return true;
}

bool Assembler::checkRegCommaRegCommaReg(int lineNumber) {
  if (programList[lineNumber].size() != 6) {
    return false;
  }

  Token token1 = programList[lineNumber][1];
  Token token2 = programList[lineNumber][2];
  Token token3 = programList[lineNumber][3];
  Token token4 = programList[lineNumber][4];
  Token token5 = programList[lineNumber][5];

  if (isValidReg(token1) && token2.getKind() == Token::COMMA &&
      isValidReg(token3) && token4.getKind() == Token::COMMA &&
      isValidReg(token5)) {
    return true;
  }

  return false;
}

bool Assembler::checkRegCommaReg(int lineNumber) {
  if (programList[lineNumber].size() != 4) {
    return false;
  }

  Token token1 = programList[lineNumber][1];
  Token token2 = programList[lineNumber][2];
  Token token3 = programList[lineNumber][3];

  if (isValidReg(token1) && token2.getKind() == Token::COMMA &&
      isValidReg(token3)) {
    return true;
  }
  return false;
}

bool Assembler::checkReg(int lineNumber) {
  if (programList[lineNumber].size() != 2) {
    return false;
  }

  Token token1 = programList[lineNumber][1];

  if (isValidReg(token1)) {
    return true;
  }
  return false;
}

bool Assembler::checkRegCommaIntLparenRegRparen(int lineNumber) {
  if (programList[lineNumber].size() != 7) {
    return false;
  }

  Token token1 = programList[lineNumber][1];
  Token token2 = programList[lineNumber][2];
  Token token3 = programList[lineNumber][3];
  Token token4 = programList[lineNumber][4];
  Token token5 = programList[lineNumber][5];
  Token token6 = programList[lineNumber][6];

  if (isValidReg(token1) && token2.getKind() == Token::COMMA &&
      isValid16sSigned(token3) && token4.getKind() == Token::LPAREN &&
      isValidReg(token5) && token6.getKind() == Token::RPAREN) {
    return true;
  }
  return false;
}

bool Assembler::checkRegCommaRegCommaTop(int lineNumber) {
  if (programList[lineNumber].size() != 6) {
    return false;
  }

  Token token1 = programList[lineNumber][1];
  Token token2 = programList[lineNumber][2];
  Token token3 = programList[lineNumber][3];
  Token token4 = programList[lineNumber][4];
  Token token5 = programList[lineNumber][5];

  if (isValidReg(token1) && token2.getKind() == Token::COMMA &&
      isValidReg(token3) && token4.getKind() == Token::COMMA) {
    if (token5.getKind() == Token::INT || token5.getKind() == Token::HEXINT) {
      if (isValid16sSigned(token5)) {
        return true;
      }
    } else if (token5.getKind() == Token::ID) {
      return true;
    }
  }
  return false;
}

bool Assembler::checkWord(int lineNumber) {
  if (programList[lineNumber].size() != 2) {
    return false;
  }
  Token token0 = programList[lineNumber][0];
  Token token1 = programList[lineNumber][1];
  if (token0.getKind() != Token::WORD) {
    return false;
  }

  if (token1.getKind() == Token::INT && token1.toNumber() >= -pow(2, 31) &&
      token1.toNumber() <= pow(2, 32) - 1) {
    return true;
  } else if (token1.getKind() == Token::HEXINT &&
             token1.toNumber() <= 0xffffffff) {
    return true;
  } else if (token1.getKind() == Token::ID) {
    return true;
  }

  return false;
}

bool Assembler::isValidReg(Token token) {
  if (token.getKind() == Token::REG && token.toNumber() >= 0 &&
      token.toNumber() <= 31) {
    return true;
  }
  return false;
}

bool Assembler::isValid16sSigned(Token token) {
  if (token.getKind() == Token::INT && token.toNumber() >= -32768 &&
      token.toNumber() <= 32767) {
    return true;
  } else if (token.getKind() == Token::HEXINT && token.toNumber() <= 0xffff) {
    return true;
  }
  return false;
}

vector<Token> Assembler::tokenizeNextLine() {
  string line;
  vector<Token> tokenLine{};
  try {
    while (tokenLine.size() == 0 && getline(cin, line)) {
      tokenLine = scan(line);
    }
  } catch (ScanningFailure &f) {
    scanningError(f.what());
    return {};
  }
  return tokenLine;
}

bool Assembler::recordLabel(Token label, int lineNumber) {
  string lexeme = label.getLexeme();
  lexeme.erase(remove(lexeme.begin(), lexeme.end(), ':'),
               lexeme.end());  // remove the semicolon at the end

  if (labelTable.find(lexeme) != labelTable.end()) {
    return false;
  }
  labelTable[lexeme] =
      lineNumber;  // programList index corresponds to the index, size =
                   // maxIndex + 1 corresponds to the newLine index

  return true;
}

int main() {
  Assembler *assembler = new Assembler{};
  assembler->assemble();

  delete assembler;
}
