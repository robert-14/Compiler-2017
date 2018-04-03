#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "symbolTable.h"
// This file is for reference only, you are not required to follow the implementation. //
// You only need to check for errors stated in the hw4 assignment document. //
int g_anyErrorOccur = 0;

DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2);
void processProgramNode(AST_NODE *programNode);
void processDeclarationNode(AST_NODE* declarationNode);
void declareIdList(AST_NODE* typeNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize);
void declareFunction(AST_NODE* returnTypeNode);
void processDeclDimList(AST_NODE* variableDeclDimList, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize);
void processTypeNode(AST_NODE* typeNode);
void processBlockNode(AST_NODE* blockNode);
void processStmtNode(AST_NODE* stmtNode);
void processGeneralNode(AST_NODE *node);
void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode);
void checkWhileStmt(AST_NODE* whileNode);
void checkForStmt(AST_NODE* forNode);
void checkAssignmentStmt(AST_NODE* assignmentNode);
void checkIfStmt(AST_NODE* ifNode);
void checkWriteFunction(AST_NODE* functionCallNode);
void checkFunctionCall(AST_NODE* functionCallNode);
void processExprRelatedNode(AST_NODE* exprRelatedNode);
void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter);
void checkReturnStmt(AST_NODE* returnNode);
void processExprNode(AST_NODE* exprNode);
void processVariableLValue(AST_NODE* idNode);
void processVariableRValue(AST_NODE* idNode);
void processConstValueNode(AST_NODE* constValueNode);
void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue);
void evaluateExprValue(AST_NODE* exprNode);
int calculateExprNode(AST_NODE* exprNode);

typedef enum ErrorMsgKind
{
    SYMBOL_IS_NOT_TYPE,
    SYMBOL_REDECLARE,
    SYMBOL_UNDECLARED,
    NOT_FUNCTION_NAME,
    TRY_TO_INIT_ARRAY,
    EXCESSIVE_ARRAY_DIM_DECLARATION,
    RETURN_ARRAY,
    VOID_VARIABLE,
    TYPEDEF_VOID_ARRAY,
    PARAMETER_TYPE_UNMATCH,
    TOO_FEW_ARGUMENTS,
    TOO_MANY_ARGUMENTS,
    RETURN_TYPE_UNMATCH,
    INCOMPATIBLE_ARRAY_DIMENSION,
    NOT_ASSIGNABLE,
    NOT_ARRAY,
    IS_TYPE_NOT_VARIABLE,
    IS_FUNCTION_NOT_VARIABLE,
    STRING_OPERATION,
    ARRAY_SIZE_NOT_INT,
    ARRAY_SIZE_NEGATIVE,
    ARRAY_SUBSCRIPT_NOT_INT,
    PASS_ARRAY_TO_SCALAR,
    PASS_SCALAR_TO_ARRAY
} ErrorMsgKind;

void printErrorMsgSpecial(AST_NODE* node1, char* name2, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node1->linenumber);
    switch(errorMsgKind)
    {
        case(PASS_ARRAY_TO_SCALAR):
            printf("Array %s passed to scalar parameter %s.\n", node1->semantic_value.identifierSemanticValue.identifierName, name2);
            break;
        case(PASS_SCALAR_TO_ARRAY):
            printf("Scalar %s passed to array parameter %s.\n", node1->semantic_value.identifierSemanticValue.identifierName, name2);
            break;
        default:
            printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
            break;
    }
}


