#include <cstdio>
#include <cctype>
#include <cstring>
#include <memory>
#include <direct.h>
#include <windows.h>
#include <vector>

#include "PreProcessor.h"
#include "reflection/Reflection.h"

using namespace zorya;

#define INDENT_PRINTF(num) for (int i = 0; i < num; i++) putchar('\t');

//char g_currentStruct[MAX_PATH] = { 0 };
std::vector<Token> g_struct_name_stack;
std::vector<int> g_depth_stack;


void clear_white_space(Tokenizer* tokenizer) {
	for (;;) {
		char tk = *tokenizer->at;
		if (tk == ' ' || tk == '\n' || tk == '\r' || tk == '\t') tokenizer->at++;
		else return;
	}
}


void get_token(Tokenizer* tokenizer, Token* current_token) {
	clear_white_space(tokenizer);

	char* start_token = tokenizer->at;
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

	current_token->type = type;
	current_token->text_length = tokenizer->at - start_token;;
	current_token->text = start_token;

}

zorya::VAR_REFL_TYPE get_var_type(Token* token, Member_Intermediate_Meta* member_meta) {
	char* tp = token->text;
	size_t len = token->text_length;

	if (member_meta->type_as_string != nullptr) free(member_meta->type_as_string);

	member_meta->type_as_string = (char*)malloc(len + 1);
	strncpy_s(member_meta->type_as_string, len + 1, tp, len);

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
	return zorya::VAR_REFL_TYPE::NOT_SUPPORTED;
}

void parse_var_definition(Tokenizer* tokenizer, Token* current_token, Member_Intermediate_Meta* member_meta) {

	zorya::VAR_REFL_TYPE var_type;

	get_token(tokenizer, current_token);
	member_meta->type_as_string = nullptr;

	//Determines variable type
	for (;;) {
		var_type = get_var_type(current_token, member_meta);

		if (*(tokenizer->at) == ' ') {
			break;
		}else{
			get_token(tokenizer, current_token);
			if (current_token->type == zorya::TKTP::COLON) {
				get_token(tokenizer, current_token); // Getting second colon of ::
				get_token(tokenizer, current_token);
				continue;
			}
		}
	}

	//Gets variable name and creates member meta
	//if (varType == zorya::VAR_REFL_TYPE::NOT_SUPPORTED) {
	//	memberMeta->typeEnum = varType;
	//	return;
	//} else {

		Token var_name_token;
		get_token(tokenizer, &var_name_token);
		get_token(tokenizer, current_token);

		if (current_token->type == zorya::TKTP::OPEN_SQUARE_BRACKET)
		{
			while (current_token->type != zorya::TKTP::CLOSE_SQUARE_BRACKET) {
				get_token(tokenizer, current_token);
			}

			get_token(tokenizer, current_token);
		}

		if (current_token->type == zorya::TKTP::SEMICOLON) {
			member_meta->name = (char*)malloc(var_name_token.text_length + 1);
			member_meta->name[var_name_token.text_length] = 0;
			strncpy_s(member_meta->name, (var_name_token.text_length + 1), var_name_token.text, var_name_token.text_length);
			
			int struct_name_length1 = 0, struct_name_length2 = 0;
			for (Token& token : g_struct_name_stack) {
				struct_name_length1 += token.text_length;
			}
			struct_name_length2 = struct_name_length1 + ((g_struct_name_stack.size() - 1) * 2);
			
			member_meta->meta_struct_basename = (char*)malloc(struct_name_length1 + 1);
			member_meta->meta_struct_basename[struct_name_length1] = 0;
			member_meta->actual_struct_identifier = (char*)malloc(struct_name_length2 + 1);
			member_meta->actual_struct_identifier[struct_name_length2] = 0;

			int i1 = 0, i2 = 0;
			for (Token& token : g_struct_name_stack) {
				strncpy_s(&member_meta->meta_struct_basename[i1], struct_name_length1 + 1 - i1, token.text, token.text_length);
				i1 += token.text_length;

				strncpy_s(&member_meta->actual_struct_identifier[i2], struct_name_length2 + 1 - i2, token.text, token.text_length);
				i2 += token.text_length;
				if (i2 < struct_name_length2) {
					strncpy_s(&member_meta->actual_struct_identifier[i2], struct_name_length2 + 1 - i2, "::", 2);
					i2 += 2;
				}
			}

			member_meta->type_enum = var_type;
			return;
		}
		else {
			member_meta->type_enum = zorya::VAR_REFL_TYPE::NOT_SUPPORTED;
			return;
		}
	//}
}

