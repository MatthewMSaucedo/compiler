#include "lexical_analyzer.h"
#include "data.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // Declares isalpa, isdigit, isalnum

/* ************************************************************************** */
/* Enumarations, Typename Aliases, Helpers Structs ************************** */
/* ************************************************************************** */

typedef enum {
    ALPHA,   // a, b, .. , z, A, B, .. Z
    DIGIT, // 0, 1, .. , 9
    SPECIAL, // '>', '=', , .. , ';', ':'
    INVALID  // Invalid symbol
} SymbolType;

/**
 * Following struct is recommended to use to keep track of the current state
 * .. of the lexer, and modify the state in other functions by passing pointer
 * .. to the state as argument.
 * */
typedef struct {
    int lineNum;         // the line number currently being processed
    int charInd;         // the index of the character currently being processed
    char* sourceCode;    // null-terminated source code string
    LexErr lexerError;   // LexErr to be filled when Lexer faces an error
    TokenList tokenList; // list of tokens
} LexerState;

/* ************************************************************************** */
/* Declarations ************************************************************* */
/* ************************************************************************** */

/**
 * Initializes the LexerState with the given null-terminated source code string.
 * Sets the other fields of the LexerState to their inital values.
 * Shallow copying is done for the source code field.
 * */
void initLexerState(LexerState*, char* sourceCode);

/**
 * Returns 1 if the given character is valid.
 * Returns 0 otherwise.
 * */
int isCharacterValid(char);

/**
 * Returns 1 if the given character is one of the special symbols of PL/0,
 * .. such as '/', '=', ':' or ';'.
 * Returns 0 otherwise.
 * */
int isSpecialSymbol(char);

/**
 * Returns the symbol type of the given character.
 * */
SymbolType getSymbolType(char);

/**
 * Checks if the given symbol is one of the reserved token.
 * If yes, returns the numerical value assigned to the corresponding token.
 * If not, returns -1.
 * For example, calling the function with symbol "const" returns 28.
 * */
int checkReservedTokens(char* symbol);

/**
 * Deterministic-finite-automaton to be entered when an alpha character is seen.
 * Simulating a state machine, consumes the source code and changes the state
 * .. of the lexer (LexerState) as required. Possibly, adds new tokens to the
 * .. token list field of the LexerState.
 * If an error is encountered, sets the LexErr field of LexerState, sets the
 * .. line number field and returns.
 * */
void DFA_Alpha(LexerState*);

/**
 * Deterministic-finite-automaton to be entered when a digit character is seen.
 * Simulating a state machine, consumes the source code and changes the state
 * .. of the lexer (LexerState) as required. Possibly, adds new tokens to the
 * .. token list field of the LexerState.
 * If an error is encountered, sets the LexErr field of LexerState, sets the
 * .. line number field and returns.
 * */
void DFA_Digit(LexerState*);

/**
 * Deterministic-finite-automaton to be entered when a special character is seen.
 * Simulating a state machine, consumes the source code and changes the state
 * .. of the lexer (LexerState) as required. Possibly, adds new tokens to the
 * .. token list field of the LexerState.
 * If an error is encountered, sets the LexErr field of LexerState, sets the
 * .. line number field and returns.
 * */
void DFA_Special(LexerState*);

/* ************************************************************************** */
/* Definitions ************************************************************** */
/* ************************************************************************** */

void initLexerState(LexerState* lexerState, char* sourceCode)
{
    lexerState->lineNum = 0;
    lexerState->charInd = 0;
    lexerState->sourceCode = sourceCode;
    lexerState->lexerError = NONE;
    
    initTokenList(&lexerState->tokenList);
}

int isCharacterValid(char c)
{
    return isalnum(c) || isspace(c) || isSpecialSymbol(c);
}

int isSpecialSymbol(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '(' || c == ')' || c == '=' || c == ',' ||
           c == '.' || c == '<' || c == '>' || c == ';' ||
           c == ':';
}

SymbolType getSymbolType(char c)
{
         if(isalpha(c))         return ALPHA;
    else if(isdigit(c))         return DIGIT;
    else if(isSpecialSymbol(c)) return SPECIAL;
    else                        return INVALID;
}

int checkReservedTokens(char* symbol)
{
    for(int i = firstReservedToken; i <= lastReservedToken; i++)
    {
        if( !strcmp(symbol, tokens[i]) )
        {
            // Symbol is the reserved token at index i.
            return i;
        }
    }

    // Symbol is not found among the reserved tokens
    return -1;
}


