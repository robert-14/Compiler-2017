#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"
#include "symbolTable.h"
#include "codeGen.h"
#include <stdio.h>
#include <stdlib.h>

#define FRAME_SIZE 172
int const_label = 0;
int branch_label = 0;
int ARoffset = -4;
int reg_list[32] = {};
int freg_list[32] = {};
FILE *output;

int get_b_label(){
	branch_label += 1;
	return branch_label;
}

int get_c_label(){
	const_label += 1;
	return const_label;
}

void gen_prologue(char *name){
	printf("gen_prologue %s\n", name);
	fprintf(output, "#function %s prologue\n", name);

	fprintf(output, "str x30, [sp, #0]\n");
	fprintf(output, "str x29, [sp, #-8]\n");
	fprintf(output, "add x29, sp, #-8\n");
	fprintf(output, "add sp, sp, #-16\n");
	fprintf(output, "ldr x30, =_frameSize_%s\n", name);
	fprintf(output, "ldr w30, [x30, #0]\n");
	fprintf(output, "sub sp, sp, w30\n");
	int offset = 8;
	for(int i = 9; i <= 15; i++){
		fprintf(output, "str x%d, [sp, #%d]\n", i, offset);
		offset += 8;
	}
	for(int i = 16; i <= 23; i++){
		fprintf(output, "str s%d, [sp, #%d]\n", i, offset);
		offset += 4;
	}
	for(int i = 19; i <= 28; i++){
		fprintf(output, "str x%d, [sp, #%d]\n", i, offset);
		offset += 8;
	}
	fprintf(output, "#prologue end\n");

}

void gen_epilogue(char *name){
	printf("gen_epilogue %s\n", name);
	fprintf(output, "#function %s epilogue\n", name);

	fprintf(output, "_end_%s:\n", name);
	int offset = 8;
	for(int i = 9; i <= 15; i++){
		fprintf(output, "ldr x%d, [sp, #%d]\n", i, offset);
		offset += 8;
	}
	for(int i = 16; i <= 23; i++){
		fprintf(output, "ldr s%d, [sp, #%d]\n", i, offset);
		offset += 4;
	}
	for(int i = 19; i <= 28; i++){
		fprintf(output, "ldr x%d, [sp, #%d]\n", i, offset);
		offset += 8;
	}
	fprintf(output, "ldr x30, [x29, #8]\n");
	fprintf(output, "mov sp, x29\n");
	fprintf(output, "add sp, sp, #8\n");
	fprintf(output, "ldr x29, [x29, #0]\n");
	fprintf(output, "RET x30\n");
	fprintf(output, ".data\n");
	fprintf(output, "_frameSize_%s: .word %d\n", name, 172 - ARoffset);
	fprintf(output, ".text\n");
	fprintf(output, "#epilogue end\n");

}

int get_reg(){
	// caller
	for(int i = 9; i <= 15; i++){
		if(reg_list[i] == 0){
			reg_list[i] = 1;
			return i;
		}
	}
	// callee
	for(int i = 19; i <= 28; i++){
		if(reg_list[i] == 0){
			reg_list[i] = 1;
			return i;
		}
	}
	return -1;
}

int get_freg(){
	for(int i = 16; i <= 23; i++){
		if(freg_list[i] == 0){
			freg_list[i] = 1;
			return i+16;
		}
	}
	return -1;
}

void free_reg(int reg, AST_NODE* node){
	if(reg >= 32){
		freg_list[reg-16] = 0;
	}else{
		reg_list[reg] = 0;
	}
	if(node != NULL){
		node->place = 0;
	}
}

void gen_head(char *name){
	printf("gen_head\n");
	fprintf(output, ".text\n");
	fprintf(output, "_start_%s:\n", name);
}

void generateCode(AST_NODE *programNode){
	printf("start\n");
	output = fopen("output.s", "w+");
	AST_NODE *child = programNode->child;
	while(child != NULL){
		if(child->nodeType == DECLARATION_NODE){
			genDeclarationNode(child);
		}else if(child->nodeType == VARIABLE_DECL_LIST_NODE){
			AST_NODE* grand_child = child->child;
			while(grand_child != NULL){
				genDeclarationNode(grand_child);
				grand_child = grand_child->rightSibling;
			}
		}
		child = child->rightSibling;
	}
	fclose(output);
}

void genDeclarationNode(AST_NODE* declarationNode){
	printf("genDeclarationNode\n");
	if(declarationNode->semantic_value.declSemanticValue.kind == VARIABLE_DECL){
		genDeclareIdList(declarationNode);
	}else if(declarationNode->semantic_value.declSemanticValue.kind == FUNCTION_DECL){
		genFunctionDeclaration(declarationNode);
	}else{ //TYPE_DECL
		// no need to handle
	}
}