void parse_reflection_params(Tokenizer * tokenizer, Token* current_token, Member_Intermediate_Meta* member_meta) {
	get_token(tokenizer, current_token);

	if (!(current_token->type == zorya::TKTP::OPEN_PARENTHESIS) || current_token->type == zorya::TKTP::END_OF_FILE) {
		return;
	}
	else {
		//Parse meta parameters defined inside PROPERTY(...)
		for (;;) {
			get_token(tokenizer, current_token);

			if (current_token->type == zorya::TKTP::CLOSE_PARENTHESIS )
				break;
		}

		//Parse the variable associated to PROPERTY(...)
		parse_var_definition(tokenizer, current_token, member_meta);
	}

}


bool parse(Tokenizer* tokenizer, Member_Intermediate_Meta* member_meta, int* member_count, int member_max_count) {
	
	Token current_token;
	g_struct_name_stack.clear();
	g_depth_stack.clear();
	g_depth_stack.push_back(0);

	for (;;) {
		get_token(tokenizer, &current_token);

		if (current_token.type == zorya::TKTP::IDENTIFIER && (strncmp(current_token.text, "PROPERTY", current_token.text_length) == 0)) {
			if (*member_count >= member_max_count) {
				return false;
			}
			else {
				parse_reflection_params(tokenizer, &current_token, &member_meta[*member_count]);
				/*if (memberMeta[*memberCount].typeEnum != zorya::VAR_REFL_TYPE::NOT_SUPPORTED) */*member_count += 1;
			}
		}
		else if (current_token.type == zorya::TKTP::IDENTIFIER && (strncmp(current_token.text, "struct", current_token.text_length) == 0)) {
			get_token(tokenizer, &current_token);	//struct name
			g_struct_name_stack.push_back(current_token);
			g_depth_stack.push_back(-1);
			//strncpy_s(g_currentStruct, sizeof(g_currentStruct), currentToken.text, currentToken.textLength);
		}
		else if (current_token.type == zorya::TKTP::OPEN_BRACES) {
			g_depth_stack.back() += 1;
		}
		else if (current_token.type == zorya::TKTP::CLOSE_BRACES) {
			int& local_depth = g_depth_stack.back();
			if (local_depth > 0) {
				local_depth -= 1;
			}else{
				if (g_depth_stack.size() > 0) g_depth_stack.pop_back();
				if (g_struct_name_stack.size() > 0) g_struct_name_stack.pop_back();
			}

		}

		if (current_token.type == zorya::TKTP::END_OF_FILE)
			break;
	}

	return true;

}

void generate_metafile(char* parsed_filename, char* relative_filepath, Member_Intermediate_Meta* member_meta, int member_count, char* pathfile_to_generate, int depth) {
	FILE* file;
	errno_t er = fopen_s(&file, pathfile_to_generate, "w");

	if (er == 0) {
		INDENT_PRINTF(depth)
		printf("!--- Generating meta file %s ---!\n", pathfile_to_generate);

		fprintf(file, "// !WARNING! : This file was generated automatically during build. DO NOT modify.\n");
		fprintf(file, "#pragma once\n\n");

		fprintf(file, "#include \"ReflectionGenerationUtils.h\"\n");
		fprintf(file, "#include \"%s\"\n", relative_filepath);
		fprintf(file, "#include <tuple>\n");
		fprintf(file, "#include \"DirectXMath.h\"\n\n");

		fprintf(file, "using namespace DirectX;\n");
		fprintf(file, "using namespace zorya;\n\n");

		for (int i = 0; i < member_count; i++)
		{
			char* current_member_struct_basename = member_meta[i].meta_struct_basename;
			fprintf(file, "template <>\nconstexpr auto get_meta<%s>() {\n\treturn std::make_tuple(\n", current_member_struct_basename);
			
			while (i < member_count && strcmp(current_member_struct_basename, member_meta[i].meta_struct_basename) == 0) {
				fprintf(file, "\t\tMember_Meta<%s>{ \"%s\", offsetof(%s, %s), zorya::VAR_REFL_TYPE::%s }", member_meta[i].type_as_string, member_meta[i].name, member_meta[i].actual_struct_identifier, member_meta[i].name, zorya::VAR_REFL_TYPE_STRING[(int)member_meta[i].type_enum]);
				if (i + 1 < member_count && strcmp(current_member_struct_basename, member_meta[i + 1].meta_struct_basename) == 0) {
					i += 1;
					fprintf(file, ",\n");
				}
				else break;
			}

			fprintf(file, "\t);\n};\n\n");
		}

		fclose(file);
	}
	else {
		printf("Failed to open for writing meta file %s\n", pathfile_to_generate);
	}
}

