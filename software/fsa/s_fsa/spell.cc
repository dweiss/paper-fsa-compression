/***	spell.cc	***/

/*	Copyright (C) Jan Daciuk, 1996-2004	*/

#include        <iostream>
#include	<fstream>
#include	<stdlib.h>
#include	<string.h>
#include        "nstr.h"
#include        "fsa.h"
#include	"common.h"
#include        "spell.h"


static const int	Buf_len = 256;	// Buffer length for chclass file


/*	simple inline utilities	*/

inline int
min(const int a, const int b);
inline int
min(const int a, const int b)
{
  return ((a < b) ? a : b);
}//min
inline int
min(const int a, const int b, const int c);
inline int
min(const int a, const int b, const int c)
{
  return min(a, min(b, c));
}//min
inline int
max(const int a, const int b);
inline int
max(const int a, const int b)
{
  return (a > b ? a : b);
}//max
int
m_abs(const int x);
int
m_abs(const int x)
{
  return (x < 0 ? -x : x);
}//m_abs


/* Name:	comp_cost
 * Class:	None.
 * Purpose:	Compares two ranked_hits structures on cost.
 * Parameters:	rh1		- (i) first structure;
 *		rh2		- (i) second structure.
 * Returns:	< 0 if rh1 < rh2
 *		= 0 if rh1 = rh2
 *		> 0 if rh1 > rh2.
 * Remarks:	Parameters must have the void pointer type for compatibility.
 */
int
comp_cost(const void *rh1, const void *rh2)
{
  return (((ranked_hits **)(rh1))[0]->cost - ((ranked_hits **)(rh2))[0]->cost);
}/*comp_cost*/


/* Name:	H_matrix
 * Class:	H_matrix
 * Purpose:	Allocates memory and initializes matrix (constructor).
 * Parameters:	distance	- (i) max edit distance allowed for candidates;
 *		max_length	- (i) max length of words.
 * Returns:	Nothing.
 * Remarks:	See Oflazer. To save space, the matrix is stored as a vector.
 *		To save time, additional raws and columns are added.
 *		They are initialized to their distance in the matrix, so
 *		that no bound checking is necessary during access.
 */
H_matrix::H_matrix(const int distance, const int max_length)
{
  row_length = max_length + 2;
  column_height = 2 * distance + 3;
  edit_distance = distance;
  int size = row_length * column_height;
  p = new int[size];
  // Initialize edges of the diagonal band to distance + 1 (i.e. distance
  // too big)
  for (int i = 0; i < row_length - distance - 1; i++) {
    p[i] = distance + 1;		// H(distance + j, j) = distance + 1
    p[size - i - 1] = distance + 1;	// H(i, distance + i) = distance + 1
  }
  // Initialize items H(i,j) with at least one index equal to zero to |i - j|
  for (int j = 0; j < 2 * distance + 1; j++) {
    p[j * row_length] = distance + 1 - j;	// H(i=0..distance+1,0)=i
    p[(j + distance + 1) * row_length + j] = j;	// H(0,j=0..distance+1)=j
  }
#ifdef DEBUG
  cerr << "Edit distance is " << edit_distance << "\n";
#endif
}//H_matrix::H_matrix

/* Name:	operator()
 * Class:	H_matrix
 * Purpose:	Provide an item of H_matrix indexed by indices.
 * Parameters:	i		- (i) row number;
 *		j		- (i) column number.
 * Returns:	Item H[i][j].
 * Remarks:	H matrix is really simulated. What is needed is only
 *		2 * edit_distance + 1 wide band around the diagonal.
 *		In fact this diagonal has been pushed up to the upper border
 *		of the matrix.
 *
 *		The matrix in the vector looks likes this:
 *		    +---------------------+
 *		0   |#####################| j=i-e-1
 *		1   |                     | j=i-e
 *		    :                     :
 *		e+1 |                     | j=i-1
 *		    +---------------------+
 *		e+2 |                     | j=i
 *		    +---------------------+
 *              e+3 |                     | j=i+1
 *		    :                     :
 *              2e+2|                     | j=i+e
 *		2e+3|#####################| j=i+e+1
 *		    +---------------------+
 */
int
H_matrix::operator()(const int i, const int j)
{
  return p[(j - i + edit_distance + 1) * row_length + j];
}//H_matrix::operator()

/* Name:	set
 * Class:	H_matrix
 * Purpose:	Set an item in H_matrix.
 * Parameters:	i		- (i) row number;
 *		j		- (i) column number;
 *		val		- (i) value to put there.
 * Returns:	Nothing.
 * Remarks:	Previously operator() returned int &, but the problem was
 *		that sometimes the values returned were computed,
 *		so one item in the matrix was chosen to store them.
 *		However, if they appeared in one expression, they were
 *		all the same variable. So now we have get & set.
 *
 *		No checking for i & j is done. They must be correct.
 */
void
H_matrix::set(const int i, const int j, const int val)
{
  p[(j - i + edit_distance + 1) * row_length + j] = val;
}//H_matrix::set


