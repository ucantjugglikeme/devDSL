#include "../headers/Parser.h"

Parser::Parser(V *tokenList) : tokenList(*tokenList), listIt(this->tokenList.begin()),
    curLineNum(1), lines((tokenList->back())->line), tree("lang", "", curLineNum){}

Parser::~Parser()
{
    for (size_t it = 0; it < tokenList.size(); ++it) { delete tokenList[it]; }
    tokenList.clear();
}

// lang -> (expr)*

int Parser::lang()
{   
    std::cout<<"in lang!\n";

    try {  while ((*listIt)->type != "EOF") { this->expr(); }  }
    catch (ParsingException &e) { return 1; }
    std::cout<<"parsed successfully!\n";
    this->tree.showTree();
    return 0;
}

// expr -> (functionDef | functionCall | assignment | opIf | opWhile | opDoWhile){1}
 
void Parser::expr()
{
    unsigned int fixedLineNum = this->curLineNum;

    const FuncV expressions =
    {
        {"functionDef", &Parser::functionDef}, {"functionCall", &Parser::functionCall},
        {"assignment", &Parser::assignment}, {"opIf", &Parser::opIf},
        {"opWhile", &Parser::opWhile}, {"opDoWhile", &Parser::opDoWhile},
        {"failure", &Parser::generateFailure}
    };
    VecIt fixedIt = listIt;

    std::cout<<"in expr!\n";
    for (KeyFunc keyFuncIt : expressions)
    {
        try
        {
            std::cout<<"trying "<<(&keyFuncIt)->first<<"\n";
            (this->*((&keyFuncIt)->second))();
            std::cout<<"matched\n";
            fixedIt = listIt;
            break;
        }
        catch (ParsingException & e)
        {
            std::cerr<<e.what()<<"\n";
            if (e.getWhy() == "failed to parse") { throw e; }
            listIt = fixedIt;
        }
    }

    ASTNode *expr = new ASTNode(Token({"expr", "", fixedLineNum}), &tree);
    this->tree.addChild(expr);
}

/*---------------------------------------------EXCEPTIONS---------------------------------------------*/

string Parser::generateException(string expected, string provided)
{
    curLineNum = (*listIt)->line;
    return "in line " + std::to_string(curLineNum) + "\nexpected: " + expected + "; " +
            provided + " was provided";
}

void Parser::generateFailure() { throw ParsingException(std::string("failed to parse")); }

/*-------------------------------------------EXPRESSIONS----------------------------------------------*/

void Parser::functionDef()
{
    // functionCall -> def{1}function{1}lBracket{1}(variable{1}((,){1}variable{1})*)?rBracket{1}block{1}
    this->keyword("def");
    this->function();
    this->lBracket();

    try { this->variable(); }
    catch (ParsingException & e) 
    { 
        this->rBracket();
        this->block();
        return;
    }
    while ((*listIt)->type != "EOF")
    {
        try { this->argsSeparator(); }
        catch (ParsingException &e) { break; }
        this->variable();
    }
    
    this->rBracket();
    this->block();
}

void Parser::functionCall()
{
    // functionCall -> function{1}lBracket{1}(value{1}((,){1}
    // (arithmeticExpression{1}|conditionalExpression{1}))*)?rBracket{1}separator?
    this->function();
    this->lBracket();

    try { this->value(); }
    catch (ParsingException &e) 
    { 
        this->rBracket();
        return;
    }
    while ((*listIt)->type != "EOF")
    {
        try { this->argsSeparator(); }
        catch (ParsingException &e) { break; }
        try { this->arithmeticExpression(); }
        catch (ParsingException &e) { this->conditionalExpression();}
    }

    this->rBracket();

    try { this->separator(); }
    catch (ParsingException &e){}
}

void Parser::block()
{
    // block -> (\{){1}
    // (functionDef|functionCall|assignment|opIf|opReturn|opWhile|opDoWhile|opBreak|opContinue)?
    // (\}){1}
    this->lBrace();

    const FuncV blockExpressions
    {
        {"functionDef", &Parser::functionDef}, {"functionCall", &Parser::functionCall},
        {"assignment", &Parser::assignment}, {"opIf", &Parser::opIf},
        {"opReturn", &Parser::opReturn}, {"opWhile", &Parser::opWhile},
        {"opDoWhile", &Parser::opDoWhile}, {"opBreak", &Parser::opBreak}, 
        {"opContinue", &Parser::opContinue}, {"failure", &Parser::generateFailure}
    };
    VecIt fixedIt = listIt;    

    std::cout<<"in block!\n";
    while ((*listIt)->type != "EOF")
    {
        try
        {
            for (KeyFunc keyFuncIt : blockExpressions)
            {
                try 
                {
                    std::cout<<"trying "<<(&keyFuncIt)->first<<"\n";
                    (this->*((&keyFuncIt)->second))();
                    std::cout<<"matched\n";
                    fixedIt = listIt;
                    break;
                }
                catch (ParsingException & e) 
                {
                    std::cerr<<e.what()<<"\n";
                    if (e.getWhy() == "failed to parse") { throw e; }
                    listIt = fixedIt;
                }
            }
        }
        catch (ParsingException & e) { this->rBrace(); break; }
    }
}

