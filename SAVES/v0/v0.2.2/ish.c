/*--------------------------------------------------------------------*/
/* ish.c                                                              */
/* Author: Shreshth Singhal                                           */
/* An interactive shell                                               */
/*--------------------------------------------------------------------*/

#define _GNU_SOURCE

#include "dynarray.h"
#include "lex.h"
#include "syn.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*--------------------------------------------------------------------*/

enum {MAX_LINE_SIZE = 1024};

enum {FALSE, TRUE};

enum {PERMISSIONS = 0600};
/* Permissions for file descriptor creation. */

static char *pcPrompt = "%";
/* Shell prompt character. */

static int iSigquitFlag = FALSE;
/* Flag for SIGQUIT signal handling. */

/*--------------------------------------------------------------------*/

static void Ish_sigquitHandler(int iSignal)
   
/* Signal handler for SIGQUIT. */

{
   /* If flag is TRUE, exit. */
   if (iSigquitFlag == TRUE)
   {
      printf("%s", "Quit\n");
      exit(0);
   }
   /* If flag is FALSE, set flag to TRUE and start timer. */
   else
   {
      printf("Type Ctrl-\\ again within 5 seconds to exit.\n%s ", 
             pcPrompt);
      fflush(NULL);
      iSigquitFlag = TRUE;
      alarm(5);
   }
}

/*--------------------------------------------------------------------*/

static void Ish_sigalrmHandler(int iSignal)
   
/* Signal handler for SIGALRM. */

{
   /* Sets flag to FALSE if timer finishes. */
   iSigquitFlag = FALSE;
}

/*--------------------------------------------------------------------*/
 
static char **Ish_createArgv(DynArray_T oCmds)

/* Create argv array corresponding to oCmds and return pointer to it. */

{
   char **apcArgv = NULL;
   Cmd_T psCmd = NULL;
   int iArgs = 0;
   int i;

   assert(oCmds != NULL);

   /* Number of arguments in command. */
   for (i = 0; i < DynArray_getLength(oCmds); i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      if (Syn_returnType(psCmd) == CMD_ARG)
         iArgs++;
   }
   
   /* Create apcArgv array, of size iArgs + 2 (to hold command 
      name, arguments and NULL terminator). */
   apcArgv = (char**)calloc((size_t)(iArgs + 2), sizeof(char*));
   if (apcArgv == NULL)
      return NULL;
   
   /* Assign values to apcArgv elements. */
   iArgs = 1;
   for (i = 0; i < DynArray_getLength(oCmds); i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      /* Command name. */
      if (Syn_returnType(psCmd) == CMD_CMD)
         apcArgv[0] = Syn_returnValue(psCmd);
      /* Command arguments. */
      if (Syn_returnType(psCmd) == CMD_ARG)
         apcArgv[iArgs++] = Syn_returnValue(psCmd);
   }
   apcArgv[iArgs] = NULL;
 
   return apcArgv;
}
   
/*--------------------------------------------------------------------*/            

static int Ish_redirectStdin(DynArray_T oCmds, char *pcProgName)

/* Redirect stdin based on oCmds. Function requires calling program's
   name for error-checking. Returns 1 (TRUE) if successful, or 0 (FALSE)
   if an error occured. */
   
{
   int i;
   int iFd;
   int iRet;
   int iErrSv;
   Cmd_T psCmd = NULL;
   char *pcFileName = NULL;

   assert(oCmds != NULL);
   assert(pcProgName != NULL);

   /* Check for stdin redirection. */
   for (i = 0; i < DynArray_getLength(oCmds); i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      /* Redirect stdin, if required. */
      if (Syn_returnType(psCmd) == CMD_STDIN)
      {
         pcFileName = Syn_returnValue(psCmd);
         if (pcFileName != NULL)
         {
            /* Create new file descriptor. */
            iFd = open(pcFileName, O_RDONLY);
            if (iFd == -1) 
            {
               iErrSv = errno; 
               fprintf(stderr, "%s: %s: %s\n", pcProgName,  pcFileName, 
                       strerror(iErrSv));
               return FALSE; 
            }
            
            /* Close stdin file descriptor. */
            iRet = close(0);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }

            /* Duplicate new file descriptor to stdin. */
            iRet = dup(iFd);
            if (iRet == -1) 
           { 
              perror(pcProgName);
               return FALSE; 
            }            
            /* Close the file descriptor created in first step. */
            iRet = close(iFd);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }
         }
         return TRUE;
      }
   }
   return TRUE;
}

