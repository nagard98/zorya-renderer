#include <cstdio>
#include <cctype>
#include <cstring>
#include <memory>
#include <direct.h>
#include <windows.h>

#include "BuildTools/PreProcessor.h"
#include "Reflection/Reflection.h"

#define INDENT_PRINTF(num) for (int i = 0; i < num; i++) putchar('\t');

char currentStruct[MAX_PATH] = { 0 };

void clearWhiteSpace(Tokenizer* tokenizer) {
	for (;;) {
		char tk = *tokenizer->at;
		if (tk == ' ' || tk == '\n' || tk == '\r' || tk == '\t') tokenizer->at++;
		else return;
	}
}


void getToken(Tokenizer* tokenizer, Token* currentToken) {
	clearWhiteSpace(tokenizer);

	char* startToken = tokenizer->at;
	zorya::TKTP type;

	switch (*tokenizer->at)
	{
	case EOF:
		tokenizer->at++;
		type = zorya::TKTP::END_OF_FILE;
		break;
	case '#':
		tokenizer->at++;
		type = zorya::TKTP::HASH;
		break;
	case '(':
		tokenizer->at++;
		type = zorya::TKTP::OPEN_PARENTHESIS;
		break;
	case ')':
		tokenizer->at++;
		type = zorya::TKTP::CLOSE_PARENTHESIS;
		break;
	case '[':
		tokenizer->at++;
		type = zorya::TKTP::OPEN_SQUARE_BRACKET;
		break;
	case ']':
		tokenizer->at++;
		type = zorya::TKTP::CLOSE_SQUARE_BRACKET;
		break;
	case '=':
		tokenizer->at++;
		type = zorya::TKTP::EQUALS;
		break;
	case ',':
		tokenizer->at++;
		type = zorya::TKTP::COMMA;
		break;
	case ':':
		tokenizer->at++;
		type = zorya::TKTP::COLON;
		break;
	case ';':
		tokenizer->at++;
		type = zorya::TKTP::SEMICOLON;
		break;
	default:
		if ((isalnum(*tokenizer->at))) {
			while ((isalnum(*tokenizer->at) || *tokenizer->at == '_') && *tokenizer->at != EOF) {
				tokenizer->at++;
			}
			type = zorya::TKTP::IDENTIFIER;
		}
		else {
			tokenizer->at++;
			type = zorya::TKTP::UNDEFINED;
		}
	}

	currentToken->type = type;
	currentToken->textLength = tokenizer->at - startToken;;
	currentToken->text = startToken;

}

zorya::VAR_REFL_TYPE getVarType(Token* token) {
	char* tp = token->text;
	size_t len = token->textLength;
	if (strncmp(tp, "int", len) == 0) return  zorya::VAR_REFL_TYPE::INT;
	if (strncmp(tp, "float", len) == 0) return zorya::VAR_REFL_TYPE::FLOAT;
	if (strncmp(tp, "uint8_t", len) == 0) return zorya::VAR_REFL_TYPE::UINT8;
	if (strncmp(tp, "uint16_t", len) == 0) return zorya::VAR_REFL_TYPE::UINT16;
	if (strncmp(tp, "uint32_t", len) == 0) return zorya::VAR_REFL_TYPE::UINT32;
	if (strncmp(tp, "uint64_t", len) == 0) return zorya::VAR_REFL_TYPE::UINT64;
	if (strncmp(tp, "XMFLOAT2", len) == 0) return zorya::VAR_REFL_TYPE::XMFLOAT2;
	if (strncmp(tp, "XMFLOAT3", len) == 0) return zorya::VAR_REFL_TYPE::XMFLOAT3;
	if (strncmp(tp, "XMFLOAT4", len) == 0) return zorya::VAR_REFL_TYPE::XMFLOAT4;
	if (strncmp(tp, "wchar_t", len) == 0) return zorya::VAR_REFL_TYPE::WCHAR;
	return zorya::VAR_REFL_TYPE::NOT_SUPPORTED;
}

