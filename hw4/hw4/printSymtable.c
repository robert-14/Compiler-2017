void printSymbolTable(){
	SymbolTable* now = symbolTable;
	while(now != NULL){
		printf("======currentLevel:%d========\n", now->currentLevel);
		for(int i = 0; i < 255; i++){
			if(now->hashTable[i] != NULL){
				SymbolTableEntry *data = now->hashTable[i];
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
		now = now->prevTable;
	}
	printf("\n\n");
	
	
	
}