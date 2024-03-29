#include "../headers/lexer.h"

// global vars
static char* source;
static long source_len;
static long current = 0;
static long start = 0;

// hash table
static Hashlist* hashtab[HASH_SIZE];

unsigned int hash(char* s){
    unsigned int hashval;
    for (hashval = 0; *s != '\0'; s++){
        hashval = *s + 31 * hashval;
    }

    return hashval % HASH_SIZE;
}

char* strdup(char* s){
    char* p = (char*)malloc(strlen(s) + 1);
    if (p != NULL) strcpy(p, s);
    return p;
}

Hashlist* lookup(char* s){
    Hashlist* h;
    for (h = hashtab[hash(s)]; h != NULL; h = h->next){
        if (strcmp(s, h->key) == 0){
            return h;
        }
    }
    return NULL;
}

Hashlist* put(char* key, int val){
    Hashlist* h;
    unsigned int hashval;
    if ((h = lookup(key)) == NULL){
        h = (Hashlist*)malloc(sizeof(*h));
        if (h == NULL || (h->key = strdup(key)) == NULL) return NULL;
        hashval = hash(key);
        h->next = hashtab[hashval];
        hashtab[hashval] = h;
    }
    h->val = val;
    return h;
}

// helper functions
char advance(){
    return source[current++];
}

bool match(char expected){
    if (current >= source_len) return false;
    if (source[current] != expected) return false;
    current++;
    return true;
}

char peek(){
    if (current >= source_len) return '\0';
    return source[current];
}

char peekNext(){
    if (current + 1 >= source_len) return '\0';
    return source[current + 1];
}

// token list
void initTokenList(TokenList* list){
    list->tokens = (Token*)malloc(sizeof(Token) * LIST_SIZE);
    list->index = 0;
    list->max = LIST_SIZE;
}

void addToken(TokenList* list, enum TokenType type){
    if (list->index == list->max){
        list->max *= 2;
        list->tokens = realloc(list->tokens, sizeof(Token) * list->max);
    }
    char* lexeme = malloc(current - start + 1);
    if (lexeme == NULL){
        fprintf(stderr, "ERROR: malloc failed in addToken\n");
        exit(1);
    }
    long i;
    for (i = start; i < current; i++) {
        lexeme[i - start] = source[i];
    }
    lexeme[i - start] = '\0';

    list->tokens[list->index].type = type;
    list->tokens[list->index].lexeme = lexeme;
    list->index++;
}


void addString(TokenList* list){
    while(peek() != '"' && current <= source_len){
        if (peek() == '\n') {
            fprintf(stderr, "ERROR: unterminated string literal\n");
            exit(1);
        }
        advance();
    }

    if (current >= source_len){
        fprintf(stderr, "ERROR: unterminated string literal\n");
        exit(1);
    }
    advance();
    addToken(list, STRING);
}

void addPreprocessor(TokenList* list){
    for (; peek() != ' ' && current < source_len; advance());
    addToken(list, PREPROC);
}

void addNumber(TokenList* list){
    while (isdigit(peek())) advance();

    if (peek() == '.' && isdigit(peekNext())){
        advance(); // consume dot '.'
        while (isdigit(peek())) advance();

    }
    
    addToken(list, NUMBER);
}

void addIdentifier(TokenList* list){
    while (isalnum(peek()) || peek() == '_') advance();

    char* text = malloc(current - start + 1);
    if (text == NULL) {
        fprintf(stderr, "ERROR: malloc failed in addIdentifier\n");
        exit(1);
    }
    int i;
    for (i = start; i < current; i++) text[i-start] = source[i];
    text[i-start] = '\0';

    Hashlist* h = lookup(text);
    free(text);
    h == NULL ? addToken(list, IDENTIFIER) : addToken(list, h->val);
}

TokenList* tokenize(char* input){
    source = input;
    source_len = strlen(input);
    TokenList* list = malloc(sizeof(TokenList));
    initTokenList(list);

    put("const", CONST);
    put("int", INT);
    put("double", DOUBLE);
    put("float", FLOAT);
    put("char", CHAR);
    put("unsigned", UNSIGNED);
    put("long", LONG);
    put("enum", ENUM);
    put("struct", STRUCT);
    put("union", UNION);
    put("typedef", TYPEDEF);
    put("return", RETURN);
    put("do", DO);
    put("while", WHILE);
    put("for", FOR);
    put("if", IF);
    put("else", ELSE);
    put("true", TRUE);
    put("false", FALSE);
    put("NULL", NIL);

    while(current < source_len){
        start = current;
        char c = advance();
        switch (c){
            case '(': addToken(list, LEFT_PAREN); break;
            case ')': addToken(list, RIGHT_PAREN); break;
            case '{': addToken(list, LEFT_BRACE); break;
            case '}': addToken(list, RIGHT_BRACE); break;
            case '[': addToken(list, LEFT_BRACKET); break;
            case ']': addToken(list, RIGHT_BRACKET); break;
            case '-': addToken(list, match('-') ? DECR : match('=') ? MINUS_EQUAL : MINUS); break;
            case '+': addToken(list, match('+') ? INCR : match('=') ? PLUS_EQUAL : PLUS); break;
            case '*': addToken(list, match('=') ? STAR_EQUAL : STAR); break;
            case '%': addToken(list, match('=') ? MODULO_EQUAL : MODULO); break;
            case '/': addToken(list, match('=') ? SLASH_EQUAL : SLASH); break;
            case '<': addToken(list, match('<') ? LEFT_SHIFT : match('=') ? LESS_EQUAL : LESS); break;
            case '>': addToken(list, match('>') ? RIGHT_SHIFT : match('=') ? GREATER_EQUAL : GREATER); break;
            case '=': addToken(list, match('=') ? EQUAL_EQUAL : EQUAL); break;
            case '!': addToken(list, match('=') ? BANG_EQUAL : BANG); break;
            case ';': addToken(list, SEMICOLON); break;
            case '.': addToken(list, DOT); break;
            case ',': addToken(list, COMMA); break;
            case '&': addToken(list, match('&') ? AND : BIT_AND); break;
            case '|': addToken(list, match('|') ? OR : BIT_OR); break;
            case '^': addToken(list, XOR); break;
            case '~': addToken(list, BIT_NOT); break;

            case ' ': {
                for (; peek() == ' ' && current < source_len; advance());
                addToken(list, WHITE_SPACE);
            } break;
            case '\r': addToken(list, CARR_SPACE); break;
            case '\t': addToken(list, TAB_SPACE); break;
            case '\n': addToken(list, NEW_LINE); break;

            case '\0': addToken(list, ENDFILE); return list;

            case '"': addString(list); break;
            case '#': addPreprocessor(list); break;

            default: {
                if (isdigit(c)){
                    addNumber(list);
                } else if (isalpha(c)) {
                    addIdentifier(list);
                } else {
                    fprintf(stderr, "Unexpected character %c\n", c);
                    exit(1);
                }
            } break;
        }
    }
    
    return list;
}