int parse_and_generate_metafile(char* file_name, char* file_path, char* generated_filename, int depth) {
	FILE* file;
	errno_t er = fopen_s(&file, file_path, "rb");
	int member_count = 0;

	if (er == 0) {
		fseek(file, 0, SEEK_END);
		size_t size_in_bytes = ftell(file);
		fseek(file, 0, SEEK_SET);

		char* buffer = (char*)malloc(size_in_bytes + sizeof(char));
		fread(buffer, sizeof(char), (size_in_bytes / sizeof(char)), file);
		buffer[(size_in_bytes / sizeof(char))] = EOF;
		fclose(file);

		Tokenizer tokenizer{ buffer };

		Member_Intermediate_Meta* member_meta = (Member_Intermediate_Meta*)malloc(sizeof(Member_Intermediate_Meta) * 255);

		INDENT_PRINTF(depth)
		printf("Parsing file %s\n", file_path);
		parse(&tokenizer, member_meta, &member_count, 255);
		
		if (member_count > 0) {
			generate_metafile(file_name, &file_path[6], member_meta, member_count, generated_filename, depth);

			for (int i = 0; i < member_count; i++) {
				free((member_meta + i)->actual_struct_identifier);
				free((member_meta + i)->meta_struct_basename);
				free((member_meta + i)->type_as_string);
				free((member_meta + i)->name);
			}
		}

		free(member_meta);
		free(buffer);
	}
	else {
		printf("Failed to open file for parsing %s\n", file_path);
	}

	return member_count;
}


int recursive_file_search(const char* base_path, FILE* reflection_file, int depth) {
	HANDLE hnd_file;
	WIN32_FIND_DATA file_data;

	char sub_path[MAX_PATH];
	strcpy_s(sub_path, MAX_PATH, base_path);
	strcat_s(sub_path, MAX_PATH, "*");

	int num_generated_files = 0;

	if ((hnd_file = FindFirstFile(sub_path, &file_data)) != INVALID_HANDLE_VALUE) {
		do {
			if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
					strcmp(file_data.cFileName, ".") != 0 &&
					strcmp(file_data.cFileName, "..") != 0 &&
					strcmp(file_data.cFileName, "reflection") != 0) {

				INDENT_PRINTF(depth)
				printf("Searching files inside directory %s\n", file_data.cFileName);
				char new_sub_path[MAX_PATH];
				strcpy_s(new_sub_path, MAX_PATH, base_path);
				strcat_s(new_sub_path, MAX_PATH, file_data.cFileName);
				strcat_s(new_sub_path, MAX_PATH, "/");

				num_generated_files += recursive_file_search(new_sub_path, reflection_file, depth + 1);
			}

		} while (FindNextFile(hnd_file, &file_data) != 0);
	}

	strcpy_s(sub_path, MAX_PATH, base_path);
	strcat_s(sub_path, MAX_PATH, "*.h");

	if ((hnd_file = FindFirstFile(sub_path, &file_data)) != INVALID_HANDLE_VALUE) {
		do {
			char new_sub_path[MAX_PATH], generated_file_name[MAX_PATH];
			strcpy_s(new_sub_path, MAX_PATH, base_path);
			strcat_s(new_sub_path, MAX_PATH, file_data.cFileName);
			

			char* next_tok = nullptr;
			strcpy_s(generated_file_name, MAX_PATH, "./src/reflection/");
			strcat_s(generated_file_name, MAX_PATH, strtok_s(file_data.cFileName, ".", &next_tok)); 
			strcat_s(generated_file_name, MAX_PATH, "_auto_generated.h");
			int member_count = parse_and_generate_metafile(file_data.cFileName, new_sub_path, generated_file_name, depth);

			if (member_count > 0) {
				fprintf(reflection_file, "#include \"%s_auto_generated.h\"\n", file_data.cFileName);
				num_generated_files += 1;
			}

		} while (FindNextFile(hnd_file, &file_data) != 0);
	}

	FindClose(hnd_file);

	return num_generated_files;
}

int main() {
	_chdir("C:\\Users\\Draga\\Documents\\GitHub\\derma-renderer");

	const char* path = "./src/reflection/Reflection_auto_generated.h";

	FILE* file;
	errno_t er = fopen_s(&file, path, "w");
	if (er == 0) {
		fprintf(file, "// !WARNING! : This file was generated automatically during build. DO NOT modify.\n");
		fprintf(file, "#pragma once\n\n");

		int num_generated_files = recursive_file_search("./src/\0", file, 0);

		printf("\nCompleted Pre-Processing: Generated %d files\n\n", num_generated_files);

		fclose(file);
	}
	else {
		printf("Couldn't open reflection auto generated file %s\n", path);
	}


	return 0;
}