void printErrorMsg(AST_NODE* node, ErrorMsgKind errorMsgKind)
{
    g_anyErrorOccur = 1;
    printf("Error found in line %d\n", node->linenumber);
    switch(errorMsgKind)
    {
        case(SYMBOL_UNDECLARED):
            printf("ID %s undeclared.\n", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case(SYMBOL_REDECLARE):
            printf("ID %s redeclared.\n", node->semantic_value.identifierSemanticValue.identifierName);
            break;
        case(TOO_FEW_ARGUMENTS):
            printf("too few arguments to function %s.\n", node->semantic_value.identifierSemanticValue.identifierName); //要傳functionStmt下的ID node進去
            break;
        case(TOO_MANY_ARGUMENTS):
            printf("too many arguments to function %s.\n", node->semantic_value.identifierSemanticValue.identifierName); //要傳functionStmt下的ID node進去
            break;
        case(RETURN_TYPE_UNMATCH):
            printf("Incompatible return type.\n");
            break;
        case(INCOMPATIBLE_ARRAY_DIMENSION):
            printf("Incompatible array dimensions.\n");
            break;
        case(ARRAY_SUBSCRIPT_NOT_INT):
            printf("Array subscript is not an integer.\n");
            break;
        case(PARAMETER_TYPE_UNMATCH):
            printf("Parameter type unmatch\n");
            break;
        default:
            printf("Unhandled case in void printErrorMsg(AST_NODE* node, ERROR_MSG_KIND* errorMsgKind)\n");
            break;
    }
}


void semanticAnalysis(AST_NODE *root)
{
    processProgramNode(root);
}


DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2)
{
    if(dataType1 == FLOAT_TYPE || dataType2 == FLOAT_TYPE) {
        return FLOAT_TYPE;
    } else {
        return INT_TYPE;
    }
}


void processProgramNode(AST_NODE *programNode)
{
    printf("[In processProgramNode]\n");
    AST_NODE* child = programNode->child;
    while( child != NULL){
        AST_NODE* grandchild = child->child;
        if( child->nodeType == VARIABLE_DECL_LIST_NODE){
            while( grandchild != NULL){
                processDeclarationNode(grandchild);
                grandchild = grandchild->rightSibling;    
            }    
        }
        else if( child->nodeType == DECLARATION_NODE){
            processDeclarationNode(child);
        }   
        else{
            //errors
        }
        child = child->rightSibling;
    }    
    printf("End processProgramNode\n");
}

  
void processDeclarationNode(AST_NODE* declarationNode)
{
    printf("[In processDeclarationNode]\n");
    AST_NODE* child = declarationNode->child;
    if( declarationNode->semantic_value.declSemanticValue.kind == VARIABLE_DECL){
        declareIdList( child, VARIABLE_ATTRIBUTE, 0);
    }
    else if( declarationNode->semantic_value.declSemanticValue.kind == TYPE_DECL){
        processTypeNode( child);
    }
    else if( declarationNode->semantic_value.declSemanticValue.kind == FUNCTION_DECL){
        declareFunction( child);

    } 
    else if( declarationNode->semantic_value.declSemanticValue.kind == FUNCTION_PARAMETER_DECL){ //never come here
        declareIdList( child, VARIABLE_ATTRIBUTE, 1);
    }
    else{
        //error;
    }    
        
}


void processTypeNode(AST_NODE* idNodeAsType) // typedef float(idNodeAstype) nfloat
{
    printf("[In processTypeNode]\n");
    SymbolAttribute* attr;
    TypeDescriptor*  typedes;
    DATA_TYPE DataType;
    char* datatype = idNodeAsType->semantic_value.identifierSemanticValue.identifierName;
    AST_NODE* child = idNodeAsType->rightSibling;

    if(strcmp(datatype, "int")==0){
        DataType = INT_TYPE;
    }
    else if(strcmp(datatype, "float")==0){ 
        DataType = FLOAT_TYPE;
    }    
    else if(strcmp(datatype, "void")==0){
        DataType = VOID_TYPE;
    }
    while(child != NULL) {
        attr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
        typedes = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
        attr->attributeKind = TYPE_ATTRIBUTE;
        attr->attr.typeDescriptor = typedes;
        if(child->semantic_value.identifierSemanticValue.kind == NORMAL_ID) {
            typedes->kind = SCALAR_TYPE_DESCRIPTOR;
            typedes->properties.dataType = DataType;
        }else if(child->semantic_value.identifierSemanticValue.kind == ARRAY_ID) {
            typedes->kind = ARRAY_TYPE_DESCRIPTOR;
            processDeclDimList(child, typedes, 0);
        }else { /* error */ }
        child->semantic_value.identifierSemanticValue.symbolTableEntry = enterSymbol(child->semantic_value.identifierSemanticValue.identifierName, attr);
        child = child->rightSibling;
    }
}


