#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_SYMBOLS 1000
#define MAX_OPCODES 200
#define MAX_LINES 10000
#define MAX_LINE_LENGTH 1024
#define MAX_MNEMONIC 10
#define MAX_OPERAND 50
#define MAX_OBJECT_CODE 20
#define HASH_TABLE_SIZE 211

#define DEFAULT_PROG_NAME "DEFAULT"
#define DEFAULT_START_ADDR 0

typedef struct Symbol
{
    char label[MAX_OPERAND];
    int address;
    struct Symbol *next;
} Symbol;

typedef struct
{
    char mnemonic[MAX_MNEMONIC];
    char opcode[3];
    int format;
} OpcodeEntry;

typedef struct
{
    int lineNum;
    char label[MAX_OPERAND];
    char opcode[MAX_MNEMONIC];
    char operand[MAX_OPERAND];
    int address;
    char objectCode[MAX_OBJECT_CODE];
    char errorMsg[256];
} LineInfo;

Symbol *symbolTable[HASH_TABLE_SIZE] = {NULL};
OpcodeEntry opcodeTable[MAX_OPCODES];
int opcodeCount = 0;

void initializeOpcodeTable();
int lookupOpcode(const char *mnemonic, char *opcode, int *format);
int addSymbol(const char *label, int address);
int lookupSymbol(const char *label);
unsigned int hash(const char *str);
void toUpperCase(char *str);
void parseLine(char *line, LineInfo *lineInfo);
int isCommentOrEmpty(const char *line);
void passOne(FILE *srcFile, LineInfo lines[], int *lineCount, int *startAddress, int *progLength, char *progName);
void passTwo(LineInfo lines[], int lineCount, int startAddress, int progLength, const char *progName, FILE *objFile, FILE *lstFile);
void trim(char *str);

void trim(char *str)
{
    char *start = str;
    while (isspace(*start))
        start++;
    memmove(str, start, strlen(start) + 1);
    int i;
    for (i = strlen(str) - 1; (i >= 0) && isspace(str[i]); i--)
        str[i] = '\0';
}

unsigned int hash(const char *str)
{
    unsigned int hash = 0;
    while (*str)
        hash = (hash << 3) + toupper(*str++);
    return hash % HASH_TABLE_SIZE;
}

void toUpperCase(char *str)
{
    for (; *str; ++str)
        *str = toupper(*str);
}

void initializeOpcodeTable()
{
    strcpy(opcodeTable[opcodeCount].mnemonic, "CLEAR");
    strcpy(opcodeTable[opcodeCount].opcode, "B4");
    opcodeTable[opcodeCount].format = 2;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "TIXR");
    strcpy(opcodeTable[opcodeCount].opcode, "B8");
    opcodeTable[opcodeCount].format = 2;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "LDA");
    strcpy(opcodeTable[opcodeCount].opcode, "00");
    opcodeTable[opcodeCount].format = 3;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "STA");
    strcpy(opcodeTable[opcodeCount].opcode, "0C");
    opcodeTable[opcodeCount].format = 3;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "STL");
    strcpy(opcodeTable[opcodeCount].opcode, "14");
    opcodeTable[opcodeCount].format = 3;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "JSUB");
    strcpy(opcodeTable[opcodeCount].opcode, "48");
    opcodeTable[opcodeCount].format = 3;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "COMP");
    strcpy(opcodeTable[opcodeCount].opcode, "28");
    opcodeTable[opcodeCount].format = 3;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "JEQ");
    strcpy(opcodeTable[opcodeCount].opcode, "30");
    opcodeTable[opcodeCount].format = 3;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "J");
    strcpy(opcodeTable[opcodeCount].opcode, "3C");
    opcodeTable[opcodeCount].format = 3;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "LDL");
    strcpy(opcodeTable[opcodeCount].opcode, "08");
    opcodeTable[opcodeCount].format = 3;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "RSUB");
    strcpy(opcodeTable[opcodeCount].opcode, "4C");
    opcodeTable[opcodeCount].format = 3;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "+JSUB");
    strcpy(opcodeTable[opcodeCount].opcode, "48");
    opcodeTable[opcodeCount].format = 4;
    opcodeCount++;

    strcpy(opcodeTable[opcodeCount].mnemonic, "+LDA");
    strcpy(opcodeTable[opcodeCount].opcode, "00");
    opcodeTable[opcodeCount].format = 4;
    opcodeCount++;
}

