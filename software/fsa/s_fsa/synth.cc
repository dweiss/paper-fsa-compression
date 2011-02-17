/***	synth.cc	***/

/*	Copyright (C) Jan Daciuk, 2010	*/


#include        <iostream>
#include	<string.h>
#include	<set>
#include	<map>
#include        "nstr.h"
#include        "fsa.h"
#include        "common.h"
#include        "synth.h"

int build_term(const char *re, int i, nfa_type &nfa);
int build_factor(const char *re, int i, nfa_type &nfa);
int buildRE(const char *re, int i, nfa_type &nfa);

/* Name:        synth_fsa
 * Class:       synth_fsa
 * Purpose:     Constructor.
 * Parameters:	ignorefiller	- (i) true if the filler character is to be
 *					ignored;
#ifdef MORPH_INFIX
 *		file_has_infixes- (i) true if the language uses infixes;
 *		file_has_prefixes
 *				- (i) true if the language uses prefixes;
#endif
 *		use_REs		- (i) true if the forms to be generated
 *					are specified with regular expressions;
 *		generate_all_forms
 *				- (i) true if all surface forms for a given
 *					canonical form have to be generated;
 *		dict_list       - (i) list of dictionary (fsa) names;
 *		language_file	- (i) file with character set.
 * Returns:     Nothing (constructor).
 * Remarks:     Only to launch fsa contructor.
 */
#ifdef MORPH_INFIX
synth_fsa::synth_fsa(const int ignorefiller,
		     const int file_has_infixes, const int file_has_prefixes,
		     const int use_REs, const int generate_all_forms,
		     word_list *dict_list, const char *language_file = NULL)
#else
synth_fsa::synth_fsa(const int ignorefiller, const int use_REs,
		     const int generate_all_forms,
		     word_list *dict_list, const char *language_file = NULL)
#endif
: fsa(dict_list, language_file)
{
#ifdef MORPH_INFIX
  morph_infixes = file_has_infixes;
  morph_prefixes = file_has_prefixes;
  pref_alloc = 0; prefix = NULL;
#endif
  ignore_filler = ignorefiller;
  useREs = use_REs;
  gen_all_forms = generate_all_forms;
  word_tags = new char[tag_alloc = Max_word_len]; // allocate memory for tags
}//synth_fsa::synth_fsa
 


/* Name:	synth_file
 * Class:	synth_fsa
 * Purpose:	Perform morphological synthesis on all words with tags
 *		in a file.
 * Parameters:	io_obj		- (i/o) where to read words with tags,
 *					and where to print analyses.
 * Returns:	Exit code.
 * Remarks:	Each input line contains a canonical form and tags
 *		separated with whitespace characters.
 *		The tags can either be an particular tag,
 *		or a regular expression matching more than one tag.
 *		This depends on a run-time option.
 */
int
synth_fsa::synth_file(tr_io &io_obj)
{
  int		allocated;
  char		*word;
  char		*tags;

  word = new char[allocated = Max_word_len];
  while (get_word(io_obj, word, allocated, Max_word_len)) {
    char *ss = word;
    // Locate white space
    while (*ss != '\0' && *ss != ' ' && *ss != 9) ss++;
    // Finish the word
    if (*ss != '\0') {
      *ss++ = '\0';
    }
    // Skip white space
    while (*ss == ' ' || *ss == 9) ss++;
    // Check format
    if (gen_all_forms && *ss != '\0') {
      std::cerr << "fsa_synth: tags given when -a specified - "
		<< "no tags are allowed\n";
      exit(3);
    }
    if (!gen_all_forms && *ss == '\0') {
      std::cerr << "fsa_synth: no tags given and no -a parameter used\n";
      exit(3);
    }
    word_length = strlen(word); word_ff = word;
    tags = ss;
    if (useREs) {
      buildDFA(tags);
    }
    if (synth_word(word, tags)) {
      io_obj.print_repls(&replacements);
      replacements.empty_list();
    }
    else
      io_obj.print_not_found();
  }
  return state;
}//synth_fsa::synth_file




/* Name:	synth_word
 * Class:	synth_fsa
 * Purpose:	Perform morphological synthesis of a word
 *		using all specified dictionaries.
 * Parameters:	word	- (i) canonical form to be synthetized;
 *		tags	- (i) categories of the word.
 * Returns:	Number of different forms of the word.
 * Remarks:	Class variable `replacements' is set to the list
 *		of possible results.
 *		tags can be empty string is regular expressions
 *		or listing of all forms are used.
 */