/*--------------------------------------------------------------------*/            

static int Ish_redirectStdout(DynArray_T oCmds, char *pcProgName)

/* Redirect stdout based on oCmds. Function requires calling program's
   name for error-checking. Returns 1 (TRUE) if successful, or 0 (FALSE)
   if an error occured. */
   
{
   int i;
   int iFd;
   int iRet;
   int iErrSv;
   Cmd_T psCmd = NULL;
   char *pcFileName = NULL;
   
   assert(oCmds != NULL);
   assert(pcProgName != NULL);

  /* Check for stdout redirection. */
   for (i = 0; i < DynArray_getLength(oCmds); i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      /* Redirect stdout, if required. */
      if (Syn_returnType(psCmd) == CMD_STDOUT)
      {
         pcFileName = Syn_returnValue(psCmd);
         if (pcFileName != NULL)
         {
            /* Create new file descriptor. */
            iFd = creat(pcFileName, PERMISSIONS);
            if (iFd == -1) 
            {
               iErrSv = errno; 
               fprintf(stderr, "%s: %s: %s\n", pcProgName,  pcFileName, 
                       strerror(iErrSv));
               return FALSE; 
            }

            /* Close stdout file descriptor. */
            iRet = close(1);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }
            
            /* Duplicate new file descriptor to stdout. */
            iRet = dup(iFd);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }
            
            /* Close the file descriptor created in first step. */
            iRet = close(iFd);
            if (iRet == -1) 
            {
               perror(pcProgName);
               return FALSE; 
            }
         }
         return TRUE;
      }
   }
   return TRUE;
}

/*--------------------------------------------------------------------*/            

static int Ish_handleCd(DynArray_T oCmds, char *pcProgName)

/* Checks if oCmds is a cd command. If so, it executes this command
   using chdir(). Returns 1 if command is cd (regardless of 
   successful or unsuccessful execution), 0 otherwise. */

{
   int i;
   int iLength;
   int iFlag = FALSE;
   int iArgs = 0;
   int iLastDir = 0;
   int iErrSv;
   Cmd_T psCmd = NULL;
   Cmd_T psArg = NULL;
   CmdType eType;
   char *pcPath = NULL;

   assert(oCmds != NULL);
   assert(pcProgName != NULL);

   iLength = DynArray_getLength(oCmds);

   /* Go through entire command. */
   for (i = 0; i < iLength; i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      eType = Syn_returnType(psCmd);
      /* Check if command name is cd. */
      if (eType == CMD_CMD)
      {
         if (strcmp(Syn_returnValue(psCmd), "cd") == 0)
            iFlag = TRUE;
         else
            break;
      }
      /* Check for number of arguments. */
      else if (eType == CMD_ARG)
      {
         iArgs++;
         psArg = psCmd;
      }
   }
   
   /* If command is not cd. */
   if (!iFlag)
      return FALSE;
   /* Execute cd. */
   else
   {
      /* Too many arguments. */
      if (iArgs > 1)
      {
         fprintf(stderr, "%s: cd: too many arguments\n", pcProgName);
         return TRUE;
      }
      /* Change directory to home. */
      else if (iArgs == 0)
      {
         pcPath = getenv("HOME");
         if (chdir(pcPath) == -1)
         {
            iErrSv = errno; 
            fprintf(stderr, "%s: cd: %s: %s\n", pcProgName, pcPath,
                    strerror(iErrSv));
            return TRUE;
         }
      }
      /* Change directory. */
      else
      {
         /* Argument "." has no effect. */
         if (strcmp(Syn_returnValue(psArg), ".") == 0);
         /* Argument ".." causes a one-level up change in directory. */
         else if (strcmp(Syn_returnValue(psArg), "..") == 0)
         {
            pcPath = get_current_dir_name();
            /* Find last '/' */
            for (i = 0; i < (int)strlen(pcPath); i++)
               if (pcPath[i] == '/')
                  iLastDir = i;
            /* Truncate pcPath at last '/' */
            if (iLastDir != 0)
               pcPath[iLastDir] = '\0';
            else /* Case where path is only '/' */
               pcPath[iLastDir + 1] = '\0';
            /* Change directory. */
            if (chdir(pcPath) == -1)
            {
               iErrSv = errno; 
               fprintf(stderr, "%s: cd: %s: %s\n", pcProgName, pcPath,
                       strerror(iErrSv));
               return TRUE;
            }
         }
         /* Change directory to given path. */
         else
         {
            pcPath = Syn_returnValue(psArg);
            if (chdir(pcPath) == -1)
            {
               iErrSv = errno;
               fprintf(stderr, "%s: cd: %s: %s\n", pcProgName, pcPath, 
                       strerror(iErrSv));
               return TRUE;
            }
         }
      }
   return TRUE;
   }
}

