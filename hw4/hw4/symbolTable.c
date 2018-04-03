#include "symbolTable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// This file is for reference only, you are not required to follow the implementation. //

int HASH(char * str) {
	int idx=0;
	while (*str){
		idx = idx << 1;
		idx+=*str;
		str++;
	}
	return (idx & (HASH_TABLE_SIZE-1));
}

SymbolTable* symbolTable;
void printSymbolTable(){
    SymbolTable* now = symbolTable;
    while(now != NULL){
        printf("======currentLevel:%d========\n", now->currentLevel);

        for(int i = 0; i < 255; i++){
            if(now->hashTable[i] != NULL){
                SymbolTableEntry *data = now->hashTable[i];
                printf("address: %d\n", data->attribute);
                while(data != NULL){
                    printf("name: %s --- ", data->name);
                    int type, kind;
                    switch(data->attribute->attributeKind){
                        case(VARIABLE_ATTRIBUTE):
                            printf("type: var --- ");
                            kind = data->attribute->attr.typeDescriptor->kind;
                            if(kind == SCALAR_TYPE_DESCRIPTOR){
                                printf("scalar --- ");
                                type = data->attribute->attr.typeDescriptor->properties.dataType;
                            }else{
                                printf("array --- ");
                                if(data->attribute->attr.typeDescriptor->properties.dataType == ERROR_TYPE){
                                    type = data->attribute->attr.typeDescriptor->properties.dataType;
                                }else{
                                    printf("dimension = %d --- ", data->attribute->attr.typeDescriptor->properties.arrayProperties.dimension);
                                    type = data->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;
                                }
                            }
                            if(type == INT_TYPE){
                                printf("int\n");
                            }else if(type == FLOAT_TYPE){
                                printf("float\n");
                            }else if(type == VOID_TYPE){
                                printf("void\n");
                            }else if(type == ERROR_TYPE){
                                printf("error\n");
                            }else{
                                printf("???\n");
                            }
                        break;
                        case(TYPE_ATTRIBUTE):
                            printf("type: typedef --- ");
                            kind = data->attribute->attr.typeDescriptor->kind;
                            if(kind == SCALAR_TYPE_DESCRIPTOR){
                                printf("scalar --- ");
                                type = data->attribute->attr.typeDescriptor->properties.dataType;
                            }else{
                                printf("array --- ");
                                printf("dimension = %d --- ", data->attribute->attr.typeDescriptor->properties.arrayProperties.dimension);
                                type = data->attribute->attr.typeDescriptor->properties.arrayProperties.elementType;
                            }
                            if(type == INT_TYPE){
                                printf("int\n");
                            }else if(type == FLOAT_TYPE){
                                printf("float\n");
                            }else if(type == VOID_TYPE){
                                printf("void\n");
                            }else{
                                printf("???\n");
                            }
                        break;
                        case(FUNCTION_SIGNATURE):
                            printf("type: function --- ");
                            printf("para count = %d : --- ", data->attribute->attr.functionSignature->parametersCount);
                            type = data->attribute->attr.functionSignature->returnType;
                            if(type == INT_TYPE){
                                printf("int\n");
                            }else if(type == FLOAT_TYPE){
                                printf("float\n");
                            }else if(type == VOID_TYPE){
                                printf("void\n");
                            }else{
                                printf("???\n");
                            }
                        break;
                        
                    }
                    data = data->prevInHashChain;
                }
            }
        }
        printf("=============================\n");
        now = now->downtable;
    }
    printf("\n\n");
    
    
    
}
SymbolTableEntry* newSymbolTableEntry(int nestingLevel)
{
    SymbolTableEntry* symbolTableEntry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
    symbolTableEntry->nextInHashChain = NULL;
    symbolTableEntry->prevInHashChain = NULL;
    //symbolTableEntry->nextInSameLevel = NULL;
    //symbolTableEntry->sameNameInOuterLevel = NULL;
    symbolTableEntry->attribute = NULL;
    symbolTableEntry->name = NULL;
    symbolTableEntry->nestingLevel = nestingLevel;
    return symbolTableEntry;
}
//free hash table memory
/*void removeFromHashTrain(int hashIndex,SymbolTableEntry* entry)
{
    while( entry != NULL ){
        SymbolTableEntry* temp = entry;
        entry = entry->nextInHashChain;
        temp->name = NULL;
        temp->attribute = NULL;
        free(temp);
    }
}*/
//add hash result of id into hash train
/*void enterIntoHashTrain(int hashindex, SymbolTableEntry* entry)
{

}*/

void initializeSymbolTable() //initialize global variable
{
    symbolTable = (SymbolTable*)malloc(sizeof(SymbolTable));
    symbolTable->downtable = NULL;
    symbolTable->uppertable = NULL;
    for(int i = 0; i < 256;i++){
        symbolTable->hashTable[i] = NULL;
    }
    symbolTable->currentLevel = 0;

}
//
void symbolTableEnd()
{
    free(symbolTable);
}
//check type declaration
SymbolTableEntry* searchSymbol(char* symbolName, SymbolAttributeKind kind)
{
    int hash_num = HASH(symbolName);
    SymbolTable* temp_sym = symbolTable;
    while( temp_sym != NULL ){    
        SymbolTableEntry* temp = temp_sym->hashTable[hash_num];    
        while(temp != NULL){
            if( !strcmp(temp->name,symbolName) && temp->attribute->attributeKind == kind){
                return temp;
            }
            temp = temp->nextInHashChain;
        } 
        temp_sym = temp_sym->downtable;
    }       
    return NULL;
}
//add id into a table
SymbolTableEntry* enterSymbol(char* symbolName, SymbolAttribute* attribute)
{
    int hash_num = HASH(symbolName),flag = 0;                   //check for redeclaration
    SymbolTableEntry* test = symbolTable->hashTable[hash_num];
    while(test != NULL){
        if( !strcmp(test->name,symbolName) ){
           flag = 1;
           break;
        }
        test = test->nextInHashChain;
    }
    if( flag){
        return NULL;
    }

    hash_num = HASH(symbolName);
    SymbolTableEntry* head = symbolTable->hashTable[hash_num];
    SymbolTableEntry* new = newSymbolTableEntry(symbolTable->currentLevel);
    symbolTable->hashTable[hash_num] = new;
    new->name = symbolName;
    new->attribute =  attribute;
    if( head == NULL ){
        new->nextInHashChain = NULL;
        return new;
    }
    else{
        new->nextInHashChain = head;
        head->prevInHashChain = new;
        return new;
    }    
}

//remove the symbol from the current scope
/*void removeSymbol(char* symbolName)
{
}*/

/*int declaredLocally(char* symbolName)
{
}*/

//add a new symbol table
void openScope()
{
    SymbolTable* new = (SymbolTable*)malloc(sizeof(SymbolTable));
    new->downtable = symbolTable;
    new->uppertable = NULL;
    for(int i = 0; i < 256;i++){
        new->hashTable[i] = NULL;
    }
    new->currentLevel = symbolTable->currentLevel + 1;
    symbolTable->uppertable = new;
    symbolTable = new;
}
//delete a symbol
void closeScope()
{
    symbolTable = symbolTable->downtable;
    symbolTable->uppertable = NULL;
}
