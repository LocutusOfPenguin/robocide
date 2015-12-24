#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "perft.h"
#include "pos.h"
#include "robocide.h"
#include "moves.h"
#include "time.h"
#include "uci.h"

typedef enum {
	UciOptionTypeCheck,  // "a checkbox that can either be true or false"
	UciOptionTypeSpin,   // "a spin wheel that can be an integer in a certain range"
	UciOptionTypeCombo,  // "a combo box that can have different predefined strings as a value"
	UciOptionTypeButton, // "a button that can be pressed to send a command to the engine"
	UciOptionTypeString  // "a text field that has a string as a value"
} UciOptionType;

typedef struct {
	void(*function)(void *userData, bool value);
	bool initial;
} UciOptionCheck;

typedef struct {
	void(*function)(void *userData, int value);
	int initial, min, max;
} UciOptionSpin;

typedef struct {
	void(*function)(void *userData, const char *value);
	char *initial, **options;
	size_t optionCount;
} UciOptionCombo;

typedef struct {
	void(*function)(void *userData);
} UciOptionButton;

typedef struct {
	void(*function)(void *userData, const char *value);
	char *initial;
} UciOptionString;

typedef struct {
	char *name;
	UciOptionType type;
	void *userData;
	union {
		UciOptionCheck check;
		UciOptionSpin spin;
		UciOptionCombo combo;
		UciOptionButton button;
		UciOptionString string;
	} o;
} UciOption;

UciOption *uciOptions=NULL;
size_t uciOptionCount=0;

const char uciBoolToString[2][8]={[false]="false", [true]="true"};

////////////////////////////////////////////////////////////////////////////////
// Private prototypes.
////////////////////////////////////////////////////////////////////////////////

void uciQuit(void);

bool uciRead(char **linePtr, size_t *lineSize); // Essentially a wrapper around getline().

void uciParseSetOption(char *string);

void uciOptionPrint(void);

UciOption *uciOptionNewBase(void);
UciOption *uciOptionFromName(const char *name);

bool uciStringToBool(const char *string);

////////////////////////////////////////////////////////////////////////////////
// Public functions.
////////////////////////////////////////////////////////////////////////////////

void uciLoop(void)
{
	// Turn off output buffering (saves us having to call fflush() after every
	// output).
	if (setvbuf(stdout, NULL, _IOLBF, 0)!=0)
		mainFatalError("Error: Could not turn off output buffering.\n");

	// Create 'working' position.
	Pos *pos=posNew(NULL);
	if (pos==NULL)
		mainFatalError("Error: Could not create 'working position'.\n");

	// Read lines from the GUI.
	char *line=NULL;
	size_t lineSize=0;
	while(1) {
		// Get line from 'GUI'.
		if (!uciRead(&line, &lineSize))
			break; // Error

		// Strip newline to simplify parsing.
		if (line[strlen(line)-1]=='\n')
			line[strlen(line)-1]='\0';

		// Parse command.
		char *savePtr, *part;
		if ((part=strtok_r(line, " ", &savePtr))==NULL)
			continue;
		if (utilStrEqual(part, "go")) {
			Move move=posGenRandomLegalMove(pos);
			char str[8];
			posMoveToStr(pos, move, str);
			uciWrite("bestmove %s\n", str);
		} else if (utilStrEqual(part, "position")) {
			// Get position (either 'startpos' or FEN string).
			if ((part=strtok_r(NULL, " ", &savePtr))==NULL)
				continue;
			if (utilStrEqual(part, "startpos")) {
				if (!posSetToFEN(pos, NULL))
					continue;
			} else if (utilStrEqual(part, "fen")) {
				char *start=part+4;
				char *end=strstr(start, "moves");
				if (end!=NULL)
					*(end-1)='\0';
				if (!posSetToFEN(pos, start))
					continue;
				if (end!=NULL)
					*(end-1)=' ';
			} else
				continue;

			// Make any moves given.
			bool inMoves=false;
			while((part=strtok_r(NULL, " ", &savePtr))!=NULL) {
				if (!inMoves && utilStrEqual(part, "moves"))
					inMoves=true;
				else if (inMoves) {
					Move move=posMoveFromStr(pos, part);
					if (!moveIsValid(move) || !posMakeMove(pos, move))
						break;
				}
			}
		}
		else if (utilStrEqual(part, "isready"))
			uciWrite("readyok\n");
		else if (utilStrEqual(part, "setoption")) {
			part=line+strlen("setoption");
			uciParseSetOption(part+1);
		} else if (utilStrEqual(part, "quit"))
			break;
		else if (utilStrEqual(part, "disp"))
			posDraw(pos);
		else if (utilStrEqual(part, "perft")) {
			if ((part=strtok_r(NULL, " ", &savePtr))==NULL)
				continue;
			unsigned int depth=atoi(part);
			perft(pos, depth);
		} else if (utilStrEqual(part, "divide")) {
			if ((part=strtok_r(NULL, " ", &savePtr))==NULL)
				continue;
			unsigned int depth=atoi(part);
			divide(pos, depth);
		} else if (utilStrEqual(part, "uci")) {
			uciWrite("id name robocide\nid author Daniel White\n");
			uciOptionPrint();
			uciWrite("uciok\n");
		}
	}

	// Clean up.
	free(line);
	posFree(pos);
	uciQuit();
}

