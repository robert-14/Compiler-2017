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
				genScalarDecl(id_list);
			}
		}else if(id_list->semantic_value.identifierSemanticValue.kind == ARRAY_ID){
			genArrayDecl(id_list);
		}else{ //WITH_INIT_ID
			//hw6 
		}
		id_list = id_list->rightSibling;
	}
}

void genScalarDecl(AST_NODE* idNode){
	printf("genScalarDecl\n");
	SymbolTableEntry* entry = idNode->semantic_value.identifierSemanticValue.symbolTableEntry;
	if(entry->nestingLevel == 0){
		// global variable
		fprintf(output, ".data\n");
		if(entry->attribute->attr.typeDescriptor->properties.dataType == INT_TYPE){
			fprintf(output, "_g_%s: .word 0\n", entry->name);
		}else if(entry->attribute->attr.typeDescriptor->properties.dataType == FLOAT_TYPE){
			fprintf(output, "_g_%s: .float 0.0\n", entry->name);
		}else{printf("error\n");}
		fprintf(output, ".text\n");
	}else{
		// local variable
		entry->offset = ARoffset;
		ARoffset -= 4;
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
	fprintf(output, "_whileTest_%d\n", label);
	AssignOrExpr(test);
	fprintf(output, "beq _whileExit_%d\n", label);
	genStmtNode(Stmt);
	fprintf(output, "b _whileTest_%d\n", label);
	fprintf(output, "_whileExit_%d:\n", label);
}

void genForStmt(AST_NODE* forNode){
	// hw6 
	AST_NODE* assignExpr1 = forNode->child;
	AST_NODE* ExprList = assignExpr1->rightSibling;
	AST_NODE* assignExpr2 = ExprList->rightSibling;
	AST_NODE* Stmt = assignExpr2->rightSibling;
	processExprList(assignExpr1);
	processExprList(ExprList);
	processExprList(assignExpr2);
	genStmtNode(Stmt);
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
	
	// implicit Type conversion (hw6)
	genIdValue(left);
	genExprNode(right);
	
	int is_global = (entryL->nestingLevel == 0);
	if(!is_global){
		if(entryL->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
			if(left->dataType == INT_TYPE){
				fprintf(output, "mov w%d, w%d\n", left->place, right->place);
				fprintf(output, "str w%d, [x29, #%d]\n", left->place, entryL->offset);
			}else if(left->dataType == FLOAT_TYPE){
				fprintf(output, "fmov s%d, s%d\n", left->place-16, right->place-16);
				fprintf(output, "str s%d, [x29, #%d]\n", left->place-16, entryL->offset);
			}else{printf("error\n");}
		}else{ //ARRAY_TYPE_DESCRIPTOR
			if(left->dataType == INT_TYPE){
				fprintf(output, "str w%d, [x%d, #0]\n", right->place, left->place);
			}else if(left->dataType == FLOAT_TYPE){
				fprintf(output, "str s%d, [x%d, #0]\n", right->place-16, left->place);
			}else{printf("error\n");}
		}
	}else{
		if(entryL->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR){
			int tmp = get_reg();
			fprintf(output, "ldr x%d, =_g_%s\n", tmp, entryL->name);
			if(left->dataType == INT_TYPE){
				fprintf(output, "mov w%d, w%d\n", left->place, right->place);
				fprintf(output, "str w%d, [x%d, #0]\n", left->place, tmp);
			}else if(left->dataType == FLOAT_TYPE){
				fprintf(output, "fmov s%d, s%d\n", left->place-16, right->place-16);
				fprintf(output, "str s%d, [x%d, #0]\n", left->place-16, tmp);
			}else{printf("error\n");}
			free_reg(tmp, NULL);
		}else{ //ARRAY_TYPE_DESCRIPTOR
			if(left->dataType == INT_TYPE){
				fprintf(output, "str w%d, [x%d, #0]\n", right->place, left->place);
			}else if(left->dataType == FLOAT_TYPE){
				fprintf(output, "str s%d, [x%d, #0]\n", right->place-16, left->place);
			}else{printf("error\n");}
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
						fprintf(output, "cset w%d, eq\n", left->place);
						fprintf(output, "cmp w%d, #0\n", left->place);
					break;
					case(BINARY_OP_GE):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, ge\n", left->place);
						fprintf(output, "cmp w%d, #0\n", left->place);
					break;
					case(BINARY_OP_LE):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, le\n", left->place);
						fprintf(output, "cmp w%d, #0\n", left->place);
					break;
					case(BINARY_OP_NE):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, ne\n", left->place);
						fprintf(output, "cmp w%d, #0\n", left->place);
					break;
					case(BINARY_OP_GT):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, gt\n", left->place);
						fprintf(output, "cmp w%d, #0\n", left->place);
					break;
					case(BINARY_OP_LT):
						fprintf(output, "cmp w%d, w%d\n", left->place, right->place);
						fprintf(output, "cset w%d, lt\n", left->place);
						fprintf(output, "cmp w%d, #0\n", left->place);
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
					fprintf(output, "scvtf s%d, w%d, #6\n", tmp-16, left->place);
					free_reg(left->place, left);
					left->place = tmp;
				}
				if(right->place < 32){
					int tmp = get_freg();
					fprintf(output, "scvtf s%d, w%d, #6\n", tmp-16, left->place);
					free_reg(right->place, right);
					right->place = tmp;
				}
				exprNode->dataType = FLOAT_TYPE;
				exprNode->place = get_freg();
				switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp){
					case(BINARY_OP_ADD):
						fprintf(output, "fadd s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_SUB):
						fprintf(output, "fsub s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_MUL):
						fprintf(output, "fmul s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_DIV):
						fprintf(output, "fdiv s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_EQ):
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset s%d, eq\n", left->place-16);
						fprintf(output, "fcmp s%d, #0.0\n", left->place-16);
					break;
					case(BINARY_OP_GE):
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset s%d, ge\n", left->place-16);
						fprintf(output, "fcmp s%d, #0.0\n", left->place-16);
					break;
					case(BINARY_OP_LE):
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset s%d, le\n", left->place-16);
						fprintf(output, "fcmp s%d, #0.0\n", left->place-16);
					break;
					case(BINARY_OP_NE):
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset s%d, ne\n", left->place-16);
						fprintf(output, "fcmp s%d, #0.0\n", left->place-16);
					break;
					case(BINARY_OP_GT):
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset s%d, gt\n", left->place-16);
						fprintf(output, "fcmp s%d, #0.0\n", left->place-16);
					break;
					case(BINARY_OP_LT):
						fprintf(output, "fcmp s%d, s%d\n", left->place-16, right->place-16);
						fprintf(output, "cset s%d, lt\n", left->place-16);
						fprintf(output, "fcmp s%d, #0.0\n", left->place-16);
					break;
					case(BINARY_OP_AND):
						fprintf(output, "and s%d, s%d, s%d\n", exprNode->place-16, left->place-16, right->place-16);
					break;
					case(BINARY_OP_OR):
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
						fprintf(output, "neg x%d, x%d\n", child->place, child->place);
					break;
					case(UNARY_OP_LOGICAL_NEGATION):
						exprNode->place = child->place;
						//0 -> 1, x(!=0) -> 0
						fprintf(output, "cmp x%d, #0\n", child->place);
						fprintf(output, "moveq x%d, #1\n", child->place);
						fprintf(output, "movne x%d, #0\n", child->place);
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
						fprintf(output, "neg s%d, s%d\n", child->place-16, child->place-16);
					break;
					case(UNARY_OP_LOGICAL_NEGATION):
						exprNode->place = child->place;
						//0 -> 1, x(!=0) -> 0
						fprintf(output, "cmp s%d, #0\n", child->place-16);
						fprintf(output, "moveq s%d, #1\n", child->place-16);
						fprintf(output, "movne s%d, #0\n", child->place-16);
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
	AST_NODE* param = Id->rightSibling; //hw6
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
	//param(hw6)
}

void genIdValue(AST_NODE* IdNode){
	printf("genIdValue\n");
	SymbolTableEntry* entry = IdNode->semantic_value.identifierSemanticValue.symbolTableEntry;
	char* name = IdNode->semantic_value.identifierSemanticValue.identifierName;
	int is_global = (entry->nestingLevel == 0);
	if(IdNode->semantic_value.identifierSemanticValue.kind == NORMAL_ID){
		if(IdNode->dataType == INT_TYPE){
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
		}else if(IdNode->dataType == FLOAT_TYPE){
			if(IdNode->place == 0){
				IdNode->place = get_freg();
			}
			if(IdNode->place == -1){
				//TODO: ???
			}else{
				if(is_global){
					int tmp = get_reg();
					fprintf(output, "ldr s%d, =_g_%s\n", IdNode->place-16, name);
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
		// array reference (hw6)
	}
}

void genReturnStmt(AST_NODE* returnNode){
	printf("genReturnStmt\n");
	fprintf(output, "#return\n");
	AST_NODE* parent = returnNode->parent;
	char *name;
	while(parent){
		if(parent->nodeType == DECLARATION_NODE){
			if(parent->semantic_value.declSemanticValue.kind == FUNCTION_DECL){
				name = parent->child->rightSibling->semantic_value.identifierSemanticValue.identifierName;
				break;
			}
		}
		parent = parent->parent;
	}
	AST_NODE* child = returnNode->child;
	genExprNode(child);
	fprintf(output, "mov x0, x%d\n", child->place);
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

void genDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize){
}

void genFunctionDeclaration(AST_NODE* declarationNode){
	printf("genFunctionDeclaration\n");
	AST_NODE* Id = declarationNode->child->rightSibling;
	char *name = Id->semantic_value.identifierSemanticValue.identifierName;
	// init
	ARoffset = -4;
	gen_head(name);
	//param(hw6)
	gen_prologue(name);
	genBlockNode(Id->rightSibling->rightSibling);
	gen_epilogue(name);
}
