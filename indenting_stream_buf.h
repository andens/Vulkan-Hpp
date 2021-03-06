#ifndef INDENTING_STREAM_BUF_INCLUDE
#define INDENTING_STREAM_BUF_INCLUDE

#include <iostream>

class IndentingOStreambuf : public std::streambuf
{
  std::streambuf*     myDest;
  bool                myIsAtStartOfLine;
  //std::string         myIndent;
  int                 myIndent;
  std::ostream*       myOwner;
  int                 myIndentStep;
protected:
  virtual int         overflow(int ch)
  {
    if (myIsAtStartOfLine && ch != '\n') {
      //myDest->sputn(myIndent.data(), myIndent.size());
      for (int i = 0; i < myIndent; ++i) {
        myDest->sputc(' ');
      }
    }
    myIsAtStartOfLine = ch == '\n';
    return myDest->sputc(ch);
  }
public:
  explicit            IndentingOStreambuf(
    std::streambuf* dest, int indent = 4)
    : myDest(dest)
    , myIsAtStartOfLine(true)
    //, myIndent(indent, ' ')
    , myIndent(0)
    , myOwner(NULL)
    , myIndentStep(indent)
  {
  }
  explicit            IndentingOStreambuf(
    std::ostream& dest, int indent = 4)
    : myDest(dest.rdbuf())
    , myIsAtStartOfLine(true)
    //, myIndent(indent, ' ')
    , myIndent(0)
    , myOwner(&dest)
    , myIndentStep(indent)
  {
    myOwner->rdbuf(this);
  }
  virtual             ~IndentingOStreambuf()
  {
    if (myOwner != NULL) {
      myOwner->rdbuf(myDest);
    }
  }
  void increase() {
    myIndent += myIndentStep;
  }
  void decrease() {
    myIndent -= myIndentStep;
    if (myIndent < 0) {
      myIndent = 0;
    }
  }
};

#endif