void genDeclareIdList(AST_NODE* declarationNode){
	printf("genDeclareIdList\n");
	AST_NODE* id_list = declarationNode->child->rightSibling;
	//Id
	while(id_list != NULL){
		if(id_list->semantic_value.identifierSemanticValue.kind == NORMAL_ID){
			SymbolTableEntry* entry = id_list->semantic_value.identifierSemanticValue.symbolTableEntry;
			if(entry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR){
				/*
				special case:
					typedef int X[10];
					X a;
					a -> NORMAL_ID
					but a is an array
					so we need to check SymbolTableEntry
				*/
				genArrayDecl(id_list);
			}else{
				genScalarDecl(id_list, 0);
			}
		}else if(id_list->semantic_value.identifierSemanticValue.kind == ARRAY_ID){
			genArrayDecl(id_list);
		}else{ //WITH_INIT_ID
			//hw6
			genScalarDecl(id_list, 1);
		}
		id_list = id_list->rightSibling;
	}
}

void genScalarDecl(AST_NODE* idNode, int isInit){
	printf("genScalarDecl\n");
	SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
	if(entry->nestingLevel == 0){
		// global variable
		fprintf(output, ".data\n");
		if(entry->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
			// hw6 init
			if(isInit == 1){
				int initValue = idNode->child->semantic_value.const1->const_u.intval;
				fprintf(output, "_g_%s: .word %d\n", entry->name, initValue);
			}else{
				fprintf(output, "_g_%s: .word 0\n", entry->name);
			}
		}else if(entry->attribute->attr.typeDescriptor->properties.dataType == FLOAT_TYPE){
			// hw6 init
			if(isInit == 1){
				float initValue = idNode->child->semantic_value.const1->const_u.fval;
				fprintf(output, "_g_%s: .float %f\n", entry->name, initValue);
			}else{
				fprintf(output, "_g_%s: .float 0.0\n", entry->name);
			}
		}else{printf("error\n");}
		fprintf(output, ".text\n");
	}else{
		// local variable
		entry->offset = ARoffset;
		ARoffset -= 4;
		// hw6 init
		if(isInit == 1){
			if(entry->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
				int initValue = idNode->child->semantic_value.const1->const_u.intval;
				int tmp = get_reg();
				fprintf(output, "mov w%d, #%d\n", tmp, initValue);
				fprintf(output, "str w%d, [x29, #%d]\n", tmp, entry->offset);
				free_reg(tmp, NULL);
			}else if(entry->attribute->attr.typeDescriptor->properties.dataType == FLOAT_TYPE){
				float initValue = idNode->child->semantic_value.const1->const_u.fval;
				int tmp = get_freg();
				fprintf(output, "fmov s%d, #%f\n", tmp-16, initValue);
				fprintf(output, "str s%d, [x29, #%d]\n", tmp-16, entry->offset);
				free_reg(tmp, NULL);
			}else{printf("error\n");}
		}
	}
}

void genArrayDecl(AST_NODE* idNode){
	printf("genArrayDecl\n");
	SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
	ArrayProperties* prop = &entry->attribute->attr.typeDescriptor->properties.arrayProperties;
	int total = 4;
	for(int i = 0; i < prop->dimension; i++){
		total *= prop->sizeInEachDimension[i];
	}
	if(entry->nestingLevel == 0){
		// global variable
		fprintf(output, ".data\n");
		fprintf(output, "_g_%s: .space %d\n", entry->name, total);
		fprintf(output, ".text\n");
	}else{
		// local variable
		ARoffset -= (total-4);
		entry->offset = ARoffset;
		ARoffset -= 4;
	}
}

void AssignOrExpr(AST_NODE* assignOrExprRelatedNode){
	printf("AssignOrExpr\n");
	if(assignOrExprRelatedNode->semantic_value.stmtSemanticValue.kind == ASSIGN_STMT){
		genAssignmentStmt(assignOrExprRelatedNode);
	}else{
		genExprNode(assignOrExprRelatedNode);
	}
	
}

void processExprList(AST_NODE* exprList){
	printf("processExprList\n");
	if(exprList->nodeType == NUL_NODE){
		return;
	}else if(exprList->nodeType == NONEMPTY_ASSIGN_EXPR_LIST_NODE){
		AST_NODE* expr = exprList->child;
		while(expr != NULL){
			AssignOrExpr(expr);
			expr = expr->rightSibling;
		}
	}else if(exprList->nodeType == NONEMPTY_RELOP_EXPR_LIST_NODE){
		AST_NODE* expr = exprList->child;
		while(expr != NULL){
			genExprNode(expr);
			expr = expr->rightSibling;
		}
	}else{ printf("error\n"); }
}

void genWhileStmt(AST_NODE* whileNode){
	printf("genWhileStmt\n");
	AST_NODE* test = whileNode->child;
	AST_NODE* Stmt = test->rightSibling;
	int label = get_b_label();
	fprintf(output, "_whileTest_%d:\n", label);
	AssignOrExpr(test);
	if(test->place == 0){
		// assignment
		genIdValue(test);
	}
	if(test->place >= 32){
		fprintf(output, "fcmp s%d, #0\n", test->place-16);
	}else{
		fprintf(output, "cmp w%d, #0\n", test->place);
	}
	free_reg(test->place, test);
	fprintf(output, "beq _whileExit_%d\n", label);
	genStmtNode(Stmt);
	fprintf(output, "b _whileTest_%d\n", label);
	fprintf(output, "_whileExit_%d:\n", label);
}