int
synth_fsa::synth_word(const char *word, const char *tags)
{
  dict_list		*dict;
#if !(defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE))
  fsa_arc_ptr		*dummy = NULL;
#endif
  char			converted = '\0';

  dictionary.reset();
  for (dict = &dictionary; dict->item(); dict->next()) {
    // Try with each dictionary
    ANNOT_SEPARATOR = dict->item()->annot_sep;
    set_dictionary(dict->item());
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
    sparse_synth_next_char(word, 0, sparse_vect->get_first(), tags);
#else
    synth_next_char(word, 0, dummy->first_node(current_dict), tags);
#endif
#ifdef CASECONV
    if (is_downcaseable(word)) {
      myflipcase((char *)word, -1);	// Convert to lowercase
      //*((char *)word) = casetab[(unsigned char)*word];
      converted = *word;
    }
#endif
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
    sparse_synth_next_char(word, 0, sparse_vect->get_first(), tags);
#else
    synth_next_char(word, 0, dummy->first_node(current_dict), tags);
#endif
#ifdef CASECONV
    if (converted) {
      // convert back to uppercase
      myflipcase((char *)word, 1);
      //*((char *)word) = casetab[(unsigned char)*word];
      fsa_arc_ptr xnt_node = dummy->first_node(current_dict);
#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
      sparse_synth_next_char(word, 0, sparse_vect->get_first(), tags);
#else
      synth_next_char(word, 0, dummy->first_node(current_dict), tags);
#endif
    }
#endif
  }

#ifdef CASECONV
  if (converted) {
    // word was uppercase
    replacements.reset();
    for (;replacements.item();replacements.next()) {
    // word is uppercase - convert to lowercase
    myflipcase((char *)word, -1);
    }
  }
#endif

  return replacements.how_many();
}//synth_fsa::synth_word



/* Name:	buildRE
 * Class:	None.
 * Purpose:	Creates a nondeterministic automaton from a regular
 *		expression.
 * Parameters:	re	- (i) regular expression string;
 *		i	- (i) index in the string where the RE starts;
 *		nfa	- (i) the automaton to be built.
 * Returns:	Nothing.
 * Remarks:	The Yamada-McNaughton or Glushkov or Beri-Sethi algorithm
 *		proved to be too difficult to implement here
 *		due to a large number of states for complements.
 *		Instead, the standard Thompson construction is used.
 */
int
buildRE(const char *re, int i, nfa_type &nfa)
{
  i = build_term(re, i, nfa);
  if (re[i] == '|') {
    int s1 = nfa.create_state();
    int s2 = nfa.create_state();
    int s3 = nfa.last_re().first;
    int s4 = nfa.last_re().second;
    nfa.create_transition(s1, 0, s3, epsil);
    nfa.create_transition(s4, 0, s2, epsil);
    do {
      i = build_term(re, i + 1, nfa);
      s3 = nfa.last_re().first;
      s4 = nfa.last_re().second;
      nfa.create_transition(s1, 0, s3, epsil);
      nfa.create_transition(s4, 0, s2, epsil);
    } while (re[i] == '|');
    nfa.implement_re(s1, s2);
  }
  return i;
}//buildRE

/* Name:	buildDFA
 * Class:	synth_fsa
 * Purpose:	Creates a deterministic automaton from a regular expression.
 * Parameters:	re	- (i) regular expression.
 * Returns:	Nothing.
 * Remarks:	None.
 */
void
synth_fsa::buildDFA(const char *re)
{
  nfa_type nfa;
  int i = ::buildRE(re, 0, nfa);
  if (re[i]) {
    std::cerr << "Unrecognized character `" << re[i]
	      << "' in regular expression " << re
	      << "at position " << i << "\n";
    exit(5);
  }
  nfa.set_final(nfa.last_re().second);
  determinize(nfa);
}//synth_fsa::buildDFA

/* Name:	build_term
 * Class:	None.
 * Purpose:	Builds an NFA for a term in a regular expression.
 * Parameters:	re	- (i) a string representing a regular expression;
 *		i	- (i) initial index of the term in re;
 *		nfa	- (i/o) nondeterministic finite-state automaton
 *			  that recognizes the regular expression.
 * Returns:	Index of the first character after the term.
 * Remarks:	A term is a concatenation of a series of factors - at least 1.
 */
int
build_term(const char *re, int i, nfa_type &nfa)
{
  i = build_factor(re, i, nfa);
  int s1 = nfa.last_re().first;
  int s2 = nfa.last_re().second;
  while (strchr("|*)]?", re[i]) == NULL) {
    i = build_factor(re, i, nfa);
    int s3 = nfa.last_re().first;
    int s4 = nfa.last_re().second;
    nfa.create_transition(s2, 0, s3, epsil);
    s2 = s4;
  }//while there are more symbols from the alphabet to be concatenated
  nfa.implement_re(s1, s2);
  return i;
}//build_term

