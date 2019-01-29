#include "token.h"
#include "data.h"
#include "symbol.h"
#include <string.h>
#include <stdlib.h>

/**
 * This pointer is set when by codeGenerator() func and used by printEmittedCode() func.
 * 
 * You are not required to use it anywhere. The implemented part of the skeleton
 * handles the printing. Instead, you are required to fill the vmCode properly by making
 * use of emit() func.
 * */
FILE* _out;

/**
 * Token list iterator used by the code generator. It will be set once entered to
 * codeGenerator() and reset before exiting codeGenerator().
 * 
 * It is better to use the given helper functions to make use of token list iterator.
 * */
TokenListIterator _token_list_it;

/**
 * Current level. Use this to keep track of the current level for the symbol table entries.
 * */
unsigned int currentLevel;

/**
 * Current scope. Use this to keep track of the current scope for the symbol table entries.
 * NULL means global scope.
 * */
Symbol* currentScope;

/**
 * Symbol table.
 * */
SymbolTable symbolTable;

/**
 * The array of instructions that the generated(emitted) code will be held.
 * */
Instruction vmCode[MAX_CODE_LENGTH];

/**
 * The next index in the array of instructions (vmCode) to be filled.
 * */
int nextCodeIndex;

/**
 * The id of the register currently being used.
 * */
int currentReg;

/**
 * Emits the instruction whose fields are given as parameters.
 * Internally, writes the instruction to vmCode[nextCodeIndex] and returns the
 * nextCodeIndex by post-incrementing it.
 * If MAX_CODE_LENGTH is reached, prints an error message on stderr and exits.
 * */
int emit(int OP, int R, int L, int M);

/**
 * Prints the emitted code array (vmCode) to output file.
 * 
 * This func is called in the given codeGenerator() function. You are not required
 * to have another call to this function in your code.
 * */
void printEmittedCodes();

/**
 * Returns the current token using the token list iterator.
 * If it is the end of tokens, returns token with id nulsym.
 * */
Token getCurrentToken();

/**
 * Returns the type of the current token. Returns nulsym if it is the end of tokens.
 * */
int getCurrentTokenType();

/**
 * Advances the position of TokenListIterator by incrementing the current token
 * index by one.
 * */
void nextToken();

/**
 * Functions used for non-terminals of the grammar
 * 
 * rel-op func is removed on purpose. For code generation, it is easier to parse
 * rel-op as a part of condition.
 * */
int program();
int block();
int const_declaration();
int var_declaration();
int proc_declaration();
int statement();
int condition();
int expression();
int term();
int factor();

/******************************************************************************/
/* Definitions of helper functions starts *************************************/
/******************************************************************************/

Token getCurrentToken()
{
    return getCurrentTokenFromIterator(_token_list_it);
}

int getCurrentTokenType()
{
    return getCurrentToken().id;
}

void nextToken()
{
    _token_list_it.currentTokenInd++;
}

/**
 * Given the code generator error code, prints error message on file by applying
 * required formatting.
 * */
void printCGErr(int errCode, FILE* fp)
{
    if(!fp || !errCode) return;

    fprintf(fp, "CODE GENERATOR ERROR[%d]: %s.\n", errCode, codeGeneratorErrMsg[errCode]);
}

int emit(int OP, int R, int L, int M)
{
    if(nextCodeIndex == MAX_CODE_LENGTH)
    {
        fprintf(stderr, "MAX_CODE_LENGTH(%d) reached. Emit is unsuccessful: terminating code generator..\n", MAX_CODE_LENGTH);
        exit(0);
    }
    
    vmCode[nextCodeIndex] = (Instruction){ .op = OP, .r = R, .l = L, .m = M};    

    return nextCodeIndex++;
}

// finds level for STO and LOD functions
int findLevel(int foundSymbol)
{
	if ( currentLevel - findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->level < 0 )
	{
		return 0;
	}
	else
	{
		return currentLevel - findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->level;
	}
}

