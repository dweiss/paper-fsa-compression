/***	common.h		***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

/*
   Common types for programs that use dictionaries in form of final state
   automata.

*/

#include	<cstring>
#include	"nstr.h"

enum	{FALSE, TRUE};

#ifdef UTF8
#define NULLCHAR	1
#define ONEBYTE		2
#define CONTBYTE	4
#define INVLBYTE	8
#define START2		16
#define	START3		32
#define START4		64
#define RESTRBYTE	128

const int utf8type[256] = {
  /* 0 - 7 (0-7) */
  NULLCHAR, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 8 - 15 (8-f) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 16 - 23 (10-17) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 24 - 31 (18-1f) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 32 - 39 (20-27) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 40 - 47 (28-2f) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 48 - 55 (30-37) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 56 - 63 (38-3f) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 64 - 71 (40-47) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 72 - 79 (48-4f) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 80 - 87 (50-57) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 88 - 95 (58-5f) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 96 - 103 (60-67) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 104 - 111 (68-6f) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 112 - 119 (70-77) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 120 - 127 (78-7f) */
  ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE, ONEBYTE,
  /* 128 - 135 (80-87) */
  CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE,CONTBYTE,CONTBYTE, 
  /* 136 - 143 (88-8f) */
  CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE,CONTBYTE,CONTBYTE,
  /* 144 - 151 (90-97) */
  CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE,CONTBYTE,CONTBYTE,
  /* 152 - 160 (98-9f) */
  CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE,CONTBYTE,CONTBYTE,
  /* 161 - 168 (a0-a7) */
  CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE,CONTBYTE,CONTBYTE,
  /* 169 - 176 (a8-af) */
  CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE,CONTBYTE,CONTBYTE,
  /* 177 - 184 (b0-b7) */
  CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE,CONTBYTE,CONTBYTE,
  /* 185 - 192 (b8-bf) */
  CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE, CONTBYTE,CONTBYTE,CONTBYTE,
  /* 193 - 200 (c0-c7) */
  INVLBYTE, INVLBYTE, START2, START2, START2, START2, START2, START2,
  /* 201 - 208 (c8-cf) */
  START2, START2, START2, START2, START2, START2, START2, START2,
  /* 209 - 216 (d0-d7) */
  START2, START2, START2, START2, START2, START2, START2, START2,
  /* 217 - 224 (d8-df) */
  START2, START2, START2, START2, START2, START2, START2, START2,
  /* 225 - 232 (e0-e7) */
  START3, START3, START3, START3, START3, START3, START3, START3,
  /* 233 - 240 (e8-ef) */
  START3, START3, START3, START3, START3, START3, START3, START3,
  /* 241 - 248 (f0-f7) */
  START4, START4, START4, START4, START4, RESTRBYTE, RESTRBYTE, RESTRBYTE,
  /* 249 - 256 (f8-ff) */
  RESTRBYTE, RESTRBYTE, RESTRBYTE, RESTRBYTE, RESTRBYTE, RESTRBYTE,
  INVLBYTE, INVLBYTE
};

inline int utf8t(const char c) { unsigned char u = c; return utf8type[u]; }

inline int utf8len(const char c) {
  switch (utf8t(c)) {

  case NULLCHAR:
  case ONEBYTE:
    return 1;

  case START2:
    return 2;

  case START3:
    return 3;

  case START4:
    return 4;
    
  case CONTBYTE:
  case INVLBYTE:
  case RESTRBYTE:
    return -1;
  }
  return -1;
}

/* Convert a wide UTF8 character starting at position i in s to its code */
inline long int str2utf(const char *s, const int i) {
  long l = 0;
  int t = utf8t(s[i]);
  if (t == ONEBYTE) {
    l = s[i];
    return l;
  }
  else if (t == START2) {
    l = (((s[i] & 0x1f) << 6) + (s[i + 1] & 0x3f));
    return l;
  }
  else if (t == START3) {
    l = ((s[i] & 0xf) << 12) + ((s[i + 1] & 0x3f) << 6) + (s[i + 2] & 0x3f);
    return l;
  }
  else if (t == START4) {
    l = ((s[i] & 7) << 18) + ((s[i + 1] & 0x3f) << 12)
      + ((s[i + 2] & 0x3f) << 6) + (s[i + 3] & 0x3f);
    return l;
  }
  else if (t == NULLCHAR) {
    return 0L;
  }
  else {
    return -1L;
  }
}

