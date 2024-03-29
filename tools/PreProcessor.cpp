#include <cstdio>
#include <cctype>
#include <cstring>
#include <memory>
#include <direct.h>
#include <windows.h>
#include <vector>

#include "PreProcessor.h"
#include "reflection/Reflection.h"

#define INDENT_PRINTF(num) for (int i = 0; i < num; i++) putchar('\t');

//char g_currentStruct[MAX_PATH] = { 0 };
std::vector<Token> g_structNameStack;
std::vector<int> g_depthStack;


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
	case '{':
		tokenizer->at++;
		type = zorya::TKTP::OPEN_BRACES;
		break;
	case '}':
		tokenizer->at++;
		type = zorya::TKTP::CLOSE_BRACES;
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
	if (strncmp(tp, "XMFLOAT2", len) == 0 || strncmp(tp, "float2", len) == 0) return zorya::VAR_REFL_TYPE::XMFLOAT2;
	if (strncmp(tp, "XMFLOAT3", len) == 0 || strncmp(tp, "float3", len) == 0) return zorya::VAR_REFL_TYPE::XMFLOAT3;
	if (strncmp(tp, "XMFLOAT4", len) == 0 || strncmp(tp, "float4", len) == 0) return zorya::VAR_REFL_TYPE::XMFLOAT4;
	if (strncmp(tp, "wchar_t", len) == 0) return zorya::VAR_REFL_TYPE::WCHAR;
	if (strncmp(tp, "MultiOption", len) == 0) return zorya::VAR_REFL_TYPE::MULTI_OPTION;
	if (strncmp(tp, "JimenezSSSModel", len) == 0) return zorya::VAR_REFL_TYPE::JIMENEZ_GAUSS;
	if (strncmp(tp, "MultiOption", len) == 0) return zorya::VAR_REFL_TYPE::JIMENEZ_SEP;
	if (strncmp(tp, "MultiOption", len) == 0) return zorya::VAR_REFL_TYPE::GOLUBEV;
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
			memberMeta->name[varNameToken.textLength] = 0;
			strncpy_s(memberMeta->name, (varNameToken.textLength + 1), varNameToken.text, varNameToken.textLength);
			
			int structNameLength1 = 0, structNameLength2 = 0;
			for (Token& token : g_structNameStack) {
				structNameLength1 += token.textLength;
			}
			structNameLength2 = structNameLength1 + ((g_structNameStack.size() - 1) * 2);
			
			memberMeta->metaStructBaseName = (char*)malloc(structNameLength1 + 1);
			memberMeta->metaStructBaseName[structNameLength1] = 0;
			memberMeta->actualStructIdentifier = (char*)malloc(structNameLength2 + 1);
			memberMeta->actualStructIdentifier[structNameLength2] = 0;

			int i1 = 0, i2 = 0;
			for (Token& token : g_structNameStack) {
				strncpy_s(&memberMeta->metaStructBaseName[i1], structNameLength1 + 1 - i1, token.text, token.textLength);
				i1 += token.textLength;

				strncpy_s(&memberMeta->actualStructIdentifier[i2], structNameLength2 + 1 - i2, token.text, token.textLength);
				i2 += token.textLength;
				if (i2 < structNameLength2) {
					strncpy_s(&memberMeta->actualStructIdentifier[i2], structNameLength2 + 1 - i2, "::", 2);
					i2 += 2;
				}
			}

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
	g_structNameStack.clear();
	g_depthStack.clear();
	g_depthStack.push_back(0);

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
			getToken(tokenizer, &currentToken);	//struct name
			g_structNameStack.push_back(currentToken);
			g_depthStack.push_back(-1);
			//strncpy_s(g_currentStruct, sizeof(g_currentStruct), currentToken.text, currentToken.textLength);
		}
		else if (currentToken.type == zorya::TKTP::OPEN_BRACES) {
			g_depthStack.back() += 1;
		}
		else if (currentToken.type == zorya::TKTP::CLOSE_BRACES) {
			int& localDepth = g_depthStack.back();
			if (localDepth > 0) {
				localDepth -= 1;
			}else{
				if (g_depthStack.size() > 0) g_depthStack.pop_back();
				if (g_structNameStack.size() > 0) g_structNameStack.pop_back();
			}

		}

		if (currentToken.type == zorya::TKTP::END_OF_FILE)
			break;
	}

	return true;

}