/* Name:	build_character_class
 * Class:	None
 * Purpose:	Builds an NFA for a character class.
 * Parameters:	re	- (i) a string representing a regular expression;
 *		i	- (i) initial index of the term in re;
 *		nfa	- (i/o) nondeterministic finite-state automaton
 *			  that recognizes the regular expression.
 * Returns:	Index of the first character after the character class.
 * Remarks:	A character class is a sequence of characters
 *		enclosed in square brackets.
 *		If the first character is ^, then the class is negated
 *		(it is the complement of characters specified in brackets).
 *		A range of characters can be written as a-z,
 *		where a is the first in the range, and z is the the last one.
 *		A dash can be written as the first (possibly after ^)
 *		or the last character.
 */
int build_character_class(const char *re, int i, nfa_type &nfa)
{
  int s1 = nfa.create_state();
  int s2 = nfa.create_state();
  int s3 = s2;
  int complement = false;
  if (re[i] == '^') {
    // Rather than accept the characters on the list,
    // accept all other characters
    complement = true;
    s3 = -1;
    i++;
  }
  if (re[i] == '-') {
    // Initial dash, treat as a regular character
    nfa.create_transition(s1, re[i++], s3, regular);
  }
  while (re[i] != '0' && re[i] != ']') {
    if (re[i] == '-') {
      // Range of characters
#ifdef UTF8
      // It is very hard to implement the range here properly
      // Find the start character code
      int j = i - 1;
      while (utf8t(re[j]) == CONTBYTE) --j;
      long ustart = str2utf(re, j);
      long ustop = str2utf(re, ++i);
      if (ustop < ustart) {
	std::cerr << "fsa_synth: Start character in range"
		  << " in a character class\n"
		  << "must have a code not smaller than"
		  << "the last character in the range\n";
	exit(5);
      }
      j = 0;
      while (++ustart <= ustop) {
	j = nfa.create_utf_path(s1, utf2str(ustart), s3);
      }
      i += j;
#else
      char start = re[i - 1];
      char stop = start;
      i++;			// take the character after the dash
      if (re[i] != 0 && re[i] != ']') {
	// Insert all characters in the range by creating transitions
	stop = re[i];
	char c = start;
	unsigned char ustart = start;
	unsigned char ustop = stop;
	if (ustop <= ustart) {
	  std::cerr << "fsa_synth: Start character in range"
		    << " in a character class\n"
		    << "must have a code not smaller than"
		    << "the last character in the range\n";
	  exit(5);
	}
	do {
	  c++;
	  nfa.create_transition(s1, c, s3, regular);
	} while (c != stop);
	i++;
      }
      else {
	// just regular dash at the end of the class
	nfa.create_transition(s1, re[i++], s3, regular);
      }
#endif
    }//re[i] == '-'
    else {
      if (re[i] == '\\') {
	// Escaped character
	i++;
      }
      // Just a regular character
#ifdef UTF8
      int u8t = utf8t(re[i]);
      if (u8t == ONEBYTE) {
#endif
	nfa.create_transition(s1, re[i++], s3, regular);
#ifdef UTF8
      }
      else if (u8t & 0x70) {	// START2 | START3 | START4
	i += nfa.create_utf_path(s1, re + i, s3);
      }
      else {
	std::cerr << "Invalid UTF8 character\n";
	exit(5);
      }
#endif //UTF8
    }
  }//while
  if (complement) {
    nfa.create_transition(s1, 0, s2, all_other);
  }
  nfa.implement_re(s1, s2);
  return i;
}//build_character_class

/* Name:	build_factor
 * Class:	None.
 * Purpose:	Builds an NFA for a factor in a regular expression.
 * Parameters:	re	- (i) a string representing a regular expression;
 *		i	- (i) initial index of the term in re;
 *		nfa	- (i/o) nondeterministic finite-state automaton
 *			  that recognizes the regular expression.
 * Returns:	Index of the first character after the factor.
 * Remarks:	A factor is ("a" is a regular character,
 *		re is regular expression):
 *		a
 *		(re)
 *		character class
 *		.
 *		a?
 *		a*
 *		a+
 *		\a
 */