/* Name:	spell_fsa
 * Class:	spell_fsa
 * Purpose:	Initialization (constructor).
 * Parameters:	dict_names      - (i) dictionary file names;
 *              distance        - (i) max edit distance of replacements;
 *		chclass_file	- (i) name of a file that specifies which
 *					characters may be replaced with
 *					which two-letter sequences and
 *					vice versa;
 *		language_file	- (i) name of a file that specifies which
 *					characters may form words.
 * Returns:     Nothing.
 * Remarks:     At least one dictionary file must be read.
 *		The language file may be used with of versions of i/o.
 *		Normally it is not needed.
 */
spell_fsa::spell_fsa(word_list *dict_names, const int distance,
		     const char *chclass_file,
		     const char *language_file)
: fsa(dict_names, language_file), H(distance, Max_word_len)
{
  edit_dist = distance;
#ifdef CHCLASS
  read_character_class_tables(chclass_file);
#endif
}//spell_fsa::spell_fsa


/* Name:	spell_file
 * Class:	spell_fsa
 * Purpose:	launch a spelling process for a given file.
 * Parameters:	distance	- (i) edit distance;
 *		force		- (i) force generation of candidates;
 *		io_obj		- (i/o) where to read words,
 *					and where to print them.
 * Returns:	Exit code.
 * Remarks:	Edit distance is reduced for shorter words.
 *		No attempt is being made to correct one-letter words.
 */
int
spell_fsa::spell_file(const int distance, const bool force, tr_io &io_obj)
{
  char		word_buffer[Max_word_len];
  char		*word = &word_buffer[0];

  edit_dist = distance;
  while (io_obj >> word) {
    if (io_obj.get_junk() != '\n') {
      cerr << "Word too long" << endl;
      continue;
    }
    word_length = strlen(word); word_ff = word;
    if (word_length < 1) {
      cerr << " not checked (too short)\n";
      continue;
    }
    e_d = (word_length <= distance ? (word_length - 1) : distance);
    if (spell_word(word, force))
      io_obj.print_OK();
    else if (replacements) {
      io_obj.print_repls(&replacements);
      replacements.empty_list();
    }
    else
      io_obj.print_not_found();
  }
  return state;
}//spell_fsa::spell_file


/* Name:	spell_word
 * Class:	spell_fsa
 * Purpose:	Finds if a word is in the dictionary, and if not, provides
 *		replacements.
 * Parameters:	word	- (i) word to be checked;
 *		force	- (i) force generation of candidates.
 * Returns:	TRUE	- if word found, FALSE otherwise.
 * Remarks:	Class variable `replacements' is set to list of replacements.
 *		current_dict + 1 means  we start from arc #1, which is
 *		meta root, i.e. it contains the address and the number of
 *		arcs of the fsa's root.
 *		In the word_syntax table, 3 means lowercase, 2 - uppercase.
 */
int
spell_fsa::spell_word(const char * word, const bool force)
{
#ifdef CASECONV
  int			converted;
#endif

  if (!force && word_in_dictionaries(word))
    return TRUE;
  else if (e_d < 1)
    return FALSE;

#ifdef CASECONV
  converted = FALSE;
  if (is_downcaseable(word)) {
    // word is uppercase - convert to lowercase
    myflipcase((char *)word, -1);
    //*((char *)word) = casetab[(unsigned char)*word];
    converted = TRUE;
  }
#endif
  find_repl_all_dicts();

#ifdef CASECONV
  if (converted) {
    // convert back to uppercase
    *(char *)word = casetab[(unsigned char)*word];
    find_repl_all_dicts();
  }
#endif
#ifdef RUNON_WORDS
  find_runon(word);
#endif
  if (results) {
    rank_replacements();
  }

#ifdef CASECONV
  if (converted) {
    // word was uppercase
    replacements.reset();
    for (;replacements.item();replacements.next()) {
      myflipcase(replacements.item(), 1);
      /*
      if (word_syntax[(unsigned char)(replacements.item()[0])] == 3)
	// replacement is lowercase - convert to uppercase
	replacements.item()[0] =
	  casetab[(unsigned char)(replacements.item()[0])];
      */
    }
  }
#endif
  return FALSE;
}//spell_fsa::spell_word

/* Name:	find_repl_all_dicts
 * Class:	spell_fsa
 * Purpose:	Finds replacements for a word in all dictionaries.
 * Parameters:	word		- (i) the word for which replacements
 *					are to be found.
 * Returns:	Nothing.
 * Remarks:	A list of replacements is returned in var replacements.
 *		The word is in word_ff class variable.
 */
