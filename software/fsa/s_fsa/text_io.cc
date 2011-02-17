/***	text_io.cc	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

#include	<iostream>
#include	<fstream>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"fsa.h"
#include	"common.h"

static const int	INIT_MAX_INP_BUF = 1024;
static const int	Buf_len = 512;

/* Name:	tr_io
 * Class:	tr_io (constructor).
 * Purpose:	Initialize stream variables, allocate input buffer.
 * Parameters:	in_file		- (i) input stream to be used with tr_io;
 *		out_file	- (i) output stream to be used with tr_io;
 *		file_name	- (i) input file name;
 *		word_chars	- (i) characters that are parts of a word
 *					(NULL means standard, i.e. A-Z,a-z).
 * Returns:	Nothing.
 * Remarks:	
 */
tr_io::tr_io(istream *in_file, ostream &out_file, const int max_line_length,
	     const char *file_name, const char *word_chars)
: output(out_file), Max_line_len(max_line_length), word_syntax(word_chars),
  input_file_name(file_name)
{
  const char *p;

  input = in_file;
  proc_state = stream_state = 1;
  buffer = new char[INIT_MAX_INP_BUF];
  *buffer = '\0';
  inp_line_no = 0;
  inp_line_char_no = 0;
  inp_word_len = 0;
  if (word_chars == NULL) {
#ifdef UTF8
    word_syntax = NULL;
#else
    word_syntax = new char[256];
    p = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 256; i++)
      word_syntax[i] = FALSE;
    while (*p) {
      word_syntax[(unsigned char)*p] = (islower(*p) ? 3 : 2);
      p++;
    }
#endif
  }
#ifdef UTF8
  word_syntax_size = 512;
#endif
}//tr_io::tr_io


/* Name:	~tr_io
 * Class:	tr_io
 * Purpose:	Deallocate memory.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	None.
 */
tr_io::~tr_io(void)
{
  delete [] buffer;
}//tr_io::~tr_io


/* Name:	operator>>
 * Class:	None (friend of class tr_io).
 * Purpose:	Read word from input and echo it on output with a trailing
 *		colon.
 * Parameters:	s		- (o) where to put the string read.
 * Returns	in_file.
 * Remarks:	One word per line is assumed. Lines longer than Max_word_len
 *		are truncated.
 */
tr_io &
tr_io::operator>>(char *s)
{
  static	buf_len = INIT_MAX_INP_BUF;
  static char	*bufp = NULL;

  if (bufp == NULL)
    bufp = buffer;

  // skip non-word characters
  while (!is_word_char(*bufp) && *bufp)
    bufp++;
  // read next line if necessary
  while (*bufp == '\0' && input->good()) {
    inp_line_no++;
    input->get(buffer, buf_len, '\n');
    input->get(junk);
    while (input->good() && junk != '\n') {
      // buffer was too small
      // allocate new buffer
      char *b = new char[buf_len + INIT_MAX_INP_BUF];
      memcpy(b, buffer, buf_len);
      delete [] buffer;
      buffer = b;
      buffer[buf_len] = junk;
      input->get(buffer + buf_len, INIT_MAX_INP_BUF - 1, '\n');
      input->get(junk);
      buf_len += INIT_MAX_INP_BUF;
    }
    bufp = buffer;
    stream_state = input->good();

    // skip non-word characters
    while (!is_word_char(*bufp) && *bufp)
      bufp++;
  }

  // word found - record its position
  inp_line_char_no = bufp - buffer;
  inp_word_len = 0;
  while (is_word_char(*bufp) && ++inp_word_len < Max_word_len)
    *s++ = *bufp++;
  *s = '\0';
  inp_word_len = (bufp - buffer) - inp_line_char_no;
  return *this;
}//tr_io::operator>>

/* Name:	print_OK
 * Class:	tr_io
 * Purpose:	Reacts to correct words (does nothing).
 * Parameters:	None.
 * Returns:	this.
 * Remarks:	Since no news is good news, good news is no news here!
 */
tr_io &
tr_io::print_OK(void)
{
  return *this;
}//tr_io::print_OK


/* Name:	print_not_found
 * Class:	tr_io
 * Purpose:	Prints "*not found*" on output.
 * Parameters:	None.
 * Returns:	this.
 * Remarks:	I know that this `i = ...' thing is horrible. g++ 2.6.0
 *		presented with `stream_state = (int)output' printed:
 *		"`class ostream' used where an `int' was expected"!!!
 *		The same was with `stream_state = int(output)'.
 */
tr_io &
tr_io::print_not_found(void)
{
  int	i;

  i = (output << input_file_name << ":" << inp_line_no << ":"
       << inp_line_char_no << ":" << inp_word_len << " *not found*\n") ? 1 : 0;
  stream_state = i;
  return *this;
}//tr_io::print_not_found

/* Name:	print_repl
 * Class:	tr_io
 * Purpose:	Prints a replacement for an incorrect word.
 * Parameters:	r		- (i) list of replacements.
 * Returns:	this.
 * Remarks:	I Know that this `i = ...' thing is horrible. g++ 2.6.0
 *		presented with `stream_state = (int)output' printed:
 *		"`class ostream' used where an `int' was expected"!!!
 *		The same was with `stream_state = int(output)'.
 */
tr_io &
tr_io::print_repls(word_list *r)
{
  int i;
  int j;

  i = (output << input_file_name << ":" << inp_line_no << ":"
       << inp_line_char_no << ":" << inp_word_len << ": ") ? 1 : 0;
  if ((stream_state = i) == 0)
    return *this;
  j = 1;
  for (r->reset(); r->item(); r->next())
    output << j++ << ") " << r->item() << " ";
  i = (output << "\n") ? 1 : 0;
  stream_state = i;
  return *this;
}//tr_io::print_repl


/* Name:	print_morph
 * Class:	tr_io
 * Purpose:	Prints a morphology list for a word.
 * Parameters:	s		- (i) list of morphological descriptions.
 * Returns:	this.
 * Remarks:	I Know that this `i = ...' thing is horrible. g++ 2.6.0
 *		presented with `stream_state = (int)output' printed:
 *		"`class ostream' used where an `int' was expected"!!!
 *		The same was with `stream_state = int(output)'.
 */
tr_io &
tr_io::print_morph(word_list *s)
{
  int i;

  i = (output << input_file_name << ":" << inp_line_no << ":"
       << inp_line_char_no << ":" << inp_word_len << ": ") ? 1 : 0;
  if ((stream_state = i) == 0)
    return *this;
  for (word_list *w = s; w; w->next())
    output << " " << w->item();
  i = (output << "\n") ? 1 : 0;
  stream_state = i;
  return *this;
}//tr_io::print_morph

/***	EOF text_io.cc	***/
