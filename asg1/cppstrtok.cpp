// $Id: cppstrtok.cpp,v 1.1 2019-04-15 14:54:04-07 - - $
// Wai Chun Leung
// wleung11

// Use cpp to scan a file and print line numbers.
// Print out each input line read in, then strtok it for
// tokens.

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>

#include "auxlib.h"
#include "string_set.h"

const string CPP = "/usr/bin/cpp -nostdinc";
string OCLIB = "";
constexpr size_t LINESIZE = 1024;

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
   size_t len = strlen (string);
   if (len == 0) return;
   char* nlpos = string + len - 1;
   if (*nlpos == delim) *nlpos = '\0';
}

// Print the meaning of a signal.
static void eprint_signal (const char* kind, int signal) {
   fprintf (stderr, ", %s %d", kind, signal);
   const char* sigstr = strsignal (signal);
   if (sigstr != nullptr) fprintf (stderr, " %s", sigstr);
}

// Print the status returned from a subprocess.
void eprint_status (const char* command, int status) {
   if (status == 0) return; 
   fprintf (stderr, "%s: status 0x%04X", command, status);
   if (WIFEXITED (status)) {
      fprintf (stderr, ", exit %d", WEXITSTATUS (status));
   }
   if (WIFSIGNALED (status)) {
      eprint_signal ("Terminated", WTERMSIG (status));
      #ifdef WCOREDUMP
      if (WCOREDUMP (status)) fprintf (stderr, ", core dumped");
      #endif
   }
   if (WIFSTOPPED (status)) {
      eprint_signal ("Stopped", WSTOPSIG (status));
   }
   if (WIFCONTINUED (status)) {
      fprintf (stderr, ", Continued");
   }
   fprintf (stderr, "\n");
}


// Run cpp against the lines of the file.
void cpplines (FILE* pipe, const char* filename) {
   int linenr = 1;
   for (;;) {
      char buffer[LINESIZE];
      // Get next line
      const char* fgets_rc = fgets (buffer, LINESIZE, pipe);
      // If null, end
      if (fgets_rc == nullptr) break;
      // Chomp off the last delim char
      chomp (buffer, '\n');
      // Current file and line number
      printf ("%s:line %d: [%s]\n", filename, linenr, buffer);
      
      // http://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
      // Format the line into linenr and inputname
      char inputname[LINESIZE];
      int sscanf_rc = sscanf (buffer, "# %d \"%[^\"]\"",
                              &linenr, inputname);
      if (sscanf_rc == 2) {
         printf ("DIRECTIVE: line %d file \"%s\"\n", linenr, inputname);
         continue;
      }

      char* savepos = nullptr;
      char* bufptr = buffer;
      for (int tokenct = 1;; ++tokenct) { // while true loop
         // tokenize the bufptr by spaces, tabs, and newlines
         char* token = strtok_r (bufptr, " \t\n", &savepos);
         bufptr = nullptr;
         // if token is empty, end loop
         if (token == nullptr) break;
         // else print token
         printf ("token %d.%d: [%s]\n",
                 linenr, tokenct, token);
      }
      ++linenr;
   }
}

void fill_stringset(FILE* pipe) {
   int linenr = 1;
   for (;;) {
      char buffer[LINESIZE];
      // Get next line
      const char* fgets_rc = fgets (buffer, LINESIZE, pipe);
      // If null, end
      if (fgets_rc == nullptr) break;
      // Chomp off the last delim char
      chomp (buffer, '\n');


      char inputname[LINESIZE];
      int sscanf_rc = sscanf (buffer, "# %d \"%[^\"]\"",
                              &linenr, inputname);
      if (sscanf_rc == 2)
         continue;

      char* savepos = nullptr;
      char* bufptr = buffer;
      for (int tokenct = 1;; ++tokenct) { // while true loop
         // tokenize the bufptr by spaces, tabs, and newlines
         char* token = strtok_r (bufptr, " \t\n", &savepos);
         bufptr = nullptr;
         // if token is empty, end loop
         if (token == nullptr) break;
         string_set::intern(token);
         // // else print formatted token
         // printf ("token %d.%d: [%s]\n",
         //         linenr, tokenct, token);
      }
      ++linenr;
   }
}

// Referenced from http://man7.org/linux/man-pages/man3/getopt.3.html
// Parses command line options
void parseargs(int argc, char** argv) {
   int opt;
   while ((opt = getopt(argc, argv, "yl@:D:")) != -1) {
      switch (opt) {
         case 'y':
            // flex placeholder
            break;
         case 'l':
            // flex placeholder
            break;
         case '@':
            set_debugflags(optarg);
            break;
         case 'D':
            OCLIB = "-D" + std::string(optarg);
            break;
         default:
            exit(-1);
      }
   }
}

int main (int argc, char** argv) {
   int exit_status = EXIT_SUCCESS;

   // Parse Args, pass in arguments excluding file
   parseargs(argc-1, argv);

   // Filename is the last argument
   string filename = argv[argc-1];
   // cout << "Filename: " << filename << endl;

   // Debug test
   DEBUGF('f',"debugging on\n");

   // Command Source with options
   // If the D option is on, send it to cpp
   string command;
   if (OCLIB.length() > 0) 
      command = CPP + " " + OCLIB + " " + filename;
   else 
      command = CPP + " " + filename;
   // printf ("command=\"%s\"\n", command.c_str());

   // Arbitrary basename of the program oc
   const char* execname = basename (argv[0]);

   FILE* pipe = popen (command.c_str(), "r"); // Open pipe

   if (pipe == nullptr) {
      // Fail if pipe didnt open correctly
      exit_status = EXIT_FAILURE;
      fprintf (stderr, "%s: %s: %s\n",
               execname, command.c_str(), strerror (errno));
   }else {
      // cpplines(pipe, filename); // Call function
      fill_stringset(pipe); // Fill stringset
      int pclose_rc = pclose (pipe); // Close pipe

      // Fail if pipe didnt close correctly
      eprint_status (command.c_str(), pclose_rc);
      if (pclose_rc != 0)
         exit_status = EXIT_FAILURE;
   }

   // string_set::dump(stdout);

   filename = filename.substr(0, filename.size()-3);
   filename += ".str";
   const char* outname = filename.c_str();
   FILE* outfile;
   outfile = fopen(outname,"w");
   string_set::dump(outfile);
   fclose(outfile);

   return exit_status;
}