void declareIdList(AST_NODE* declarationNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize)
{
    printf("[In declareIdList]====================\n");
    AST_NODE* id1, *id2;
    id1 = declarationNode;
    id2 = id1->rightSibling;

    DATA_TYPE data_type;
    //printf("attr address: %d\n", attr);
    char* name = id1->semantic_value.identifierSemanticValue.identifierName;

    //check the type
    if( !strcmp(name, "int") ){
        data_type = INT_TYPE;
    }
    else if( !strcmp(name, "float")){ 
        data_type = FLOAT_TYPE;
    }    
    else if( !strcmp(name,"void")){
        printErrorMsg(id1,VOID_VARIABLE);
        return;
    }
    else{                                   //typedef
        SymbolTableEntry* type = searchSymbol(name,TYPE_ATTRIBUTE); 
        if( type == NULL){                          //no declare before
            printErrorMsg(id1,SYMBOL_UNDECLARED); //print id name instead of type
            return;
        }    
        if(type->attribute->attributeKind != TYPE_ATTRIBUTE) { 
            printErrorMsg(declarationNode, SYMBOL_IS_NOT_TYPE);
            return;
        }
        if(type->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
            data_type = type->attribute->attr.typeDescriptor->properties.dataType;
        }
        else {
            data_type = type->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;
        }
    }    

    

    //check variable name
    while( id2 != NULL){
        SymbolAttribute* attr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
        attr->attributeKind = VARIABLE_ATTRIBUTE;
        TypeDescriptor* temp = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
        if( id2->semantic_value.identifierSemanticValue.kind == NORMAL_ID) {
            temp->kind = SCALAR_TYPE_DESCRIPTOR;
            temp->properties.dataType = data_type;
        }
        if( id2->semantic_value.identifierSemanticValue.kind == ARRAY_ID) {
            temp->kind = ARRAY_TYPE_DESCRIPTOR;
            temp->properties.arrayProperties.elementType = data_type;
            printf("mam iam here\n");
            processDeclDimList(id2, temp, ignoreArrayFirstDimSize);
        }
        if( id2->semantic_value.identifierSemanticValue.kind == WITH_INIT_ID) {
            temp->kind = SCALAR_TYPE_DESCRIPTOR;
            temp->properties.dataType = data_type;
        }
        
        attr->attr.typeDescriptor = temp;
        id2->semantic_value.identifierSemanticValue.symbolTableEntry = enterSymbol(id2->semantic_value.identifierSemanticValue.identifierName, attr);
        if(id2->semantic_value.identifierSemanticValue.symbolTableEntry == NULL){
            printErrorMsg(id2,SYMBOL_REDECLARE);
        }
        printSymbolTable();
        //printf("#######    %d   #######\n",id2->attr.typeDescriptor->kind);
        id2 = id2->rightSibling;  
    }
}

void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
{
}

void checkWhileStmt(AST_NODE* whileNode)
{
    printf("[In checkWhileStmt]\n");
    AST_NODE *cond, *block;
    cond = whileNode->child;
    block = cond->rightSibling;

    processGeneralNode(cond);
    openScope();
    processBlockNode(block);
    closeScope();
}


void checkForStmt(AST_NODE* forNode)    //check each element in for 
{
    printf("[In checkForStmt]\n");
    AST_NODE *init, *cond, *incr, *block;
    init = forNode;
    cond = init->rightSibling;
    incr = cond->rightSibling;
    block = incr->rightSibling;

    AST_NODE* child = init->child;
    switch(init->nodeType) {                //stmt is child of assign expr
        case NONEMPTY_ASSIGN_EXPR_LIST_NODE:
            while(child != NULL) {
                if(child->nodeType == STMT_NODE)
                    processStmtNode(child);
                child = child->rightSibling;
            }
            break;
        case NUL_NODE:
            break;
        default:
            break;
    }
    child = cond->child;
    switch(cond->nodeType) {
        case NONEMPTY_RELOP_EXPR_LIST_NODE:
            while(child != NULL) {
                processGeneralNode(child);
                child = child->rightSibling;
            }
            break;
        case NUL_NODE:
            break;
        default:
            break;
    }
    child = incr->child;
    switch(incr->nodeType) {
        case NONEMPTY_ASSIGN_EXPR_LIST_NODE:
            while(child != NULL) {
                if(child->nodeType == STMT_NODE)
                    processStmtNode(child);
                child = child->rightSibling;
            }
            break;
        case NUL_NODE:
            break;
        default:
            break;
    }
    openScope();
    processBlockNode(block);
    closeScope();
}