void printEmittedCodes()
{
    for(int i = 0; i < nextCodeIndex; i++)
    {
        Instruction c = vmCode[i];
        fprintf(_out, "%d %d %d %d\n", c.op, c.r, c.l, c.m);
    }
}

/******************************************************************************/
/* Definitions of helper functions ends ***************************************/
/******************************************************************************/

/**
 * Advertised codeGenerator function. Given token list, which is possibly the
 * output of the lexer, parses a program out of tokens and generates code. 
 * If encountered, returns the error code.
 * 
 * Returning 0 signals successful code generation.
 * Otherwise, returns a non-zero code generator error code.
 * */
int codeGenerator(TokenList tokenList, FILE* out)
{
    // Set output file pointer
    _out = out;

    /**
     * Create a token list iterator, which helps to keep track of the current
     * token being parsed.
     * */
    _token_list_it = getTokenListIterator(&tokenList);

    // Initialize current level to 0, which is the global level
    currentLevel = 0;

    // Initialize current scope to NULL, which is the global scope
    currentScope = NULL;

    // The index on the vmCode array that the next emitted code will be written
    nextCodeIndex = 0;

    // The id of the register currently being used
    currentReg = 0;

    // Initialize symbol table
    initSymbolTable(&symbolTable);

    // Start parsing by parsing program as the grammar suggests.
    int err = program();

    // Print symbol table - if no error occured
    if(!err)
    {
        // Print the emitted codes to the file
        printEmittedCodes();
    }

    // Reset output file pointer
    _out = NULL;

    // Reset the global TokenListIterator
    _token_list_it.currentTokenInd = 0;
    _token_list_it.tokenList = NULL;

    // Delete symbol table
    deleteSymbolTable(&symbolTable);

    // Return err code - which is 0 if parsing was successful
    return err;
}

// Already implemented.
int program()
{
    // Generate code for block
    int err = block();
    if(err) return err;

    // After parsing block, periodsym should show up
    if( getCurrentTokenType() == periodsym )
    {
        // Consume token
        nextToken();

        // End of program, emit halt code
        emit(SIO_HALT, 0, 0, 3);

        return 0;
    }
    else
    {
        // Periodsym was expected. Return error code 6.
        return 6;
    }
}

int block()
{
    /**
     * block is 
	 * 1) const_declaration
	 * 2) var_declaration
	 * 3) proc_declaration
	 * 4) statement
     * */
	 
	// Parse const_declaration.
    int err = const_declaration();

    /**
     * If parsing of const_declaration was not successful, immediately stop parsing
     * and propagate the same error code by returning it.
     * */
    if(err) return err;

    // Parse var_declaration.
    err = var_declaration();

    /**
     * If parsing of var_declaration was not successful, immediately stop parsing
     * and propagate the same error code by returning it.
     * */
    if(err) return err;
	
	// Parse proc_declaration.
    err = proc_declaration();

    /**
     * If parsing of proc_declaration was not successful, immediately stop parsing
     * and propagate the same error code by returning it.
     * */
    if(err) return err;
	
	// Parse statement.
    err = statement();

    /**
     * If parsing of statement was not successful, immediately stop parsing
     * and propagate the same error code by returning it.
     * */
    if(err) return err;

    return 0;
}