bool uciWrite(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	bool success=(vprintf(format, ap)>=0);
	va_end(ap);
	return success;
}

bool uciOptionNewCheck(const char *name, void(*function)(void *userData, bool value), void *userData, bool initial) {
	// Allocate memory needed.
	char *nameMem=malloc(strlen(name)+1);
	if (nameMem==NULL)
		return false;

	// Allocate new option.
	UciOption *option=uciOptionNewBase();
	if (option==NULL) {
		free(nameMem);
		return false;
	}

	// Set data.
	option->name=nameMem;
	strcpy(option->name, name);
	option->type=UciOptionTypeCheck;
	option->userData=userData;
	option->o.check.function=function;
	option->o.check.initial=initial;

	return true;
}

bool uciOptionNewSpin(const char *name, void(*function)(void *userData, int value), void *userData, int min, int max, int initial) {
	// Allocate any memory needed.
	char *nameMem=malloc(strlen(name)+1);
	if (nameMem==NULL)
		return false;

	// Allocate new option.
	UciOption *option=uciOptionNewBase();
	if (option==NULL) {
		free(nameMem);
		return false;
	}

	// Set data.
	option->name=nameMem;
	strcpy(option->name, name);
	option->type=UciOptionTypeSpin;
	option->userData=userData;
	option->o.spin.function=function;
	option->o.spin.min=min;
	option->o.spin.max=max;
	option->o.spin.initial=initial;

	return true;
}

bool uciOptionNewCombo(const char *name, void(*function)(void *userData, const char *value), void *userData, const char *initial, size_t optionCount, ...) {
	// Allocate any memory needed.
	char *nameMem=malloc(strlen(name)+1);
	char *initialMem=malloc(strlen(initial)+1);
	char **optionsMem=malloc(optionCount*sizeof(char *));
	if (nameMem==NULL || initialMem==NULL || optionsMem==NULL) {
		free(nameMem);
		free(initialMem);
		free(optionsMem);
		return false;
	}

	// Parse variable number of options.
	va_list ap;
	va_start(ap, optionCount);
	size_t i;
	for(i=0;i<optionCount;++i) {
		char *arg=va_arg(ap, char *);
		optionsMem[i]=malloc(strlen(arg)+1);
		if (optionsMem[i]==NULL) {
			va_end(ap);
			size_t j;
			for(j=0;j<i;++j)
				free(optionsMem[j]);
			free(nameMem);
			free(initialMem);
			free(optionsMem);
			return false;
		}
		strcpy(optionsMem[i], arg);
	}
	va_end(ap);

	// Allocate new option.
	UciOption *option=uciOptionNewBase();
	if (option==NULL) {
		free(nameMem);
		free(initialMem);
		for(i=0;i<optionCount;++i)
			free(optionsMem[i]);
		free(optionsMem);
		return false;
	}

	// Set data.
	option->name=nameMem;
	strcpy(option->name, name);
	option->type=UciOptionTypeCombo;
	option->userData=userData;
	option->o.combo.function=function;
	option->o.combo.initial=initialMem;
	strcpy(option->o.combo.initial, initial);
	option->o.combo.options=optionsMem;
	option->o.combo.optionCount=optionCount;

	return true;
}

bool uciOptionNewButton(const char *name, void(*function)(void *userData), void *userData) {
	// Allocate any memory needed.
	char *nameMem=malloc(strlen(name)+1);
	if (nameMem==NULL)
		return false;

	// Allocate new option.
	UciOption *option=uciOptionNewBase();
	if (option==NULL) {
		free(nameMem);
		return false;
	}

	// Set data.
	option->name=nameMem;
	strcpy(option->name, name);
	option->type=UciOptionTypeButton;
	option->userData=userData;
	option->o.button.function=function;

	return true;
}