void checkAssignmentStmt(AST_NODE* assignmentNode)
{
    printf("[In checkAssignmentStmt]\n");
    AST_NODE *left, *right, *temp;
    DATA_TYPE ldata, rdata;
    left = assignmentNode;
    right = left->rightSibling;
    if(right->rightSibling != NULL) { /* Assignment has three arguments?! */}
    /* Check left identifier */
    processVariableLValue(left);
    if(left->nodeType == IDENTIFIER_NODE) {
        char* id_name = left->semantic_value.identifierSemanticValue.identifierName;
        printf("I am %s\n",id_name);
        SymbolTableEntry* entry = searchSymbol(id_name,VARIABLE_ATTRIBUTE);
        if(entry != NULL) {
            /* variable undeclared */
            //printErrorMsg( id_name,SYMBOL_UNDECLARED);
            //printf("i find you\n");
            SymbolAttribute* attr = entry->attribute;
            if(attr->attributeKind == VARIABLE_ATTRIBUTE) {                     
                if(attr->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR)
                    ldata = attr->attr.typeDescriptor->properties.dataType;
                if(attr->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR)
                    ldata = attr->attr.typeDescriptor->properties.arrayProperties.elementType;
            }
        }
    }


    /* Check right const, expr, id */
    //    printf("right->nodeType = %d identifier = %d\n", right->nodeType, IDENTIFIER_NODE);
    switch(right->nodeType) {
        case EXPR_NODE:
            processExprNode(right);
            break;
        case CONST_VALUE_NODE:
            processConstValueNode(right);
            break;
        case IDENTIFIER_NODE:
            processVariableRValue(right);
            break;
        case STMT_NODE:
            if(right->semantic_value.stmtSemanticValue.kind == FUNCTION_CALL_STMT) {
                checkFunctionCall(right->child);
                if(right->child != NULL) {
                    /* check return type? */;
                }
            }else
                printErrorMsg(assignmentNode, NOT_ASSIGNABLE);
            break;
        default:
            printErrorMsg(assignmentNode, NOT_ASSIGNABLE);
            break;
    }
}
void processExprNode(AST_NODE* exprNode){           //check element in under expr node
    if(exprNode->nodeType != EXPR_NODE) { 
        return;
    }
    AST_NODE *lvalue, *rvalue;
    switch( exprNode->semantic_value.exprSemanticValue.kind) {  //check each element in expr(whether declare...)
        case BINARY_OPERATION:
            lvalue = exprNode->child;
            rvalue = lvalue->rightSibling;
            processGeneralNode(lvalue);
            processGeneralNode(rvalue);
        case UNARY_OPERATION:
            lvalue = exprNode->child;
            processGeneralNode(lvalue);
        default:
            break;
    }
}

void checkIfStmt(AST_NODE* ifNode)
{
    printf("[In checkIfStmt]\n");
    AST_NODE *cond, *block, *remaining;
    cond = ifNode;
    block = cond->rightSibling;
    remaining = block->rightSibling;

    processGeneralNode(cond);
    openScope();
    processBlockNode(block);
    closeScope();
    if(remaining != NULL) {
        switch(remaining->nodeType) {
            case STMT_NODE:             //else if
                checkIfStmt(remaining);
            case BLOCK_NODE:            //else , beacuse struct is no the same as ifSTMT 
                openScope();
                processBlockNode(remaining);
                closeScope();
            default:
                break;
        }
    }
}

void checkWriteFunction(AST_NODE* functionCallNode)
{
    AST_NODE* param = functionCallNode->rightSibling->child;
    if(param == NULL) { printErrorMsg(functionCallNode, TOO_FEW_ARGUMENTS); return; }
    if(param->rightSibling != NULL) { 
        printErrorMsg(functionCallNode, TOO_MANY_ARGUMENTS); 
        return;
    }
    /* accept const, id */
}