/*--------------------------------------------------------------------*/            

static int Ish_handleSetenv(DynArray_T oCmds, char *pcProgName)

/* Checks if oCmds is a setenv command. If so, it executes this command
   using setenv(). Returns 1 if command is setenv (regardless of
   whether execution is successful or not), 0 otherwise. */

{
   int i;
   int iLength;
   int iFlag = FALSE;
   int iArgs = 0;
   int iErrSv;
   Cmd_T psCmd = NULL;
   Cmd_T psArg = NULL;
   char *pcArgs[2];
   char *pcArg = NULL;
   CmdType eType;
   
   assert(oCmds != NULL);
   assert(pcProgName != NULL);

   iLength = DynArray_getLength(oCmds);

   /* Go through entire command. */
   for (i = 0; i < iLength; i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      eType = Syn_returnType(psCmd);
      /* Check if command name is setenv. */
      if (eType == CMD_CMD)
      {
         if (strcmp(Syn_returnValue(psCmd), "setenv") == 0)
            iFlag = TRUE;
         else
            break;
      }
      /* Checks for number of arguments. */
      else if (eType == CMD_ARG)
      {
         iArgs++;
         psArg = psCmd;
      }
   }
   
   /* If command is not setenv. */
   if (!iFlag)
      return FALSE;
   /* Execute setenv. */
   else
   {
      /* Too many arguments. */
      if (iArgs > 2)
      {
         fprintf(stderr, "%s: setenv: too many arguments\n", 
                 pcProgName);
         return TRUE;
      }
      /* No arguments. */
      else if (iArgs == 0)
      {
         fprintf(stderr, "%s: setenv: missing variable\n", pcProgName);
         return TRUE;
      }
      /* Set variable to empty. */
      else if (iArgs == 1)
      {
         pcArg = Syn_returnValue(psArg);
         if(setenv(pcArg, "", 1) == -1)
         {
            iErrSv = errno; 
            fprintf(stderr, "%s: setenv: %s: %s\n", pcProgName, pcArg,
                    strerror(iErrSv));
            return TRUE;
         }
      }
      /* Set value of variable. */
      else
      {
         int j = 0;
         /* Find the 2 arguments. */
         for (i = 0; i < iLength; i++)
         {
            psCmd = (Cmd_T)DynArray_get(oCmds, i);
            if (Syn_returnType(psCmd) == CMD_ARG)
               pcArgs[j++] = Syn_returnValue(psCmd);
         }
         if(setenv(pcArgs[0], pcArgs[1], 1) == -1)
         {
            iErrSv = errno; 
            fprintf(stderr, "%s: setenv: %s: %s\n", pcProgName, pcArgs[0],
                    strerror(iErrSv));
            return TRUE;
         }
      }
   }
   return TRUE;
}