bool uciOptionNewString(const char *name, void(*function)(void *userData, const char *value), void *userData, const char *initial) {
	// Allocate any memory need.
	char *nameMem=malloc(strlen(name)+1);
	char *initialMem=malloc(strlen(initial)+1);
	if (nameMem==NULL || initialMem==NULL) {
		free(nameMem);
		free(initialMem);
		return false;
	}

	// Allocate new option.
	UciOption *option=uciOptionNewBase();
	if (option==NULL) {
		free(nameMem);
		free(initialMem);
		return false;
	}

	// Set data.
	option->name=nameMem;
	strcpy(option->name, name);
	option->type=UciOptionTypeString;
	option->userData=userData;
	option->o.string.function=function;
	option->o.string.initial=initialMem;
	strcpy(option->o.string.initial, initial);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
// Private functions
////////////////////////////////////////////////////////////////////////////////

void uciQuit(void) {
	// Free each option.
	unsigned int i, j;
	for(i=0;i<uciOptionCount;++i) {
		UciOption *option=&uciOptions[i];

		free(option->name);
		switch(option->type) {
			case UciOptionTypeCheck: break;
			case UciOptionTypeSpin: break;
			case UciOptionTypeButton: break;
			case UciOptionTypeCombo:
				free(option->o.combo.initial);
				for(j=0;j<option->o.combo.optionCount;++j)
					free(option->o.combo.options[j]);
				free(option->o.combo.options);
			break;
			case UciOptionTypeString:
				free(option->o.string.initial);
			break;
		}
	}

	// Free array of options.
	free(uciOptions);
	uciOptions=NULL;
	uciOptionCount=0;
}

bool uciRead(char **linePtr, size_t *lineSize) {
	return (getline(linePtr, lineSize, stdin)>=0);
}

void uciParseSetOption(char *string) {
	// Extract name and value arguments.
	char *name=strstr(string, "name");
	if (name==NULL || name[4]=='\0')
		return;
	name+=5; // Skip 'name '.
	char *value=strstr(string, "value");
	char *nameEnd=string+strlen(string);
	if (value!=NULL) {
		nameEnd=value-1;
		value+=6; // Skip 'value '.
	}
	*nameEnd='\0';

	// Find option with given name.
	UciOption *option=uciOptionFromName(name);
	if (option==NULL)
		return;

	// Each type of option is handled separately.
	switch(option->type) {
		case UciOptionTypeCheck:
			if (value!=NULL)
				(*option->o.check.function)(option->userData, uciStringToBool(value));
		break;
		case UciOptionTypeSpin:
			if (value!=NULL) {
				int intValue=atoi(value);
				intValue=utilMax(intValue, option->o.spin.min);
				intValue=utilMin(intValue, option->o.spin.max);
				(*option->o.spin.function)(option->userData, intValue);
			}
		break;
		case UciOptionTypeCombo:
			if (value!=NULL) {
				unsigned int i;
				for(i=0;i<option->o.combo.optionCount;++i)
					if (utilStrEqual(option->o.combo.options[i], value)) {
						(*option->o.combo.function)(option->userData, value);
						break;
					}
			}
		break;
		case UciOptionTypeButton:
			(*option->o.button.function)(option->userData);
		break;
		case UciOptionTypeString:
			if (value!=NULL)
				(*option->o.string.function)(option->userData, value);
		break;
	}
}

void uciOptionPrint(void) {
	unsigned int i, j;
	for(i=0;i<uciOptionCount;++i) {
		UciOption *option=&uciOptions[i];
		switch(option->type) {
			case UciOptionTypeCheck:
				uciWrite("option name %s type check default %s\n", option->name, uciBoolToString[option->o.check.initial]);
			break;
			case UciOptionTypeSpin:
				uciWrite("option name %s type spin default %i min %i max %i\n", option->name, option->o.spin.initial, option->o.spin.min, option->o.spin.max);
			break;
			case UciOptionTypeCombo:
				uciWrite("option name %s type combo default %s", option->name, option->o.combo.initial);
				for(j=0;j<option->o.combo.optionCount;++j)
					uciWrite(" var %s", option->o.combo.options[j]);
				uciWrite("\n");
			break;
			case UciOptionTypeButton:
				uciWrite("option name %s type button\n", option->name);
			break;
			case UciOptionTypeString:
				uciWrite("option name %s type string default %s\n", option->name, option->o.string.initial);
			break;
		}
	}
}

UciOption *uciOptionNewBase(void) {
	UciOption *tempPtr=realloc(uciOptions, (uciOptionCount+1)*sizeof(UciOption));
	if (tempPtr==NULL)
		return NULL;
	uciOptions=tempPtr;
	return &uciOptions[uciOptionCount++];
}

UciOption *uciOptionFromName(const char *name) {
	unsigned int i;
	for(i=0;i<uciOptionCount;++i)
		if (utilStrEqual(uciOptions[i].name, name))
			return &uciOptions[i];
	return NULL;
}

bool uciStringToBool(const char *string) {
	assert(string!=NULL);
	return utilStrEqual(string, "true");
}