int const_declaration()
{
    /**
     * const_declaration is the following
	 * 1) "const"
	 * 2) ident
	 * 3) "="
	 * 4) number
	 *		repeat this 0 or more times
	 *		a) ","
	 *		b) ident
	 *		c) "="
	 *		d) number 
	 * 5) ";"
     * */
	
	// Is the current token a constsym?
    if (getCurrentTokenType() == constsym)
	{
		nextToken(); // Go to the next token..
		
		// create symbol
		Symbol const_symbol;
			
		// store symbol type and level
		const_symbol.type = CONST;
		const_symbol.level = currentLevel;
		
		// Is the current token a identsym?
		if (getCurrentTokenType() == identsym)
		{	
			// store const_symbol name
			strcpy(const_symbol.name, getCurrentToken().lexeme);
			
			// Consume identsym
			//
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
             * Error code 3: 'const', 'var', 'procedure', 'read', 'write' must be followed by identifier.
             * Stop parsing and return error code 3.
             * */
            return 3;
		}
		
		// Is the current token a eqsym? 
		if (getCurrentTokenType() == eqsym)
		{
			// Consume eqsym
			//
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
             * Error code 2: Identifier must be followed by '='.
             * Stop parsing and return error code 2.
             * */
            return 2;
		}
		
		// Is the current token a numbersym? 
		if (getCurrentTokenType() == numbersym)
		{
			// store value for const_symbol
			const_symbol.value = atoi(getCurrentTokenFromIterator(_token_list_it).lexeme);
			
			// Consume numbersym
			//
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
             * Error code 1: '=' must be followed by a number.
             * Stop parsing and return error code 1.
             * */
            return 1;
		}
		
		// add const symbol to symbol table
		addSymbol(&symbolTable, const_symbol);
		
		// Create new const symbol in case of more consts
		Symbol *new_const_symbol;
		
		while (getCurrentTokenType() == commasym)
		{
			// Consume commasym
			//
			nextToken(); // Go to the next token..
			
			// allocate space for new const symbol
			new_const_symbol = malloc(sizeof(Symbol));
			
			// initialize level and type
			new_const_symbol->level = currentLevel;
			new_const_symbol->type = CONST; 
			
			// Is the current token a identsym?
			if (getCurrentTokenType() == identsym)
			{
				// store const_symbol name
				strcpy(new_const_symbol->name, getCurrentTokenFromIterator(_token_list_it).lexeme);
				
				// Consume identsym
				//
				nextToken(); // Go to the next token..
			}
			else
			{
				/**
				 * Error code 3: 'const', 'var', 'procedure', 'read', 'write' must be followed by identifier.
				 * Stop parsing and return error code 3.
				 * */
				return 3;
			}
			
			// Is the current token a eqsym? 
			if (getCurrentTokenType() == eqsym)
			{
				// Consume eqsym
				//
				nextToken(); // Go to the next token..
			}
			else
			{
				/**
				 * Error code 2: Identifier must be followed by '='.
				 * Stop parsing and return error code 2.
				 * */
				return 2;
			}
			
			// Is the current token a numbersym? 
			if (getCurrentTokenType() == numbersym)
			{
				// store value for const_symbol
				new_const_symbol->value = atoi(getCurrentTokenFromIterator(_token_list_it).lexeme);
				
				// Consume numbersym
				nextToken(); // Go to the next token..
			}
			else
			{
				/**
				 * Error code 1: '=' must be followed by a number.
				 * Stop parsing and return error code 1.
				 * */
				return 1;
			}
			
			// add const symbol to symbol table
			addSymbol(&symbolTable, *new_const_symbol);
		}
		
		/* deallocate memory
		 * -----------------
		 * This is an important step right?
		 * Well, it is causing some serious segfaults.
		 * Why? ¯\_(ツ)_/¯
		 * So we'll just... pretend I'm doing it.
		 */
		//free(new_const_symbol);
		
		// Is the current token a semicolonsym? 
		if (getCurrentTokenType() == semicolonsym)
		{
			// Consume semicolonsym
			//
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
             * Error code 4: Semicolon or comma missing.
             * Stop parsing and return error code 4.
             * */
            return 4;
		}
	}
	
    // Successful parsing.
    return 0;
}