/*--------------------------------------------------------------------*/            

static int Ish_handleUnsetenv(DynArray_T oCmds, char *pcProgName)

/* Checks if oCmds is an unsetenv command. If so, it executes this 
   command using unsetenv(). Returns 1 if successful, 0 otherwise. */

{
   int i;
   int iLength;
   int iFlag = FALSE;
   int iArgs = 0;
   int iErrSv;
   char *pcArg = NULL;
   Cmd_T psCmd = NULL;
   Cmd_T psArg = NULL;
   CmdType eType;

   assert(oCmds != NULL);
   assert(pcProgName != NULL);
   
   iLength = DynArray_getLength(oCmds);

   /* Go through entire command. */
   for (i = 0; i < iLength; i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      eType = Syn_returnType(psCmd);
      /* Check if command name is unsetenv. */
      if (eType == CMD_CMD)
      {
         if (strcmp(Syn_returnValue(psCmd), "unsetenv") == 0)
            iFlag = TRUE;
         else
            break;
      }
      /* Checks for number of arguments. */
      else if (eType == CMD_ARG)
      {
         iArgs++;
         psArg = psCmd;
      }
   }
   
   /* If command is not unsetenv. */
   if (!iFlag)
      return FALSE;
   /* Execute unsetenv. */
   else
   {
      /* Too many arguments. */
      if (iArgs > 1)
      {
         fprintf(stderr, "%s: unsetenv: too many arguments\n", 
                 pcProgName);
         return TRUE;
      }
      /* No arguments. */
      else if (iArgs == 0)
      {
         fprintf(stderr, "%s: unsetenv: missing variable\n", 
                 pcProgName);
         return TRUE;
      }
      /* Remove value of variable. */
      else if (iArgs == 1)
      {
         pcArg = Syn_returnValue(psArg);
         if(unsetenv(pcArg) == -1)
         {
            iErrSv = errno; 
            fprintf(stderr, "%s: unsetenv: %s: %s\n", pcProgName, pcArg,
                    strerror(iErrSv));
            return TRUE;
         }
      }  
   }
   return TRUE;
}

/*--------------------------------------------------------------------*/            

static int Ish_handleExit(DynArray_T oCmds, char *pcProgName)

/* Checks if oCmds is an exit command. If so, it executes this command
   using exit(). Returns 1 if successful, 0 otherwise. */

{
   int i;
   int iLength;
   int iFlag = FALSE;
   int iArgs = 0;
   Cmd_T psCmd = NULL;
   CmdType eType;

   assert(oCmds != NULL);
   assert(pcProgName != NULL);

   iLength = DynArray_getLength(oCmds);

   /* Go through entire command. */
   for (i = 0; i < iLength; i++)
   {
      psCmd = (Cmd_T)DynArray_get(oCmds, i);
      eType = Syn_returnType(psCmd);
      /* Check if command name is exit. */
      if (eType == CMD_CMD)
      {
         if (strcmp(Syn_returnValue(psCmd), "exit") == 0)
            iFlag = TRUE;
         else
            break;
      }
      /* Check for number of arguments. */
      else if (eType == CMD_ARG)
         iArgs++;
   }
   
   /* If command is not exit. */
   if (!iFlag)
      return FALSE;
   /* Execute command. */
   else
   {
      /* Too many arguments. */
      if (iArgs > 0)
      { 
         fprintf(stderr, "%s: exit: too many arguments\n", pcProgName);
         return TRUE;
      }
      /* Execute exit command. */
      else
         exit(EXIT_SUCCESS);
   }
}

/*--------------------------------------------------------------------*/
  

static void Ish_cleanup(DynArray_T oTokens, DynArray_T oCmds)

/* Free oTokens and oCmds arrays. */