void
spell_fsa::find_repl_all_dicts(void)
{
  dict_list		*dict;
#if !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  fsa_arc_ptr		*dummy = NULL;		/* to get to static fields */
#endif

  dictionary.reset();
  for (dict = &dictionary; dict->item(); dict->next()) {
    set_dictionary(dict->item());
#ifdef CHCLASS
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
    sparse_find_repl(0, sparse_vect->get_first(), 0, 0);
#else //!(FLEXIBLE&STOPBIT&SPARSE)
    find_repl(0, dummy->first_node(current_dict), 0, 0);
#endif //!(FLEXIBLE&STOPBIT&SPARSE)
#else //!CHCLASS
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
    find_repl(0, sparse_vect->get_first());
#else
    find_repl(0, dummy->first_node(current_dict));
#endif //!(FLEXIBLE&STOPBIT&SPARSE)
#endif //!CHCLASS
  }
}//spell_fsa::find_repl_all_dicts


#ifdef RUNON_WORDS
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	find_runon
 * Class:	spell_fsa
 * Purpose:	Split the word in two, and check if the resulting words
 *		are in the dictionary.
 * Parameters:	word		- (i) the word to be checked.
 * Returns:	Results.
 * Remarks:	This is a more generic version, so that we can rely
 *		on other functions for using the sprase matrix representation.
 */
hit_list *
spell_fsa::find_runon(const char *word)
{
  ranked_hits	word_found;

  if (word_length > 1 && e_d > 0) {

    if (word_length + 1 >= cand_alloc)
      grow_string(candidate, cand_alloc, Max_word_len);

    // Put the break at the beginning
    strcpy(candidate + 1, word);
    for (int i = 1; i < word_length - 1; i++) {
      // Move the word break by one character
      candidate[i - 1] = candidate[i];
      candidate[i] = '\0';	// terminate first word
      if (word_in_dictionaries(candidate) &&
	  word_in_dictionaries(candidate + 2)) {
	// Both part of the word divided at two at i found in dictionaries
	candidate[i] = ' ';
	word_found.list_item = nstrdup(candidate);
	word_found.dist = 1;
	word_found.cost = 1;		// for the moment
	results.insert_sorted(&word_found);
      }//if
    }//for
  }//if
  return &results;
}//spell_fsa::find_runon

#else //!(FLEXIBLE&STOPBIT&SPARSE)

/* Name:	find_runon
 * Class:	spell_fsa
 * Purpose:	Split the word in two, and check if the resulting words
 *		are in the dictionary.
 * Parameters:	word		- (i) the word to be checked.
 * Returns:	Results.
 * Remarks:	This could be simpler, but speed was at stake.
 *		The present version does not check the case.
 *		It is assumed that no fillers are used.
 */
hit_list *
spell_fsa::find_runon(const char *word)
{
  ranked_hits	word_found;
  fsa_arc_ptr	start;
  fsa_arc_ptr	next_node;
  dict_desc	*d;
  fsa_arc_ptr	*dummy = NULL;		// to get to static fields
  arc_pointer	local_dict;	// local to this function,
  				// as word_in_dictionary could change it
  int		last_letter_found;


  if (word_length > 1 && e_d > 0) {

    if (word_length + 1 >= cand_alloc)
      grow_string(candidate, cand_alloc, Max_word_len);

    for (int i = 0; i < dictionary.how_many(); i++) {
      // Prepare dictionary
      dictionary.reset();
      for (int ii = 0; ii < i; ii++)
	dictionary.next();
      d = dictionary.item();
      set_dictionary(d);
      local_dict = current_dict;
      // Prepare words
      strcpy(candidate + 1, word);
      *candidate = *word;
      candidate[1] = ' ';
      start = dummy->first_node(local_dict);
      for (int j = 2; j < word_length; j++) {
	set_dictionary(d);
	next_node = start.set_next_node(local_dict);
	last_letter_found = FALSE;
	// Find last letter of the first word
	forallnodes(k) {
	  if (next_node.get_letter() == candidate[j - 2]) {
	    last_letter_found = TRUE;
	    start = next_node;
	    break;
	  }
	}
	// Find if the second word is in the dictionaries
	if (last_letter_found && next_node.is_final() &&
	    word_in_dictionaries(candidate + j)) {
	  word_found.list_item = nstrdup(candidate);
	  word_found.dist = 1;
	  word_found.cost = 1;		// for the moment
	  results.insert_sorted(&word_found);
	}
	if (last_letter_found)
	  break;
	// Prepare new pair
	candidate[j - 1] = candidate[j];
	candidate[j] = ' ';
      }
    }
  }
  return &results;
}//spell_fsa::find_runon
#endif
#endif