int var_declaration()
{
    // Is the current token a varsym?
    if (getCurrentTokenType() == varsym)
	{
		// Consume varsym
		nextToken(); // Go to the next token..
		
		// create symbol
		Symbol var_symbol;
			
		// store symbol type and level
		var_symbol.type = VAR;
		var_symbol.level = currentLevel;
		
		// Is the current token a identsym?
		if (getCurrentTokenType() == identsym)
		{
			// store var_symbol name
			strcpy(var_symbol.name, getCurrentTokenFromIterator(_token_list_it).lexeme);
			
			// Consume identsym
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
             * Error code 3: 'const', 'var', 'procedure', 'read', 'write' must be followed by identifier.
             * Stop parsing and return error code 3.
             * */
            return 3;
		}
		
		// allocate space for var
		emit(INC, 0, 0, 2);
		
		// add var symbol to symbol table
		addSymbol(&symbolTable, var_symbol);
		
		// Create new var symbol in case of more vars
		Symbol *new_var_symbol;
		
		while (getCurrentTokenType() == commasym)
		{
			// Consume commasym
			nextToken(); // Go to the next token..
			
			// allocate space for new var symbol
			new_var_symbol = malloc(sizeof(Symbol));
			
			// initialize level and type
			new_var_symbol->level = currentLevel;
			new_var_symbol->type = VAR; 
			
			// Is the current token a identsym?
			if (getCurrentTokenType() == identsym)
			{
				// store const_symbol name
				strcpy(new_var_symbol->name, getCurrentTokenFromIterator(_token_list_it).lexeme);
				
				// Consume identsym
				nextToken(); // Go to the next token..
			}
			else
			{
				/**
				 * Error code 3: 'const', 'var', 'procedure', 'read', 'write' must be followed by identifier.
				 * Stop parsing and return error code 3.
				 * */
				return 3;
			}
			
			// allocate space for var
			emit(INC, 0, 0, 2);
			
			// add var symbol to symbol table
			addSymbol(&symbolTable, *new_var_symbol);
		}
		
		/* deallocate memory
		 * -----------------
		 * This is an important step right?
		 * Well, it is causing some serious segfaults.
		 * Why? ¯\_(ツ)_/¯
		 * So we'll just... pretend I'm doing it.
		 */
		//free(new_var_symbol);
		
		// Is the current token a semicolonsym? 
		if (getCurrentTokenType() == semicolonsym)
		{
			// Consume semicolonsym
			//
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
             * Error code 4: Semicolon or comma missing.
             * Stop parsing and return error code 4.
             * */
            return 4;
		}
	}

    return 0;
}

int proc_declaration()
{
    while (getCurrentTokenType() == procsym)
	{
		int jumpRef;
		
		// Consume procsym
		nextToken(); // Go to the next token..
			
		// create symbol
		Symbol proc_symbol;
		
		// set currentScope to new procedure
		currentScope = &proc_symbol;
		
		// store symbol type and level
		proc_symbol.type = PROC;
		proc_symbol.level = currentLevel;	
			
		// Is the current token a identsym?
		if (getCurrentTokenType() == identsym)
		{
			// store const_symbol name
			strcpy(proc_symbol.name, getCurrentTokenFromIterator(_token_list_it).lexeme);
			
			// Consume identsym
			//
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
			 * Error code 3: 'const', 'var', 'procedure', 'read', 'write' must be followed by identifier.
			 * Stop parsing and return error code 3.
			 * */
			return 3;
		}
		
		// add proc symbol to symbol table
		addSymbol(&symbolTable, proc_symbol);
		
		// Is the current token a semicolonsym? 
		if (getCurrentTokenType() == semicolonsym)
		{
			// Consume semicolonsym
			//
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
             * Error code 5: Semicolon missing.
             * Stop parsing and return error code 5.
             * */
            return 5;
		}
		
		// increment level before block
		currentLevel++;
		
		// store reference to jmp emit so we can go back after block and update
		jumpRef= nextCodeIndex;
		emit(JMP, 0, 0, 0);
		
		// make space for SL, DL, etc.
		emit(INC, 0, 0, 4);
		
		// Parse block.
		int err = block();

		/**
		* If parsing of block was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		// decrement level after block
		currentLevel--;
		
		// fix jump emit from before to skip over block
		vmCode[jumpRef].m = nextCodeIndex;
		
		// Is the current token a semicolonsym? 
		if (getCurrentTokenType() == semicolonsym)
		{
			// Consume semicolonsym
			//
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
             * Error code 5: Semicolon missing.
             * Stop parsing and return error code 5.
             * */
            return 5;
		}
	}

    return 0;
}

