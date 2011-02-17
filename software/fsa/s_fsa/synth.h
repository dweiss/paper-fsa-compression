/***	synth.h		***/

/*	Copyright (c) Jan Daciuk, 2010	*/

#include	<vector>
#include	<set>
#include	<stdlib.h>

  
enum trans_genre { regular, epsil, any_symbol, all_other };

class state_type {
public:
  int			first_trans;	// first transition number
  bool			final;
  int			any_trans; 	// target for all labels
  int			other_trans;	// target for every other label
  std::set<char>	alphabet;	// labels on outgoing transitions
  state_type(void) : first_trans(-1), final(false) {
    any_trans = other_trans = -1;
  }
};

class trans_type {
public:
  int		target;			// target state number
  char		label;
  int		next;			// next transition number
  trans_genre	genre;
  trans_type(const int t, const char l, const trans_genre g)
    : target(t), label(l), next(-1), genre(g) {}
};

typedef std::pair<int, int> re_type;

/* Name:	nfa_type
 * Purpose:	Implements a nondeterministic finite-state automaton (NFA).
 * Remarks:	To save space and time, special transitions are implemented.
 *		Apart from regular and epsilon transitions,
 *		transitions for all symbols ("." in regular expressions)
 *		and for all other symbols are implemented.
 */
class nfa_type {
public:
  std::vector<state_type>	states;
  std::vector<trans_type>	transitions;
  std::vector<re_type>		re;
  std::set<char>		alphabet;
  int				free_trans; // first free transition number
  // Create a non-final state with no outgoing transitions
  int create_state(void) {
    state_type s;
    states.push_back(s);
    return states.size() - 1;
  }
  // Create a transition
  void create_transition(const int source, const char label, const int target,
			 trans_genre genre) {
    trans_type t(target, label, genre);
    int trans_no = -1;
    if (genre == any_symbol) {
      states[source].any_trans = target;
      return;
    }
    else if (genre == all_other) {
      states[source].other_trans = target;
      return;
    }
    // Create transition entry
    if (free_trans == -1) {
      trans_no = transitions.size();
      transitions.push_back(t);
    }
    else {
      trans_no = free_trans;
      free_trans = transitions[trans_no].next;
      transitions[trans_no] = t;
    }
    // Update transition list for the source state
    if (states[source].first_trans == -1) {
      // State has no transitions so far
      states[source].first_trans = trans_no;
    }
    else {
      int tn = states[source].first_trans;
      if (label <= transitions[tn].label) {
	// Put the new transition as the first one on the list
	transitions[trans_no].next = tn;
	states[source].first_trans = trans_no;
      }
      else {
	while (transitions[tn].next != -1 &&
	       label > transitions[transitions[tn].next].label) {
	  tn = transitions[tn].next;
	}
	if (transitions[tn].next != -1) {
	  // Put in front of the next transition
	  transitions[trans_no].next = transitions[tn].next;
	  transitions[tn].next = trans_no;
	}
	else {
	  transitions[tn].next = trans_no;
	}
      }
    }
    if (label != 0) {
      states[source].alphabet.insert(label);
    }
  }//create_transition

  // Record what the first and the last state implementing the reg. expr. is
  void implement_re(const int start, const int stop) {
    re.push_back(re_type(start, stop));
  }

  // Return the first and the last state implementing the regular expression
  re_type last_re(void) {return re.back(); }

  // ss may already contain states
  void epsilon_closure(const int start_state, std::set<int> &ss) {
    if (ss.find(start_state) == ss.end()) {
      // State not yet in ss
      if (start_state != -1) {
	// Just a regular state, not the sink - add it
	if (ss.size() == 1 && *(ss.begin()) == -1) {
	  // ss contained only sink state, remove it as any state + sink = state
	  ss.erase(-1);
	}
	ss.insert(start_state);
	for (int i = states[start_state].first_trans; i != -1;
	     i = transitions[i].next) {
	  if (transitions[i].label == 0 && transitions[i].genre == epsil) {
	    epsilon_closure(transitions[i].target, ss);
	  }
	}
      }
      else if (ss.size() == 0) {
	// Since ss is empty, make it contain the sink (current start_state)
	ss.insert(start_state);
      }
    }
  }//epsilon_closure