int
build_factor(const char *re, int i, nfa_type &nfa)
{
  switch (re[i]) {

  case '(':
    i = buildRE(re, i + 1, nfa);
    if (re[i] != ')') {
      std::cerr << "Missing `)' in regular expression " << re
		<< " at position " << i << "\n";
      exit(5);
    }
    i++;
    break;

  case '[':
    i = build_character_class(re, i + 1, nfa);
    if (re[i] != ']') {
      std::cerr << "Missing `]' in regular expression " << re
		<< " at position " << i << "\n";
      exit(5);
    }
    i++;
    break;

  case '.':			// any symbol
    {
      int s01 = nfa.create_state();
      int s02 = nfa.create_state();
      nfa.create_transition(s01, 0, s02, any_symbol);
      nfa.implement_re(s01, s02);
      i++;
    }
    break;

  case '\\':
    i++;			// fall through
  default:			// alphabet symbol
    {
      int s1 = nfa.create_state();
      int s2 = nfa.create_state();
#ifdef UTF8
      int u8t = utf8t(re[i]);
      if (u8t == ONEBYTE) {
#endif
	nfa.create_transition(s1, re[i++], s2, regular);
#ifdef UTF8
      }
      else if (u8t == START2 || u8t == START3 || u8t == START4) {
	i += nfa.create_utf_path(s1, re + i, s2);
      }
      else {
	std::cerr << "Invalid UTF8 character\n";
	exit(5);
      }
#endif
      nfa.implement_re(s1, s2);
    }
    break;
  }//switch

  switch (re[i]) {

  case '*':			// Kleene star
    {
      int s1 = nfa.create_state();
      int s2 = nfa.create_state();
      int s3 = nfa.last_re().first;
      int s4 = nfa.last_re().second;
      nfa.create_transition(s1, 0, s3, epsil);
      nfa.create_transition(s4, 0, s2, epsil);
      nfa.create_transition(s4, 0, s3, epsil);
      nfa.create_transition(s1, 0, s2, epsil);
      nfa.implement_re(s1, s2);
      i++;
    }
    break;

  case '+':			// Kleene plus
    {
      int s1 = nfa.create_state();
      int s2 = nfa.create_state();
      int s3 = nfa.last_re().first;
      int s4 = nfa.last_re().second;
      nfa.create_transition(s1, 0, s3, epsil);
      nfa.create_transition(s4, 0, s2, epsil);
      nfa.create_transition(s4, 0, s3, epsil);
      nfa.implement_re(s1, s2);
      i++;
    }
    break;

  case '?':			// optional (alternative with epsilon)
    {
      int s1 = nfa.create_state();
      int s2 = nfa.create_state();
      int s3 = nfa.last_re().first;
      int s4 = nfa.last_re().second;
      nfa.create_transition(s1, 0, s3, epsil);
      nfa.create_transition(s4, 0, s2, epsil);
      nfa.create_transition(s1, 0, s2, epsil);
      nfa.implement_re(s1, s2);
      i++;
    }
    break;

  }//switch
  return i;
}//build_factor

/* Name:	determinize
 * Class:	synth_fsa
 * Purpose:	Determinizes a nondeterministic automaton (NFA).
 * Parameters:	nfa	- (i/o) NFA to be determinized.
 * Returns:	Nothing.
 * Remarks:	The DFA is a class variable.
 *		To speed up processing, nfa can be destroyed.
 */