int lookupOpcode(const char *mnemonic, char *opcode, int *format)
{
    for (int i = 0; i < opcodeCount; i++)
    {
        if (strcmp(opcodeTable[i].mnemonic, mnemonic) == 0)
        {
            if (opcode != NULL)
                strcpy(opcode, opcodeTable[i].opcode);
            if (format != NULL)
                *format = opcodeTable[i].format;
            return 1;
        }
    }
    return 0;
}

int addSymbol(const char *label, int address)
{
    if (strlen(label) == 0)
        return 1;

    unsigned int index = hash(label);
    Symbol *current = symbolTable[index];
    while (current)
    {
        if (strcmp(current->label, label) == 0)
        {
            return 0;
        }
        current = current->next;
    }

    Symbol *newSymbol = (Symbol *)malloc(sizeof(Symbol));
    if (!newSymbol)
    {
        fprintf(stderr, "Memory allocation error for symbol '%s'\n", label);
        exit(1);
    }
    strcpy(newSymbol->label, label);
    newSymbol->address = address;
    newSymbol->next = symbolTable[index];
    symbolTable[index] = newSymbol;
    return 1;
}

int lookupSymbol(const char *label)
{
    unsigned int index = hash(label);
    Symbol *current = symbolTable[index];
    while (current)
    {
        if (strcmp(current->label, label) == 0)
            return current->address;
        current = current->next;
    }
    return -1;
}

void parseLine(char *line, LineInfo *lineInfo)
{
    lineInfo->lineNum = 0;
    lineInfo->label[0] = '\0';
    lineInfo->opcode[0] = '\0';
    lineInfo->operand[0] = '\0';
    lineInfo->objectCode[0] = '\0';
    lineInfo->errorMsg[0] = '\0';

    char *token = strtok(line, " \t\n");
    if (!token)
        return;

    char tempMnemonic[MAX_MNEMONIC];
    strcpy(tempMnemonic, token);
    toUpperCase(tempMnemonic);

    if (lookupOpcode(tempMnemonic, NULL, NULL) ||
        strcmp(tempMnemonic, "START") == 0 ||
        strcmp(tempMnemonic, "END") == 0 ||
        strcmp(tempMnemonic, "BYTE") == 0 ||
        strcmp(tempMnemonic, "WORD") == 0 ||
        strcmp(tempMnemonic, "RESW") == 0 ||
        strcmp(tempMnemonic, "RESB") == 0 ||
        strcmp(tempMnemonic, "BASE") == 0 ||
        strcmp(tempMnemonic, "NOBASE") == 0)
    {
        strcpy(lineInfo->opcode, tempMnemonic);
        token = strtok(NULL, " \t\n");
        if (token)
            strcpy(lineInfo->operand, token);
    }
    else
    {
        strcpy(lineInfo->label, token);
        token = strtok(NULL, " \t\n");
        if (token)
        {
            strcpy(lineInfo->opcode, token);
            toUpperCase(lineInfo->opcode);
            token = strtok(NULL, " \t\n");
            if (token)
                strcpy(lineInfo->operand, token);
        }
    }
}

int isCommentOrEmpty(const char *line)
{
    while (*line && isspace(*line))
        line++;
    if (*line == ';' || *line == '\0')
        return 1;
    return 0;
}