/* Convert a UTF8 code to a wide character string. The string is local. */
/* Take care to copy the result!!! */
inline char *utf2str(const long int s) {
  static char buffer[5];  /* In the current standard, UTF8 has up to 4 chars */
  long int l = s;
  int k = (l < 0x800L ? (l < 0x80L ? 1 : 2) : (l < 0x10000L ? 3 : 4));
  for (int i = 0; i < k - 1; i++) {
    buffer[k - 1 - i] = ((l & 0x3fL) | 0x80) & 0xff;
    l = (l >> 6);
  }//for k
  buffer[0] = (0xf0 << (4 - k)) | l;
  buffer[k] = 0;
  return buffer;
}

/* Name:	my_cmp_str_ndx
 * Class:	None.
 * Purpose:	Compare two strings.
 * Parameters:	a1	- (i) first string to compare;
 *		a2	- (i) second string to compare.
 * Returns:	0 if a1 = a2,
 *		-1 if a1 < a2,
 *		1 if a1 > a2.
 * Remarks:	To be used in qsort.
 */
int my_cmp_str_ndx(const void *a1, const void *a2);
#endif //UTF8

const int	Max_word_len = 120;     /* max word length in input file */
const int	LIST_INIT_SIZE = 16;	/* initial list capacity */
const int	LIST_STEP_SIZE = 8;	/* list size increment */
const int	MAX_NOT_CYCLE = 1024;   /* max length of candidate (if
					 greater, treated as cycle) */
using namespace std;

/* Defines a dictionary with its inherent properties */
struct dict_desc {
  fsa_arc_ptr	dict;		/* dictionary itself (fsa) */
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  SparseVector	*sparse_vect;	/* sparse vector part of the dictionary */
#endif
  int		no_of_arcs;	/* number of arcs */
  char          filler;		/* filler ("empty") character */
  char		annot_sep;	/* separates words from annotations */
  char		gtl;		/* size of goto field */
  char		entryl;		/* size of entries field */
#ifdef WEIGHTED
  int		goto_offset;	/* offset of the goto field in arcs */
  int		weighted;	/* TRUE if arcs weighted */
#endif
};/*dict_desc*/

/* Name:	comp
 * Class:	None.
 * Purpose:	Compare two list items.
 * Parameters:	it1		- (i) first item;
 *		it2		- (i) second item.
 * Returns:	< 0 if it1 < it2;
 *		= 0 if it1 = it2;
 *		> 0 if it1 > it2.
 * Remarks:	None.
 */
inline int
comp(const char *it1, const char *it2)
{
  return strcmp(it1, it2);
}/*comp*/

/* Name:	comp
 * Class:	None.
 * Purpose:	Compare two list items.
 * Parameters:	it1		- (i) first item;
 *		it2		- (i) second item.
 * Returns:	< 0 if it1 < it2;
 *		= 0 if it1 = it2;
 *		> 0 if it1 > it2.
 * Remarks:	Provided because needed by template.
 */
inline int
comp(const dict_desc *it1, const dict_desc *it2)
{
  return (it1->dict.arc - it2->dict.arc);
}/*comp*/


/* Name:	new_copy
 * Class:	None.
 * Purpose:	makes a copy of the item in dynamic memory.
 * Parameters:	it		- (i) item to be copied.
 * Returns:	The copy in dynamic memory.
 * Remarks:	None.
 */
inline char *
new_copy(const char *it)
{
  return nstrdup(it);
}/*new_copy*/

/* Name:	new_copy
 * Class:	None.
 * Purpose:	Makes a copy of the item in dynamic memory.
 * Parameters:	it		- (i) item to be copied.
 * Returns:	The copy in dynamic memory.
 * Remarks:	Actually, no copy is made here. It is not needed, as
 *		the copy is prepared before putting the item on the list.
 */
inline dict_desc *
new_copy(const dict_desc *it)
{
  dict_desc *dd = new dict_desc;
  memcpy(dd, it, sizeof(dict_desc));
  return dd;
}/*new_copy*/

/* Class name:	list
 * Purpose:	Implements basic list facility template.
 * Remarks:	Copies of items are put onto the list.
 */
template <class T>
class list {
protected:
  T	**items;	/* list items */
  T	**next_item;	/* current item */
  int	no_of_items;	/* number of items on the list */
  int	allocated;	/* max number of items allowed without reallocation*/
  int	c;		/* this must be declared here - bugs in compilers */