void
synth_fsa::determinize(nfa_type &nfa)
{
  std::vector<std::set<int> > dfa_states;
  std::map<std::set<int>,int> nfa_dfa_map;
  std::set<int> ds;
  dfa_state d;
  nfa.epsilon_closure(nfa.last_re().first, ds);
  dfa.reset();
  dfa_states.push_back(ds);
  nfa_dfa_map[ds] = 0;
  d.first_trans = -1;
  d.number_trans = 0;
  d.final = nfa.final_set(ds);
  dfa.states.push_back(d);
  int first_in_queue = 0;
  int last_in_queue = 0;
  while (first_in_queue <= last_in_queue) {
    // For each new DFA state
    std::set<char> out_alphabet;
    ds.clear();
    for (std::set<int>::iterator si = dfa_states[first_in_queue].begin();
	 si != dfa_states[first_in_queue].end(); si++) {
      // Collect symbols on outgoing transitions for the whole set
      // of NFA states forming the current DFA state
      out_alphabet.insert(nfa.states[*si].alphabet.begin(),
			  nfa.states[*si].alphabet.end());
    }
    for (std::set<char>::iterator a = out_alphabet.begin();
	 a != out_alphabet.end(); a++) {
      ds = nfa.delta_set(dfa_states[first_in_queue], *a);
      if (nfa_dfa_map.find(ds) == nfa_dfa_map.end()) {
	// New DFA state
	dfa_states.push_back(ds);
	d.first_trans = -1;
	d.number_trans = 0;
	d.final = nfa.final_set(ds);
	dfa.states.push_back(d);
	nfa_dfa_map[ds] = ++last_in_queue;
	// Create a transition from the current DFA state (first_in_queue)
	// to the newly created state
	dfa.create_transition(first_in_queue, *a,  last_in_queue);
      }
      else {
	// Existing DFA state
	// Create a transition from the current DFA state (first_in_queue)
	// to an existing DFA state with the same NFA states set
	dfa.create_transition(first_in_queue, *a, nfa_dfa_map[ds]);
      }
    }//for a
    // Now create transitions for dot and complements
    ds.clear();
    for (std::set<int>::iterator si = dfa_states[first_in_queue].begin();
	 si != dfa_states[first_in_queue].end(); si++) {
      // Collect states that are targets of of transitions on any symbol
      // or on complements of a set of symbols
      if (nfa.states[*si].any_trans != -1) {
	nfa.epsilon_closure(nfa.states[*si].any_trans, ds);
      }
      else if (nfa.states[*si].other_trans != -1) {
	nfa.epsilon_closure(nfa.states[*si].other_trans, ds);
      }
    }//for si
    if (nfa_dfa_map.find(ds) == nfa_dfa_map.end()) {
      // New DFA state
      dfa_states.push_back(ds);
      d.first_trans = -1;
      d.number_trans = 0;
      d.final = nfa.final_set(ds);
      dfa.states.push_back(d);
      nfa_dfa_map[ds] = ++last_in_queue;
      // Create a transition for all other symbols
      // from the current DFA state (first_in_queue)
      // to the newly created state
      dfa.create_transition(first_in_queue, 0,  last_in_queue);
    }
    else {
      // Create a transition for all other symbols
      // from the current DFA state (first_in_queue)
      // to an existing DFA state
      dfa.create_transition(first_in_queue, 0,  nfa_dfa_map[ds]);
    }
    first_in_queue++;
  }//while queue not empty
}//synth_fsa::determinize

#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
/* Name:	sparse_synth_next_char
 * Class:	synth_fsa
 * Purpose:	Consider the next node in morphological synthesis.
 * Parameters:	word	- (i) canonical form of an inflected form
 *				to be synthesised;
 *		level	- (i) how many characters of the word have been
 *				considered so far;
 *		start	- (i) look at that node;
 *		tags	- (i) categories of the inflected form
 *				to be synthesised.
 * Returns:	Number of different syntheses of the word with the tags.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different syntheses of the word.
 *		Entries in the dictionary have the following format:
 *		canonical_form+tags+Kending
 *		where:
 *		canonical_form is the canonical form of a word,
 *		K specifies how many characters from the end of canonical_form
 *			do not match those from the respective inflected form;
 *			that number is computed as K - 'A',
 *		ending is the ending of the inflected form,
 *		tags is a set of categories (annotation) of the inflected form.
 *		Example:
 *		cry+Vpp+Bied means that the canonical form is "cry",
 *		tags are "Vpp", and the inflected form is "cried":
 *		('B' - 'A' = 1, "cry" - 1 letter at end = "cr",
 *		"cr"+"ied"="cried").
 *		If MORPH_INFIXES is defined, and -I flag was used,
 *		entries in the dictionary are supposed to have the following
 *		format:
 *		canonical_form+tags+prefix/infix+MKending
 *		where:
 *		canonical_form, tags, and K have the same meaning as above,
 *		prefix/infix is a prefix or an infix that should be prepended
 *		or inserted to the canonical form,
 *		M is the position of the infix, with M='A' meaning prefix,
 *		M='B' meaning an infix inserted after the first character,
 *		M='C' - an infix inserted after the second character, and so on.
 *		If MORPH_INFIXES is defined, and -P flag was used,
 *		entries in the dictionary are supposed to have the following
 *		format:
 *		canonical_form+tags+prefix+Kending
 *		where the items keep their meaning from the format with infixes.
 */
int
synth_fsa::sparse_synth_next_char(const char *word, const int level,
				  const long start, const char *tags)
{
  bool found = false;
  int lev = level;
  long current = start;
  long next;
  fsa_arc_ptr *dummy;
  do {
    found = false;
    if (*word == '\0') {
      if ((next = sparse_vect->get_target(current, ANNOT_SEPARATOR)) != -1L) {
	if (useREs) {
	  synth_RE(lev, current_dict + next
#ifdef NUMBERS
		   + dummy->entryl
#endif //NUMBERS
		   );
	}
	else if (gen_all_forms) {
	  skip_tags(lev, current_dict + next
#ifdef NUMBERS
		    + dummy->entryl
#endif //NUMBERS
		    );
	}
	else {
	  synth_tags(lev, current_dict + next
#ifdef NUMBERS
		     + dummy->entryl
#endif //NUMBERS
		     , tags);
	}
      }
    }
    else {
      if ((next = sparse_vect->get_target(current, *word, tags)) != -1L) {
	word++;
	lev++;
	current = next;
	found = true;
      }
    }
  } while (found);
  return replacements.how_many();
}//synth_fsa::sparse_synth_next_char