void Parser::assignment()
{
    // assignment -> variable{1}(=){1}(arithmeticExpression|conditionalExpression){1}
    this->variable();

    this->assignOp();

    VecIt fixedIt = listIt;
    try { this->arithmeticExpression(); this->endLine(); }
    catch (ParsingException & e) { listIt = fixedIt; this->conditionalExpression(); }
}

void Parser::allocation()
{
    this->keyword("new");

    this->constructor();

    this->lBracket();

    try { this->value(); }
    catch (ParsingException &e) 
    { 
        this->rBracket();
        return;
    }
    while ((*listIt)->type != "EOF")
    {
        try { this->argsSeparator(); }
        catch (ParsingException &e) { break; }
        try { this->arithmeticExpression(); }
        catch (ParsingException &e) { this->conditionalExpression();}
    }

    this->rBracket();

    try { this->separator(); }
    catch (ParsingException &e){}

}

void Parser::arithmeticExpression()
{
    // arithmeticExpression -> ( (\(arithmeticExpression{1}\)mathOp{1})?notEndLine{1}
    // unaryOP?value{1}unaryOp?((mathOp{1}arithmeticExpression{1})|((;)?)){1}
    do 
    {
        try
        {
            try { this->lBracket(); }
            catch (ParsingException & e) { break; }
            this->arithmeticExpression();
            this->rBracket();
            try { this->mathOp(); }
            catch (ParsingException & e) { return; }
        }
        catch (ParsingException & e) { throw e; }
    } while (false);

    this->notEndLine();
    try { this->unaryOp(); }
    catch(ParsingException & e){}
    try { this->value(); }
    catch (ParsingException &e) 
    { 
        try { this->functionCall(); }
        catch (ParsingException &e) { this->allocation(); } 
    }
    try { this->unaryOp(); }
    catch(ParsingException & e){}

    try { this->mathOp(); }
    catch (ParsingException & e)
    {
        try { this->separator(); }
        catch (ParsingException & e){}
        return;
    }
    this->arithmeticExpression();
}

void Parser::conditionalExpression()
{
    // conditionalExpression -> ( (\(conditionalExpression{1}\)(comprOp{1}|logicalOp))?notEndLine{1}
    // arithmeticExpression{1}(((comprOp{1}|logicalOp{1})conditionalExpression{1})|((;)?)){1}
    try { this->logicalNegation(); }
    catch (ParsingException & e){}

    do 
    {
        try
        {
            try { this->lBracket(); }
            catch (ParsingException & e) { break; }
            this->conditionalExpression();
            this->rBracket();
            try { this->comprOp(); break; }
            catch (ParsingException & e) {}
            try { this->logicalOp(); }
            catch (ParsingException & e) { return; }
        }
        catch (ParsingException & e) { throw e; }
    } while (false);

    this->notEndLine();
    this->arithmeticExpression();

    do
    {
        try { this->comprOp(); break; }
        catch (ParsingException & e){}
        try { this->logicalOp(); }
        catch (ParsingException & e)
        {
            try { this->separator(); }
            catch (ParsingException & e){}
            return;
        }
    } while (false);

    this->conditionalExpression();
}

void Parser::opReturn()
{
    // opReturn -> (return){1}(arithmeticExpression{1}|conditionalExpression{1}|allocation{1})?
    // separator?
    this->keyword("return");

    VecIt fixedIt = listIt;
    try { this->arithmeticExpression(); this->endLine(); }
    catch (ParsingException & e) { listIt = fixedIt; this->conditionalExpression(); }
}

void Parser::opBreak()
{
    // opBreak -> (break){1}separator?
    this->keyword("break");

    try { this->separator(); }
    catch (ParsingException &e){}
}

void Parser::opContinue()
{
    // opContinue -> (continue){1}separator?
    this->keyword("continue");

    try { this->separator(); }
    catch (ParsingException &e){}
}

void Parser::opIf()
{
    // opIf -> (if){1}(\(){1}conditionalExpression{1}(\)){1}block{1}opElif?opElse?
    this->keyword("if");
    this->lBracket();
    this->conditionalExpression();
    this->rBracket();
    this->block();

    try { this->opElif(); }
    catch (ParsingException & e){}

    try { this->opElse(); }
    catch (ParsingException & e){}
}

