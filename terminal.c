#include <stdlib.h>   // for env()
#include <string.h>
#include <curses.h>
#include <term.h>
// #include <termios.h>
#include <unistd.h>     // STDIN_IN, etc
#include <stdbool.h>
#include <assert.h>

#include "export.h"
#include "terminal.h"


bool term_is_setup = false;

EXPORT bool launch_terminal(void)
{
   if (!term_is_setup)
   {
      int error_return;
      int result = setupterm((char*)NULL, STDIN_FILENO, &error_return);
      if (result)
      {
         switch(error_return)
         {
            case 1: printf("curses unavailable in hardcopy mode.\n"); break;
            case 0: printf("curses unavailable in generic terminal.\n"); break;
            case -1: printf("curses unavailable, terminfo database missing.\n"); break;
         }
         return false;
      }
      else
         term_is_setup = true;
   }

   return term_is_setup;
}

/**
 * @brief Get more inclusive capname from deprecated capcode
 *
 * Given a two-character capcode (like those used for LESS_TERMCAP_
 * environment variables), get the associated capname to be used
 * for retrieving a termcap value.
 */
EXPORT const char *capname_from_code(const char *capcode)
{
   const char * const *names = strnames;
   const char * const *codes = strcodes;
   const char * const *ptr = codes;
   while (*ptr)
   {
      if (strcmp(*ptr, capcode)==0)
      {
         int index = ptr - strcodes;
         return names[index];
      }
      ++ptr;
   }

   return NULL;
}

/**
 * @brief Set a string pointer to a termcap string matching the capname
 */
EXPORT bool get_termcap_by_name(const char **value, const char *capname)
{
   const char *capval = tigetstr(capname);
   if (capval)
   {
      *value = capval;
      return true;
   }

   return false;
}


/**
 * @brief Get console string from termcap code
 *
 * Use this function to get console strings, from how to set bold,
 * italic or other modes, to getting or setting console values like
 * screen size, cursor position, etc.
 *
 * First checked is an override value from an environment variable
 * with the "LESS_TERMCAP_" prefix, then the value is sought from the
 * terminfo database.
 *
 * The @p value pointer-to-pointer argument will be set with the
 * system/environment string if found.
 *
 * @param "value"    pointer to string pointer to which the value will be referenced
 * @param "capcode"  2-character termcap code for which to search
 * @return true if the termcap value was found, false if not.
 */
EXPORT bool get_termcap_by_code(const char **value, const char *capcode)
{
   launch_terminal();

   char buff[16];
   snprintf(buff, sizeof(buff), "LESS_TERMCAP_%-s2", capcode);

   const char *capstr = getenv(buff);
   if (capstr == NULL)
   {
      const char *capname = capname_from_code(capcode);
      if (capname)
         return get_termcap_by_name(value, capname);
         // capstr = tigetstr(capname);
   }

   if (capstr)
   {
      *value = capstr;
      return true;
   }

   return false;
}

/**
 * @brief Prepare terminal to respond to an individual keystroke
 *
 * This function combines the function of possibly-missing function
 * `cfmakeraw` for raw input along with setting read parameters to
 * respond to a single keystroke.
 *
 * @param 'tos'   initialized termios variable to be modified
 */
void set_rawread_mode(struct termios* tos)
{
   // Perform cfxmakeraw() transformations (refer to
   // cfmakeraw(3), search for 'Raw mode'):
   tos->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                           |INLCR|IGNCR|ICRNL|IXON);
   tos->c_oflag &= ~OPOST;
   tos->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
   tos->c_cflag &= ~(CSIZE|PARENB);
   tos->c_cflag |= CS8;

   // termios(3), search "read with interbyte timeout"
   tos->c_cc[VMIN] = 1;
   tos->c_cc[VTIME] = 1;
}



