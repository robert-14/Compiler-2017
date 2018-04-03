#include "header.h"
#include "symbolTable.h"

void gen_prologue(char *name);
void gen_epilogue(char *name);
int get_reg();
int get_freg();
void free_reg();
void genConstValue(AST_NODE* constNode);
void genIdValue(AST_NODE* IdNode);
void genArrayDecl(AST_NODE* idNode);
void genScalarDecl(AST_NODE* idNode, int isInit);
void gen_head(char *name);
void generateCode(AST_NODE *programNode);
void genDeclarationNode(AST_NODE* declarationNode);
void genDeclareIdList(AST_NODE* typeNode);
void genFunctionDeclaration(AST_NODE* returnTypeNode);
int genDimNode(AST_NODE* idNode);
void genBlockNode(AST_NODE* blockNode);
void genStmtNode(AST_NODE* stmtNode);
void AssignOrExpr(AST_NODE* assignOrExprRelatedNode);
void genWhileStmt(AST_NODE* whileNode);
void genForStmt(AST_NODE* forNode);
void genAssignmentStmt(AST_NODE* assignmentNode);
void genIfStmt(AST_NODE* ifNode);
void genFunctionCall(AST_NODE* functionCallNode);
void genReturnStmt(AST_NODE* returnNode);
void genExprNode(AST_NODE* exprNode);