  void add_item(const T *new_item, T **where_to_add) {
    if (no_of_items >= allocated) {
      allocated += LIST_STEP_SIZE;
      next_item = new T *[allocated];
      memcpy(next_item, items, no_of_items * sizeof(T *));
      where_to_add = (where_to_add - items) + next_item;
      delete [] items;
      items = next_item;
    }
    for (next_item = items + no_of_items - 1; next_item >= where_to_add;
	 --next_item)
      next_item[1] = *next_item;
    next_item = where_to_add;
    no_of_items++;
    *next_item = new_copy(new_item);
  }
public:
  list(void) { next_item = items = new T *[allocated = LIST_INIT_SIZE];
	       no_of_items = 0; }
  ~list(void) { delete [] items; }
  operator int(void) const { return (no_of_items != 0); }
  void insert(const T *new_item) { add_item(new_item, items + no_of_items); }
  int insert_sorted(const T *new_item) {
    for (next_item = items; next_item < items + no_of_items; next_item++) {
      if ((c = comp(new_item, *next_item)) > 0) {
	/* item not on the list - add a new one */
	add_item(new_item, next_item);
	return TRUE;
      }
      else if (c == 0) {
	/* item found, no need to insert it */
	return FALSE;
      }
    }
    /* item not found, insert it */
    add_item(new_item, next_item);
    return TRUE;
  }
  T **start(void) const { return items; }
  void reset(void) { next_item = items; }
  void empty_list(void) {
    for (c = 0; c < no_of_items; c++) {
      delete [] items[c];
    }
    // delete [] items;
//    next_item = items = new T *[allocated = LIST_INIT_SIZE];
    next_item = NULL;
    no_of_items = 0;
  }
  T *item(void) const { return ((next_item < items + no_of_items) ?
				*next_item : ((T *)NULL)); }
  T *next(void) {
    if (next_item++ < items + no_of_items-1) {
      return *next_item;
    }
    else
      return (T *)NULL;
  }
  int how_many(void) const { return no_of_items; }
};/*class list*/



typedef		list<char>		word_list;
typedef         list<dict_desc>         dict_list;

#ifdef UTF8
struct word_syntax_type {
  unsigned char		chr;	/* subsequent byte of a UTF8 character */
  int			follow;	/* next node in a UTF8 character tree */
  int			c_case; /* case of the character */
  unsigned char		*other_c; /* the same character in the other case */
};
#endif

/* Class name:	tr_io
 * Purpose:	Provide a class for input and output handling.
 * Methods:	tr_io		- initialize;
 *		~tr_io		- deallocate memory;
 *		>>		- read word from input;
 *		print_OK	- do something when word found (in spelling);
 *		print_not_found	- do something when word (& replacements)
 *					not found;
 *		print_repls	- print replacements for the word;
 *		print_morph	- print word morphology;
 *		int		- return state of the last stream used;
 * Fields:	input		- input stream;
 *		output		- output stream;
 *		proc_state	- state of input processing;
 *		stream_state	- state of i/o;
 *		buffer		- input buffer;
 *		word_syntax	- which characters constitute a part of word;
 *		Max_line_len	- max length of input lines.
 * Remarks:	Operator>> must make sure that the returned string does not
 *		exceed Max_word_len.
 */
class tr_io {
protected:
  istream	*input;			/* input stream */
  ostream	&output;		/* output stream */
  int		proc_state;		
  int		stream_state;
  char		*buffer;		/* word buffer */
#ifdef UTF8
  word_syntax_type *word_syntax; 	/* which characters form words */
#else
  char		*word_syntax;		/* which characters form words */
#endif
  int		Max_line_len;		/* initial line length */
  int		inp_buf_len;		/* current input line length */
  const char	*input_file_name;
#ifndef UTF8
  char		case_tab;		/* for case conversion */
#endif
  int		inp_line_no;		/* input line number */
  int		inp_line_char_no;	/* character number in input line */
  int		inp_word_len;		/* input word length */
  char		junk;			/* last character read (normally
					   '\n') */

#ifdef UTF8
  int is_word_char(unsigned char *s) {
    unsigned char *p = s;
    int base = 0;
    while (*p && base != -1 && word_syntax[*p + base].chr == *p) {
      base = word_syntax[*p++ + base].follow;
    }
    if (*p == '\0') {
      return 1;
    }
    return 0;
  }
#else
  int is_word_char(unsigned c) { return word_syntax[(unsigned char)c]; }
#endif
public:
  tr_io(istream *in_file, ostream &out_file, const int max_line_length,
	const char *file_name,
#ifdef UTF8
	word_syntax_type
#else
	const char
#endif
	*word_chars = NULL);
  ~tr_io(void);
  tr_io &operator>>(char *s);
  char get_junk(void) { return junk; }
  void set_buf_len(const int l) { inp_buf_len = l; }
  tr_io &print_OK(void);
  tr_io &print_not_found(void);
  tr_io &print_repls(word_list *r);
  tr_io &print_morph(word_list *s);
  tr_io &print_line(const char *s);
  operator int(void) const { return stream_state; }
};/*tr_io*/