  // Find a set of targets for transitions from a given set of NFA states
  // on a given label
  // The sink state (-1) deserves special treatment
  std::set<int> delta_set(std::set<int> &source_set, const char label) {
    std::set<int> ts;
    for (std::set<int>::iterator si = source_set.begin();
	 si != source_set.end(); si++) {
      if (*si != -1) {
	bool label_found = false;
	for (int t = states[*si].first_trans; t != -1;
	     t = transitions[t].next) {
	  if (transitions[t].label == label) {
	    epsilon_closure(transitions[t].target, ts);
	    label_found = true;
	    break;
	  }
	}
	if (!label_found) {
	  if (states[*si].any_trans != -1) {
	    epsilon_closure(states[*si].any_trans, ts);
	  }
	  else if (states[*si].other_trans != -1) {
	    epsilon_closure(states[*si].other_trans, ts);
	  }
	}
      }
    }
    return ts;
  }

  // Return true if the set of states contains a final state
  bool final_set(std::set<int> &state_set) {
    bool f = false;
    for (std::set<int>::iterator s = state_set.begin(); s != state_set.end();
	 s++) {
      f = (f || states[*s].final);
    }
    return f;
  }

  // Traverse a transition with a given label.
  // Note that in Thopson construction, a state can have more than one
  // outgoing epsilon-transition, but only one regular outgoing transition
  // with a given label, so delta returns one state number.
  // Also, we are not interested in transitions with meta labels
  // like any character or any other character
  int delta(const int s, const char l) {
    int t = states[s].first_trans;
    while (t != -1 && transitions[t].label != l) {
      t = transitions[t].next;
    }
    if (t != -1) {
      return transitions[t].target;
    }
    else {
      return -1;
    }
  }//delta

#ifdef UTF8
  // Create a path of states from start_state to stop state with transitions
  // labeled with subsequent bytes of the string s.
  // States start_state and stop_state are already created.
  // 
  int create_utf_path(const int start_state, const char *str,
		       const int stop_state) {
    int s1 = start_state;
    int s2 = stop_state;
    int u8t = utf8t(*str);
    int l = (u8t == ONEBYTE ? 1
	     : (u8t == START2 ? 2
		: (u8t == START3 ? 3
		   : (u8t == START4 ? 4 : -1))));
    if (l == -1) {
      std::cerr << "fsa_synth: Invalid UTF8 character\n";
      exit(5);
    }
    const char *s = str;
    while (--l > 0) {
      if ((s2 = delta(s1, *s)) == -1) {
	// Prefix bytes not common with some other character
	s2 = create_state();
	create_transition(s1, *s, s2, regular);
      }
      // Else prefix bytes already used by some other character
      s1 = s2;
      s++;
    }
    if ((s2 = delta(s1, *s)) == -1) {
      create_transition(s1, *s, stop_state, regular);
    }
    else {
      // Transition already there - this should not happen in Thompson constr.
      std::cerr << "Utf8 transition already present!\n";
    }
    return ++s - str;
  }//create_utf_path
#endif

public:
  void set_final(const int state_number) { states[state_number].final = true; }
  nfa_type(void) { free_trans = -1; }
};/*nfa_type*/

struct dfa_state {
  int	first_trans;		/* first transition number */
  int	number_trans;
  bool	final;
};

class dfa_transition {
 public:
  int	target;			/* target state number */
  char	label;

  dfa_transition(const char l, const int t) { label = l; target = t; }
};

class dfa_type {
 public:
  std::vector<dfa_state>	states;
  std::vector<dfa_transition>	transitions;
  int				start_state;

  int get_start_state(void) { return start_state; }