{
   DynArray_map(oTokens, Lex_freeToken, NULL);
   DynArray_free(oTokens);
   DynArray_map(oCmds, Syn_freeCmd, NULL);
   DynArray_free(oCmds);
}

/*--------------------------------------------------------------------*/            

static void Ish_executeCommand(char *acLine, DynArray_T oTokens, 
                               DynArray_T oCmds, char **apcArgv,
                               char *pcProgName)

/* Execute a command contained in acLine. Note that function requires 
   calling program's name for error-checking. */

{
   int iSuccessful;  
   int iErrSv;
   
   pid_t iPid;
   /* Process ID. */

   void (*pfRet)(int);
   /* Signal handling. */
   
   assert(acLine != NULL);
   /* No other asserts here because other params are empty arrays. */

   oTokens = DynArray_new(0);
   oCmds = DynArray_new(0);
   
   if (oTokens == NULL)
   {
      fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
      return;
   }
   if (oCmds == NULL)
   {
      fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
      return;
   }
   
   /* Lexical analysis stage. */
   iSuccessful = Lex_lexLine(acLine, oTokens, pcProgName);
   /* Check if successful lexical analysis, and if token
      array size is greater than zero. */
   if ((!iSuccessful) || (!DynArray_getLength(oTokens)))
   {  
      Ish_cleanup(oTokens, oCmds);
      return;
   }
  
   /* Syntactical parsing stage. */
   iSuccessful = Syn_synLine(oTokens, oCmds, pcProgName);
   /* Check if successful syntactical parsing. */
   if (!iSuccessful)
   {  
      Ish_cleanup(oTokens, oCmds);
      return;
   }  

   /* Check for shell built-in commands. */
   if (Ish_handleCd(oCmds, pcProgName))
   {  
      Ish_cleanup(oTokens, oCmds);
      return;
   }
   else if (Ish_handleSetenv(oCmds, pcProgName))
   {  
      Ish_cleanup(oTokens, oCmds);
      return;
   } 
   else if (Ish_handleUnsetenv(oCmds, pcProgName))
   {  
      Ish_cleanup(oTokens, oCmds);
      return;
   } 
   else if (Ish_handleExit(oCmds, pcProgName))
   {  
      Ish_cleanup(oTokens, oCmds);
      return;
   } 
      
   /* Create apcArgv array. */
   apcArgv = Ish_createArgv(oCmds);
   if (apcArgv == NULL)
   {
      fprintf(stderr, "%s: cannot allocate memory\n", pcProgName);
      return;
   }
   
   /* Flush buffers and fork. */
   fflush(NULL);
   iPid = fork();
   if(iPid == -1)
   {
      perror(pcProgName);
      return;
   }
   
   if (iPid == 0)
   {
      /* Child process. */

      /* Restore SIGINT to default behaviour. */
      pfRet = signal(SIGINT, SIG_DFL);
      assert(pfRet != SIG_ERR);

      /* Restore SIGQUIT to default behaviour. */
      pfRet = signal(SIGQUIT, SIG_DFL);
      assert(pfRet != SIG_ERR);

      /* Redirect stdin and stdout. */
      if(!Ish_redirectStdin(oCmds, pcProgName))
         exit(EXIT_FAILURE);
      if(!Ish_redirectStdout(oCmds, pcProgName))
         exit(EXIT_FAILURE);
      
      /* Execute command. */
      execvp(apcArgv[0], apcArgv);
      iErrSv = errno;
      fprintf(stderr, "%s: %s: %s\n", pcProgName, apcArgv[0], 
              strerror(iErrSv));
      exit(EXIT_FAILURE);
   }
   
   /* Parent process. */
   iPid = wait(NULL);
   if (iPid == -1)
   {
      perror(pcProgName);
      return;
   }
   /* Free argv array. */
   free(apcArgv);
   
   /* Free oTokens and oCmds arrays. */
   DynArray_map(oTokens, Lex_freeToken, NULL);
   DynArray_free(oTokens);
   DynArray_map(oCmds, Syn_freeCmd, NULL);
   DynArray_free(oCmds);
}