/**
 * Deterministic-finite-automaton to be entered when an alpha character is seen.
 * Simulating a state machine, consumes the source code and changes the state
 * .. of the lexer (LexerState) as required. Possibly, adds new tokens to the
 * .. token list field of the LexerState.
 * If an error is encountered, sets the LexErr field of LexerState, sets the
 * .. line number field and returns.
 * */
void DFA_Alpha(LexerState* lexerState)
{
    // There are two possible cases for symbols starting with alpha:
    // Case.1) A reversed token (a reserved word or 'odd')
    // Case.2) An ident

    // In both cases, symbol should not exceed 11 characters.
    // Read 11 or less alpha-numeric characters
    // If it exceeds 11 alnums, fill LexerState error and return
    // Otherwise, try to recognize if the symbol is reserved.
    //   If yes, tokenize by one of the reserved symbols
    //   If not, tokenize as ident.

    // For adding a token to tokenlist, you could create a token, fill its 
    // .. fields as required and use the following call:
    // addToken(&lexerState->tokenList, token);

	int i = 0, reservedToken;
	char c, lexeme[MAX_LEXEME_LENGTH + 1];
	
	// while the next char is alphanumeric or a digit
	// .. we must consider it as one lexeme
	for (i = 1, c = lexerState->sourceCode[lexerState->charInd]; 
		isalpha(c) || isdigit(c);
		i++, c = lexerState->sourceCode[lexerState->charInd])
	{
		// increment charInd
		lexerState->charInd++;
			
		if (i-1 > MAX_LEXEME_LENGTH)
		{
			// fill LexerState error and return
			lexerState->lexerError = NAME_TOO_LONG;
			return;
		}
		else
		{
			// store as lexeme
			lexeme[i-1] = c;
		}
	}
	
	// debug
	i--;
	
	// terminate lexeme
	lexeme[i] = '\0';
	
	// check is lexeme is greater than 11 characters
	if ( i > 11)
	{
		// fill LexerState error and return
		lexerState->lexerError = NAME_TOO_LONG;
		return;
	}
	
	//check if lexeme is a reserved word
	if (checkReservedTokens(lexeme) != -1)
	{
		// save value of reserved token in tokens[] array
		reservedToken = checkReservedTokens(lexeme);
	
		// create token
		Token token;
		token.id = reservedToken;
		strcpy(token.lexeme, lexeme);
		
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	else
	{
		// create token
		Token token;
		token.id = 2;
		strcpy(token.lexeme, lexeme);
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}

    return;
}


/**
 * Deterministic-finite-automaton to be entered when a digit character is seen.
 * Simulating a state machine, consumes the source code and changes the state
 * .. of the lexer (LexerState) as required. Possibly, adds new tokens to the
 * .. token list field of the LexerState.
 * If an error is encountered, sets the LexErr field of LexerState, sets the
 * .. line number field and returns.
 * */
void DFA_Digit(LexerState* lexerState)
{
    // There are three cases for symbols starting with number:
    // Case.1) It is a well-formed number
    // Case.2) It is an ill-formed number exceeding 5 digits - Lexer Error!
    // Case.3) It is an ill-formed variable name starting with digit - Lexer Error!

    // Tokenize as numbersym only if it is case 1. Otherwise, set the required
    // .. fields of lexerState to corresponding LexErr and return.

    // For adding a token to tokenlist, you could create a token, fill its 
    // .. fields as required and use the following call:
    // addToken(&lexerState->tokenList, token);

	int i = 0, yesAlpha = 0, reservedToken;
	char c, lexeme[MAX_LEXEME_LENGTH + 1];
	
	// while the next char is alphanumeric or a digit
	// .. we must consider it as one lexeme
	for (i = 1, c = lexerState->sourceCode[lexerState->charInd]; 
		isalpha(c) || isdigit(c);
		i++, c = lexerState->sourceCode[lexerState->charInd])
	{
		// increment charInd
		lexerState->charInd++;
			
		// make note if character isalpha
		if (isalpha(c))
		{
			yesAlpha = 1;
		}
		
		if (i-1 > MAX_LEXEME_LENGTH)
		{
			// fill LexerState error and return
			lexerState->lexerError = NUM_TOO_LONG;
			return;
		}
		else
		{
			// store as lexeme
			lexeme[i-1] = c;
		}
		
		// check if lexeme is a well-formed number
		if (yesAlpha)
		{
			// fill LexerState error and return
			lexerState->lexerError = NONLETTER_VAR_INITIAL;
			return;
		}
	}
	
	// debug
	i--;
	
	// terminate lexeme
	lexeme[i] = '\0';
	
	// check is lexeme is greater than 5 digits
	if ( i > 5)
	{
		// fill LexerState error and return
		lexerState->lexerError = NUM_TOO_LONG;
		return;
	}
	
	// create token
	Token token;
	token.id = 3;
	strcpy(token.lexeme, lexeme);
	
	// add token to list
	addToken(&lexerState->tokenList, token);
		
    return;
}

void DFA_Special(LexerState* lexerState)
{
    // There are three cases for symbols starting with special:
    // Case.1: Beginning of a comment: "/*"
    // Case.2: Two character special symbol: "<>", "<=", ">=", ":="
    // Case.3: One character special symbol: "+", "-", "(", etc.

    // For case.1, you are recommended to consume all the characters regarding
    // .. the comment, and return. This way, lexicalAnalyzer() func can decide
    // .. what to do with the next character.

    // For case.2 and case.3, you could consume the characters, add the 
    // .. corresponding token to the tokenlist of lexerState, and return.

    // For adding a token to tokenlist, you could create a token, fill its 
    // .. fields as required and use the following call:
    // addToken(&lexerState->tokenList, token);

	int i = 0, exit = 0, reservedToken;
	
	// check for the two character symbols
	// check for /*
	if (lexerState->sourceCode[lexerState->charInd] == '/')
	{
		//check to see if there is a following asterisk
		if (lexerState->sourceCode[lexerState->charInd + 1] == '*')
		{
			lexerState->charInd++;
			
			// we need to move forward until we see a terminating */
			while (exit == 0)
			{
				lexerState->charInd++;
				
				// check for an additional asterisk
				if (lexerState->sourceCode[lexerState->charInd] == '*')
				{
					// check if next char is a '/'
					if (lexerState->sourceCode[lexerState->charInd + 1] == '/')
					{
						// this is the end of the comment.
						exit = 1;
						
						// move charInd to after the comment
						// /* ... */ 
						// _________^  this is where charIndex will now point
						lexerState->charInd++;
						lexerState->charInd++;
						return;
					}
				}
			}
		}
		// it is the slashsym
		else
		{
			// create token
			Token token;
			token.id = slashsym;
			strcpy(token.lexeme, "/");
	
			// add token to list
			addToken(&lexerState->tokenList, token);
		}
	}
	// check for <= and <>
	else if (lexerState->sourceCode[lexerState->charInd] == '<')
	{
		//check to see if there is a following =
		if (lexerState->sourceCode[lexerState->charInd + 1] == '=')
		{
			// advance charInd
			lexerState->charInd++;
			
			// create token
			Token token;
			token.id = leqsym;
			strcpy(token.lexeme, "<=");
	
			// add token to list
			addToken(&lexerState->tokenList, token);
		}
		else if (lexerState->sourceCode[lexerState->charInd + 1] == '>')
		{
			// advance charInd
			lexerState->charInd++;
			
			// create token
			Token token;
			token.id = neqsym;
			strcpy(token.lexeme, "<>");
	
			// add token to list
			addToken(&lexerState->tokenList, token);
		}
		else
		{
			// create token
			Token token;
			token.id = lessym;
			strcpy(token.lexeme, "<");
	
			// add token to list
			addToken(&lexerState->tokenList, token);
		}
	}
	// check for :=
	else if (lexerState->sourceCode[lexerState->charInd] == ':')
	{
		// check to see if there is a following =
		if (lexerState->sourceCode[lexerState->charInd + 1] == '=')
		{
			// advance charInd
			lexerState->charInd++;
			
			// create token
			Token token;
			token.id = becomessym;
			strcpy(token.lexeme, ":=");
	
			// add token to list
			addToken(&lexerState->tokenList, token);
		}
		else
		{
			// invalid symbol :
			
			// create token
			/*Token token;
			token.id = sym;
			strcpy(token.lexeme, ":");
	
			// add token to list
			addToken(&lexerState->tokenList, token);
			*/
		}
	}
	// check for >=
	else if (lexerState->sourceCode[lexerState->charInd] == '>')
	{
		// check to see if there is a following =
		if (lexerState->sourceCode[lexerState->charInd + 1] == '=')
		{
			// advance charInd
			lexerState->charInd++;
			
			// create token
			Token token;
			token.id = geqsym;
			strcpy(token.lexeme, ">=");
	
			// add token to list
			addToken(&lexerState->tokenList, token);
		}
		else
		{
			// create token
			Token token;
			token.id = gtrsym;
			strcpy(token.lexeme, ">");
	
			// add token to list
			addToken(&lexerState->tokenList, token);
		}
	}
	// check for +
	else if (lexerState->sourceCode[lexerState->charInd] == '+')
	{
		// create token
		Token token;
		token.id = plussym;
		strcpy(token.lexeme, "+");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	// check for )
	else if (lexerState->sourceCode[lexerState->charInd] == ')')
	{
		// create token
		Token token;
		token.id = rparentsym;
		strcpy(token.lexeme, ")");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	// check for -
	else if (lexerState->sourceCode[lexerState->charInd] == '-')
	{
		// create token
		Token token;
		token.id = minussym;
		strcpy(token.lexeme, "-");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	// check for =
	else if (lexerState->sourceCode[lexerState->charInd] == '=')
	{
		// create token
		Token token;
		token.id = eqsym;
		strcpy(token.lexeme, "=");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	// check for ,
	else if (lexerState->sourceCode[lexerState->charInd] == ',')
	{
		// create token
		Token token;
		token.id = commasym;
		strcpy(token.lexeme, ",");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	// check for * 
	else if (lexerState->sourceCode[lexerState->charInd] == '*')
	{
		// create token
		Token token;
		token.id = multsym;
		strcpy(token.lexeme, "*");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	// check for ;
	else if (lexerState->sourceCode[lexerState->charInd] == ';')
	{
		// create token
		Token token;
		token.id = semicolonsym;
		strcpy(token.lexeme, ";");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	// check for /
	else if (lexerState->sourceCode[lexerState->charInd] == '/')
	{
		// create token
		Token token;
		token.id = slashsym;
		strcpy(token.lexeme, "/");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	// check for (
	else if (lexerState->sourceCode[lexerState->charInd] == '(')
	{
		// create token
		Token token;
		token.id = lparentsym;
		strcpy(token.lexeme, "(");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	// check for .
	else if (lexerState->sourceCode[lexerState->charInd] == '.')
	{
		// create token
		Token token;
		token.id = periodsym;
		strcpy(token.lexeme, ".");
	
		// add token to list
		addToken(&lexerState->tokenList, token);
	}
	
	lexerState->charInd++;
	
    return;
}

LexerOut lexicalAnalyzer(char* sourceCode)
{
    if(!sourceCode)
    {
        fprintf(stderr, "ERROR: Null source code string passed to lexicalAnalyzer()\n");
        
        LexerOut lexerOut;
        lexerOut.lexerError = NO_SOURCE_CODE;
        lexerOut.errorLine = -1;

        return lexerOut;
    }

    // Create & init lexer state
    LexerState lexerState;
    initLexerState(&lexerState, sourceCode);

    // While not end of file, and, there is no lexer error
    // .. continue lexing
    while( lexerState.sourceCode[lexerState.charInd] != '\0' &&
        lexerState.lexerError == NONE )
    {
        char currentSymbol = lexerState.sourceCode[lexerState.charInd];

        // Skip spaces or new lines until an effective character is seen
        while(currentSymbol == ' ' || currentSymbol == '\n')
        {
            // Advance line number if required
            if(currentSymbol == '\n')
                lexerState.lineNum++;

            // Advance to the following character
            currentSymbol = lexerState.sourceCode[++lexerState.charInd];
        }

        // After recognizing spaces or new lines, make sure that the EOF was
        // .. not reached. If it was, break the loop.
        if(lexerState.sourceCode[lexerState.charInd] == '\0')
        {
            break;
        }

        // Take action depending on the current symbol's type
        switch(getSymbolType(currentSymbol))
        {
            case ALPHA:
                DFA_Alpha(&lexerState);
                break;
            case DIGIT:
                DFA_Digit(&lexerState);
                break;
            case SPECIAL:
                DFA_Special(&lexerState);
                break;
            case INVALID:
                lexerState.lexerError = INV_SYM;
                break;
        }
    }

    // Prepare LexerOut to be returned
    LexerOut lexerOut;

    if(lexerState.lexerError != NONE)
    {
        // Set LexErr
        lexerOut.lexerError = lexerState.lexerError;

        // Set the number of line the error encountered
        lexerOut.errorLine = lexerState.lineNum;

        lexerOut.tokenList = lexerState.tokenList;
    }
    else
    {
        // No error!
        lexerOut.lexerError = NONE;
        lexerOut.errorLine = -1;
        
        // Copy the token list

        // The scope of LexerState ends here. The ownership of the tokenlist
        // .. is being passed to LexerOut. Therefore, neither deletion of the
        // .. tokenlist nor deep copying of the tokenlist is required.
        lexerOut.tokenList = lexerState.tokenList;
    }

    return lexerOut;
}