void parseVarDef(Tokenizer* tokenizer, Token* currentToken, MemberIntermediateMeta* memberMeta) {

	zorya::VAR_REFL_TYPE varType;

	getToken(tokenizer, currentToken);

	//Determines variable type
	for (;;) {
		varType = getVarType(currentToken);

		if (varType != zorya::VAR_REFL_TYPE::NOT_SUPPORTED || (currentToken->type == zorya::TKTP::SEMICOLON)) {
			break;
		}else{
			getToken(tokenizer, currentToken);
			if (currentToken->type == zorya::TKTP::COLON) {
				getToken(tokenizer, currentToken); // Getting second colon of ::
				getToken(tokenizer, currentToken);
				continue;
			}
		}
	}

	//Gets variable name and creates member meta
	if (varType == zorya::VAR_REFL_TYPE::NOT_SUPPORTED) {
		memberMeta->type = varType;
		return;
	} else {
		Token varNameToken;
		getToken(tokenizer, &varNameToken);
		getToken(tokenizer, currentToken);

		if (currentToken->type == zorya::TKTP::OPEN_SQUARE_BRACKET)
		{
			while (currentToken->type != zorya::TKTP::CLOSE_SQUARE_BRACKET) {
				getToken(tokenizer, currentToken);
			}

			getToken(tokenizer, currentToken);
		}

		if (currentToken->type == zorya::TKTP::SEMICOLON) {
			memberMeta->name = (char*)malloc(varNameToken.textLength + 1);
			strncpy_s(memberMeta->name, (varNameToken.textLength + 1), varNameToken.text, varNameToken.textLength);
			
			memberMeta->structName = (char*)malloc(strlen(currentStruct) + 1);
			strncpy_s(memberMeta->structName, (strnlen_s(currentStruct, MAX_PATH) + 1), currentStruct, strnlen_s(currentStruct, MAX_PATH));

			memberMeta->type = varType;
			return;
		}
		else {
			memberMeta->type = zorya::VAR_REFL_TYPE::NOT_SUPPORTED;
			return;
		}
	}
}

void parseReflectionParams(Tokenizer * tokenizer, Token* currentToken, MemberIntermediateMeta* memberMeta) {
	getToken(tokenizer, currentToken);

	if (!(currentToken->type == zorya::TKTP::OPEN_PARENTHESIS) || currentToken->type == zorya::TKTP::END_OF_FILE) return;
	else {
		for (;;) {
			getToken(tokenizer, currentToken);

			if (currentToken->type == zorya::TKTP::CLOSE_PARENTHESIS )
				break;
		}

		parseVarDef(tokenizer, currentToken, memberMeta);
	}

}


bool parse(Tokenizer* tokenizer, MemberIntermediateMeta* memberMeta, int* memberCount, int memberMaxCount) {
	Token currentToken;

	for (;;) {
		getToken(tokenizer, &currentToken);

		if (currentToken.type == zorya::TKTP::IDENTIFIER && (strncmp(currentToken.text, "PROPERTY", currentToken.textLength) == 0)) {
			if (*memberCount >= memberMaxCount) {
				return false;
			}
			else {
				parseReflectionParams(tokenizer, &currentToken, &memberMeta[*memberCount]);
				if (memberMeta[*memberCount].type != zorya::VAR_REFL_TYPE::NOT_SUPPORTED) *memberCount += 1;
			}
		}
		else if (currentToken.type == zorya::TKTP::IDENTIFIER && (strncmp(currentToken.text, "struct", currentToken.textLength) == 0)) {
			getToken(tokenizer, &currentToken);
			strncpy_s(currentStruct, sizeof(currentStruct), currentToken.text, currentToken.textLength);
		}

		if (currentToken.type == zorya::TKTP::END_OF_FILE)
			break;
	}

	return true;

}

void generateMetaFile(char* parsedFileName, MemberIntermediateMeta* memberMeta, int memberCount, char* pathFileToGenerate, int depth) {
	FILE* file;
	errno_t er = fopen_s(&file, pathFileToGenerate, "w");

	if (er == 0) {
		INDENT_PRINTF(depth)
		printf("Generating meta file %s\n", pathFileToGenerate);

		fprintf(file, "// !WARNING! : This file was generated automatically during build. DO NOT modify.\n");
		fprintf(file, "#pragma once\n\n");

		fprintf(file, "#include \"Reflection.h\"\n");
		fprintf(file, "#include <%s.h>\n\n", parsedFileName);

		for (int i = 0; i < memberCount; i++)
		{
			char* currentStruct = memberMeta[i].structName;
			fprintf(file, "const MemberMeta %s_meta[] = {\n", currentStruct);
			
			while (i < memberCount && strcmp(currentStruct, memberMeta[i].structName) == 0) {
				fprintf(file, "\t{ \"%s\", offsetof(%s, %s), zorya::VAR_REFL_TYPE::%s }, \n", memberMeta[i].name, currentStruct, memberMeta[i].name, zorya::VAR_REFL_TYPE_STRING[memberMeta[i].type]);
				if (i+1 < memberCount && strcmp(currentStruct, memberMeta[i + 1].structName) == 0) i += 1;
				else break;
			}

			fprintf(file, "};\n\n");

			fprintf(file, "BUILD_FOREACHFIELD(%s)\n\n", memberMeta[i].structName);
		}

		fclose(file);
	}
	else {
		printf("Failed to open for writing meta file %s\n", pathFileToGenerate);
	}
}

