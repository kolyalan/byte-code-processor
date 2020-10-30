#include <cstdio>
#include <string>
#include <vector>
#include <cmath>

enum Lexem_type {
    NONE,
    COMMAND,
    CONSTANT_DOUBLE
};

struct Lexem{
    Lexem_type type;
    std::string command_name;
    double constant_double;
};

char parser_errors[][100] = {
    "Unexpected symbol after double value",
    "Unexpected symbol after command or identifier",
    "Unexpected symbol found"
};

void cry_parser_error(unsigned error_code, char * filename, int line_num, char *line_ptr, char *err_ptr) {
    if (error_code >= sizeof(parser_errors)/sizeof(parser_errors[0])) {
        fprintf(stderr, "Unknown error in parser with error code %d\n", error_code);
        return;
    }
    fprintf(stderr, "Error: %s\n", parser_errors[error_code]);
    fprintf(stderr, "In file %s:%d:%ld\n", filename, line_num, err_ptr - line_ptr + 1);
    fprintf(stderr, "%s", line_ptr);
    fprintf(stderr, "%*c\n", (int)(err_ptr - line_ptr) + 1, '^');
}

bool is_end_line(char c) {
    return c == 0 || c == '\n' || c == '\r' || c == '#';
}

int parse (FILE * in, char * filename, std::vector<Lexem> &lexem_list) {
    char *cur_line = NULL;
    size_t cur_line_len = 0;
    Lexem cur = {.type = NONE, .command_name = {}, .constant_double = NAN};
    int line_num = 0;
    while(getline(&cur_line, &cur_line_len, in) > 0) {
        line_num++;
        bool cmd_read = false;
        bool num_read = false;
        char *ptr = cur_line;
        for (; ptr < cur_line + cur_line_len; ptr++) {
            if (isspace(*ptr)) {
                continue;
            }
            if (is_end_line(*ptr)) {
                break;
            } else if (cmd_read && ('0' <= *ptr && *ptr <= '9')) {
                // We believe, that if at least 1 digit found the conversion is correct
                cur.type = CONSTANT_DOUBLE;
                cur.constant_double = strtod(ptr, &ptr);
                cur.command_name = "";
                if (!isspace(*ptr) && !is_end_line(*ptr)) {
                    cry_parser_error(0, filename, line_num, cur_line, ptr);
                    free(cur_line);
                    return 1;
                }
                lexem_list.push_back(cur);
                num_read = true;
                if (is_end_line(*ptr)) {
                    break;
                }
            } else if (!cmd_read && isalpha(*ptr)) {
                char *begin = ptr;
                while (isalpha(*ptr)) {
                    ptr++;
                }
                if (!isspace(*ptr) && !is_end_line(*ptr)) {
                    cry_parser_error(1, filename, line_num, cur_line, ptr);
                    free(cur_line);
                    return 1;
                }
                cur.type = COMMAND;
                cur.command_name = std::string(begin, ptr);
                cur.constant_double = NAN;
                lexem_list.push_back(cur);
                cmd_read = true;
                if (is_end_line(*ptr)) {
                    break;
                }
            } else {
                cry_parser_error(2, filename, line_num, cur_line, ptr);
                free(cur_line);
                return 1;
            }
        }
    }
    free(cur_line);
    return 0;
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Not enough arguments passed\n");
        printf("Please call this program as: %s infile outfile", argv[0]);
        return 0;
    }
    char * in_file = argv[1];
    char * out_file = argv[2];
    FILE * in = fopen(in_file, "r");
    if (!in) {
        printf("Unable to open file: %s", in_file);
        return 0;
    }
    FILE * out = fopen(out_file, "wb");
    if (!out) {
        printf("Unable to open (or create) file: %s", out_file);
    }
    
    std::vector<Lexem> lexem_list;

    if (parse(in, in_file, lexem_list) != 0) {
        return 1;
    }

    for (size_t i = 0; i < lexem_list.size(); i++) {
        printf("%s\t%s\t%lf\n", (lexem_list[i].type == COMMAND)?"CMD":"NUM", 
                        lexem_list[i].command_name.c_str(), lexem_list[i].constant_double);
    }



}