void generateMetaFile(char* parsedFileName, char* relativeFilePath, MemberIntermediateMeta* memberMeta, int memberCount, char* pathFileToGenerate, int depth) {
	FILE* file;
	errno_t er = fopen_s(&file, pathFileToGenerate, "w");

	if (er == 0) {
		INDENT_PRINTF(depth)
		printf("!--- Generating meta file %s ---!\n", pathFileToGenerate);

		fprintf(file, "// !WARNING! : This file was generated automatically during build. DO NOT modify.\n");
		fprintf(file, "#pragma once\n\n");

		fprintf(file, "#include \"ReflectionGenerationUtils.h\"\n");
		fprintf(file, "#include \"%s\"\n\n", relativeFilePath);

		for (int i = 0; i < memberCount; i++)
		{
			char* currentMemberStructBaseName = memberMeta[i].metaStructBaseName;
			fprintf(file, "const MemberMeta %s_meta[] = {\n", currentMemberStructBaseName);
			
			while (i < memberCount && strcmp(currentMemberStructBaseName, memberMeta[i].metaStructBaseName) == 0) {
				fprintf(file, "\t{ \"%s\", offsetof(%s, %s), zorya::VAR_REFL_TYPE::%s }, \n", memberMeta[i].name, memberMeta[i].actualStructIdentifier, memberMeta[i].name, zorya::VAR_REFL_TYPE_STRING[memberMeta[i].type]);
				if (i+1 < memberCount && strcmp(currentMemberStructBaseName, memberMeta[i + 1].metaStructBaseName) == 0) i += 1;
				else break;
			}

			fprintf(file, "};\n\n");

			fprintf(file, "BUILD_FOREACHFIELD3(%s, %s_meta)\n\n", memberMeta[i].actualStructIdentifier, currentMemberStructBaseName);
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
		fseek(file, 0, SEEK_END);
		size_t sizeInBytes = ftell(file);
		fseek(file, 0, SEEK_SET);

		char* buffer = (char*)malloc(sizeInBytes + sizeof(char));
		fread(buffer, sizeof(char), (sizeInBytes / sizeof(char)), file);
		buffer[(sizeInBytes / sizeof(char))] = EOF;
		fclose(file);

		Tokenizer tokenizer{ buffer };

		MemberIntermediateMeta* memberMeta = (MemberIntermediateMeta*)malloc(sizeof(MemberIntermediateMeta) * 255);

		INDENT_PRINTF(depth)
		printf("Parsing file %s\n", filePath);
		parse(&tokenizer, memberMeta, &memberCount, 255);
		
		if (memberCount > 0) {
			generateMetaFile(fileName, &filePath[6], memberMeta, memberCount, generatedFilename, depth);

			for (int i = 0; i < memberCount; i++) {
				free((memberMeta + i)->actualStructIdentifier);
				free((memberMeta + i)->metaStructBaseName);
				free((memberMeta + i)->name);
			}
		}

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
					strcmp(fileData.cFileName, "reflection") != 0) {

				INDENT_PRINTF(depth)
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
			strcpy_s(generatedFilename, MAX_PATH, "./src/reflection/");
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

	const char* path = "./src/reflection/Reflection_auto_generated.h";

	FILE* file;
	errno_t er = fopen_s(&file, path, "w");
	if (er == 0) {
		fprintf(file, "// !WARNING! : This file was generated automatically during build. DO NOT modify.\n");
		fprintf(file, "#pragma once\n\n");

		int numGeneratedFiles = recursiveFileSearch("./src/\0", file, 0);

		printf("\nCompleted Pre-Processing: Generated %d files\n\n", numGeneratedFiles);

		fclose(file);
	}
	else {
		printf("Couldn't open reflection auto generated file %s\n", path);
	}


	return 0;
}