/* Class name:	fsa
 * Purpose:	Provide an environment for a spelling process.
 * Methods:	fsa	- read automatons from given files,
 *				and put them on a list;
 *		~fsa	- deallocate memory;
 *		int	- get spelling process status;
 *		spell_file	- check or correct words from file;
 *		read_fsa	- reads dictionaries from files of names
 *				  from a list;
 *		spell_word	- checks spelling of a word (and finds
 *				  replacements if needed);
 *		word_in_dictionary
 *				- finds if a word is in dictionary;
 *		find_replacements
 *				- find correct words similar to the misspelled;
 *		rank_repl	- sort the list of replacements.
 * Variables:	dictionary	- list of dictionaries (FSAs);
 *		current_dict	- current dictionary (from the list);
 *		results		- unsorted list of replacements for a
 *				  misspelled word with additional information;
 *		replacements	- sorted list of replacements for a
 *				  (misspelled) word;
 *		edit_dist	- max edit distance(misspelled word, repl.);
 *		e_d		- maxedit distance for the current word
 *				  e_d <= edit_dist;
 *		state		- state of the spelling process;
 *		word_ff		- word read from file (current word);
 *		word_length	- length of current word;
 *		char_eq		- table of character equivalences.
 */
class fsa {
protected:
  dict_list		dictionary;	/* list of dictionaries */
  arc_pointer	 	current_dict;	/* current dictionary */
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  SparseVector		*sparse_vect; 	/* sparse vector part */
#endif
  word_list		replacements;	/* list of words */
  int			state;
  char			*candidate;	/* current replacement */
  int			cand_alloc;	/* size of candidate string */
  int			word_length;	/* length of word being processed */
  char			*word_ff;	/* word being processed */
  char			casetab[256];	/* case conversion table */
#ifdef UTF8
  word_syntax_type	*word_syntax;
  unsigned int		word_syntax_size;	/* length of word_syntax */
#else
  char			*word_syntax;	/* characters that form words */
#endif
  char			FILLER;		/* character to be ignored */
  char			ANNOT_SEPARATOR;/* separates parts of entries */
#ifdef WEIGHTED
  int			weighted; 	/* whether the automaton is weighted */
#endif //WEIGHTED

  int read_fsa(const char *dict_file_name);
  int word_in_dictionary(const char *word, fsa_arc_ptr start);
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  int sparse_word_in_dictionary(const char *word, long int start);
#endif
  void set_dictionary(dict_desc *dict);
  int word_in_dictionaries(const char *word);
  int read_language_file(const char *file_name);
  void invent_language(void);
  bool is_downcaseable(const char *s);
#ifdef UTF8
  int fit_syntax_tab(unsigned char **strings_to_fit,
		     unsigned int first_char, unsigned int last_char,
		     unsigned int char_index, unsigned int &origin);
#endif
  bool myflipcase(char *s, const int direction);
public:
  fsa(word_list *dict_names, const char *language_file = NULL);
#ifdef UTF8
  const word_syntax_type *get_syntax(void) const { return word_syntax; }
#else
  const char *get_syntax(void) const { return word_syntax; }
#endif
  virtual ~fsa(void) { delete [] word_syntax; delete [] candidate; }
  operator int(void) const {return state; }
};/*fsa*/


/* Name:	get_word
 * Class:	None.
 * Purpose:	Read a line from input, allocating more memory if necessary.
 * Parameters:	io_obj		- (i/o) where to read from;
 *		word		- (o) line to be read;
 *		allocated	- (i/o) size of buffer before/after read;
 *		alloc_step	- how much allocated may differ between calls.
 * Returns:	io_obj.
 * Remarks:	This is necessary to prevent buffer overflows.
 *		It is assumed that a word is one line.
 *		An ifdef is needed for text_io input.
 */
tr_io &
get_word(tr_io &io_obj, char *&word, int &allocated,
	 const int alloc_step);

/***	EOF common.h	***/