#else //!(FLEXIBLE&STOPBIT&NEXTBIT)

/* Name:	synth_next_char
 * Class:	synth_fsa
 * Purpose:	Consider the next node in morphological synthesis.
 * Parameters:	word	- (i) canonical form of an inflected form
 *				to be synthesised;
 *		level	- (i) how many characters of the word have been
 *				considered so far;
 *		start	- (i) look at children of that node;
 *		tags	- (i) categories of the inflected form
 *				to be synthesised.
 * Returns:	Number of different syntheses of the word with the tags.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 *		Entries in the dictionary have the following format:
 *		canonical_form+tags+Kending
 *		where:
 *		canonical_form is the canonical form of a word,
 *		K specifies how many characters from the end of canonical_form
 *			do not match those from the respective inflected form;
 *			that number is computed as K - 'A',
 *		ending is the ending of the inflected form,
 *		tags is a set of categories (annotation) of the inflected form.
 *		Example:
 *		cry+Vpp+Bied means that the canonical form is "cry",
 *		tags are "Vpp", and the inflected form is "cried":
 *		('B' - 'A' = 1, "cry" - 1 letter at end = "cr",
 *		"cr"+"ied"="cried").
 *		If MORPH_INFIXES is defined, and -I flag was used,
 *		entries in the dictionary are supposed to have the following
 *		format:
 *		canonical_form+tags+prefix/infix+MKending
 *		where:
 *		canonical_form, tags, and K have the same meaning as above,
 *		prefix/infix is a prefix or an infix that should be prepended
 *		or inserted to the canonical form,
 *		M is the position of the infix, with M='A' meaning prefix,
 *		M='B' meaning an infix inserted after the first character,
 *		M='C' - an infix inserted after the second character, and so on.
 *		If MORPH_INFIXES is defined, and -P flag was used,
 *		entries in the dictionary are supposed to have the following
 *		format:
 *		canonical_form+tags+prefix+Kending
 *		where the items keep their meaning from the format with infixes.
 */
int
synth_fsa::synth_next_char(const char *word, const int level,
			   fsa_arc_ptr start, const char *tags)
{
  bool found = false;
  int lev = level;
  do {
    found = false;
    fsa_arc_ptr next_node = start.set_next_node(current_dict);
    if (*word == '\0') {
      forallnodes(i) {
	if (next_node.get_letter() == ANNOT_SEPARATOR) {
	  if (useREs) {
	    synth_RE(lev, next_node, dfa.get_start_state());
	  }
	  else if (gen_all_forms) {
	    skip_tags(lev, next_node);
	  }
	  else {
	    synth_tags(lev, next_node, tags);
	  }
	  break;
	}
      }
    }
    else {
      forallnodes(j) {
	if (*word == next_node.get_letter()) {
	  word++;
	  lev++;
	  start = next_node;
	  found = true;
	  break;
	}
      }
    }
  } while (found);
  return replacements.how_many();
}//synth_fsa::synth_next_char
#endif //!(FLEXIBLE&STOPBIT&NEXTBIT)

/* Name:	synth_tags
 * Class:	synth_fsa
 * Purpose:	Synthetizes an inflected word by following its tags
 *		in a dictionary.
 * Parameters:	level	- (i) how many characters there are in the canonical
 *				form;
 *		start	- (i) look at that node;
 *		tags	- (i) tags of the inflected form to be synthesized.
 * Returns:	Number of different syntheses available in the analysed part
 *		of the automaton.
 * Remarks:	Tags are only followed in the automaton. They choose
 *		the path to correct codes that tranform the canonical form
 *		into inflected form.
 *		When the tags end, the next label to follow is the annotation
 *		separator.
 *		Depending on the compile option MORPH_INFIXES,
 *		and the runtime options -P and -I, the next label might be:
 *		1. the deletion code K stating how many characters to remove
 *			from the end of the canonical form; it is followed
 *			by the ending of the inflected form;
 *		2. a prefix or an infix if MORPH_INFIXES compile option
 *			was specified and -P or -I flag was used
 *			to invoke fsa_synth.
 */