#ifdef CHCLASS
/* Name:	read_character_class_tables
 * Class:	spell_fsa
 * Purpose:	Read file with character classes and prepare tables.
 * Parameters:	a_file		- (i) name of file with class descriptions.
 * Returns:	TRUE if file read successfully, FALSE otherwise.
 * Remarks:	The format of the character class file is as follows:
 *			The first character in the first line is a comment
 *			character. Each line that begins with that character
 *			is a comment.
 *			Note: it is usually `#' or ';'.
 *
 *			Data lines contain two columns. The columns contain
 *			one or two characters. The sum of the characters
 *			from both columns must be 3 (1+2 or 2+1). The columns
 *			must be separated by spaces or tabs.
 *			The first column represents what may occur in text.
 *			The second column represents what should be there.
 *			Those definitions do not affect recognition of errors.
 *			They make the list of possible replacements longer.
 *			Characters are represented by themselves, the file
 *			is binary.
 *		The format of the character class tables:
 *			A table contains 256 pointers to strings. One letter
 *			column character is the index to a string of pairs
 *			of characters taken from the other column.
 *
 *			If the length of the first column in the file is 1,
 *			the data for that line is written into first_column
 *			table, otherwise it is written into second_column
 *			table.
 *			All other characters are represented by themselves.
 *
 *		It is possible to invoke the function with empty (NULL)
 *		file name. This creates empty, but valid first_column and
 *		second_column.
 *
 *		Note: Line length is not checked, though buffer overflow
 *		is not a danger here.
 */
int
spell_fsa::read_character_class_tables(const char *file_name)
{
  int		i, j, k;
  char		comment_char;
  char		junk;
  unsigned char	buffer[Buf_len];
  char		seq_buf[Buf_len];

  // allocate memory for character class tables and initialize them
  first_column = new char *[256];
  second_column = new char *[256];
  for (i = 0; i < 256; i++) {
    first_column[i] = NULL; second_column[i] = NULL;
  }

  // See if a file is specified
  if (file_name == NULL)
    return FALSE;

  // open character class file
  ifstream chcl_f(file_name, ios::in /* | ios::nocreate */);
  if (chcl_f.bad()) {
    cerr << "Cannot open character class file `" << file_name << "'\n";
    return FALSE;
  }

  // Recognize comment character
  if (chcl_f.get((char *)buffer, Buf_len, '\n'))
    comment_char = buffer[0];
  else
    return FALSE;
  chcl_f.get(junk);

  // Process lines
  while (chcl_f.get((char *)buffer, Buf_len, '\n')) {
    chcl_f.get(junk);
    if (buffer[0] != comment_char) {	// Data line
      // first column
      for (i = 0; buffer[i] != ' ' && buffer[i] != '\t' && buffer[i]; i++)
	;
      // now i points to the character imediately behind the first column
      for (j = i; buffer[j] == ' ' || buffer[j] == '\t'; j++)
	;
      // now j points to the first character of the second column
      for (k = j; buffer[k] && buffer[k] != ' ' && buffer[k] != '\t'; k++)
	;
      // now k points to the first character behind the second column
      if (i == 1 && k - j == 2) {
	seq_buf[0] = buffer[j];
	seq_buf[1] = buffer[j+1];
	if (first_column[buffer[0]]) {
	  if (strlen(first_column[buffer[0]]) < Buf_len - 3) {
	    strcpy(seq_buf + 2, first_column[buffer[0]]);
	    delete [] first_column[buffer[0]];
	  }
	  else {
	    cerr << "Too many equivalent sequences. Ignored" << endl;
	  }
	}
	else
	  seq_buf[2] = '\0';
	first_column[buffer[0]] = nstrdup(seq_buf);
      }
      else if (i == 2 && k - j == 1) {
	seq_buf[0] = buffer[0];
	seq_buf[1] = buffer[1];
	if (second_column[buffer[j]]) {
	  if (strlen(second_column[buffer[j]]) < Buf_len - 3) {
	    strcpy(seq_buf + 2, second_column[buffer[j]]);
	    delete [] first_column[buffer[j]];
	  }
	  else {
	    cerr << "Too many equivalent sequences. Ignored" << endl;
	  }
	}
	else
	  seq_buf[2] = '\0';
	second_column[buffer[j]] = nstrdup(seq_buf);
      }
      else {
	cerr << "Illegal format in the following line from " << file_name
	  << endl << buffer << endl;
      }
    }
  }
  return TRUE;
}//read_character_class_tables



/* Name:	match_candidate
 * Class:	spell_fsa
 * Purpose:	Match two last letters of the candidate against the penultimate
 *		letter of the word.
 * Parameters:	i		- (i) current index of the word;
 *		j		- (j) current index of the candidate.
 * Result:	TRUE if letters match, FALSE otherwise.
 * Remarks:	first_column is a vector of strings with indices being
 *		characters. The strings are normally empty, but for single
 *		letters specified in the first column in the character class
 *		files, they contain one or more equivalent pairs of characters.
 */
int
spell_fsa::match_candidate(const int i, const int j)
{
  char	*c;
  char	c0, c1;

  if (i > 0 && (c = first_column[(unsigned char)(word_ff[i-1])]) && j > 0)
    for (c0 = candidate[j-1], c1 = candidate[j]; *c; c+= 2)
      if (c[0] == c0 && c[1] == c1)
	return TRUE;
  return FALSE;
}//spell_fsa::match_candidate