void genForStmt(AST_NODE* forNode){
	// hw6 
	printf("genForStmt\n");
	AST_NODE* assignList = forNode->child;
	AST_NODE* testList = assignList->rightSibling;
	AST_NODE* assignList2 = testList->rightSibling;
	AST_NODE* Stmt = assignList2->rightSibling;
	int label = get_b_label();
	// init list
	assignList = assignList->child;
	while(assignList != NULL){
		if(assignList->nodeType == NUL_NODE) break;
		genAssignmentStmt(assignList);
		assignList = assignList->rightSibling;
	}
	// condition
	fprintf(output, "_forTest_%d:\n", label);
	testList = testList->child; // skip NONEMPTY_RELOP_EXPR_LIST_NODE
	printf("test\n");	
	while(testList != NULL){
		if(testList->nodeType == NUL_NODE) break;
		genExprNode(testList);
		if(testList->place == 0){
			// assignment
			genIdValue(testList);
		}
		if(testList->place >= 32){
			fprintf(output, "fcmp s%d, #0\n", testList->place-16);
		}else{
			fprintf(output, "cmp w%d, #0\n", testList->place);
		}
		free_reg(testList->place, testList);
		fprintf(output, "beq _forExit_%d\n", label);
		testList = testList->rightSibling;
	}
	fprintf(output, "b _forStmt_%d\n", label);
	// assignment list at the end of each loop
	fprintf(output, "_forExpr_%d:\n", label);
	assignList2 = assignList2->child;
	while(assignList2 != NULL){
		if(assignList2->nodeType == NUL_NODE) break;
		genAssignmentStmt(assignList2);
		assignList2 = assignList2->rightSibling;
	}
	fprintf(output, "b _forTest_%d\n", label);
	// body
	fprintf(output, "_forStmt_%d:\n", label);
	genStmtNode(Stmt);
	fprintf(output, "b _forExpr_%d\n", label);
	// exit
	fprintf(output, "_forExit_%d:\n", label);
}

void genIfStmt(AST_NODE* ifNode){
	printf("genIfStmt\n");
	AST_NODE* test = ifNode->child;
	AST_NODE* stmt1 = test->rightSibling;
	AST_NODE* stmt2 = stmt1->rightSibling;
	AssignOrExpr(test);
	int label = get_b_label();
	if(test->place == 0){
		// assignment
		genIdValue(test);
	}
	if(test->place >= 32){
		fprintf(output, "fcmp s%d, #0\n", test->place-16);
	}else{
		fprintf(output, "cmp w%d, #0\n", test->place);
	}
	free_reg(test->place, test);
	fprintf(output, "beq _elseLabel_%d\n", label);
	genStmtNode(stmt1);
	fprintf(output, "b _ifExitLabel_%d\n", label);
	fprintf(output, "_elseLabel_%d:\n", label);
	if(stmt2->nodeType != NUL_NODE){
		genStmtNode(stmt2);
	}
	fprintf(output, "_ifExitLabel_%d:\n", label);
}

