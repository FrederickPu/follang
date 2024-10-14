#include <iostream>
#include "parserlexer.h"

// example definition syntax: def f() forall x, x
int main() {
    std::map<Binop, int> BinopPrecedence;
    BinopPrecedence[op_and] = 40;
    BinopPrecedence[op_or] = 20;

    auto parser = new ParserLexer(BinopPrecedence);

    // Prime the first token.
    fprintf(stderr, "ready> ");
    parser->getNextToken();

    parser->MainLoop();

    return 0;
}
