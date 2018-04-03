struct symtab{
	char lexeme[256];
	struct symtab *front;
	struct symtab *back;
	int line;
	int counter;
    struct symtab *sort_front;
    struct symtab *sort_back;
};
 /*struct sort_array
{
    char name[256];
    int  number;
};*/
typedef struct symtab symtab;
symtab * lookup(char *name);
void insert(char *name);