/* Name:	match_word
 * Class:	spell_fsa
 * Purpose:	Match this and the next letter of the word against the last
 *		letter of the candidate.
 * Parameters:	i		- (i) current index of the word;
 *		j		- (i) current index of the candidate.
 * Result:	TRUE if letters match, FALSE otherwise.
 * Remarks:	second_column is a vector of strings with indices being
 *		characters. The strings are normally empty, but for single
 *		letters specified in the second column in the character class
 *		files, they contain one or more equivalent pairs of characters.
 */
int
spell_fsa::match_word(const int i, const int j)
{
  char	*c;
  char	c0, c1;

  if ((c = second_column[(unsigned char)(candidate[j])]))
    for (c0 = word_ff[i], c1 = word_ff[i+1]; *c; c += 2)
      if (c[0] == c0 && c[1] == c1)
	return TRUE;
  return FALSE;
}//spell_fsa::match_word

#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_find_repl
 * Class:	spell_fsa
 * Purpose:	Create a list of candidates for a misspelled word.
 * Parameters:	depth		- (i) current length of replacements;
 *		start		- (i) start with that node;
 *		word_index	- (i) index of the next character to be
 *					considered in word_ff;
 *		cand_index	- (i) index of the next character in candidate
 *					to be considered.
 * Returns:	TRUE if candidates found, FALSE otherwise.
 * Remarks:	A (partial) list of candidates is stored in results.
 *		See Kemal Oflazer - "Error-tolerant Finite State Recognition
 *		with Applications to Morphological Analysis and Spelling
 *		Correction", cmp-lg/9504031. Modified.
 *		This contains modifications for classes of strings.
 *		The idea is to treat replacing one string of characters for
 *		another string representing the same (or very similar) sound
 *		as a simple (one-character) replacement. This constitutes an
 *		orthographic (not typographic) error.
 *		Typos happen when one knows how to spell the word,
 *		but hits the wrong key. Orthographic errors happen when
 *		one does not know how to spell the word. E.g. in Polish,
 *		`rz' and `\.z' (z with a dot above - one letter) sound exactly
 *		the same, and one must remember the correct spelling.
 *		As the number of people with their brains damaged by the TV
 *		increases, this type of errors must be taken into
 *		consideration.
 */