void passOne(FILE *srcFile, LineInfo lines[], int *lineCount, int *startAddress, int *progLength, char *progName)
{
    char line[MAX_LINE_LENGTH];
    int locctr = 0;
    int lineNum = 0;
    int firstExecAddress = 0;
    int programStarted = 0;

    while (fgets(line, sizeof(line), srcFile))
    {
        lineNum++;
        if (isCommentOrEmpty(line))
            continue;

        parseLine(line, &lines[*lineCount]);
        lines[*lineCount].lineNum = lineNum;

        if (strcmp(lines[*lineCount].opcode, "START") == 0)
        {
            if (lines[*lineCount].operand[0] != '\0')
            {
                locctr = (int)strtol(lines[*lineCount].operand, NULL, 16);
                *startAddress = locctr;
                if (strlen(lines[*lineCount].label) > 0)
                    strcpy(progName, lines[*lineCount].label);
                else
                    strcpy(progName, DEFAULT_PROG_NAME);
            }
            else
            {
                locctr = DEFAULT_START_ADDR;
                strcpy(progName, DEFAULT_PROG_NAME);
            }
            lines[*lineCount].address = locctr;
            (*lineCount)++;
            programStarted = 1;
            break;
        }
        else
        {
            locctr = DEFAULT_START_ADDR;
            strcpy(progName, DEFAULT_PROG_NAME);
            break;
        }
    }

    while (fgets(line, sizeof(line), srcFile))
    {
        lineNum++;
        if (isCommentOrEmpty(line))
            continue;

        parseLine(line, &lines[*lineCount]);
        lines[*lineCount].lineNum = lineNum;
        lines[*lineCount].address = locctr;

        if (lines[*lineCount].label[0] != '\0')
        {
            if (!addSymbol(lines[*lineCount].label, locctr))
            {
                snprintf(lines[*lineCount].errorMsg, sizeof(lines[*lineCount].errorMsg),
                         "Duplicate or invalid symbol '%s'", lines[*lineCount].label);
            }
        }

        if (strcmp(lines[*lineCount].opcode, "END") == 0)
        {
            if (lines[*lineCount].operand[0] != '\0')
            {
                firstExecAddress = lookupSymbol(lines[*lineCount].operand);
                if (firstExecAddress == -1)
                {
                    snprintf(lines[*lineCount].errorMsg, sizeof(lines[*lineCount].errorMsg),
                             "Undefined symbol '%s'", lines[*lineCount].operand);
                }
            }
            (*lineCount)++;
            break;
        }
        else if (strcmp(lines[*lineCount].opcode, "BYTE") == 0)
        {
            char constant[MAX_OPERAND];
            strcpy(constant, lines[*lineCount].operand);
            if (constant[0] == 'C')
            {
                int len = strlen(constant) - 3;
                locctr += len;
            }
            else if (constant[0] == 'X')
            {
                int len = (strlen(constant) - 3) / 2;
                locctr += len;
            }
        }
        else if (strcmp(lines[*lineCount].opcode, "WORD") == 0)
        {
            locctr += 3;
        }
        else if (strcmp(lines[*lineCount].opcode, "RESW") == 0)
        {
            locctr += 3 * atoi(lines[*lineCount].operand);
        }
        else if (strcmp(lines[*lineCount].opcode, "RESB") == 0)
        {
            locctr += atoi(lines[*lineCount].operand);
        }
        else if (strcmp(lines[*lineCount].opcode, "BASE") == 0 ||
                 strcmp(lines[*lineCount].opcode, "NOBASE") == 0)
        {
        }
        else
        {
            char opcodeStr[3];
            int format;
            if (lookupOpcode(lines[*lineCount].opcode, opcodeStr, &format))
            {
                locctr += format;
            }
            else
            {
                snprintf(lines[*lineCount].errorMsg, sizeof(lines[*lineCount].errorMsg),
                         "Invalid opcode '%s'", lines[*lineCount].opcode);
            }
        }

        (*lineCount)++;
    }

    *progLength = locctr - *startAddress;
}

