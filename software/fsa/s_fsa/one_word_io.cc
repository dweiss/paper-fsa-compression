/***	one_word_io.cc	***/

/*	Copyright (C) Jan Daciuk, 1996-2011	*/

#include	<iostream>
#include	<stdlib.h>
#include	<string.h>
#include	"fsa.h"
#include	"common.h"

/* Name:	tr_io
 * Class:	tr_io (constructor).
 * Purpose:	Initialize stream variables, allocate buffer if needed.
 * Parameters:	in_file		- (i) input stream to be used with tr_io;
 *		out_file	- (i) output stream to be used with tr_io;
 *		file_name	- (i) input file name;
 *		word_chars	- (i) characters that can form words
 *					(NULL means standard).
 * Returns:	Nothing.
 * Remarks:	I/O operations in one_word_io module are so simple, that
 *		no buffer is allocated.
 *		The input_file_name variable is not used here.
 */
tr_io::tr_io(istream *in_file, ostream &out_file, const int max_line_length,
	     const char *file_name,
#ifdef UTF8
	     word_syntax_type *
#else
	     const char *
#endif
	     word_chars)
: output(out_file),
  word_syntax(
#ifndef UTF8
	      (char *)
#endif
	      word_chars),
  Max_line_len(max_line_length), input_file_name(file_name)
{
  input = in_file;
  inp_buf_len = max_line_length;
  proc_state = stream_state = 1;
}//tr_io::tr_io


/* Name:	~tr_io
 * Class:	tr_io
 * Purpose:	Deallocate memory.
 * Parameters:	None.
 * Returns:	Nothing.
 * Remarks:	Since no buffer is allocated, there is nothing to deallocate,
 *		and generally there is nothing to do.
 */
tr_io::~tr_io(void)
{
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
  input->getline(s, inp_buf_len, '\n');
  bool read_more = input->fail();
  stream_state = !input->eof();
  junk = '\n';
  if (read_more) {
    junk = '\0';
    input->clear();		// Stroustrup just forgot to mention that
    output << s;
  }
  else {
    output << s << ":";
  }
  return *this;
}//tr_io::operator>>

/* Name:	print_OK
 * Class:	tr_io
 * Purpose:	Prints OK on output.
 * Parameters:	None.
 * Returns:	this.
 * Remarks:	I know that this `i = ...' thing is horrible.
 */
tr_io &
tr_io::print_OK(void)
{
  int i;

  i = (output << " *OK*\n") ? 1 : 0;
  stream_state = i;
  return *this;
}//tr_io::print_OK


/* Name:	print_not_found
 * Class:	tr_io
 * Purpose:	Prints "*not found*" on output.
 * Parameters:	None.
 * Returns:	this.
 * Remarks:	I know that this `i = ...' thing is horrible.
 */
tr_io &
tr_io::print_not_found(void)
{
  int	i;

  i = (output << " *not found*\n") ? 1 : 0;
  stream_state = i;
  return *this;
}//tr_io::print_not_found

/* Name:	print_repl
 * Class:	tr_io
 * Purpose:	Prints a replacement for an incorrect word.
 * Parameters:	r		- (i) list of replacements.
 * Returns:	this.
 * Remarks:	I Know that this `i = ...' thing is horrible.
 */
tr_io &
tr_io::print_repls(word_list *r)
{
  int i;
  int is_first = 1;

  for (r->reset(); r->item(); r->next()) {
    output << (is_first ? " " : ", ") << r->item();
    is_first = 0;
  }
  i = (output << "\n") ? 1 : 0;
  stream_state = i;
  return *this;
}//tr_io::print_repl


/* Name:	print_morph
 * Class:	tr_io
 * Purpose:	Prints a morphology list for a word.
 * Parameters:	s		- (i) list of morphological descriptions.
 * Returns:	this.
 * Remarks:	I Know that this `i = ...' thing is horrible.
 */
tr_io &
tr_io::print_morph(word_list *s)
{
  int i;

  for (s->reset(); s->item(); s->next())
    output << " " << s->item();
  i = (output << "\n") ? 1 : 0;
  stream_state = i;
  return *this;
}//tr_io::print_morph


/* Name:	print_line
 * Class:	tr_io
 * Purpose:	Prints a string on the output.
 * Parameters:	s		- (i) the string.
 * Returns:	this.
 * Remarks:	A new-line character is appended.
 */
tr_io &
tr_io::print_line(const char *s)
{
  int i;
  i = (output << s << "\n") ? 1 : 0;
  stream_state = i;
  return *this;
}//tr_io::print_line


/***	EOF one_word_io	***/