void genAssignmentStmt(AST_NODE* assignmentNode){
	printf("genAssignmentStmt\n");
	fprintf(output, "#assignment\n");
	AST_NODE* left = assignmentNode->child;
	AST_NODE* right = left->rightSibling;
	SymbolTableEntry* entryL = left->semantic_value.identifierSemanticValue.symbolTableEntry;
	printf("=============%s\n", entryL->name);
	printf("=============%d\n", entryL->attribute->attr.typeDescriptor->properties.dataType);
	// implicit Type conversion (hw6)
	genExprNode(right);
	
	int is_global = (entryL->nestingLevel == 0);
	if(!is_global){
		if(entryL->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
			int left_type = entryL->attribute->attr.typeDescriptor->properties.dataType;
			if(left_type == INT_TYPE){
				if(right->dataType == FLOAT_TYPE){
					int tmp = get_reg();
					fprintf(output, "fcvtzs w%d, s%d\n", tmp, right->place-16);
					free_reg(right->place, right);
					right->place = tmp;
				}
				fprintf(output, "str w%d, [x29, #%d]\n", right->place, entryL->offset);
			}else{
				if(right->dataType == INT_TYPE){
					int tmp = get_freg();
					fprintf(output, "scvtf s%d, w%d\n", tmp-16, right->place);
					free_reg(right->place, right);
					right->place = tmp;
				}
				fprintf(output, "str s%d, [x29, #%d]\n", right->place-16, entryL->offset);				
			}
		}else{ //ARRAY_TYPE_DESCRIPTOR
			int size_reg = genDimNode(left);
			//TODO int float
			if(left->dataType == INT_TYPE){
				int tmp = get_reg();
				fprintf(output, "mov w%d, #%d\n", tmp, entryL->offset);
				fprintf(output, "add w%d, w%d, w%d\n", size_reg, size_reg, tmp);
				fprintf(output, "mov w%d, w%d\n", left->place, right->place);
				fprintf(output, "str w%d, [x29, x%d]\n", left->place, size_reg);
				free_reg(tmp, NULL);
			}else if(left->dataType == FLOAT_TYPE){
				int tmp = get_reg();
				fprintf(output, "mov w%d, #%d\n", tmp, entryL->offset);
				fprintf(output, "add w%d, w%d, w%d\n", size_reg, size_reg, tmp);
				fprintf(output, "fmov s%d, s%d\n", left->place-16, right->place-16);
				fprintf(output, "str s%d, [x29, x%d]\n", left->place-16, size_reg);
				free_reg(tmp, NULL);
			}else{printf("error\n");}
			free_reg(size_reg, NULL);
		}
	}else{
		if(entryL->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
			int left_type = entryL->attribute->attr.typeDescriptor->properties.dataType;
			int tmp = get_reg();
			fprintf(output, "ldr x%d, =_g_%s\n", tmp, entryL->name);
			if(left_type == INT_TYPE){
				if(right->dataType == FLOAT_TYPE){
					int tmp = get_reg();
					fprintf(output, "fcvtzs w%d, s%d\n", tmp, right->place-16);
					free_reg(right->place, right);
					right->place = tmp;
				}
				fprintf(output, "str w%d, [x%d, #0]\n", right->place, tmp);
			}else{
				if(right->dataType == INT_TYPE){
					int tmp = get_freg();
					fprintf(output, "scvtf s%d, w%d\n", tmp-16, right->place);
					free_reg(right->place, right);
					right->place = tmp;
				}
				fprintf(output, "str s%d, [x%d, #0]\n", right->place-16, tmp);			
			}
			free_reg(tmp, NULL);
		}else{ //ARRAY_TYPE_DESCRIPTOR
			//TODO int float
			int tmp = get_reg();
			fprintf(output, "ldr x%d, =_g_%s\n", tmp, entryL->name);
			int size_reg = genDimNode(left);
			if(left->dataType == INT_TYPE){
				fprintf(output, "mov w%d, w%d\n", left->place, right->place);
				fprintf(output, "str w%d, [x%d, x%d]\n", left->place, tmp, size_reg);
			}else if(left->dataType == FLOAT_TYPE){
				fprintf(output, "fmov s%d, s%d\n", left->place-16, right->place-16);
				fprintf(output, "str s%d, [x%d, x%d]\n", left->place-16, tmp, size_reg);
			}else{printf("error\n");}
			free_reg(size_reg, NULL);
			free_reg(tmp, NULL);
		}
	}
	free_reg(left->place, left);
	free_reg(right->place, right);
}