void checkFunctionCall(AST_NODE* functionCallNode)
{
    printf("[In checkFunctionCall]\n");
    char* func_name = functionCallNode->semantic_value.identifierSemanticValue.identifierName;
    if( strcmp(func_name, "write")==0) {
        checkWriteFunction(functionCallNode);
        return;
    }
    //printf("my name is %s\n",func_name );
    SymbolTableEntry* func = searchSymbol( func_name,FUNCTION_SIGNATURE);       //func is the function in symboltable
    if(func == NULL) { 
        //printf("7pupu\n");
        printErrorMsg(functionCallNode, SYMBOL_UNDECLARED); 
        return; 
    }
    if(func->attribute->attributeKind != FUNCTION_SIGNATURE) { 
        return;
    }
    checkParameterPassing(func->attribute->attr.functionSignature->parameterList, functionCallNode->rightSibling);
}

void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter)
{
    printf("[In checkParameterPassing]\n");
    AST_NODE* actpara = actualParameter->child;
    while(formalParameter != NULL) {                        //no enough parameter
        if(actpara == NULL) { 
            printErrorMsg(actualParameter->leftmostSibling, TOO_FEW_ARGUMENTS);
             return; 
         }
        
        switch(formalParameter->type->kind) {
            case SCALAR_TYPE_DESCRIPTOR:
                /* actualP can be expr const id, no array */
                if(actpara->nodeType == IDENTIFIER_NODE) {
                    char* id_name = actpara->semantic_value.identifierSemanticValue.identifierName;
                    SymbolTableEntry* id = searchSymbol(id_name,VARIABLE_ATTRIBUTE);
                    if(id == NULL) { 
                        printErrorMsg(actualParameter, SYMBOL_UNDECLARED);
                    }
                    if(id->attribute->attributeKind == FUNCTION_SIGNATURE) { 
                        printErrorMsg(actualParameter, IS_FUNCTION_NOT_VARIABLE);
                    }
                    else if(id->attribute->attributeKind == TYPE_ATTRIBUTE) {
                        printErrorMsg(actualParameter, IS_TYPE_NOT_VARIABLE);
                    }
                    else if(id->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR) {
                        int para_count = 0, actual_count = id->attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
                        AST_NODE* temp = actpara->child;
                        while(temp != NULL) { 
                            temp = temp->rightSibling;
                            para_count++;
                        }
                        if(para_count < actual_count) {                                                             //array element is not a number
                            printErrorMsgSpecial(actpara, formalParameter->parameterName,  PASS_ARRAY_TO_SCALAR);
                        }
                        else if(para_count > actual_count) { 
                            printErrorMsg(actualParameter, INCOMPATIBLE_ARRAY_DIMENSION);
                        }
                    }
                }
                if(actpara->nodeType == CONST_VALUE_NODE) {}
                if(actpara->nodeType == EXPR_NODE) {}
                break;
            case ARRAY_TYPE_DESCRIPTOR:
                if(actpara->nodeType != IDENTIFIER_NODE) { 
                    printErrorMsg(actualParameter, NOT_ARRAY);
                }
                char* id_name = actpara->semantic_value.identifierSemanticValue.identifierName;
                int dim_count = formalParameter->type->properties.arrayProperties.dimension;
                int para_count = 0;
                int para_total;
                AST_NODE* temp = actpara->child;
                SymbolTableEntry* id = searchSymbol(id_name,VARIABLE_ATTRIBUTE);
                if(id == NULL) { 
                    printErrorMsg(actualParameter, SYMBOL_UNDECLARED);
                    return;
                    }
                if(id->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
                    printErrorMsgSpecial(actpara, formalParameter->parameterName, PASS_SCALAR_TO_ARRAY);
                    return;
                }
                para_total = id->attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
                while(temp != NULL) {
                    temp=temp->rightSibling;
                    para_count++;
                }

                if(dim_count != para_total-para_count) { 
                    printErrorMsg(actualParameter, INCOMPATIBLE_ARRAY_DIMENSION);
                }
                break;
            default:
                break;
        }

        formalParameter = formalParameter->next;
        actpara = actpara->rightSibling;
    }
    if(actpara != NULL){
        printErrorMsg(actualParameter->leftmostSibling, TOO_MANY_ARGUMENTS);
    }
}


