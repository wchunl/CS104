// $Id: main.cpp,v 1.18 2017-10-19 16:02:14-07 - - $
// Wai Chun Leung
// wleung11
// Shineng Tang
// stang38


#include <string>
#include <vector>
using namespace std;

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "astree.h"
#include "auxlib.h"
#include "lyutils.h"
#include "string_set.h"

const string cpp_name = "/usr/bin/cpp -nostdinc";
string cpp_command;
string OCLIB = "";
FILE* tok_out;

// Open a pipe from the C preprocessor.
// Exit failure if can't.
// Assigns opened pipe to FILE* yyin.
void cpp_popen (string filename) {
    if (OCLIB.length() > 0)
        cpp_command = cpp_name + " " + OCLIB + " " + filename;
    else 
        cpp_command = cpp_name + " " + filename;
    yyin = popen (cpp_command.c_str(), "r");
    if (yyin == nullptr) {
        syserrprintf (cpp_command.c_str());
        exit (exec::exit_status);
    }else {
        if (yy_flex_debug) {
            fprintf (stderr, "-- popen (%s), fileno(yyin) = %d\n",
                    cpp_command.c_str(), fileno (yyin));
        }
        lexer::newfilename (cpp_command);
    }
}

void cpp_pclose() {
    int pclose_rc = pclose (yyin);
    eprint_status (cpp_command.c_str(), pclose_rc);
    if (pclose_rc != 0) exec::exit_status = EXIT_FAILURE;
}

void scan_opts (int argc, char** argv) {
    opterr = 0;
    yy_flex_debug = 0;
    yydebug = 0;
    lexer::interactive = isatty (fileno (stdin))
                        and isatty (fileno (stdout));
    for(;;) {
        int opt = getopt (argc, argv, "@:D:ly");
        if (opt == EOF) break;
        switch (opt) {
            case '@': set_debugflags (optarg);   break;
            case 'l': yy_flex_debug = 1;         break;
            case 'y': yydebug = 1;               break;
            case 'D': OCLIB = "-D" + std::string(optarg);      break;
            default:  errprintf ("bad option (%c)\n", optopt); break;
        }
    }
    if (optind > argc) {
        errprintf ("Usage: %s [-ly] [filename]\n",
                    exec::execname.c_str());
        exit (exec::exit_status);
    }
    const char* filename = optind == argc ? "-" : argv[optind];
    cpp_popen (filename);
}

int main (int argc, char** argv) {
    exec::execname = basename (argv[0]);

    // Call scanopts, opens pipe
    scan_opts (argc, argv);

    // yylex token trace
    if (yydebug or yy_flex_debug) {
        fprintf (stderr, "Command:");
        for (char** arg = &argv[0]; arg < &argv[argc]; ++arg) {
            fprintf (stderr, " %s", *arg);
        }
        fprintf (stderr, "\n");
    }

    // Set filenames
    char* filename = argv[argc-1];
    string basename = string(filename).substr(0,string(filename).
                                    find('.'));
    string str_out_name = basename + ".str";
    string tok_out_name = basename + ".tok";

    // Open token file for streaming
    tok_out = fopen(tok_out_name.c_str(),"w");
    if(tok_out == NULL){
        exec::exit_status = EXIT_FAILURE;
    }else{
        // Call yylex() on file, mem leaks ignored
        for(;;){
            int t = yylex();
            if(t == YYEOF) break;
        }
    }

    // Close file and pipe
    fclose(tok_out);
    cpp_pclose();
    yylex_destroy();

    // Dump stringset into str file
    FILE* str_out = fopen(str_out_name.c_str(),"w");
    string_set::dump(str_out);
    int fclose_rc = fclose(str_out);
    if (fclose_rc != 0) exec::exit_status = EXIT_FAILURE;

    return exec::exit_status;
}