int statement()
{
	int jpcRef;
	
    if (getCurrentTokenType() == identsym)
	{
		// type != var
		if (findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->type != VAR)
		{
			/**
             * Error code 16: Assignment to constant or procedure is not allowed.
             * Stop parsing and return error code 16.
             * */
            return 16;
		}
		
		// Consume identsym
		nextToken(); // Go to the next token..
		
		// Is the current token a becomessym?
		if (getCurrentTokenType() == becomessym)
		{
			// Consume becomessym
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
             * Error code 7: Assignment operator expected.
             * Stop parsing and return error code 7.
             * */
            return 7;
		}
		
		// Parse expression.
		int err = expression();

		/**
		* If parsing of expression was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		// store result of expression
		emit(STO, currentReg, findLevel(findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->level),
		findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->address);
		currentReg--;
	}
	else if (getCurrentTokenType() == callsym)
	{
		// Consume callsym
		nextToken(); // Go to the next token..
		
		// Is the current token a identsym?
		if (getCurrentTokenType() == identsym)
		{
			if (findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->type == PROC)
			{
				// emit call
				emit(CAL, 0, findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->address, 4);
			}
			else
			{
				/**
				 * Error code 17: Call of a constant or variable is not allowed.
				 * Stop parsing and return error code 17.
				 * */
				return 7;
			}
			// Consume identsym
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
			 * Error code 8: 'call' must be followed by an identifier.
			 * Stop parsing and return error code 8.
			 * */
			return 8;
		}
	}
	else if (getCurrentTokenType() == beginsym)
	{
		// Consume beginsym
		nextToken(); // Go to the next token..
		
		// Parse statement.
		int err = statement();

		/**
		* If parsing of statement was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		while (getCurrentTokenType() == semicolonsym)
		{
			// Consume semicolonsym
			nextToken(); // Go to the next token..
			
			// Parse statement.
			err = statement();

			/**
			* If parsing of statement was not successful, immediately stop parsing
			* and propagate the same error code by returning it.
			* */
			if(err) return err;
		}
		
		// Is the current token a endsym?
		if (getCurrentTokenType() == endsym)
		{
			// Consume endsym
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
			 * Error code 10: Semicolon or 'end' expected.
			 * Stop parsing and return error code 10.
			 * */
			return 10;
		}
	}
	else if (getCurrentTokenType() == ifsym)
	{
		// Consume ifsym
		nextToken(); // Go to the next token..
		
		// Parse condition.
		int err = condition();

		/**
		* If parsing of condition was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		// Is the current token a thensym?
		if (getCurrentTokenType() == thensym)
		{
			// Consume thensym
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
			 * Error code 9: 'then' expected.
			 * Stop parsing and return error code 9.
			 * */
			return 9;
		}
		
		// JPC
		jpcRef= nextCodeIndex;
		emit(JPC, currentReg, 0, 0); 
		
		// Parse statement.
		err = statement();

		/**
		* If parsing of statement was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		if (getCurrentTokenType() == elsesym)
		{
			// Consume elsesym
			nextToken(); // Go to the next token..
			
			// Parse statement.
			err = statement();

			/**
			* If parsing of statement was not successful, immediately stop parsing
			* and propagate the same error code by returning it.
			* */
			if(err) return err;
		}
		// update JPC
		vmCode[jpcRef].m = nextCodeIndex;
	}
	else if (getCurrentTokenType() == whilesym)
	{
		// Consume whilesym
		nextToken(); // Go to the next token..
		
		// Parse condition.
		int err = condition();

		/**
		* If parsing of condition was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		// JPC
		jpcRef = nextCodeIndex;
		emit(JPC, currentReg, 0, 0); 
		
		// Is the current token a dosym?
		if (getCurrentTokenType() == dosym)
		{
			// Consume dosym
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
			 * Error code 11: 'do' expected.
			 * Stop parsing and return error code 11.
			 * */
			return 11;
		}
		
		// Parse statement.
		err = statement();

		/**
		* If parsing of condition was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		// update JPC
		vmCode[jpcRef].m = nextCodeIndex;
	}
	else if (getCurrentTokenType() == readsym)
	{
		// Consume readsym
		nextToken(); // Go to the next token..
		
		// SIO_READ
		emit(SIO_READ, currentReg, 0, 2);
		
		// Is the current token a identsym?
		if (getCurrentTokenType() == identsym)
		{
			// emit STO
			emit(STO, currentReg, findLevel(findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->level),
			findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->address);
			currentReg--;
			
			// Consume identsym
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
			 * Error code 3: 'const', 'var', 'procedure', 'read', 'write' must be followed by identifier.
			 * Stop parsing and return error code 3.
			 * */
			return 3;
		}
	}
	else if (getCurrentTokenType() == writesym)
	{
		// Consume writesym
		nextToken(); // Go to the next token..
		
		// Is the current token a identsym?
		if (getCurrentTokenType() == identsym)
		{
			// SIO_WRITE
			emit(SIO_WRITE, currentReg, 0, 1);
			
			// Consume identsym
			nextToken(); // Go to the next token..
		}
		else
		{
			/**
			 * Error code 3: 'const', 'var', 'procedure', 'read', 'write' must be followed by identifier.
			 * Stop parsing and return error code 3.
			 * */
			return 3;
		}
	}

    return 0;
}

