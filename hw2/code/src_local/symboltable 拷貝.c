#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"header.h"

#define TABLE_SIZE	256

symtab *sort_head = NULL;
symtab * hash_table[TABLE_SIZE];
extern int linenumber;

int HASH(char * str){
	int idx=0;
	while(*str){
		idx = idx << 1;
		idx+=*str;
		str++;
	}	
	return (idx & (TABLE_SIZE-1));
}

/*returns the symbol table entry if found else NULL*/

symtab * lookup(char *name){
	int hash_key;
	symtab* symptr;
	if(!name)
		return NULL;
	hash_key=HASH(name);
	symptr=hash_table[hash_key];

	while(symptr){
		if(!(strcmp(name,symptr->lexeme)))
			return symptr;
		symptr=symptr->front;
	}
	return NULL;
}

void insert_sortlist(symtab *cur){
	if( sort_head == NULL){
		sort_head = cur;
		cur->sort_front = NULL;
		cur->sort_back = NULL;
		return;		
	}
	symtab *tmp;
	tmp = sort_head;
	if( strcmp( cur->lexeme, tmp->lexeme) < 0){
		cur->sort_front = NULL;
		cur->sort_back = tmp;
		tmp->sort_front = cur;
		sort_head = cur;
	}
	else{
		while( tmp->sort_back != NULL && strcmp( cur->lexeme, tmp->lexeme) > 0){
			tmp = tmp->sort_back;
		}
		if( strcmp( cur->lexeme, tmp->lexeme) > 0 ){
			cur->sort_back = NULL;
			cur->sort_front = tmp;
			tmp->sort_back = cur;
			return;
		}
		cur->sort_back = tmp;
		cur->sort_front = tmp->sort_front;
		tmp->sort_front = cur;
		cur->sort_front->sort_back = cur;
	}	
}

void insertID(char *name){
	int hash_key;
	symtab* ptr;
	symtab* symptr=(symtab*)malloc(sizeof(symtab));	
	
	hash_key=HASH(name);
	ptr=hash_table[hash_key];
	
	if(ptr==NULL){
		/*first entry for this hash_key*/
		hash_table[hash_key]=symptr;
		symptr->front=NULL;
		symptr->back=symptr;
	}
	else{
		symptr->front=ptr;
		ptr->back=symptr;
		symptr->back=symptr;
		hash_table[hash_key]=symptr;	

	}
	
	strcpy(symptr->lexeme,name);
	symptr->line=linenumber;
	symptr->counter=1;
	insert_sortlist(symptr);
}

void printSym(symtab* ptr) 
{
	    printf(" Name = %s \n", ptr->lexeme);
	    printf(" References = %d \n", ptr->counter);
}

void printSymTab()
{
    symtab *cur;
    cur = sort_head;
    printf("Frequency of identifiers:\n");
    while( cur != NULL){
    	printf("%s\t%d\n",cur->lexeme,cur->counter);
    	cur = cur->sort_back;
    }
    //int i;
    //printf("----- Symbol Table ---------\n");
    /*for (i=0; i<TABLE_SIZE; i++)
    {
        symtab* symptr;
		symptr = hash_table[i];
		while (symptr != NULL)
		{

            //printf("====>  index = %d \n", i);
	   		//printSym(symptr);
	   		symptr=symptr->front;
		}
    }*/
}