void processExprRelatedNode(AST_NODE* exprRelatedNode)
{
}

void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue)
{
}

void evaluateExprValue(AST_NODE* exprNode)
{
}


int calculateExprNode(AST_NODE* exprNode)         //過程中有負數v小數沒關係
{
    //if(exprNode->nodeType == EXPR_NODE) { return; }
    printf("[In calculateExprNode]\n");
    AST_NODE *lvalue, *rvalue;
    if( exprNode->nodeType == CONST_VALUE_NODE){
        if( exprNode->semantic_value.const1->const_type == INTEGERC){
            int result = exprNode->semantic_value.const1->const_u.intval;
            return result;
        }
        else if( exprNode->semantic_value.const1->const_type == FLOATC){
            printErrorMsg(exprNode,ARRAY_SUBSCRIPT_NOT_INT);
            return -100000;                                             //??
        }
    }
    if( exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION) {
        int l = 0, r = 0;
        lvalue = exprNode->child;
        rvalue = lvalue->rightSibling;
        if( lvalue->nodeType != CONST_VALUE_NODE ){
            l = calculateExprNode(lvalue);        
        }
        if( rvalue->nodeType != CONST_VALUE_NODE ){
            r = calculateExprNode(rvalue);        
        }
        switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp){
            case BINARY_OP_ADD:
                if( l == -100000 || r == -100000){
                    return -100000;
                }
                return (l+r);
            case BINARY_OP_SUB:
                if( l == -100000 || r == -100000){
                    return -100000;
                }
                return (l-r);

            case BINARY_OP_MUL:
                if( l == -100000 || r == -100000){
                    return -100000;
                }
                return (l*r);

            case BINARY_OP_DIV:
                if( l == -100000 || r == -100000 || l%r){
                    return -100000;
                }
                return (l/r);
        }
    }
    else if( exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION){      
        lvalue = exprNode->child;
        int result = calculateExprNode(lvalue);
        switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
            case UNARY_OP_POSITIVE:
                if( result == -100000){
                    return -100000;
                }
                return (result);
            case UNARY_OP_NEGATIVE:
                if( result == -100000){
                    return -100000;
                }
                return (-result);

            case BINARY_OP_MUL:
                if( result == -100000){
                    return -100000;
                }
                return (!result);
        }
    }         
}


void processVariableLValue(AST_NODE* idNode)    //ex. id in while condition
{
    printf("[In processVariableLValue]\n");
    int lparam_count = 0;
    if(idNode->nodeType != IDENTIFIER_NODE)
        printErrorMsg(idNode, NOT_ASSIGNABLE);
    SymbolTableEntry* id = searchSymbol(idNode->semantic_value.identifierSemanticValue.identifierName, VARIABLE_ATTRIBUTE);
    if(id == NULL) {
        printErrorMsg(idNode, SYMBOL_UNDECLARED);
        //printf("back here\n");
        return;
    }
    if(id->attribute->attributeKind == TYPE_ATTRIBUTE)
        printErrorMsg(idNode, IS_TYPE_NOT_VARIABLE);
    if(id->attribute->attributeKind == FUNCTION_SIGNATURE)
        printErrorMsg(idNode, IS_FUNCTION_NOT_VARIABLE);
    if(id->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR) {
        AST_NODE *temp = idNode->child;
        while(temp != NULL) { 
            lparam_count++; 
            temp = temp->rightSibling; 
        }
        if(lparam_count < id->attribute->attr.typeDescriptor->properties.arrayProperties.dimension) //dimension wrong
            printErrorMsg(idNode, PASS_ARRAY_TO_SCALAR);
    }
}

