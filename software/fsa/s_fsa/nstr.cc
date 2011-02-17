/***	nstr.cc		***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

#include	<string.h>
#include	"nstr.h"

/* Name:	nstrdup
 * Class:	None.
 * Purpose:	Create a copy of the given string in dynamically allocated
 *		memory.
 * Parameters:	word	- (i) string to copy.
 * Returns:	A pointer to the copy.
 * Remarks:	strdup that uses `new' instead of `malloc'.
 */
char *
nstrdup(const char *word)
{
  char *n = new char[strlen(word) + 1];
  return strcpy(n, word);
}//nstrdup

/* Name:	nnstrdup
 * Class:	None.
 * Purpose:	Create a copy of the first characters of the given string
 *		in dynamically allocated memory.
 * Parameters:	word	- (i) string to copy;
 *		n	- (i) number of characters to copy.
 * Returns:	A pointer to the copy.
 * Remarks:	Uses new, not malloc.
 */
char *
nnstrdup(const char *word, const int n)
{
  char *s = new char[n + 1];
  strncpy(s, word, n);
  s[n] = '\0';
  return s;
}//nnstrdup

/* Name:	grow_string
 * Class:	None.
 * Purpose:	Make a copy of a string with more space allocated.
 * Parameters:	s		- (i/o) the string / its copy;
 *		allocated	- (i/o) size of old/new string;
 *		alloc_step	- (i) size of old string - size of new string.
 * Returns:	New string.
 * Remarks:	None.
 */
char *
grow_string(char *&s, int &allocated, const int alloc_step)
{
  char *new_s;

  new_s = new char[allocated + alloc_step];
  memcpy(new_s, s, (size_t)allocated);
  allocated += alloc_step;
  delete [] s;
  s = new_s;
  return new_s;
}

/***	EOF nstr.cc	***/