hit_list *
spell_fsa::sparse_find_repl(const int depth, const long int start,
			    const int word_index, const int cand_index)
{
  int		dist = 0;
  ranked_hits	word_found;
  long int 	next;
  fsa_arc_ptr 	*dummy;

  if (depth + 1 >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

//   for (unsigned char i = sparse_vect->get_minchar();
//        i <= sparse_vect->get_maxchar(); i++) {
//     char cc = (char)i;
  const char *a6t = sparse_vect->get_alphabet() + 1;
  for (int i = 1; i < sparse_vect->get_alphabet_size(); i++) {
    char cc = *a6t++;
    if ((next = sparse_vect->get_target(start, cc)) != -1L) {
      candidate[cand_index] = cc;
      if (match_candidate(word_index, cand_index)) {
	// The last two letters from candidate, and the previous letter
	// from word_ff match
	if (cc == ANNOT_SEPARATOR) {
	  // Move to annotations
	  find_repl(depth, dummy->first_node(current_dict) + next,
		    word_index, cand_index + 1);
	}
	else {
	  // Continue with the sparse vector
	  sparse_find_repl(depth, next, word_index, cand_index + 1);
	}
	if (m_abs(word_length - 1 - depth) <= e_d &&
	    sparse_vect->is_final(start, cc) &&
	    (dist = ed(word_length - 2 - (word_index - depth), depth - 2,
		       word_length - 2, cand_index - 2)) + 1 <= e_d) {
	  candidate[cand_index + 1] = '\0';	// restore candidate's length
	  word_found.list_item = nstrdup(candidate);
	  word_found.dist = dist;
	  word_found.cost = dist;		// for the moment
	  results.insert_sorted(&word_found);
	}
      }
      if (cuted(depth, word_index, cand_index) <= e_d) {
	if (cc == ANNOT_SEPARATOR) {
	  // Move to annotations
	  find_repl(depth + 1, dummy->first_node(current_dict) + next,
		    word_index + 1, cand_index + 1);
	}
	else {
	  // Continue with sparse vector
	  sparse_find_repl(depth + 1, next, word_index + 1, cand_index + 1);
	}
	if (match_word(word_index, cand_index)) {
	  if (cc == ANNOT_SEPARATOR) {
	    // Move to annotations
	    find_repl(depth + 1, dummy->first_node(current_dict) + next,
		      word_index + 2, cand_index + 1);
	  }
	  else {
	    // Continue with the sparse vector
	    sparse_find_repl(depth + 1, next, word_index + 2, cand_index + 1);
	  }
	  if (m_abs(word_length - 1 - depth) <= e_d &&
	      sparse_vect->is_final(start, cc) &&
	      (word_length > 2 && match_word(word_length - 2, cand_index) &&
	       (dist = ed(word_length - 3 - (word_index - depth), depth - 1,
			  word_length - 3, cand_index -1) + 1) <= e_d)) {
	    word_found.list_item = nstrdup(candidate);
	    word_found.dist = dist;
	    word_found.cost = dist;		// for the moment
	    results.insert_sorted(&word_found);
	  }
	}
	candidate[cand_index + 1] = '\0';	// restore candidate's length
	if (m_abs(word_length - 1 - depth) <= e_d &&
	    sparse_vect->is_final(start, cc) &&
	    (dist = ed(word_length - 1 - (word_index - depth), depth,
		       word_length - 1, cand_index) <= e_d)) {
	  word_found.list_item = nstrdup(candidate);
	  word_found.dist = dist;
	  word_found.cost = dist;		// for the moment
	  results.insert_sorted(&word_found);
	}
      }
    }
  }
  return &results;
}//spell_fsa::sparse_find_repl
#endif

/* Name:	find_repl
 * Class:	spell_fsa
 * Purpose:	Create a list of candidates for a misspelled word.
 * Parameters:	depth		- (i) current length of replacements;
 *		start		- (i) staart with that node;
 *		word_index	- (i) index of the next character to be
 *					considered in word_ff;
 *		cand_index	- (i) index of the next character in candidate
 *					to be considered.
 * Returns:	TRUE if candidates found, FALSE otherwise.
 * Remarks:	A (partial) list of candidates is stored in results.
 *		See Kemal Oflazer - "Error-tolerant Finite State Recognition
 *		with Applications to Morphological Analysis and Spelling
 *		Correction", cmp-lg/9504031. Modified.
 *		This contains modifications for classes of strings.
 *		The idea is to treat replacing one string of characters for
 *		another string representing the same (or very similar) sound
 *		as a simple (one-character) replacement. This constitutes an
 *		orthographic (not typographic) error.
 *		Typos happen when one knows how to spell the word,
 *		but hits the wrong key. Orthographic errors happen when
 *		one does not know how to spell the word. E.g. in Polish,
 *		`rz' and `\.z' (z with a dot above - one letter) sound exactly
 *		the same, and one must remember the correct spelling.
 *		As the number of people with their brains damaged by the TV
 *		increases, this type of errors must be taken into
 *		consideration.
 */
hit_list *
spell_fsa::find_repl(const int depth, fsa_arc_ptr start,
		     const int word_index, const int cand_index)
{
  fsa_arc_ptr	next_node = start.set_next_node(current_dict);
  int		dist = 0;
  ranked_hits	word_found;

  if (depth + 1 >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

  forallnodes(i) {
    candidate[cand_index] = next_node.get_letter();
    if (match_candidate(word_index, cand_index)) {
      // The last two letters from candidate, and the previous letter
      // from word_ff match
      find_repl(depth, next_node, word_index, cand_index + 1);
      if (m_abs(word_length - 1 - depth) <= e_d &&
	  next_node.is_final() &&
	  (dist = ed(word_length - 2 - (word_index - depth), depth - 2,
		     word_length - 2, cand_index - 2)) + 1 <= e_d) {
	candidate[cand_index + 1] = '\0';	// restore candidate's length
	word_found.list_item = nstrdup(candidate);
	word_found.dist = dist;
	word_found.cost = dist;		// for the moment
	results.insert_sorted(&word_found);
      }
    }
    if (cuted(depth, word_index, cand_index) <= e_d) {
      find_repl(depth + 1, next_node, word_index + 1, cand_index + 1);
      if (match_word(word_index, cand_index)) {
	find_repl(depth + 1, next_node, word_index + 2, cand_index + 1);
	if (m_abs(word_length - 1 - depth) <= e_d &&
	    next_node.is_final() &&
	    (word_length > 2 && match_word(word_length - 2, cand_index) &&
	     (dist = ed(word_length - 3 - (word_index - depth), depth - 1,
			word_length - 3, cand_index -1) + 1) <= e_d)) {
	  word_found.list_item = nstrdup(candidate);
	  word_found.dist = dist;
	  word_found.cost = dist;		// for the moment
	  results.insert_sorted(&word_found);
	}
      }
      candidate[cand_index + 1] = '\0';		// restore candidate's length
      if (m_abs(word_length - 1 - depth) <= e_d &&
	  next_node.is_final() &&
	  (dist = ed(word_length - 1 - (word_index - depth), depth,
		     word_length - 1, cand_index) <= e_d)) {
	word_found.list_item = nstrdup(candidate);
	word_found.dist = dist;
	word_found.cost = dist;		// for the moment
	results.insert_sorted(&word_found);
      }
    }
  }
  return &results;
}//spell_fsa::find_repl


/* Name:	ed
 * Class:	spell_fsa
 * Purpose:	Calculates edit distance.
 * Parameters:	i		-(i) length of first word (here: misspelled)-1;
 *		j		-(i) length of second word (here: candidate)-1;
 *		word_index	-(i) first word character index;
 *		cand_index	-(i) second word character index
 * Returns:	Edit distance between the two words.
 * Remarks:	See Oflazer.
 */
int
spell_fsa::ed(const int i, const int j, const int word_index,
	      const int cand_index)
{
  int	result;
  int	a, b, c;	// not really necessary: tradition int &H::operator()

#ifdef DEBUG
  cerr << "Calculating ed(" << i << ", " << j << ")\n";
#endif
  if (word_ff[word_index] == candidate[cand_index]) {
    // last characters are the same
    result = H(i, j);
  }
  else if (word_index > 0 && cand_index > 0 &&
	   word_ff[word_index] == candidate[cand_index - 1] &&
	   word_ff[word_index - 1] == candidate[cand_index]) {
    // last two characters are transposed
    a = H(i - 1, j - 1);	// transposition, e.g. ababab, ababba
    b = H(i + 1, j);		// deletion,      e.g. abab,   aba
    c = H(i, j + 1);		// insertion      e.g. aba,    abab
    result = 1 + min(a, b, c);
  }
  else {
    // otherwise
    a = H(i, j);		// replacement,   e.g. ababa,  ababb
    b = H(i + 1, j);		// deletion,      e.g. ab,     a
    c = H(i, j + 1);		// insertion      e.g. a,      ab
    result = 1 + min(a, b, c);
  }

#ifdef DEBUG
  cerr << "ed(" << i << ", " << j << ") = " << result << "\n";
#endif
  H.set(i + 1, j + 1, result);
  return result;
}//spell_fsa::ed

/* Name:	cuted
 * Class:	spell_fsa
 * Purpose:	Calculates cut-off edit distance.
 * Parameters:	depth		- (i) current length of candidates;
 *		word_index	- (i) index of the next character in word;
 *		cand_index	- (i) index of the next character in candidate.
 * Returns:	Cut-off edit distance.
 * Remarks:	See Oflazer.
 */
int
spell_fsa::cuted(const int depth, const int word_index, const int cand_index)
{
  int l = max(0, depth - e_d);		// min chars from word to consider - 1
  int u = min(word_length-1 - (word_index - depth),
	      depth + e_d);		// max chars from word to consider - 1
  int min_ed = e_d + 1;			// what is to be computed
  int wi = word_index + l - depth;
  int d;

#ifdef DEBUG
  cerr << "cuted(" << depth << ")\n";
#endif
  for (int i = l; i <= u; i++, wi++) {
    if ((d = ed(i, depth, wi, cand_index)) < min_ed)
      min_ed = d;
  }

#ifdef DEBUG
  cerr << "cuted(" << depth << ") = " << min_ed << "\n";
#endif
  return min_ed;
}//spell_fsa::cuted

#else //!CHCLASS

#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_find_repl
 * Class:	spell_fsa
 * Purpose:	Create a list of candidates for a misspelled word.
 * Parameters:	depth		- (i) current length of replacements;
 *		start		- (i) start with that node.
 * Returns:	TRUE if candidates found, FALSE otherwise.
 * Remarks:	A (partial) list of candidates is stored in results.
 *		See Kemal Oflazer - "Error-tolerant Finite State Recognition
 *		with Applications to Morphological Analysis and Spelling
 *		Correction", cmp-lg/9504031. Modified.
 */
hit_list *
spell_fsa::sparse_find_repl(const int depth, const long int start)
{
  int		dist = 0;
  ranked_hits	word_found;
  fsa_arc_ptr 	*dummy;

  if (depth + 1 >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

//   for (unsigned char i = sparse_vect->get_minchar();
//        i <= sparse_vect->get_maxchar(); i++) {
//     char cc = (char)i;
  char *a6t = sparse_vect->get_alphabet() + 1;
  for (int i = 1; i < sparse_vect->get_alphabet_size(); i++) {
    char cc = *a6t++;
    if ((next = sparse_vect->get_target(start, cc)) != -1L) {
      candidate[depth] = cc;
      if (cuted(depth) <= e_d) {
	if (cc == ANNOT_SEPARATOR) {
	  // Move to annotations
	  find_repl(depth + 1, dummy->first_node(current_dict) + next);
	}
	else {
	  // Stay in the sparse vector
	  sparse_find_repl(depth + 1, next);
	}
	candidate[depth + 1] = '\0';	// restore candidate's length

	if (m_abs(word_length - 1 - depth) <= e_d &&
	    (dist = ed(word_length - 1, depth)) <= e_d &&
	    sparse_vect->is_final(start, cc)) {
	  word_found.list_item = nstrdup(candidate);
	  word_found.dist = dist;
	  word_found.cost = dist;		// for the moment
	  results.insert_sorted(&word_found);
	}//if replacement found
      }//if distance within limits
    }//if there is a transition with the label
  }//for all possible labels
  return &results;
}//spell_fsa::sparse_find_repl
#endif

/* Name:	find_repl
 * Class:	spell_fsa
 * Purpose:	Create a list of candidates for a misspelled word.
 * Parameters:	depth		- (i) current length of replacements;
 *		start		- (i) start with that node.
 * Returns:	TRUE if candidates found, FALSE otherwise.
 * Remarks:	A (partial) list of candidates is stored in results.
 *		See Kemal Oflazer - "Error-tolerant Finite State Recognition
 *		with Applications to Morphological Analysis and Spelling
 *		Correction", cmp-lg/9504031. Modified.
 */
hit_list *
spell_fsa::find_repl(const int depth, fsa_arc_ptr start)
{
  fsa_arc_ptr	next_node = start.set_next_node(current_dict);
  int		dist = 0;
  ranked_hits	word_found;
//  int		kids = fsa_children(start);

  if (depth + 1 >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

//  for (int i = 0; i < kids; i++, inc_next_node(next_node)) {
  forallnodes(i) {
    candidate[depth] = next_node.get_letter();
    if (cuted(depth) <= e_d) {
      find_repl(depth + 1, next_node);
      candidate[depth + 1] = '\0';	// restore candidate's length

      if (m_abs(word_length - 1 - depth) <= e_d &&
	  (dist = ed(word_length - 1, depth)) <= e_d &&
	  next_node.is_final()) {
	word_found.list_item = nstrdup(candidate);
	word_found.dist = dist;
	word_found.cost = dist;		// for the moment
	results.insert_sorted(&word_found);
      }
    }
  }
  return &results;
}//spell_fsa::find_repl


/* Name:	ed
 * Class:	spell_fsa
 * Purpose:	Calculates edit distance.
 * Parameters:	i		-(i) length of first word (here: misspelled)-1;
 *		j		-(i) length of second word (here: candidate)-1.
 * Returns:	Edit distance between the two words.
 * Remarks:	See Oflazer.
 */
int
spell_fsa::ed(const int i, const int j)
{
  int	result;
  int	a, b, c;	// not really necessary: tradition int &H::operator()

#ifdef DEBUG
  cerr << "Calculating ed(" << i << ", " << j << ")\n";
#endif
  if (word_ff[i] == candidate[j]) {
    // last characters are the same
    result = H(i, j);
  }
  else if (i > 0 && j > 0 && word_ff[i] == candidate[j - 1] &&
	   word_ff[i - 1] == candidate[j]) {
    // last two characters are transposed
    a = H(i - 1, j - 1);	// transposition, e.g. ababab, ababba
    b = H(i + 1, j);		// deletion,      e.g. abab,   aba
    c = H(i, j + 1);		// insertion      e.g. aba,    abab
    result = 1 + min(a, b, c);
  }
  else {
    // otherwise
    a = H(i, j);		// replacement,   e.g. ababa,  ababb
    b = H(i + 1, j);		// deletion,      e.g. ab,     a
    c = H(i, j + 1);		// insertion      e.g. a,      ab
    result = 1 + min(a, b, c);
  }

#ifdef DEBUG
  cerr << "ed(" << i << ", " << j << ") = " << result << "\n";
#endif
  H.set(i + 1, j + 1, result);
  return result;
}//spell_fsa::ed

/* Name:	cuted
 * Class:	spell_fsa
 * Purpose:	Calculates cut-off edit distance.
 * Parameters:	depth		- (i) current length of candidates.
 * Returns:	Cut-off edit distance.
 * Remarks:	See Oflazer.
 */
int
spell_fsa::cuted(const int depth)
{
  int l = max(0, depth - e_d);		// min chars from word to consider - 1
  int u = min(word_length-1, depth+e_d);// max chars from word to consider - 1
  int min_ed = e_d + 1;			// what is to be computed
  int d;

#ifdef DEBUG
  cerr << "cuted(" << depth << ")\n";
#endif
  for (int i = l; i <= u; i++) {
    if ((d = ed(i, depth)) < min_ed)
      min_ed = d;
  }

#ifdef DEBUG
  cerr << "cuted(" << depth << ") = " << min_ed << "\n";
#endif
  return min_ed;
}//spell_fsa::cuted
#endif //!CHCLASS

/* Name:	rank_replacements
 * Class:	spell_fsa
 * Purpose:	Sort the list of candidates according to their cost.
 * Parameters:	None.
 * Returns:	TRUE if replacements found, FALSE otherwise.
 * Remarks:	Cost is neglected. To be corrected later.
 *		Cost should reflect relative frequency of errors.
 */
int
spell_fsa::rank_replacements(void)
{
  if (results.how_many()) {
    results.reset();
    /* sort list of possible replacements */
    qsort(results.start(), results.how_many(), sizeof(ranked_hits *),
	  comp_cost);
    for (int i = 0; i < results.how_many(); results.next(), i++)
      replacements.insert(results.item()->list_item);

    results.empty_list();
    return TRUE;
  }
  return FALSE;
}/*spell_fsa::rank_replacements*/

/***	EOF spell.cc	***/