void processVariableRValue(AST_NODE* idNode)        //check the variable ex. id in while condition
{
    printf("[In processVariableRValue]\n");
    if(idNode->nodeType != IDENTIFIER_NODE) { 
        return; 
    }
    int lparam_count = 0;
    char* id_name = idNode->semantic_value.identifierSemanticValue.identifierName;
    SymbolTableEntry* id = searchSymbol(id_name, VARIABLE_ATTRIBUTE);

    if(id == NULL) {
        printErrorMsg(idNode, SYMBOL_UNDECLARED);
        return;
    }
    /* Check not type, ID and function OK */
    if(id->attribute->attributeKind == TYPE_ATTRIBUTE){
        printErrorMsg(idNode, IS_TYPE_NOT_VARIABLE);
    }

    /* Check ID and array, function element */
    
    if(id->attribute->attributeKind == VARIABLE_ATTRIBUTE) {
        printf("why not here2\n");
        printf("this is %s\n", id->name);
        printf(" kind  = %d",id->attribute->attr.typeDescriptor->kind);
        printSymbolTable();
        if(id->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR) {
            printf("why not here\n");
            AST_NODE *temp = idNode->child;
            while(temp != NULL) { 
                lparam_count++;
                temp = temp->rightSibling;
            }
            if(lparam_count < id->attribute->attr.typeDescriptor->properties.arrayProperties.dimension)
                printErrorMsg(idNode, NOT_ASSIGNABLE);
        }
    }
    if(id->attribute->attributeKind == FUNCTION_SIGNATURE)
        checkFunctionCall(idNode);
}


void processConstValueNode(AST_NODE* constValueNode)    //check const(do nothing)
{
    if(constValueNode->nodeType != CONST_VALUE_NODE) { 
        return; 
    }
}


void checkReturnStmt(AST_NODE* returnNode)
{
    printf("[In checkReturnStmt]\n");
    AST_NODE* temp = returnNode->parent;
    while(temp->semantic_value.declSemanticValue.kind != FUNCTION_DECL){
        temp = temp->parent;
    }
    switch( returnNode->nodeType){
        case EXPR_NODE:
            processExprNode(returnNode);    //check element in expr
            break;
        case CONST_VALUE_NODE:
            printf("right place\n");
            if( returnNode->semantic_value.const1->const_type == INTEGERC){
                printf("here is the problem\n");
                if( strcmp(temp->child->semantic_value.identifierSemanticValue.identifierName, "int") )
                                printErrorMsg(returnNode, RETURN_TYPE_UNMATCH);
            }
            else if( returnNode->semantic_value.const1->const_type == FLOATC){
               if( strcmp(temp->child->semantic_value.identifierSemanticValue.identifierName, "float") )
                    printErrorMsg(returnNode, RETURN_TYPE_UNMATCH);
            }
            break;
        case NUL_NODE:
            if( temp->child->dataType != VOID_TYPE){
                printErrorMsg(returnNode, RETURN_TYPE_UNMATCH);
            }    
            return;
        case IDENTIFIER_NODE:
            break;
        default:
            return;        
    }
}


void processBlockNode(AST_NODE* blockNode)
{
    printf("[In processBlockNode]\n");
    AST_NODE* now = blockNode->child;
    while(now != NULL) {                    // check declaration => stmt
        AST_NODE* child = now->child;
        switch(now->nodeType) {
            case VARIABLE_DECL_LIST_NODE:
                while(child != NULL) {
                    processDeclarationNode(child);
                    child = child->rightSibling;
                }
                break;
            case STMT_LIST_NODE:
                while(child != NULL) {
                    processStmtNode(child);
                    child = child->rightSibling;
                }
                break;
            default:
                break;
        }
        now = now->rightSibling;
    }
}


void processStmtNode(AST_NODE* stmtNode)
{
    printf("[In processStmtNode]\n");
    if(stmtNode->nodeType != STMT_NODE) { /* Error */ return; }
    switch(stmtNode->semantic_value.stmtSemanticValue.kind) {    //check the stm type
        case WHILE_STMT:
            checkWhileStmt(stmtNode->child);
            break;
        case FOR_STMT:
            checkForStmt(stmtNode->child);
            break;
        case ASSIGN_STMT:
            checkAssignmentStmt(stmtNode->child);
            break;
        case IF_STMT:
            checkIfStmt(stmtNode->child);
            break;
        case FUNCTION_CALL_STMT:
            checkFunctionCall(stmtNode->child);
            break;
        case RETURN_STMT:
            checkReturnStmt(stmtNode->child);
            break;
        default:
            break;
    }
}