int
synth_fsa::synth_tags(const int level, fsa_arc_ptr start, const char *tags)
{
  const char *t = tags;
  fsa_arc_ptr next_node = start.set_next_node(current_dict);
  if (*t == '\0') {
    forallnodes(i) {
      if (next_node.get_letter() == ANNOT_SEPARATOR) {
	fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
#ifdef MORPH_INFIXES
	if (morph_infixes || morph_infixes) {
	  synth_prefix(level, 0, nxt_node);
	}
	else
#endif
	  synth_stem(level, 0, 0, nxt_node);
      }
    }
  }
  else {
    forallnodes(j) {
      if (*t == next_node.get_letter()) {
	t++;
	synth_tags(level, next_node, t);
	break;
      }
    }
  }
  return replacements.how_many();
}//synth_fsa::synth_tags

/* Name:	skip_tags
 * Class:	synth_fsa
 * Purpose:	Finds all possible tags for a canonical form,
 *		and all associated surface forms.
 * Parameters:	level		- (i) how many characters there are
 *					in the canonical form;
 *		start		- (i) look at that node.
 * Returns:	The number of different morphological syntheses in the analysed
 *		part of the automaton.
 * Remarks:	Tags are recorded in word_tags.
 */
int
synth_fsa::skip_tags(const int level, fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start.set_next_node(current_dict);
  forallnodes(i) {
    if (next_node.get_letter() == ANNOT_SEPARATOR) {
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
#ifdef MORPH_INFIXES
      if (morph_infixes || morph_infixes) {
	synth_prefix(level, 0, nxt_node);
      }
      else
#endif
	synth_stem(level, 0, 0, nxt_node);
    }
    else {
      // A tag character, record it
      if (tag_length >= tag_alloc) {
	grow_string(word_tags, tag_alloc, Max_word_len);
      }
      word_tags[tag_length++] = next_node.get_letter();
      skip_tags(level, next_node);
      --tag_length;
    }
  }//forallnodes
  return replacements.how_many();
}//synth_fsa::skip_tags

/* Name:	synth_RE
 * Class:	synth_fsa
 * Purpose:	Recognizes tags matching a regular expression.
 * Parameters:	level		- (i) how many characters there are
 *					in the canonical form;
 *		start		- (i) look at that node;
 *		dstate		- (i) current DFA state.
 * Returns:	The number of different morphological syntheses in the analysed
 *		part of the automaton.
 * Remarks:	Tags are recorded in word_tags.
 */
int
synth_fsa::synth_RE(const int level, fsa_arc_ptr start, const int dstate)
{
  fsa_arc_ptr next_node = start.set_next_node(current_dict);
  int	ns = -1;		// next DFA state
  forallnodes(i) {
    if (next_node.get_letter() == ANNOT_SEPARATOR) {
      if (dfa.is_final(dstate)) {
	fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
#ifdef MORPH_INFIXES
	if (morph_infixes || morph_infixes) {
	  synth_prefix(level, 0, nxt_node);
	}
	else
#endif
	  synth_stem(level, 0, 0, nxt_node);
      }
    }
    else if ((ns = dfa.delta(dstate, next_node.get_letter())) != -1) {
      // A matching tag character, record it
      if (tag_length >= tag_alloc) {
	grow_string(word_tags, tag_alloc, Max_word_len);
      }
      word_tags[tag_length++] = next_node.get_letter();
      synth_RE(level, next_node, ns);
      --tag_length;
    }
  }//forallnodes
  return replacements.how_many();
}//synth_fsa::synth_RE



#ifdef MORPH_INFIX



/* Name:	synth_prefix
 * Class:	synth_fsa
 * Purpose:	Follow and store a prefix or an infix.
 * Parameters:	level		- (i) how many characters there are
 *					in the inflected form;
 *		plen		- (i) current prefix/infix length;
 *		start		- (i) look at this node.
 * Returns:	The number of different morphological syntheses in the analysed
 *		part of the automaton.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 *		The prefix or infix is stored in the variable `prefix'.
 *		The next character after the prefix/infix in the automaton
 *		should be the annotation separator.
 */
int
synth_fsa::synth_prefix(const int level, const int plen,
			fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start;
  int	c;

  if (plen >= pref_alloc)
    grow_string(prefix, pref_alloc, Max_word_len);

  forallnodes(i) {
    if ((c = next_node.get_letter()) == ANNOT_SEPARATOR) {
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
      if (morph_infixes) {
	synth_infix(level, plen, nxt_node);
      }
      else {
	synth_stem(level, plen, 0, nxt_node);
      }
    }
    else {
      prefix[plen] = c;
      synth_prefix(level, plen + 1, next_node);
    }
    /*
    if ((delete_length = (next_node.get_letter() - 'A')) >= 0 &&
	delete_length < word_length) {
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
      synth_stem(delete_length, delete_position, level, nxt_node);
    }
    */
  }
  return replacements.how_many();
}//synth_fsa::synth_prefix