int parseAndGenerateMetaFile(char* fileName, char* filePath, char* generatedFilename, int depth) {
	FILE* file;
	errno_t er = fopen_s(&file, filePath, "rb");
	int memberCount = 0;

	if (er == 0) {
		INDENT_PRINTF(depth)
		printf("Parsing file %s\n", filePath);

		fseek(file, 0, SEEK_END);
		size_t sizeInBytes = ftell(file);
		fseek(file, 0, SEEK_SET);

		char* buffer = (char*)malloc(sizeInBytes + sizeof(char));
		fread(buffer, sizeof(char), (sizeInBytes / sizeof(char)), file);
		buffer[(sizeInBytes / sizeof(char))] = EOF;
		fclose(file);

		Tokenizer tokenizer{ buffer };

		MemberIntermediateMeta* memberMeta = (MemberIntermediateMeta*)malloc(sizeof(MemberIntermediateMeta) * 255);

		parse(&tokenizer, memberMeta, &memberCount, 255);

		if(memberCount > 0) generateMetaFile(fileName, memberMeta, memberCount, generatedFilename, depth);

		free(memberMeta);
		free(buffer);
	}
	else {
		printf("Failed to open file for parsing %s\n", filePath);
	}

	return memberCount;
}


int recursiveFileSearch(const char* basePath, FILE* reflectionFile, int depth) {
	HANDLE fileHnd;
	WIN32_FIND_DATA fileData;

	char subPath[MAX_PATH];
	strcpy_s(subPath, MAX_PATH, basePath);
	strcat_s(subPath, MAX_PATH, "*");

	int numGeneratedFiles = 0;

	if ((fileHnd = FindFirstFile(subPath, &fileData)) != INVALID_HANDLE_VALUE) {
		do {
			if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
					strcmp(fileData.cFileName, ".") != 0 &&
					strcmp(fileData.cFileName, "..") != 0 &&
					strcmp(fileData.cFileName, "Reflection") != 0) {

				printf("Searching files inside directory %s\n", fileData.cFileName);
				char newSubPath[MAX_PATH];
				strcpy_s(newSubPath, MAX_PATH, basePath);
				strcat_s(newSubPath, MAX_PATH, fileData.cFileName);
				strcat_s(newSubPath, MAX_PATH, "/");

				numGeneratedFiles += recursiveFileSearch(newSubPath, reflectionFile, depth + 1);
			}

		} while (FindNextFile(fileHnd, &fileData) != 0);
	}

	strcpy_s(subPath, MAX_PATH, basePath);
	strcat_s(subPath, MAX_PATH, "*.h");

	if ((fileHnd = FindFirstFile(subPath, &fileData)) != INVALID_HANDLE_VALUE) {
		do {
			char newSubPath[MAX_PATH], generatedFilename[MAX_PATH];
			strcpy_s(newSubPath, MAX_PATH, basePath);
			strcat_s(newSubPath, MAX_PATH, fileData.cFileName);
			

			char* nextTok = nullptr;
			strcpy_s(generatedFilename, MAX_PATH, "./include/Reflection/");
			strcat_s(generatedFilename, MAX_PATH, strtok_s(fileData.cFileName, ".", &nextTok)); 
			strcat_s(generatedFilename, MAX_PATH, "_auto_generated.h");
			int memberCount = parseAndGenerateMetaFile(fileData.cFileName, newSubPath, generatedFilename, depth);

			if (memberCount > 0) {
				fprintf(reflectionFile, "#include \"%s_auto_generated.h\"\n", fileData.cFileName);
				numGeneratedFiles += 1;
			}

		} while (FindNextFile(fileHnd, &fileData) != 0);
	}

	FindClose(fileHnd);

	return numGeneratedFiles;
}

int main() {
	_chdir("C:\\Users\\Draga\\Documents\\GitHub\\derma-renderer");

	const char* path = "./include/Reflection/Reflection_auto_generated.h";

	FILE* file;
	errno_t er = fopen_s(&file, path, "w");
	if (er == 0) {
		fprintf(file, "// !WARNING! : This file was generated automatically during build. DO NOT modify.\n");
		fprintf(file, "#pragma once\n\n");

		int numGeneratedFiles = recursiveFileSearch("./include/\0", file, 0);

		printf("\nCompleted Pre-Processing: Generated %d files\n\n", numGeneratedFiles);

		fclose(file);
	}
	else {
		printf("Couldn't open reflection auto generated file %s\n", path);
	}


	return 0;
}