void passTwo(LineInfo lines[], int lineCount, int startAddress, int progLength, const char *progName, FILE *objFile, FILE *lstFile)
{
    int baseAddress = 0;
    int useBase = 0;

    fprintf(objFile, "H%-6s%06X%06X\n", progName, startAddress, progLength);
    fprintf(lstFile, "H%-6s %06X %06X\n", progName, startAddress, progLength);

    char textRecord[70] = "T";
    int currentRecordStart = 0;
    int currentRecordLength = 0;

    int firstExecAddress = startAddress;

    for (int i = 0; i < lineCount; i++)
    {
        if (strcmp(lines[i].opcode, "START") != 0 &&
            strcmp(lines[i].opcode, "END") != 0 &&
            lines[i].objectCode[0] != '\0')
        {
            firstExecAddress = lines[i].address;
            break;
        }
    }

    for (int i = 0; i < lineCount; i++)
    {
        LineInfo *currentLine = &lines[i];
        char objCode[MAX_OBJECT_CODE] = "";

        if (strcmp(currentLine->opcode, "START") == 0 ||
            strcmp(currentLine->opcode, "END") == 0)
        {
            if (currentRecordLength > 0)
            {
                sprintf(&textRecord[7], "%02X", currentRecordLength);
                fprintf(objFile, "%s\n", textRecord);
                fprintf(lstFile, "%s\n", textRecord);
                // Reset text record
                strcpy(textRecord, "T");
                currentRecordLength = 0;
            }
            continue;
        }

        if (strcmp(currentLine->opcode, "BYTE") == 0)
        {
            char constant[MAX_OPERAND];
            strcpy(constant, currentLine->operand);
            if (constant[0] == 'C')
            {
                char hexStr[2 * MAX_OPERAND] = "";
                for (int j = 2; j < strlen(constant) - 1; j++)
                {
                    char temp[3];
                    sprintf(temp, "%02X", constant[j]);
                    strcat(hexStr, temp);
                }
                strcpy(objCode, hexStr);
            }
            else if (constant[0] == 'X')
            {
                strcpy(objCode, &constant[2]);
            }
        }
        else if (strcmp(currentLine->opcode, "WORD") == 0)
        {
            int value = atoi(currentLine->operand);
            sprintf(objCode, "%06X", value);
        }
        else if (strcmp(currentLine->opcode, "BASE") == 0)
        {
            if (lookupSymbol(currentLine->operand) != -1)
            {
                baseAddress = lookupSymbol(currentLine->operand);
                useBase = 1;
            }
            else
            {
                fprintf(stderr, "Error: Undefined symbol '%s' for BASE directive at line %d\n",
                        currentLine->operand, currentLine->lineNum);
            }
            continue;
        }
        else if (strcmp(currentLine->opcode, "NOBASE") == 0)
        {
            useBase = 0;
            continue;
        }
        else
        {
            char opcodeStr[3];
            int format;
            if (lookupOpcode(currentLine->opcode, opcodeStr, &format))
            {
                if (format == 1)
                {
                    strcpy(objCode, opcodeStr);
                }
                else if (format == 2)
                {

                    strcpy(objCode, opcodeStr);
                }
                else if (format == 3 || format == 4)
                {
                    int ni = 3;
                    int x = 0, b = 0, p = 0, e = 0;
                    int disp = 0;

                    if (currentLine->opcode[0] == '+')
                    {
                        e = 1;
                        format = 4;
                    }

                    char operandCopy[MAX_OPERAND];
                    strcpy(operandCopy, currentLine->operand);
                    toUpperCase(operandCopy);

                    if (operandCopy[0] == '#')
                    {
                        ni = 1;
                        strcpy(operandCopy, &operandCopy[1]);
                    }
                    else if (operandCopy[0] == '@')
                    {
                        ni = 2;
                        strcpy(operandCopy, &operandCopy[1]);
                    }
                    else
                    {
                        ni = 3;
                    }

                    int targetAddress = 0;
                    if (strcmp(operandCopy, "") != 0)
                    {
                        targetAddress = lookupSymbol(operandCopy);
                        if (targetAddress == -1)
                        {
                            fprintf(stderr, "Error: Undefined symbol '%s' at line %d\n",
                                    operandCopy, currentLine->lineNum);
                        }
                    }

                    if (format == 3)
                    {
                        disp = targetAddress - (currentLine->address + 3);
                        if (disp >= -2048 && disp <= 2047)
                        {
                            p = 1;
                        }
                        else if (useBase && (targetAddress - baseAddress) >= 0 && (targetAddress - baseAddress) <= 4095)
                        {
                            disp = targetAddress - baseAddress;
                            b = 1;
                        }
                        else
                        {
                            disp = 0;
                            fprintf(stderr, "Error: Displacement out of range for symbol '%s' at line %d\n",
                                    operandCopy, currentLine->lineNum);
                        }
                    }
                    else if (format == 4)
                    {
                        disp = targetAddress;
                        b = 0;
                        p = 0;
                    }

                    int opcodeInt = (int)strtol(opcodeStr, NULL, 16);
                    opcodeInt = (opcodeInt & 0xFC) | ni;

                    if (format == 3)
                    {
                        int xbpe = (x << 3) | (b << 2) | (p << 1) | e;
                        sprintf(objCode, "%02X%04X", opcodeInt, (xbpe << 12) | (disp & 0xFFF));
                    }
                    else if (format == 4)
                    {
                        e = 1;
                        sprintf(objCode, "%02X%05X", opcodeInt, disp);
                    }
                }
            }
            else
            {
            }
        }

        if (objCode[0] != '\0')
        {
            if (currentRecordLength == 0)
            {
                sprintf(textRecord + 1, "%06X", currentLine->address);
                strcat(textRecord, "00");
                strcat(textRecord, objCode);
                currentRecordLength += strlen(objCode) / 2;
            }
            else
            {
                if ((currentRecordLength + strlen(objCode) / 2) > 30)
                {
                    sprintf(&textRecord[7], "%02X", currentRecordLength);
                    fprintf(objFile, "%s\n", textRecord);
                    fprintf(lstFile, "%s\n", textRecord);

                    strcpy(textRecord, "T");
                    sprintf(textRecord + 1, "%06X", currentLine->address);
                    strcat(textRecord, "00");
                    strcat(textRecord, objCode);
                    currentRecordLength = strlen(objCode) / 2;
                }
                else
                {
                    strcat(textRecord, objCode);
                    currentRecordLength += strlen(objCode) / 2;
                }
            }
        }

        fprintf(lstFile, "%04X  %-6s %-6s %-10s %s\n",
                currentLine->address,
                currentLine->label,
                currentLine->opcode,
                currentLine->operand,
                currentLine->objectCode);

        strcpy(currentLine->objectCode, objCode);
    }

    if (currentRecordLength > 0)
    {
        sprintf(&textRecord[7], "%02X", currentRecordLength);
        fprintf(objFile, "%s\n", textRecord);
        fprintf(lstFile, "%s\n", textRecord);
    }

    fprintf(objFile, "E%06X\n", firstExecAddress);
    fprintf(lstFile, "E %06X\n", firstExecAddress);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <source file path>\n", argv[0]);
        return 1;
    }

    initializeOpcodeTable();

    FILE *srcFile = fopen(argv[1], "r");
    if (!srcFile)
    {
        perror("Error opening source file");
        return 1;
    }

    LineInfo lines[MAX_LINES];
    int lineCount = 0;
    int startAddress = 0;
    int progLength = 0;
    char progName[MAX_OPERAND] = DEFAULT_PROG_NAME;

    passOne(srcFile, lines, &lineCount, &startAddress, &progLength, progName);
    fclose(srcFile);

    FILE *objFile = fopen("output.obj", "w");
    if (!objFile)
    {
        perror("Error creating object file");
        return 1;
    }

    FILE *lstFile = fopen("output.lst", "w");
    if (!lstFile)
    {
        perror("Error creating listing file");
        fclose(objFile);
        return 1;
    }

    passTwo(lines, lineCount, startAddress, progLength, progName, objFile, lstFile);
    fclose(objFile);
    fclose(lstFile);

    printf("\nAssembly completed successfully.\n");
    printf("Object Program Generated: output.obj\n");
    printf("Listing File Generated: output.lst\n");
    return 0;
}