void processGeneralNode(AST_NODE *node)
{
    printf("[In processGeneralNode]\n");
    switch(node->nodeType) {
        case IDENTIFIER_NODE:
            processVariableRValue(node);
            break;
        case CONST_VALUE_NODE:
            processConstValueNode(node);
            break;
        case EXPR_NODE:
            processExprNode(node);
            break;
        case NUL_NODE:
            break;
        default:
            break;
    }
}

void processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{
    printf("[In processDeclDimList]\n");
    int dim = 0;
    AST_NODE* dim_num = idNode->child;

    // ignoreFirstDimsize
    if( ignoreFirstDimSize && dim_num->nodeType == NUL_NODE){
        dim_num = dim_num->rightSibling;
    }
    while( dim_num != NULL){              //check the value
        if( dim_num->nodeType == CONST_VALUE_NODE){         
            if(dim_num->semantic_value.const1->const_type == INTEGERC) {
                if(dim_num->semantic_value.const1->const_u.intval > 0)
                    typeDescriptor->properties.arrayProperties.sizeInEachDimension[dim] = dim_num->semantic_value.const1->const_u.intval;
                else
                    printErrorMsg(idNode, ARRAY_SIZE_NEGATIVE);
            } 
            else{
                printErrorMsg(idNode, ARRAY_SIZE_NOT_INT);
            }
        } else if(dim_num->nodeType == EXPR_NODE) {
            /*TODO*/ /* processExprNode*/
            int num = calculateExprNode(dim_num);
            if( num == -100000){
                return;    
            }
            typeDescriptor->properties.arrayProperties.sizeInEachDimension[dim] = num;  
        } 
        else {
            printErrorMsg(idNode, INCOMPATIBLE_ARRAY_DIMENSION);
        }
        dim++;                                                  
        dim_num = dim_num->rightSibling;
    }
    typeDescriptor->properties.arrayProperties.dimension = dim;

}

void declareFunction(AST_NODE* declarationNode)
{
    printf("[In declareFunction]\n");
    AST_NODE *type, *func_name, *parameter_list, *block;             //func element
    type = declarationNode;
    func_name = type->rightSibling;
    parameter_list = func_name->rightSibling;
    block = parameter_list->rightSibling;

    int para_num = 0;
    DATA_TYPE data_type;
    SymbolAttribute* attr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
    FunctionSignature* func_sig = malloc(sizeof(FunctionSignature));
    char* f_name = func_name->semantic_value.identifierSemanticValue.identifierName;
    char* name = type->semantic_value.identifierSemanticValue.identifierName;

    //add function name to symbol table
    if( !strcmp(name, "int")){ 
        data_type = INT_TYPE;
    }
    else if( !strcmp(name, "float")){
        data_type = FLOAT_TYPE; 
    }
    attr->attributeKind = FUNCTION_SIGNATURE;
    attr->attr.functionSignature = func_sig;
    func_sig->returnType = data_type;
    func_sig->parameterList = NULL;
    func_name->semantic_value.identifierSemanticValue.symbolTableEntry = enterSymbol( f_name, attr); 
    //printf("this func is %s\n", f_name);
    if( func_name->semantic_value.identifierSemanticValue.symbolTableEntry == NULL){
        printErrorMsg(func_name,SYMBOL_REDECLARE);
    }

    openScope();

    /* Parameter parsing */
    if( parameter_list->nodeType == PARAM_LIST_NODE) {
        //printf("i am parameter xDD\n");
        AST_NODE* para = parameter_list->child;
        Parameter* prev_para = NULL;
        while(para != NULL) {
            Parameter* param = malloc(sizeof(Parameter));
            //printf("GGG\n");
            processDeclarationNode(para);
            param->parameterName = para->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
            param->type = para->child->rightSibling->semantic_value.identifierSemanticValue.symbolTableEntry->attribute->attr.typeDescriptor;
            if( func_sig->parameterList == NULL)
                func_sig->parameterList = param;
            else
                prev_para->next = param;
            prev_para = param;
            para = para->rightSibling;
            para_num++;
        }
    }else { /* Parse Tree Error: no param? */ }
    func_sig->parametersCount = para_num;

    /* Block Node Parsing */
    processBlockNode(block);

    closeScope();

}