/*--------------------------------------------------------------------*/

int main(int argc, char *argv[])
   
{
   int i;

   char acLine[MAX_LINE_SIZE];
   /* String to hold one line of command. */

   DynArray_T oTokens = NULL;
   /* Array to hold tokens after lexical analysis. */
   
   DynArray_T oCmds = NULL;
   /* Array to hold command after syntactical parsing. */
 
   char **apcArgv = NULL;
   /* argv array for command. */

   int iLength;
   char *pcPath = NULL;
   char *pcTemp = NULL;
   /* Path of .ishrc. */

   FILE *psFile = NULL;
   /* File to hold .ishrc. */

   sigset_t sSet;
   void (*pfRet)(int);
   int iRet;
   /* Signal handling. */

   /* Make sure SIGINT signals are not blocked. */
   sigemptyset(&sSet);
   sigaddset(&sSet, SIGINT);
   iRet = sigprocmask(SIG_UNBLOCK, &sSet, NULL);
   assert(iRet == 0);

   /* Make sure SIGQUIT signals are not blocked. */
   sigemptyset(&sSet);
   sigaddset(&sSet, SIGQUIT);
   iRet = sigprocmask(SIG_UNBLOCK, &sSet, NULL);
   assert(iRet == 0);

   /* Make sure SIGALRM signals are not blocked. */
   sigemptyset(&sSet);
   sigaddset(&sSet, SIGALRM);
   iRet = sigprocmask(SIG_UNBLOCK, &sSet, NULL);
   assert(iRet == 0);

   /* Ignore SIGINT signals. */
   pfRet = signal(SIGINT, SIG_IGN);
   assert(pfRet != SIG_ERR);

   /* Install Ish_sigquitHandler as handler for SIGQUIT signals. */
   pfRet = signal(SIGQUIT, Ish_sigquitHandler);
   assert(pfRet != SIG_ERR);

   /* Install Ish_sigalrmHandler as handler for SIGALRM signals. */
   pfRet = signal(SIGALRM, Ish_sigalrmHandler);
   assert(pfRet != SIG_ERR);

   /* Display shell prompt. */
   printf("%s ", pcPrompt);

   /* Set path to .ishrc file. */
   pcTemp = getenv("HOME");
   if(pcTemp != NULL)
   {
      iLength = (int)strlen(pcTemp);
      /* Allocate enough space to hold /.ishrc along with HOME dir. */
      pcPath = (char*)malloc(iLength + strlen("/.ishrc") + 1);
      if (pcPath == NULL)
         fprintf(stderr, "%s: cannot allocate memory\n", argv[0]);
      else
      {
         for(i = 0; i < iLength; i++)
            pcPath[i] = pcTemp[i];
         pcPath[i] = '\0'; 
         pcPath = strcat(pcPath, "/.ishrc");
         /*  Open .ishrc as a stream */
         psFile = fopen(pcPath, "r");
         free(pcPath);
         /* Read, print and execute from .ishrc. */
         if(psFile != NULL)
         {
            while (fgets(acLine, MAX_LINE_SIZE, psFile) != NULL)
            {
               printf("%s", acLine);
               if(strlen(acLine))
                  Ish_executeCommand(acLine, oTokens, oCmds, apcArgv, argv[0]);
               acLine[0] = '\0';
               printf("%s ", pcPrompt);
            }
         }
      }
   }

   /* Read lines from stdin. */
   while (fgets(acLine, MAX_LINE_SIZE, stdin) != NULL)
   {
      if(strlen(acLine))
         Ish_executeCommand(acLine, oTokens, oCmds, apcArgv, argv[0]);
      acLine[0] = '\0';
      printf("%s ", pcPrompt);
   }

   printf("%s", "\n");
   
   return 0;
}

/*--------------------------------------------------------------------*/