void genExprNode(AST_NODE* exprNode){
	printf("genExprNode\n");
	if(exprNode->nodeType == CONST_VALUE_NODE){
		// constant
		genConstValue(exprNode);
	}else if(exprNode->nodeType == STMT_NODE){
		// function call
		genFunctionCall(exprNode);
	}else if(exprNode->nodeType == IDENTIFIER_NODE){
		// id
		genIdValue(exprNode);
	}else if(exprNode->nodeType == EXPR_NODE){
		// operator
		if(exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION){
			AST_NODE* left = exprNode->child;
			AST_NODE* right = left->rightSibling;
			genExprNode(left);
			genExprNode(right);
			if(left->dataType == INT_TYPE && right->dataType == INT_TYPE){
				exprNode->dataType = INT_TYPE;
				exprNode->place = get_reg();
				switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp){
					case(BINARY_OP_ADD):
						fprintf(output, "add w%d, w%d, w%d\n", exprNode->place, left->place, right->place);
					break;
					case(BINARY_OP_SUB):
						fprintf(output, "sub w%d, w%d, w%d\n", exprNode->place, left->place, right->place);
					break;
					case(BINARY_OP_MUL):
						fprintf(output, "mul w%d, w%d, w%d\n", exprNode->place, left->place, right->place);
					break;
					case(BINARY_OP_DIV):
						fprintf(output, "sdiv w%d, w%d, w%d\n", exprNode->place, left->place, right->place);
					break;
					case(BINARY_OP_EQ):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, eq\n", exprNode->place);
					break;
					case(BINARY_OP_GE):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, ge\n", exprNode->place);
					break;
					case(BINARY_OP_LE):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, le\n", exprNode->place);
					break;
					case(BINARY_OP_NE):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, ne\n", exprNode->place);
					break;
					case(BINARY_OP_GT):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, gt\n", exprNode->place);
					break;
					case(BINARY_OP_LT):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, lt\n", exprNode->place);
					break;
					case(BINARY_OP_AND):
						fprintf(output, "and w%d, w%d, w%d\n", exprNode->place, left->place, right->place);
					break;
					case(BINARY_OP_OR):
						fprintf(output, "orr w%d, w%d, w%d\n", exprNode->place, left->place, right->place);
					break;
				}
				free_reg(left->place, left);
				free_reg(right->place, right);
			}else{ //type conversion (float)
				// conversion (hw6)
				if(left->place < 32){
					int tmp = get_freg();
					fprintf(output, "scvtf s%d, w%d\n", tmp-16, left->place);
					free_reg(left->place, left);
					left->place = tmp;
				}
				if(right->place < 32){
					int tmp = get_freg();
					fprintf(output, "scvtf s%d, w%d\n", tmp-16, right->place);
					free_reg(right->place, right);
					right->place = tmp;
				}
				switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp){
					case(BINARY_OP_ADD):
						exprNode->dataType = FLOAT_TYPE;
						exprNode->place = get_freg();
						fprintf(output, "fadd s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_SUB):
						exprNode->dataType = FLOAT_TYPE;
						exprNode->place = get_freg();
						fprintf(output, "fsub s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_MUL):
						exprNode->dataType = FLOAT_TYPE;
						exprNode->place = get_freg();
						fprintf(output, "fmul s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_DIV):
						exprNode->dataType = FLOAT_TYPE;
						exprNode->place = get_freg();
						fprintf(output, "fdiv s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_EQ):
						exprNode->dataType = INT_TYPE;
						exprNode->place = get_reg();
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset w%d, eq\n", exprNode->place);
					break;
					case(BINARY_OP_GE):
						exprNode->dataType = INT_TYPE;
						exprNode->place = get_reg();
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset w%d, ge\n", exprNode->place);
					break;
					case(BINARY_OP_LE):
						exprNode->dataType = INT_TYPE;
						exprNode->place = get_reg();
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset w%d, le\n", exprNode->place);
					break;
					case(BINARY_OP_NE):
						exprNode->dataType = INT_TYPE;
						exprNode->place = get_reg();
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset w%d, ne\n", exprNode->place);
					break;
					case(BINARY_OP_GT):
						exprNode->dataType = INT_TYPE;
						exprNode->place = get_reg();
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset w%d, gt\n", exprNode->place);
					break;
					case(BINARY_OP_LT):
						exprNode->dataType = INT_TYPE;
						exprNode->place = get_reg();
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset w%d, lt\n", exprNode->place);
					break;
					case(BINARY_OP_AND):
						exprNode->dataType = FLOAT_TYPE;
						exprNode->place = get_freg();
						fprintf(output, "and s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_OR):
						exprNode->dataType = FLOAT_TYPE;
						exprNode->place = get_freg();
						fprintf(output, "orr s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
				}
				free_reg(left->place, left);
				free_reg(right->place, right);
			}
		}else{ //UNARY_OPERATION
			AST_NODE* child = exprNode->child;
			genExprNode(child);
			if(child->dataType == INT_TYPE){
				exprNode->dataType = INT_TYPE;
				switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
					case(UNARY_OP_POSITIVE):
						exprNode->place = child->place;
					break;
					case(UNARY_OP_NEGATIVE):
						exprNode->place = child->place;
						fprintf(output, "neg w%d, w%d\n", child->place, child->place);
					break;
					case(UNARY_OP_LOGICAL_NEGATION):
						exprNode->place = child->place;
						//0 -> 1, x(!=0) -> 0
						fprintf(output, "cmp w%d, #0\n", child->place);
						fprintf(output, "cset w%d, eq\n", child->place);
					break;
				}
				if(child->place != exprNode->place){
					free_reg(child->place, child);
				}
			}else{ //FLOAT_TYPE
				exprNode->dataType = FLOAT_TYPE;
				switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
					case(UNARY_OP_POSITIVE):
						exprNode->place = child->place;
					break;
					case(UNARY_OP_NEGATIVE):
						exprNode->place = child->place;
						fprintf(output, "fneg s%d, s%d\n", child->place-16, child->place-16);
					break;
					case(UNARY_OP_LOGICAL_NEGATION):
						exprNode->place = get_reg();
						//0 -> 1, x(!=0) -> 0
						exprNode->dataType = INT_TYPE;
						fprintf(output, "fcmp s%d, #0\n", child->place-16);
						fprintf(output, "cset w%d, eq\n", exprNode->place);
						free_reg(child->place, child);
					break;
				}
				if(child->place != exprNode->place){
					free_reg(child->place, child);
				}
			}
		}
	}else{printf("error\n");}
}

void genConstValue(AST_NODE* constNode){
	printf("genConstValue\n");
	CON_Type* val = constNode->semantic_value.const1;
	int label = get_c_label();
	if(val->const_type == INTEGERC){
		fprintf(output, ".data\n");
		fprintf(output, "_CONSTANT_%d: .word %d\n", label, val->const_u.intval);
		fprintf(output, ".align 3\n");
		fprintf(output, ".text\n");
		constNode->place = get_reg();
		fprintf(output, "ldr w%d, _CONSTANT_%d\n", constNode->place, label);
	}else if(val->const_type == FLOATC){
		fprintf(output, ".data\n");
		fprintf(output, "_CONSTANT_%d: .float %f\n", label, val->const_u.fval);
		fprintf(output, ".align 3\n");
		fprintf(output, ".text\n");
		constNode->place = get_freg();
		fprintf(output, "ldr s%d, _CONSTANT_%d\n", constNode->place-16, label);
	}else{ // STRINGC
		fprintf(output, ".data\n");
		//printf("%d\n", strlen(val->const_u.sc));
		val->const_u.sc[strlen(val->const_u.sc)-1] = '\0';
		fprintf(output, "_CONSTANT_%d: .ascii %s\\000\"\n", label, val->const_u.sc);
		fprintf(output, ".align 3\n");
		fprintf(output, ".text\n");
		constNode->place = get_reg();
		int tmp = get_reg();
		fprintf(output, "ldr x%d, =_CONSTANT_%d\n", constNode->place, label);
		free_reg(tmp, NULL);
	}
}

void writeCall(AST_NODE* writeNode){
	printf("writeCall\n");
	AST_NODE* param = writeNode->child->rightSibling->child;
	if(param->dataType == INT_TYPE){
		genExprNode(param);
		fprintf(output, "mov w0, w%d\n", param->place);
		fprintf(output,"bl _write_int\n");
	}else if(param->dataType == FLOAT_TYPE){
		genExprNode(param);
		fprintf(output, "fmov s0, s%d\n", param->place-16);
		fprintf(output,"bl _write_float\n");
	}else if(param->dataType == CONST_STRING_TYPE){
		genExprNode(param);
		fprintf(output, "mov x0, x%d\n", param->place);
		fprintf(output,"bl _write_str\n");
	}else{printf("error\n");}
	writeNode->place = param->place;
	param->place = 0;
}

void readCall(AST_NODE* readNode){
	printf("readCall\n");
	fprintf(output, "bl _read_int\n");
	readNode->place = get_reg();
	fprintf(output, "mov w%d, w0\n", readNode->place);
}

void freadCall(AST_NODE* freadNode){
	printf("freadCall\n");
	fprintf(output, "bl _read_float\n");
	freadNode->place = get_freg();
	fprintf(output, "fmov s%d, s0\n", freadNode->place-16);
}

void genFunctionCall(AST_NODE* functionCallNode){
	printf("genFunctionCall\n");
	AST_NODE* Id = functionCallNode->child;
	AST_NODE* param = Id->rightSibling;
	SymbolTableEntry* entry = Id->semantic_value.identifierSemanticValue.symbolTableEntry;
	char *name = Id->semantic_value.identifierSemanticValue.identifierName;
	if(strcmp("read", name) == 0){
		readCall(functionCallNode);
		return;
	}else if(strcmp("write", name) == 0){
		writeCall(functionCallNode);
		return;
	}else if(strcmp("fread", name) == 0){
		freadCall(functionCallNode);
		return;
	}
	//param hw6
	int param_count = entry->attribute->attr.functionSignature->parametersCount;
	int param_type[param_count];
	Parameter* param_list = entry->attribute->attr.functionSignature->parameterList;
	// reverse order
	for(int i = param_count-1; i >= 0; i--){
		param_type[i] = param_list->type->properties.dataType;
		param_list = param_list->next;
	}
	param = param->child; // skip NONEMPTY_RELOP_EXPR_LIST_NODE
	int arg_reg = 1;
	while(param != NULL){
		if(param->nodeType == NUL_NODE) break;
		genExprNode(param);
		if(param->place == 0){
			// assignment
			genIdValue(param);
		}
		if(param->place >= 32){ //float
			if(param_type[arg_reg-1] == INT_TYPE){
				int tmp = get_reg();
				fprintf(output, "fcvtzs w%d, s%d\n", tmp, param->place-16);
				free_reg(param->place, param);
				param->place = tmp;
				fprintf(output, "mov w%d, w%d\n", arg_reg, param->place);
			}else{
				fprintf(output, "fmov w%d, s%d\n", arg_reg, param->place-16); // fmov s0, s%d can't work
			}
		}else{ //int
			if(param_type[arg_reg-1] == INT_TYPE){
				fprintf(output, "mov w%d, w%d\n", arg_reg, param->place);
			}else{
				int tmp = get_freg();
				fprintf(output, "scvtf s%d, w%d\n", tmp-16, param->place);
				free_reg(param->place, param);
				param->place = tmp;
				fprintf(output, "fmov w%d, s%d\n", arg_reg, param->place-16); // fmov s0, s%d can't work
			}
		}
		arg_reg += 1;
		free_reg(param->place, param);
		param = param->rightSibling;
	}
	fprintf(output, "bl _start_%s\n", name);
	// return value
	if(entry->attribute->attr.functionSignature->returnType == FLOAT_TYPE){
		functionCallNode->place = get_freg();
		fprintf(output, "fmov s%d, w0\n", functionCallNode->place-16);
	}else if(entry->attribute->attr.functionSignature->returnType == INT_TYPE){
		functionCallNode->place = get_reg();
		fprintf(output, "mov w%d, w0\n", functionCallNode->place);
	}else if(entry->attribute->attr.functionSignature->returnType == VOID_TYPE){
	}else{printf("error\n");printf("%d\n", entry->attribute->attr.functionSignature->returnType);}
}

void genIdValue(AST_NODE* IdNode){
	printf("genIdValue\n");
	SymbolTableEntry* entry = IdNode->semantic_value.identifierSemanticValue.symbolTableEntry;
	char* name = IdNode->semantic_value.identifierSemanticValue.identifierName;
	int is_global = (entry->nestingLevel == 0);
	if(IdNode->semantic_value.identifierSemanticValue.kind == NORMAL_ID){
		if(entry->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){ //***check entry instead of node(type conversion)(hw6)
			if(IdNode->place == 0){
				IdNode->place = get_reg();
			}
			if(IdNode->place == -1){
				//TODO: ???
			}else{
				if(is_global){
					int tmp = get_reg();
					fprintf(output, "ldr x%d, =_g_%s\n", tmp, name);
					fprintf(output, "ldr w%d, [x%d,#0]\n", IdNode->place, tmp);
					free_reg(tmp, NULL);
				}else{
					fprintf(output, "ldr w%d, [x29,#%d]\n", IdNode->place, entry->offset);
				}
			}
		}else if(entry->attribute->attr.typeDescriptor->properties.dataType == FLOAT_TYPE){
			if(IdNode->place == 0){
				IdNode->place = get_freg();
			}
			if(IdNode->place == -1){
				//TODO: ???
			}else{
				if(is_global){
					int tmp = get_reg();
					fprintf(output, "ldr x%d, =_g_%s\n", tmp, name);
					fprintf(output, "ldr s%d, [x%d,#0]\n", IdNode->place-16, tmp);
					free_reg(tmp, NULL);
				}else{
					fprintf(output, "ldr s%d, [x29,#%d]\n", IdNode->place-16, entry->offset);
				}
			}
		}else{
			printf("error\n");
		}
	}else if(IdNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID){
		int size_reg = genDimNode(IdNode);
		if(entry->attribute->attr.typeDescriptor->properties.arrayProperties.elementType == INT_TYPE){ //***check entry instead of node(type conversion)(hw6)
			if(IdNode->place == 0){
				IdNode->place = get_reg();
			}
			if(IdNode->place == -1){
				//TODO: ???
			}else{
				if(is_global){
					int tmp = get_reg();
					fprintf(output, "ldr x%d, =_g_%s\n", tmp, name);
					fprintf(output, "ldr w%d, [x%d,x%d]\n", IdNode->place, tmp, size_reg);
					free_reg(tmp, NULL);
				}else{
					int tmp = get_reg();
					fprintf(output, "mov w%d, #%d\n", size_reg, entry->offset);
					fprintf(output, "add w%d, w%d, w%d\n", tmp, tmp, size_reg);
					fprintf(output, "ldr w%d, [x29,x%d]\n", IdNode->place, tmp);
					free_reg(tmp, NULL);
				}
			}
		}else if(entry->attribute->attr.typeDescriptor->properties.arrayProperties.elementType == FLOAT_TYPE){
			if(IdNode->place == 0){
				IdNode->place = get_freg();
			}
			if(IdNode->place == -1){
				//TODO: ???
			}else{
				if(is_global){
					int tmp = get_reg();
					fprintf(output, "ldr x%d, =_g_%s\n", tmp, name);
					fprintf(output, "ldr s%d, [x%d,x%d]\n", IdNode->place-16, tmp, size_reg);
					free_reg(tmp, NULL);
				}else{
					int tmp = get_reg();
					fprintf(output, "mov w%d, #%d\n", size_reg, entry->offset);
					fprintf(output, "add w%d, w%d, w%d\n", tmp, tmp, size_reg);
					fprintf(output, "ldr s%d, [x29,x%d]\n", IdNode->place-16, tmp);
					free_reg(tmp, NULL);
				}
			}
		}
		free_reg(size_reg, NULL);
	}
}

void genReturnStmt(AST_NODE* returnNode){
	printf("genReturnStmt\n");
	fprintf(output, "#return\n");
	AST_NODE* parent = returnNode->parent;
	char *name;
	int returnType;
	while(parent){
		if(parent->nodeType == DECLARATION_NODE){
			if(parent->semantic_value.declSemanticValue.kind == FUNCTION_DECL){
				name = parent->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
				printf("%s\n", name);
				SymbolTableEntry* entry = parent->child->semantic_value.identifierSemanticValue.symbolTableEntry;
				returnType = parent->child->dataType;
				printf("%d\n", returnType);
				break;
			}
		}
		parent = parent->parent;
	}
	AST_NODE* child = returnNode->child;
	genExprNode(child);
	//type conversion (hw6)
	if(child->place < 32){ //int
		if(returnType == INT_TYPE){
			fprintf(output, "mov x0, x%d\n", child->place);
		}else{
			int tmp = get_freg();
			fprintf(output, "scvtf s%d, w%d\n", tmp-16, child->place);
			free_reg(child->place, child);
			child->place = tmp;
			fprintf(output, "fmov w0, s%d\n", child->place-16); // fmov s0, s%d can't work
		}
	}else{ //float
		if(returnType == INT_TYPE){
			int tmp = get_reg();
			fprintf(output, "fcvtzs w%d, s%d\n", tmp, child->place-16);
			free_reg(child->place, child);
			child->place = tmp;
			fprintf(output, "mov x0, x%d\n", child->place);
		}else{
			fprintf(output, "fmov w0, s%d\n", child->place-16); // fmov s0, s%d can't work
		}
	}
	
	free_reg(child->place, child);
	fprintf(output, "b _end_%s\n", name);
}

void genBlockNode(AST_NODE* blockNode){
	printf("genBlockNode\n");
	AST_NODE* child = blockNode->child;
	while(child != NULL){
		if(child->nodeType == STMT_LIST_NODE){
			AST_NODE* grand_child = child->child;
			while(grand_child != NULL){
				genStmtNode(grand_child);
				grand_child = grand_child->rightSibling;
			}
		}else if(child->nodeType == VARIABLE_DECL_LIST_NODE){
			AST_NODE* grand_child = child->child;
			while(grand_child != NULL){
				genDeclarationNode(grand_child);
				grand_child = grand_child->rightSibling;
			}
		}
		child = child->rightSibling;
	}
}

void genStmtNode(AST_NODE* stmtNode){
	printf("genStmtNode\n");
	if(stmtNode->nodeType == BLOCK_NODE){
		genBlockNode(stmtNode);
		return;
	}
	int Kind = stmtNode->semantic_value.stmtSemanticValue.kind;
	if(Kind == WHILE_STMT){
		genWhileStmt(stmtNode);
	}else if(Kind == FOR_STMT){
		genForStmt(stmtNode);
	}else if(Kind == ASSIGN_STMT){
		genAssignmentStmt(stmtNode);
	}else if(Kind == IF_STMT){
		genIfStmt(stmtNode);
	}else if(Kind == FUNCTION_CALL_STMT){
		genFunctionCall(stmtNode);
		free_reg(stmtNode->place, stmtNode);
	}else if(Kind == RETURN_STMT){
		genReturnStmt(stmtNode);
	}
}

int genDimNode(AST_NODE* idNode){
	printf("genDimNode\n");
	fprintf(output, "#get dim\n");
	// multi dimension array reference (hw6)
	SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
	int dim = entry->attribute->attr.typeDescriptor->properties.arrayProperties.dimension;
	int arr_size[dim];
	for(int i = 0; i < dim; i++){
		arr_size[i] = entry->attribute->attr.typeDescriptor->properties.arrayProperties.sizeInEachDimension[i];
	}
	AST_NODE* dim_list = idNode->child;
	int size_reg = get_reg();
	fprintf(output, "mov w%d, #0\n", size_reg);
	for(int i = 0; i < dim; i++){
		int dim_size = 1;
		for(int j = i+1; j < dim; j++){
			dim_size *= arr_size[j];
		}
		printf("dim_size:%d\n", dim_size);
		genExprNode(dim_list);
		if(dim_list->place == 0){
			dim_list->place = get_reg();
		}
		int tmp = get_reg();
		fprintf(output, "mov w%d, #%d\n", tmp, dim_size*4);
		//size += ((dim_list->semantic_value.const1->const_u.intval)*dim_size)*4;
		fprintf(output, "mul w%d, w%d, w%d\n", dim_list->place, dim_list->place, tmp);
		fprintf(output, "add w%d, w%d, w%d\n", size_reg, size_reg, dim_list->place);
		free_reg(dim_list->place, dim_list);
		free_reg(tmp, NULL);
		dim_list = dim_list->rightSibling;
	}
	return size_reg;
}

void getParameterDeclaration(AST_NODE* paramNode){
	printf("getParameterDeclaration\n");
	/*
	------------ <= fp + 24 (old sp)
	|  param2  |
	------------ <= fp + 20
	|  param1  |
	------------ <= fp + 16
	| ret(x29) |
	------------ <= fp + 8
	|  old fp  |
	------------ <= fp
	...
	*/
	int paramOffset, param_count = 0;
	paramNode = paramNode->child; // skip PARAM_LIST_NODE
	AST_NODE* tmp = paramNode;
	while(tmp != NULL){
		param_count += 1;
		tmp = tmp->rightSibling;
	}
	if(param_count != 0){
		fprintf(output, "sub sp, sp, #%d\n", param_count*4 + 8);
		paramOffset = 16 + (param_count-1)*4;
	}else{return;}
	while(paramNode != NULL){
		AST_NODE* Id = paramNode->child->rightSibling;
		SymbolTableEntry* entry = Id->semantic_value.identifierSemanticValue.symbolTableEntry;
		entry->offset = paramOffset;
		fprintf(output, "str w%d, [sp, #%d]\n", (paramOffset-16)/4 + 1, paramOffset-8);
		paramOffset -= 4;
		paramNode = paramNode->rightSibling;
	}
}

void genFunctionDeclaration(AST_NODE* declarationNode){
	printf("genFunctionDeclaration\n");
	AST_NODE* Id = declarationNode->child->rightSibling;
	AST_NODE* param = Id->rightSibling;
	char *name = Id->semantic_value.identifierSemanticValue.identifierName;
	// init
	ARoffset = -4;
	gen_head(name);
	//param(hw6)
	getParameterDeclaration(param);

	gen_prologue(name);
	genBlockNode(Id->rightSibling->rightSibling);
	gen_epilogue(name);
}