void Parser::opElif()
{
    // opElif -> (elif){1}(\(){1}conditionalExpression{1}(\)){1}block{1}opElif?opElse?
    this->keyword("elif");
    this->lBracket();
    this->conditionalExpression();
    this->rBracket();
    this->block();

    try { this->opElif(); }
    catch (ParsingException & e){}

    try { this->opElse(); }
    catch (ParsingException & e){}
}

void Parser::opElse()
{
    // opElse -> (else){1}block{1}
    this->keyword("else");
    this->block();
}

void Parser::opDoWhile()
{
    // opDoWhile -> (do){1}block{1}(while){1}(\(){1}conditionalExpression{1}(\)){1}separator?
    this->keyword("do");
    this->block();
    this->keyword("while");
    this->lBracket();
    this->conditionalExpression();
    this->rBracket();
    try { this->separator(); }
    catch(ParsingException & e){}
}

void Parser::opWhile()
{
    // opWhile -> (while){1}(\(){1}conditionalExpression{1}(\)){1}block{1}
    this->keyword("while");
    this->lBracket();
    this->conditionalExpression();
    this->rBracket();
    this->block();
}

/*-------------------------------------------------ATHOMS---------------------------------------------*/

void Parser::keyword(string concrete)
{
    if (std::strcmp((*listIt)->value.c_str(), concrete.c_str()) != 0) 
    {
        throw ParsingException(generateException(concrete, (*listIt)->type)); 
    }
    else { listIt++; }
}

void Parser::constructor()
{
    if ((*listIt)->type != "CONSTRUCTOR")
    {
        throw ParsingException(generateException(string("constructor"), (*listIt)->type));
    }
    else { listIt++; }
}

void Parser::function()
{
    if ((*listIt)->type != "FUNCTION")
    {
        throw ParsingException(generateException(string("function"), (*listIt)->type));
    }
    else { listIt++; }
}

void Parser::lBracket()
{
    if ((*listIt)->type != "L_BRACKET")
    {
        throw ParsingException(generateException(std::string("'('"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::variable()
{
    if ((*listIt)->type != "VARIABLE")
    {
        throw ParsingException(generateException(std::string("variable"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::argsSeparator()
{
    if ((*listIt)->type != "ARG_SEPARATOR")
    {
        throw ParsingException(generateException(std::string("','"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::rBracket()
{
    if ((*listIt)->type != "R_BRACKET")
    {
        throw ParsingException(generateException(std::string("')'"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::lBrace()
{
    if ((*listIt)->type != "L_BRACE")
    {
        throw ParsingException(generateException(std::string("'{'"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::rBrace()
{
    if ((*listIt)->type != "R_BRACE")
    {
        throw ParsingException(generateException(std::string("'}'"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::assignOp()
{
    if ((*listIt)->type != "ASSIGN_OPERATOR")
    {
        throw ParsingException(generateException(std::string("'='"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::unaryOp()
{
    if ((*listIt)->type != "UNARY_OPERATOR")
    {
        throw ParsingException(generateException(std::string("unary operator"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::logicalNegation()
{
    if ((*listIt)->type != "LOGICAL_NEGATION")
    {
        throw ParsingException(generateException(std::string("'!'"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::value()
{
    if ((*listIt)->type != "INTEGER" and (*listIt)->type != "STRING" and
             (*listIt)->type != "VARIABLE" and (*listIt)->type != "FLOAT" and
             (*listIt)->type != "CONSTANT_KW")
    {
        throw ParsingException(generateException(std::string("value"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::mathOp()
{
    if ((*listIt)->type != "MATH_OPERATOR")
    {
        throw ParsingException(generateException(std::string("math. operator"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::separator()
{
    if ((*listIt)->type != "SEPARATOR")
    {
        throw ParsingException(generateException(std::string("';'"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::comprOp()
{
    if ((*listIt)->type != "COMPRASION_OPERATOR")
    {
        throw ParsingException(generateException(std::string("compr. operator"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::logicalOp()
{
    if ((*listIt)->value != "and" and (*listIt)->value != "or" and
        (*listIt)->value != "nand" and (*listIt)->value != "nor")
    {
        throw ParsingException(generateException(std::string("logical operator"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::endLine()
{
    listIt--;
    int prevTokenLine  = (*listIt)->line;
    string probablySep = (*listIt)->type;

    listIt++;
        curLineNum    = (*listIt)->line;

    if (curLineNum == prevTokenLine or probablySep == "SEPARATOR")
    {
        throw ParsingException(generateException(std::string("'\\n' or ';'"), (*listIt)->type));
    }
}

void Parser::notEndLine()
{
    listIt--;
    int prevTokenLine = (*listIt)->line;

    listIt++;
        curLineNum    = (*listIt)->line;

    if (curLineNum > prevTokenLine)
    {
        throw ParsingException(generateException(std::string("value"), (*listIt)->type));
    }
}