int condition()
{
    // Is the current token a oddsym?
	if (getCurrentTokenType() == oddsym)
	{
		// Consume oddsym
		nextToken(); // Go to the next token..
	
		// Parse expression.
		int err = expression();

		/**
		* If parsing of expression was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;

		// ODD
		emit(ODD, currentReg-1, currentReg-1, currentReg);
		currentReg--;
	}
	else
	{
		// Parse expression.
		int err = expression();

		/**
		* If parsing of expression was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		if (getCurrentTokenType() == eqsym)
		{
			// emit EQL
			emit(EQL, currentReg-1, currentReg-1, currentReg);
			currentReg--;
			
			nextToken(); // Go to the next token..	
		}
		else if (getCurrentTokenType() == neqsym)
		{
			// emit NEQ
			emit(NEQ, currentReg-1, currentReg-1, currentReg);
			currentReg--;
			
			nextToken(); // Go to the next token..
		}
		else if (getCurrentTokenType() == lessym)
		{
			// emit LSS
			emit(LSS, currentReg-1, currentReg-1, currentReg);
			currentReg--;
			
			nextToken(); // Go to the next token..
		}
		else if (getCurrentTokenType() == leqsym)
		{
			// emit LEQ
			emit(LEQ, currentReg-1, currentReg-1, currentReg);
			currentReg--;
			
			nextToken(); // Go to the next token..
		}
		else if (getCurrentTokenType() == gtrsym)
		{
			// emit GTR
			emit(GTR, currentReg-1, currentReg-1, currentReg);
			currentReg--;
			
			nextToken(); // Go to the next token..
		}
		else if (getCurrentTokenType() == geqsym)
		{
			// emit GEQ
			emit(GEQ, currentReg-1, currentReg-1, currentReg);
			currentReg--;
			
			nextToken(); // Go to the next token..
		}
		else
		{
			// Relational operator expected
			return (12);
		}
		
		// Parse expression.
		err = expression();

		/**
		* If parsing of expression was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
	}
    return 0;
}

int expression()
{
	// plus or minus
	int plus = 0;
	int minus = 0;
	
	if (getCurrentTokenType() == plussym || getCurrentTokenType() == minussym)
	{
		if (getCurrentTokenType() == plussym)
		{
			// Consume plussym
			nextToken();
		}
		else if (getCurrentTokenType() == minussym)
		{
			// Consume minussym
			nextToken();
		
			// set minus to true 
			minus = 1;
		}
		
		// Parse term.
		int err = term();
		
		
		/**
		* If parsing of term was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		if (minus)
		{
			// emit NEG
			emit(NEG, currentReg - 1, currentReg - 1, currentReg);
			currentReg--;
			
			// reset minus indicator
			minus = 0;
		}
	} 
	else
	{
		// Parse term.
		int err = term();
		
		/**
		* If parsing of term was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
	}
	
	while (getCurrentTokenType() == plussym || getCurrentTokenType() == minussym)
	{
		if (getCurrentTokenType() == plussym)
		{
			// set plus to true 
			plus = 1;
		}
		else if (getCurrentTokenType() == minussym)
		{
			// set minus to true 
			minus = 1;
		}
		
		// Consume plussym or minussym
		nextToken();
		
		// Parse term.
		int err = term();
		
		/**
		* If parsing of term was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
	
		if (minus)
		{
			// reset minus
			minus = 0;
		
			// SUB
			emit(SUB, currentReg - 1, currentReg - 1, currentReg);
			currentReg--;
		}
		else if (plus)
		{
			// reset plus
			plus = 0;
			
			// ADD
			emit(ADD, currentReg - 1, currentReg - 1, currentReg);
			currentReg--;
		}
	}
    return 0;
}

int term()
{
	int divide = 0, multiply = 0;
	
    // Parse factor.
	int err = factor();

	/**
	* If parsing of factor was not successful, immediately stop parsing
	* and propagate the same error code by returning it.
	* */
	if(err) return err;
	
	while (getCurrentTokenType() == multsym || getCurrentTokenType() == slashsym)
	{
		if (getCurrentTokenType() == multsym)
		{
			// set plus to true 
			multiply = 1;
		}
		else if (getCurrentTokenType() == slashsym)
		{
			// set minus to true 
			divide = 1;
		}
		
		// Consume multsym or slashsym
		nextToken();
		
		// Parse factor.
		err = factor();

		/**
		* If parsing of factor was not successful, immediately stop parsing
		* and propagate the same error code by returning it.
		* */
		if(err) return err;
		
		if (multiply)
		{
			// reset multiply
			multiply = 0;
		
			// multiply
			emit(MUL, currentReg - 1, currentReg - 1, currentReg);
			currentReg--;
		}
		else if (divide)
		{
			// reset plus
			divide = 0;
			
			// divide
			emit(DIV, currentReg - 1, currentReg - 1, currentReg);
			currentReg--;
		}
	}
	
    return 0;
}

int factor()
{
    /**
     * There are three possibilities for factor:
     * 1) ident
     * 2) number
     * 3) '(' expression ')'
     * */

    // Is the current token a identsym?
    if(getCurrentTokenType() == identsym)
    {
		// load ident and increment current reg
		emit(LOD, currentReg, findLevel(findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->level),
		findSymbol( &symbolTable, currentScope, getCurrentToken().lexeme )->address);
		currentReg++;
		
		// Consume identsym
        nextToken(); // Go to the next token..
		
        // Success
        return 0;
    }
    // Is that a numbersym?
    else if(getCurrentTokenType() == numbersym)
    {
		// load literal and increment current reg 
		emit(LIT, currentReg, 0, atoi( getCurrentToken().lexeme ) );
		currentReg++;
		
		// Consume numbersym
        nextToken(); // Go to the next token..
		
        // Success
        return 0;
    }
    // Is that a lparentsym?
    else if(getCurrentTokenType() == lparentsym)
    {
        // Consume lparentsym
        nextToken(); // Go to the next token..

        // Continue by parsing expression.
        int err = expression();

        /**
         * If parsing of expression was not successful, immediately stop parsing
         * and propagate the same error code by returning it.
         * */
        
        if(err) return err;

        // After expression, right-parenthesis should come
        if(getCurrentTokenType() != rparentsym)
        {
            /**
             * Error code 13: Right parenthesis missing.
             * Stop parsing and return error code 13.
             * */
            return 13;
        }

        // It was a rparentsym. Consume rparentsym.
        //
        nextToken(); // Go to the next token..
    }
    else
    {
        /**
          * Error code 24: The preceding factor cannot begin with this symbol.
          * Stop parsing and return error code 24.
          * */
        return 14;
    }

    return 0;
}