  /* It is assumed that transitions are grouped by source state */
  void create_transition(const int source, const char label, const int target){
    if (states[source].first_trans == -1) {
      /* First transition of the state */
      states[source].first_trans = transitions.size();
      dfa_transition t(label, target);
      transitions.push_back(t);
      states[source].number_trans = 1;
    }
    else if (states[source].number_trans + states[source].first_trans
	     != int(transitions.size())) {
      std::cerr << "fsa_synth: error in determinization\n";
      exit(6);
    }
    else {
      dfa_transition t(label, target);
      transitions.push_back(t);
      states[source].number_trans++;
    }
  }//create_transition

  int delta(const int source, const char label) {
    int t = states[source].first_trans;
    /* Note that the meta transition is always the last one */
    for (int i = 0; i < states[source].number_trans; i++) {
      if (transitions[t].label == label || transitions[t].label == '\0') {
	return transitions[t].target;
      }
      t++;
    }
    return -1;
  }

  bool is_final(const int state_no) { return states[state_no].final; }

  void reset(void) { states.clear(); transitions.clear(); start_state = 0; }
};//dfa_type

class synth_fsa : public fsa {
protected:
#ifdef MORPH_INFIX
  int	morph_infixes;		/* TRUE if -I flag used */
  int	morph_prefixes;		/* TRUE if -P flag used */
  char	*prefix;		/* prefix or infix */
  char	*word_tags;		/* tags associated with a word form */
  int	pref_alloc;		/* memory allocated for prefix or infix */
  int	tag_alloc;		/* memory allocated for tags */
  int	tag_length;		/* length of tag */
#endif /*MORPH_INFIX*/
  int	ignore_filler;		/* TRUE if -F flag used */
  int	useREs;		/* TRUE if tags are regular expressions */
  int	gen_all_forms;	/* TRUE if all surface form have to be generated */
  dfa_type dfa;
public:
  /* Initialize dictionaries */
#ifdef MORPH_INFIX
  synth_fsa(const int ignorefiller, const int file_has_infixes,
	    const int file_has_prefixes, const int use_reg_expr,
	    const int gen_all,
	    word_list *dict_names, const char *language_file);
#else
  synth_fsa(const int ignorefiller, const int use_reg_expr,
	    const int gen_all,
	    word_list *dict_names, const char *language_file);
#endif /*MORPH_INFIX*/

  virtual ~synth_fsa(void) {}

  /* Perform generation of forms specified on input */
  int synth_file(tr_io &io_obj);

  /* Generate forms from one description */
  int synth_word(const char *word, const char *tags);

#if defined(FLEXIBLE) && defined(STOPBIT) && defined(SPARSE)
  int sparse_synth_next_char(const char *word, const int level,
			     const long start, const char *tags);
#else
  int synth_next_char(const char *word, const int level, fsa_arc_ptr start,
		      const char *tags);
#endif
#ifdef MORPH_INFIX
  int synth_infix(const int level, const int inflen, fsa_arc_ptr start);
  int synth_prefix(const int delete_position, const int level,
		   fsa_arc_ptr start);
  int synth_stem(const int delete_length, const int delete_position,
		 const int level, fsa_arc_ptr start);
#else
  int synth_stem(const int level, fsa_arc_ptr start);
#endif
  int synth_tags(const int level, fsa_arc_ptr start, const char *tags);
  int skip_tags(const int level, fsa_arc_ptr start);
  int synth_RE(const int level, fsa_arc_ptr start, const int dstate);
  int synth_rest(const int level, fsa_arc_ptr start);
  int buildRE(const char *re, int i, nfa_type &nfa);

  // Build (not necessarilly minimal) deterministic finite-state automaton (DFA)
  // from a regular expression contained in the string re
  void buildDFA(const char *re);

  /* Name:	determinize
   * Class:	synth_fsa
   * Purpose:	Determinizes a nondeterministic automaton (NFA).
   * Parameters:	nfa	- (i/o) NFA to be determinized.
   * Returns:	Nothing.
   * Remarks:	The DFA is a class variable.
   *		To speed up processing, nfa can be destroyed.
   */
  // Determinize an NFA. The resulting DFA is a class variable
  void determinize(nfa_type &nfa);

};/*class synth_fsa*/


/***	EOF synth.fsa	***/