/* Name:	synth_infix
 * Class:	synth_fsa
 * Purpose:	Recognize a code and calculate the position of the infix.
 * Parameters:	level	- (i) how many characters there are
 *					in the inflected form;
 *		inflen	- (i) infix length;
 *		start	- (i) look at this node.
 * Returns:	The number of different morphological syntheses in the analysed
 *		part of the automaton.
 * Remarks:	The canonical form was recognized in the automaton,
 *		the infix was followed in the automaton and copied
 *		to the variable `prefix', and the next label in the automaton
 *		is the coded infix position. 'A' means prefix,
 *		'B' means the infix goes right after the first character
 *		of the canonical form, 'C' -- after the second one, and so on.
 */
int
synth_fsa::synth_infix(const int level, const int inflen, fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start;
  int inf_pos;			// position of the infix
  forallnodes(i) {
    if ((inf_pos = (next_node.get_letter() - 'A')) >= 0 &&
	inf_pos < word_length) {
      synth_stem(level, inflen, inf_pos, next_node);
    }
  }
  return replacements.how_many();
}//synth_fsa::synth_infix
#endif


/* Name:	synth_stem
 * Class:	synth_fsa
 * Purpose:	Establish how many characters from the end of the canonical
 *		form must be deleted to form the inflected form (with possibly
 *		some new characters).
 * Parameters:	level		- (i) how many characters there are
 *					in the canonical form;
 *		plen		- (i) length of a prefix/infix;
 *		infix_pos	- (i) position of the infix (0=prefix);
 *		start		- (i) look at that node.
 * Returns:	Number of different analysis available in the analysed part
 *		of the automaton.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 *		At this point, the canonical form is recognized.
 *		In case of defined MORPH_INFIXES compile option,
 *		and either -P or -I runtime option, a prefix or infix
 *		was copied from the dictionary to the variable `prefix'.
 *
 *		If plen is greater than zero, then 
 */
int
synth_fsa::synth_stem(const int level, const int plen,
		      const int infix_pos, fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start;
  int reject_from_word;
  int rest_start = word_length;

  while (level +  plen >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

  forallnodes(i) {
    if ((reject_from_word = (next_node.get_letter() - 'A')) >= 0 &&
	reject_from_word <= word_length) {
      if (plen > 0) {
	if (infix_pos > 0) {
	  // An infix
	  strncpy(candidate, word_ff, infix_pos); // copy the part before infix
	  strcpy(candidate + infix_pos, prefix);  // copy infix
	  strncpy(candidate + infix_pos + plen, // copy rest
		  word_ff, word_length - infix_pos - reject_from_word);
	}
	else {
	  // A prefix
	  strcpy(candidate, prefix); // copy the prefix
	  strncpy(candidate + plen, word_ff, word_length - reject_from_word);
	}
      }
      else {
	strncpy(candidate, word_ff, word_length - reject_from_word);
      }
      rest_start = word_length + plen - reject_from_word;
      if (next_node.is_final()) {
	candidate[rest_start] = '\0';
	replacements.insert_sorted(candidate);
      }
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
      synth_rest(rest_start, nxt_node);
    }
  }
  return replacements.how_many();
}//synth_fsa::synth_stem


/* Name:	synth_rest
 * Class:	synth_fsa
 * Purpose:	Append inflected word ending at the end of the candidate.
 * Parameters:	level	- (i) how many characters there are so far
 *				in the candidate;
 *		start	- (i) look at this node.
 * Returns:	The number of different morphological syntheses in the analysed
 *		part of the automaton.
 * Remarks:	Class variable `replacements' is set to the list
 *		of different analyses of the word.
 */
int
synth_fsa::synth_rest(const int level, fsa_arc_ptr start)
{
  fsa_arc_ptr next_node = start;

  if (level >= cand_alloc)
    grow_string(candidate, cand_alloc, Max_word_len);

  if (start.arc != current_dict) {
    forallnodes(i) {
      if (!ignore_filler || next_node.get_letter() != FILLER) {
	candidate[level] = next_node.get_letter();
      }
      if (next_node.is_final()) {
	candidate[level + 1] = '\0';
	replacements.insert_sorted(candidate);
      }
      fsa_arc_ptr nxt_node = next_node.set_next_node(current_dict);
      synth_rest(level + 1, nxt_node);
    }
  }
  return replacements.how_many();
}//synth_fsa::synth_rest




/***	EOF synth.cc	***/
