;; jspell.el --- spell checking using finite state automata

;; Copyright (C) 1994, 1995, 1997 Free Software Foundation, Inc.

;; Authors         : Ken Stevens <k.stevens@ieee.org>
;;                 : Modified by Jan Daciuk to be used with fsa programs.
;; Last Modified On: April 1997
;; Update Revision : 
;; Syntax          : emacs-lisp
;; Status	   : Release with version 0.7 of fsa
;; Version	   : 
;; Bug Reports	   : jandac@pg.gda.pl

;; This file is intended to be used with GNU Emacs.

;; GNU Emacs is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; GNU Emacs is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs; see the file COPYING.  If not, write to the
;; Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.

;;; Commentary:

;; INSTRUCTIONS
;;
;;  This code contains a section of user-settable variables that you should
;; inspect prior to installation.  Look past the end of the history list.
;; Set them up for your locale and the preferences of the majority of the
;; users.  Otherwise the users may need to set a number of variables
;; themselves.
;;  You particularly may want to change the default dictionary for your
;; country and language.
;;
;;
;; To fully install this, add this file to your Emacs Lisp directory and
;; compile it with M-X byte-compile-file.  Then add the following to the
;; appropriate init file:
;;
;;  (autoload 'jspell-word "jspell"
;;    "Check the spelling of word in buffer." t)
;;  (autoload 'jaccent-word "jspell"
;;    "Restore diacritics for word in buffer." t)
;;  (global-set-key "\e$" 'jspell-word)
;;  (autoload 'jspell-region "jspell"
;;    "Check the spelling of region." t)
;;  (autoload 'jaccent-region "jspell"
;;    "Restore diacritics in region." t)
;;  (autoload 'jspell-buffer "jspell"
;;    "Check the spelling of buffer." t)
;;  (autoload 'jaccent-buffer "jspell"
;;    "Restore diacritics in buffer." t)
;;  (autoload 'jspell-change-dictionary "jspell"
;;    "Change jspell dictionary." t)
;;  (autoload 'jspell-message "jspell"
;;    "Check spelling of mail message or news post.")
;;
;;  This one installes menus:
;;
;;  (define-key-after
;;    (lookup-key global-map [menu-bar edit])
;;    [jspell] '("Jspell" . jspell-menu-map) 'ispell)
;;
;;  Depending on the mail system you use, you may want to include these:
;;
;;  (add-hook 'news-inews-hook 'jspell-message)
;;  (add-hook 'mail-send-hook  'jspell-message)
;;  (add-hook 'mh-before-send-letter-hook 'jspell-message)
;;
;;
;; Jspell has a nroff parser (the default). Currently, there is no TeX parser,
;; as fsa utilities do not have it.
;; The parsing is controlled by the variable jspell-parser.  Currently
;; it is just a "toggle" between TeX and nroff, but if more parsers are
;; added it will be updated.  See the variable description for more info.
;;
;;
;; TABLE OF CONTENTS
;;
;;   jspell-word
;;   jaccent-word
;;   jspell-region
;;   jaccent-region
;;   jspell-buffer
;;   jaccent-buffer
;;   jspell-message
;;   jspell-continue
;;   jspell-complete-word
;;   jspell-change-dictionary
;;   jspell-kill-jspell
;;   jspell-pdict-save
;;
;;
;; Commands in jspell-region:
;; Character replacement: Replace word with choice.  May query-replace.
;; ' ': Accept word this time.
;; 'i': Accept word and insert into private dictionary.
;; 'a': Accept word for this session.
;; 'A': Accept word and place in buffer-local dictionary.
;; 'r': Replace word with typed-in value.  Rechecked.
;; 'R': Replace word with typed-in value. Query-replaced in buffer. Rechecked.
;; '?': Show these commands
;; 'x': Exit spelling buffer.  Move cursor to original point.
;; 'X': Exit spelling buffer.  Leave cursor at the current point.
;; 'q': Quit spelling session (Kills jspell process).
;; 'l': Look up typed-in replacement in alternate dictionary.  Wildcards okay.
;; 'u': Like 'i', but the word is lower-cased first.
;; 'm': Like 'i', but allows one to include dictionary completion info.
;; 'C-l': redraws screen
;; 'C-r': recursive edit
;; 'C-z': suspend emacs or iconify frame
;;
;; Buffer-Local features:
;; There are a number of buffer-local features that can be used to customize
;;  jspell for the current buffer.  This includes language dictionaries,
;;  personal dictionaries, parsing, and local word spellings.  Each of these
;;  local customizations are done either through local variables, or by
;;  including the keyword and argument(s) at the end of the buffer (usually
;;  prefixed by the comment characters).  See the end of this file for
;;  examples.  The local keywords and variables are:
;;
;;  jspell-dictionary-keyword   language-dictionary
;;      uses local variable jspell-local-dictionary
;;  jspell-pdict-keyword        personal-dictionary
;;      uses local variable jspell-local-pdict
;;  jspell-parsing-keyword      mode-arg extended-char-arg
;;  jspell-words-keyword        any number of local word spellings
;;
;;
;; BUGS:
;;  Highlighting in version 19 still doesn't work on tty's.
;;  On some versions of emacs, growing the minibuffer fails.
;;  TeX mode does not word, as fsa utilities do not have TeX parser.
;;
;; HISTORY
;;
;; Adaptation for jspell, jandac@pg.gda.pl, March 1997
;;
;; Revision 2.38  1996/5/30	ethanb@phys.washington.edu
;; Update ispell-message for gnus 5 (news-inews-hook => message-send-hook;
;; different header for quoted message).
;;
;; Revision 2.37  1995/6/13 12:05:28	stevens
;; Removed autoload from ispell-dictionary-alist. *choices* mode-line shows
;; misspelled word.  Block skip for pgp & forwarded messages added.
;; RMS: the autoload changes had problems and I removed them.
;;
;; Revision 2.36  1995/2/6 17:39:38	stevens
;; Properly adjust screen with different ispell-choices-win-default-height
;; settings.  Skips SGML entity references.
;;
;; Revision 2.35  1995/1/13 14:16:46	stevens
;; Skips SGML tags, ispell-change-dictionary fix for add-hook, assure personal
;; dictionary is saved when called from the menu
;;
;; Revision 2.34  1994/12/08 13:17:41  stevens
;; Interaction corrected to function with all 3.1 ispell versions.
;;
;; Revision 2.33  1994/11/24 02:31:20  stevens
;; Repaired bug introduced in 2.32 that corrupts buffers when correcting.
;; Improved buffer scrolling. Nondestructive buffer selections allowed.
;;
;; Revision 2.32  1994/10/31 21:10:08  geoff
;; Many revisions accepted from RMS/FSF.  I think (though I don't know) that
;; this represents an 'official' version.
;;
;; Revision 2.31  1994/5/31 10:18:17  stevens
;; Repaired comments.  buffer-local commands executed in `ispell-word' now.
;; German dictionary described for extended character mode.  Dict messages.
;;
;; Revision 2.30  1994/5/20 22:18:36  stevens
;; Continue ispell from ispell-word, C-z functionality fixed.
;;
;; Revision 2.29  1994/5/12 09:44:33  stevens
;; Restored ispell-use-ptys-p, ispell-message aborts sends with interrupt.
;; defined fn ispell
;;
;; Revision 2.28  1994/4/28 16:24:40  stevens
;; Window checking when ispell-message put on gnus-inews-article-hook jwz.
;; prefixed ispell- to highlight functions and horiz-scroll fn.
;; Try and respect case of word in ispell-complete-word.
;; Ignore non-char events.  Ispell-use-ptys-p commented out. Lucid menu.
;; Better interrupt handling.  ispell-message improvements from Ethan.
;;
;; Revision 2.27
;; version 18 explicit C-g handling disabled as it didn't work. Added
;; ispell-extra-args for ispell customization (jwz)
;;
;; Revision 2.26  1994/2/15 16:11:14  stevens
;; name changes for copyright assignment.  Added word-frags in complete-word.
;; Horizontal scroll (John Conover). Query-replace matches words now.  bugs.
;;
;; Revision 2.25
;; minor mods, upgraded ispell-message
;;
;; Revision 2.24
;; query-replace more robust, messages, defaults, ispell-change-dict.
;;
;; Revision 2.23  1993/11/22 23:47:03  stevens
;; ispell-message, Fixed highlighting, added menu-bar, fixed ispell-help, ...
;;
;; Revision 2.22
;; Added 'u' command.  Fixed default in ispell-local-dictionary.
;; fixed affix rules display.  Tib skipping more robust.  Contributions by
;; Per Abraham (parser selection), Denis Howe, and Eberhard Mattes.
;;
;; Revision 2.21  1993/06/30 14:09:04  stevens
;; minor bugs. (nroff word skipping fixed)
;;
;; Revision 2.20  1993/06/30 14:09:04  stevens
;;
;; Debugging and contributions by: Boris Aronov, Rik Faith, Chris Moore,
;;  Kevin Rodgers, Malcolm Davis.
;; Particular thanks to Michael Lipp, Jamie Zawinski, Phil Queinnec
;;  and John Heidemann for suggestions and code.
;; Major update including many tweaks.
;; Many changes were integrations of suggestions.
;; lookup-words rehacked to use call-process (Jamie).
;; ispell-complete-word rehacked to be compatible with the rest of the
;; system for word searching and to include multiple wildcards,
;; and it's own dictionary.
;; query-replace capability added.  New options 'X', 'R', and 'A'.
;; buffer-local modes for dictionary, word-spelling, and formatter-parsing.
;; Many random bugs, like commented comments being skipped, fix to
;; keep-choices-win, fix for math mode, added pipe mode choice,
;; fixed 'q' command, ispell-word checks previous word and leave cursor
;; in same location.  Fixed tib code which could drop spelling regions.
;; Cleaned up setq calls for efficiency. Gave more context on window overlays.
;; Assure context on ispell-command-loop.  Window lossage in look cmd fixed.
;; Due to pervasive opinion, common-lisp package syntax removed. Display
;; problem when not highlighting.
;;
;; Revision 2.19  1992/01/10  10:54:08  geoff
;; Make another attempt at fixing the "Bogus, dude" problem.  This one is
;; less elegant, but has the advantage of working.
;;
;; Revision 2.18  1992/01/07  10:04:52  geoff
;; Fix the "Bogus, Dude" problem in ispell-word.
;;
;; Revision 2.17  1991/09/12  00:01:42  geoff
;; Add some changes to make ispell-complete-word work better, though
;; still not perfectly.
;;
;; Revision 2.16  91/09/04  18:00:52  geoff
;; More updates from Sebastian, to make the multiple-dictionary support
;; more flexible.
;;
;; Revision 2.15  91/09/04  17:30:02  geoff
;; Sebastian Kremer's tib support
;;
;; Revision 2.14  91/09/04  16:19:37  geoff
;; Don't do set-window-start if the move-to-window-line moved us
;; downward, rather than upward.  This prevents getting the buffer all
;; confused.  Also, don't use the "not-modified" function to clear the
;; modification flag;  instead use set-buffer-modified-p.  This prevents
;; extra messages from flashing.
;;
;; Revision 2.13  91/09/04  14:35:41  geoff
;; Fix a spelling error in a comment.  Add code to handshake with the
;; ispell process before sending anything to it.
;;
;; Revision 2.12  91/09/03  20:14:21  geoff
;; Add Sebastian Kremer's multiple-language support.
;;
;;
;; Walt Buehring
;; Texas Instruments - Computer Science Center
;; ARPA:  Buehring%TI-CSL@CSNet-Relay
;; UUCP:  {smu, texsun, im4u, rice} ! ti-csl ! buehring
;;
;; ispell-region and associated routines added by
;; Perry Smith
;; pedz@bobkat
;; Tue Jan 13 20:18:02 CST 1987
;;
;; extensively modified by Mark Davies and Andrew Vignaux
;; {mark,andrew}@vuwcomp
;; Sun May 10 11:45:04 NZST 1987
;;
;; Ken Stevens  ARPA: k.stevens@ieee.org
;; Tue Jan  3 16:59:07 PST 1989
;; This file has overgone a major overhaul to be compatible with ispell
;; version 2.1.  Most of the functions have been totally rewritten, and
;; many user-accessible variables have been added.  The syntax table has
;; been removed since it didn't work properly anyway, and a filter is
;; used rather than a buffer.  Regular expressions are used based on
;; ispell's internal definition of characters (see ispell(4)).
;; Some new updates:
;; - Updated to version 3.0 to include terse processing.
;; - Added a variable for the look command.
;; - Fixed a bug in ispell-word when cursor is far away from the word
;;   that is to be checked.
;; - Ispell places the incorrect word or guess in the minibuffer now.
;; - fixed a bug with 'l' option when multiple windows are on the screen.
;; - lookup-words just didn't work with the process filter.  Fixed.
;; - Rewrote the process filter to make it cleaner and more robust
;;   in the event of a continued line not being completed.
;; - Made ispell-init-process more robust in handling errors.
;; - Fixed bug in continuation location after a region has been modified by
;;   correcting a misspelling.
;; Mon 17 Sept 1990
;;
;; Sebastian Kremer <sk@thp.uni-koeln.de>
;; Wed Aug  7 14:02:17 MET DST 1991
;; - Ported ispell-complete-word from Ispell 2 to Ispell 3.
;; - Added ispell-kill-ispell command.
;; - Added ispell-dictionary and ispell-dictionary-alist variables to
;;   support other than default language.  See their docstrings and
;;   command ispell-change-dictionary.
;; - (ispelled it :-)
;; - Added ispell-skip-tib variable to support the tib bibliography
;;   program.

;; Jan Daciuk <jandac@pg.gda.pl> April 1997:
;; - modified for use with fsa utilities.
;;
;; Jan Daciuk <jandac@pg.gda.pl> June 1997:
;; - modified to include morphology.
;;


;; **********************************************************************
;; The following variables should be set according to personal preference
;; and location of binaries:
;; **********************************************************************


;;; Code:

(defvar jspell-debug-var nil
  "*Holds debugging info.") ;; jd

(defvar guessing-available t
  "*If true, jmorph tries to perform approximate morphological analysis
based on word endings. This option needs fsa_guess program.")

(defvar jspell-doing-morphology nil
  "True if morphological analysis is performed.
Affects the format of corrections.")

(defvar jmorph-format "<WORD LEXEM=\"%s\" TAG=\"%s\">%s</WORD>"
  "*Format of morphotactic annotations. The meaning of the 3 strings
is determined by jmorph-order.")

(defvar jmorph-order '(2 3 1)
  "*Order of appearence of the inflected word, lexeme,
and tag in jmorph-format.")

(defvar jspell-morph-sep "+"
  "*Character that separates lexemes form tags in output of morphological
analysis.")

(defvar jspell-highlight-p t
  "*Highlight spelling errors when non-nil.")

(defvar jspell-highlight-face 'highlight
  "*The face used for Jspell highlighting.  For Emacses with overlays.
Possible values are `highlight', `modeline', `secondary-selection',
`region', and `underline'.
This variable can be set by the user to whatever face they desire.
It's most convenient if the cursor color and highlight color are
slightly different.")

(defvar jspell-check-comments t
  "*If nil, don't check spelling of comments.")

(defvar jspell-query-replace-choices nil
  "*Corrections made throughout region when non-nil.
Uses `query-replace' (\\[query-replace]) for corrections.")

(defvar jspell-skip-tib nil
  "*Does not spell check `tib' bibliography references when non-nil.
Skips any text between strings matching regular expressions
`jspell-tib-ref-beginning' and `jspell-tib-ref-end'.

TeX users beware:  Any field starting with [. will skip until a .] -- even
your whole buffer -- unless you set `jspell-skip-tib' to nil.  That includes
a [.5mm] type of number....")

(defvar jspell-tib-ref-beginning "[[<]\\."
  "Regexp matching the beginning of a Tib reference.")

(defvar jspell-tib-ref-end "\\.[]>]"
  "Regexp matching the end of a Tib reference.")

(defvar jspell-keep-choices-win t
  "*When not nil, the `*Choices*' window remains for spelling session.
This minimizes redisplay thrashing.")

(defvar jspell-choices-win-default-height 2
  "*The default size of the `*Choices*' window, including status line.
Must be greater than 1.")

(defvar jspell-program-name "jspell"
  "Program invoked by \\[jspell-word] and \\[jspell-region] commands.")

(defvar jspell-accent-program "jaccent"
  "Program invoked by \\[jaccent-word] and \\[jaccent-region] commands.")

(defvar jspell-spelling-program "jspell"
  "Program invoked by \\[jspell-word] and \\[jspell-region] commands.")

(defvar jspell-morphing-program "jmorph"
  "Program invoked by \\[jmorph-word] and \\[jmorph-region] commands.")

(defvar jspell-guessing-program "jguess"
  "Program for approximate morphological analysis based on endings.")

(defvar jspell-alternate-dictionary
  (cond ((file-exists-p "/usr/dict/web2") "/usr/dict/web2")
	((file-exists-p "/usr/share/dict/web2") "/usr/share/dict/web2")
	((file-exists-p "/usr/dict/words") "/usr/dict/words")
	((file-exists-p "/usr/lib/dict/words") "/usr/lib/dict/words")
	((file-exists-p "/usr/share/dict/words") "/usr/share/dict/words")
	((file-exists-p "/sys/dict") "/sys/dict")
	(t "/usr/dict/words"))
  "*Alternate dictionary for spelling help.")

(defvar jspell-complete-word-dict jspell-alternate-dictionary
  "*Dictionary used for word completion.")

(defvar jspell-grep-command "egrep"
  "Name of the grep command for search processes.")

(defvar jspell-grep-options "-i"
  "String of options to use when running the program in `jspell-grep-command'.
Should probably be \"-i\" or \"-e\".
Some machines (like the NeXT) don't support \"-i\"")

(defvar jspell-look-command "look"
  "Name of the look command for search processes.
This must be an absolute file name.")

(defvar jspell-look-p (file-exists-p jspell-look-command)
  "*Non-nil means use `look' rather than `grep'.
Default is based on whether `look' seems to be available.")

(defvar jspell-have-new-look nil
  "*Non-nil means use the `-r' option (regexp) when running `look'.")

(defvar jspell-look-options (if jspell-have-new-look "-dfr" "-df")
  "String of command options for `jspell-look-command'.")

(defvar jspell-use-ptys-p nil
  "When non-nil, Emacs uses ptys to communicate with Jspell.
When nil, Emacs uses pipes.")

(defvar jspell-following-word nil
  "*Non-nil means `jspell-word' checks the word around or after point.
Otherwise `jspell-word' checks the preceding word.")

(defvar jspell-help-in-bufferp nil
  "*Non-nil means display interactive keymap help in a buffer.
Otherwise use the minibuffer.")

(defvar jspell-quietly nil
  "*Non-nil means suppress messages in `jspell-word'.")

(defvar jaccent-automatically t
  "*Non-nil means diacritics are added automatically
when there is only one choice.")

(defvar jmorph-automatically t
  "*Non-nil means morphotactic annotations are added automatically
(i.e. without asking) when there is only one choice.")

(defvar jspell-format-word (function upcase)
  "*Formatting function for displaying word being spell checked.
The function must take one string argument and return a string.")

;;;###autoload
(defvar jspell-personal-dictionary nil
  "*File name of your personal spelling dictionary, or nil.
If nil, the default personal dictionary, \"~/.jspell_DICTNAME\" is used,
where DICTNAME is the name of your default dictionary.")

(defvar jspell-private-dictionary "~/.jspell-words"
  "*The file that is used to store words with `I' command.")

(defvar jmorph-private-dictionary "~/.jmorph-words"
  "*The file that stores results of approximate morphological analysis.")

(defvar jspell-silently-savep nil
  "*When non-nil, save the personal dictionary without confirmation.")

;;; This variable contains the current dictionary being used if the jspell
;;; process is running.  Otherwise it contains the global default.
(defvar jspell-dictionary nil
  "If non-nil, a dictionary to use instead of the default one.
This is passed to the jspell process using the `-d' switch and is
used as key in `jspell-dictionary-alist' (which see).

You should set this variable before your first use of Emacs spell-checking
commands in the Emacs session, or else use the \\[jspell-change-dictionary]
command to change it.  Otherwise, this variable only takes effect in a newly
started Jspell process.")

(defvar jspell-extra-args nil
  "*If non-nil, a list of extra switches to pass to the Jspell program.
For example, '(\"-W\" \"3\") to cause it to accept all 1-3 character
words as correct.  See also `jspell-dictionary-alist', which may be used
for language-specific arguments.")

;;; The preparation of the menu bar menu must be autoloaded
;;; because otherwise this file gets autoloaded every time Emacs starts
;;; so that it can set up the menus and determine keyboard equivalents.

;;;###autoload
(defvar jspell-dictionary-alist-1	
  '((nil "english.fsa"			; default (english.fsa)
     "A-Za-z"				; case chars
     "[']"				; other chars
     nil				; many-othercars-p
     nil				; common additional parameters
     nil				; spell-specific parameters
     nil				; accent-specific parameters
     ("-d" "english.fsm")		; morph-specific parameters
     ("-d" "english.atp")		; guess-specific parameters
     nil				; accented character (binary)
     nil)				; accented characters (TeX notation)

    ("english" "english.fsa"		; make english explicitly selectable
     "A-Za-z"				; case chars
     "[']"				; other chars
     nil				; many-otherchars-p
     nil				; common additional parameters
     nil				; spell-specific parameters
     nil				; accent-specific parameters
     ("-d" "english.fsm")		; morph-specific parameters
     ("-d" "english.atp")		; guess-specific parameters
     nil				; accented characters (binary)
     nil)				; accented characters (TeX notation)

    ("british" "british.fsa"		; british version
     "A-Za-z"				; case chars
     "[']"				; other chars
     nil				; many-otherchars-p
     nil				; common additional parameters
     nil				; spell-specific parameters
     nil				; accent-specific parameters
     ("-d" "british.fsm")		; morph-specific parameters
     ("-d" "british.atp")		; guess-specific parameters
     nil				; accented characters (binary)
     nil)				; accented characters (TeX notation)

;    ("deutsch-TeX" "deutsch.fsa"		; TeX mode
;     "a-zA-Z\"" "[']" t ("-l" "de.lang") "de.acc"
;     ("-d" "deutch.fsm") ("-d" "deutsch.atp")
;     ("\304" "\326" "\334" "\344" "\366" "\337" "\374")
;     ; A umlaut, O umlaut, U umlaut, a umlaut, o umlaut, scharfes s, u umlaut
;     ("\\\"A" "\\\"O" "\\\"U" "\\\"a" "\\\"o" "{\\ss}" "\\\"u"))
    ("deutsch" "deutsch.fsa"		; german
     "a-zA-Z\304\326\334\344\366\337\374" ; case chars
     "[']"				; other chars
     t					; many-otherchars-p
     ("-l" "de.lang")			; common additional parameters
     nil				; spell-specific parameters
     ("-a" "de.acc")			; accent-specific parameters
     ("-d" "deutsch.fsm")		; morph-specific parameters
     ("-d" "deutsch.atp")		; guess-specific parameters
     nil				; accented characters (binary)
     nil)				; accented characters (TeX notation)

;    ("nederlands-TeX" "nederlands.fsa"	; TeX mode
;     "A-Za-z" "[']" t nil "nl.acc" nil nil
;     ; this is ridiculous - they cannot use ALL of them!!! (jd)
;     ("\300" "\301" "\302" "\303" "\304" "\305" "\307" "\310"
;      "\311" "\312" "\313" "\314" "\315" "\316" "\317" "\322"
;      "\323" "\324" "\325" "\326" "\331" "\332" "\333" "\334"
;      "\340" "\341" "\342" "\343" "\344" "\345" "\347" "\350"
;      "\351" "\352" "\353" "\354" "\355" "\356" "\357" "\361"
;      "\362" "\363" "\364" "\365" "\366" "\371" "\372" "\373"
;      "\374")
;     ("\\`A" "\\'A" "\\^A" "\\~A" "\\\"A" "\\AA" "\\c{C}" "\\`E"
;      "\\'E" "\\^E" "\\\"E" "\\`I" "\\'I" "\\^I" "\\\"I" "\\`O"
;      "\\'O" "\\^O" "\\~O" "\\\"O" "\\`U" "\\'U" "\\^U" "\\\"U"
;      "\\`a" "\\'a" "\\^a" "\\~a" "\\\"a" "\\aa" "\\c{c}" "\\`e"
;      "\\'e" "\\^e" "\\\"e" "\\`{\\i}" "\\'{\\i}" "\\^{\\i}" "\\\"{\\i}" "\\~n"
;      "\\`o" "\\'o" "\\^o" "\\~o" "\\\"o" "\\`u" "\\'u" "\\^u"
;      "\\\"u"))
    ("nederlands" "nederlands.fsa"	; dutch
     "A-Za-z\300-\305\307\310-\317\322-\326\331-\334\340-\345\347\350-\357\361\362-\366\371-\374"
     "[']"				; other chars
     t					; many-otherchars-p
     nil				; common additional parameters
     nil				; spell-specific parameters
     ("-a" "ne.acc")			; accent-specific parameters
     ("-d" "nederlands.fsm")		; morph-specific parameters
     ("-d" "nederlands.atp")		; guess-specific parameters
     nil				; accented characters (binary)
     nil)))				; accented characters (TeX notation)

;;;###autoload
(defvar jspell-dictionary-alist-2
  '(("svenska" "svenska.fsa"		; swedish
     "A-Za-z}{|\\133\\135\\\\"		; case chars
     "[']"				; other chars
     nil				; many-otherchars-p
     nil				; common additional parameters
     nil				; spell-specific parameters
     ("-a" "s.acc")			; accent-specific parameters
     ("-d" "svenska.fsm")		; morph-specific parameters
     ("-d" "svenska.atp")		; guess-specific parameters
     nil				; accented characters (binary)
     nil)				; accented characters (TeX notation)

;    ("svenska-TeX" "svenska.fsa"	; TeX mode
;     "A-Za-z\345\344\366\305\304\366"
;     "[']" nil nil "se.acc" nil nil
;     ("\345" "\344" "\366" "\305" "\304" "\366")
;     ("\\å" "\\\"a" "\\\"o" "\\Å" "\\\"A" "\\\"o"))
;    ("francais-TeX" "francais.fsa"	; TeX mode
;     "A-Za-z" "[---'^`\"]" t nil "fr.acc" nil nil
;     ("\300" "\302" "\306" "\307" "\310" "\311" "\312"
;      "\313" "\316" "\317" "\324" "\331" "\333" "\334"
;      "\340" "\342" "\347" "\350" "\351" "\352" "\353"
;      "\356" "\357" "\364" "\371" "\373" "\374")
;     ("\\`A" "\\^A" "\\Æ" "\\c{C}" "\\`E" "\\'E" "\\^E"
;      "\\\"E" "\\^I" "\\\"I" "\\^O" "\\`U" "\\^U" "\\\"U"
;      "\\`a" "\\^a" "\\c{c}" "\\`e" "\\'e" "\\^e" "\\\"e"
;      "\\^{\i}" "\\\"{\i}" "\\^o" "\\`u" "\\^u" "\\\"u"))
    ("francais" "francais.fsa"		; french
     "A-Za-z\300\302\306\307\310\311\312\313\316\317\324\331\333\334\340\342\347\350\351\352\353\356\357\364\371\373\374"
     "[---']"				; other chars
     t					; many-otherchars-p
     ("-l" "fr.lang")			; common additional parameters
     nil				; spell-specific parameters
     ("-a" "fr.acc")			; accent-specific parameters
     ("-d" "francais.fsm")		; morph-specific parameters
     ("-d" "francais.atp")		; guess-specific parameters
     nil				; accented characters (binary)
     nil)				; accented characters (TeX notation)

    ("dansk" "dansk.fsa"		; danish
     "A-Z\306\330\305a-z\346\370\345"	; case chars
     ""					; other chars
     nil				; many otherchars-p
     nil				; common additional parameters
     nil				; spell-specific parameters
     ("-a" "dk.acc")			; accent-specific parameters
     ("-d" "dansk.fsm")			; morph-specific parameters
     ("-d" "dansk.atp")			; guess-specific parameters
     nil				; accented characters (binary)
     nil)				; accented characters (TeX notation)

    ("polski" "polski.fsa"		; polish
     "A-Za-z\261\346\352\263\361\363\266\274\277\241\306\312\243\321\323\246\254\257"
     "[-]"				; other chars
     nil				; many-otherchars-p
     ("-l" "pl.lang")			; common additional parameters
     ("-r" "pl.chcl")			; spell-specific parameters
     ("-a" "pl.acc")			; accent-specific parameters
     ("-d" "polski.fsm")		; morph-specific parameters
     ("-d" "polski.atp")		; guess-specific parameters
     nil				; accented characters (binary)
     nil)				; accented characters (TeX notation)
    ))


;;; jspell-dictionary-alist is set up from two subvariables above
;;; to avoid having very long lines in loaddefs.el.
;;;###autoload
(defvar jspell-dictionary-alist
  (append jspell-dictionary-alist-1 jspell-dictionary-alist-2)
  "An alist of dictionaries and their associated parameters.

Each element of this list is also a list:

\(DICTIONARY-NAME DICTIONARY-FILE CASECHARS OTHERCHARS MANY-OTHERCHARS-P
        JSPELL-ARGS SPELL_SPECIFIC ACCENT_SPECIFIC
        MORPH-SPECIFIC GUESS-SPECIFIC
        ACCENTED-CHARS ESCAPED-CHARS\)

DICTIONARY-NAME is a possible value of variable `jspell-dictionary', nil
means the default dictionary.

DICTIONARY-FILE is the name of a file that holds the specified dictionary.
The same DICTIONARY-FILE may serve many DICTIONARY-NAME - the differences
may occur in in other parameters.

CASECHARS is a string of valid characters that comprise a word.

OTHERCHARS is a regular expression of other characters that are valid
in word constructs.  Otherchars cannot be adjacent to each other in a
word, nor can they begin or end a word.  This implies we can't check
\"Stevens'\" as a correct possessive and other correct formations.

Hint: regexp syntax requires the hyphen to be declared first here.

MANY-OTHERCHARS-P is non-nil if many otherchars are to be allowed in a
word instead of only one.

JSPELL-ARGS is a list of common additional arguments passed to all the
subprocesses.

ACCENT-FILE is the name of a file containing correspondances between
characters without diacritics, and those with them.

SPELL_SPECIFIC is a list of additional parameters used only in a spelling
process.

ACCENT-SPECIFIC is a list of parameters used only in a diacritic restoration
process.

MORPH-SPECIFIC is a list of parameters used only in morphological analysis.
This list includes a morphological dictionary; the default dictionary is
not passed to the morphology program.

GUESS-SPECIFIC is a list of parameters used only in approximate morphological
analysis. This list includes an a tergo lexicon of word endings with
morphological descriptions; the default dictionary is not passed
to the morphology guessing program.

ACCENTED-CHARS is a list of characters with diacritics. All words
in a dictionary have diacritics. If used in TeX mode, words from text
should be converted to 8 bit characters, checked against dictionary,
and converted back.

ESCAPED-CHARS is a list of strings that are TeX equivalents
of the corresponding 8 bit characters from ACCENTED-CHARS.
Note that it is possible to express the same character in TeX
in at least two ways, e.g. \\\"a and \\\"{a}. Use the method you prefer.")

;;;###autoload
(defvar jspell-menu-map nil "Key map for jspell menu")

;;;###autoload
(defvar jspell-menu-lucid nil "Spelling menu for Lucid Emacs.")

;;; Break out lucid menu and split into several calls to avoid having
;;; long lines in loaddefs.el.  Detect need off following constant.

;;;###autoload
(defconst jspell-menu-map-needed	; make sure this is not Lucid Emacs
  (and (not jspell-menu-map)
;;; This is commented out because it fails in Emacs.
;;; due to the fact that menu-bar is loaded much later than loaddefs.
;;;       ;; make sure this isn't Lucid Emacs
;;;       (featurep 'menu-bar)
       (not (string-match "Lucid" emacs-version))))

;;; Set up dictionary
;;;###autoload
(if jspell-menu-map-needed
    (let ((dicts (reverse (cons (cons "default" nil) jspell-dictionary-alist)))
	  name)
      (setq jspell-menu-map (make-sparse-keymap "Jspell"))
      ;; add the dictionaries to the bottom of the list.
      (while dicts
	(setq name (car (car dicts))
	      dicts (cdr dicts))
	(if (stringp name)
	    (define-key jspell-menu-map (vector (intern name))
	      (cons (concat "Select " (capitalize name))
		    (list 'lambda () '(interactive)
			  (list 'jspell-change-dictionary name))))))))

;;; define commands in menu in opposite order you want them to appear.
;;;###autoload
(if jspell-menu-map-needed
    (progn
      (define-key jspell-menu-map [jspell-change-dictionary]
	'("Change Dictionary" . jspell-change-dictionary))
      (define-key jspell-menu-map [jspell-kill-jspell]
	'("Kill Process" . jspell-kill-jspell))
      (define-key jspell-menu-map [jspell-pdict-save]
	'("Save Dictionary" . (lambda () (interactive) (jspell-pdict-save t t))))
      (define-key jspell-menu-map [jspell-complete-word]
	'("Complete Word" . jspell-complete-word))
      (define-key jspell-menu-map [jspell-complete-word-interior-frag]
	'("Complete Word Frag" . jspell-complete-word-interior-frag))))

;;;###autoload
(if jspell-menu-map-needed
    (progn
      (define-key jspell-menu-map [jspell-continue]
	'("Continue Check" . jspell-continue))
      (define-key jspell-menu-map [jspell-word]
	'("Check Word" . jspell-word))
      (define-key jspell-menu-map [jspell-region]
	'("Check Region" . jspell-region))
      (define-key jspell-menu-map [jspell-buffer]
	'("Check Buffer" . jspell-buffer))
      (define-key jspell-menu-map [jaccent-word]
	'("Add Accents to Word" . jaccent-word))
      (define-key jspell-menu-map [jaccent-region]
	'("Add Accents in Region" . jaccent-region))
      (define-key jspell-menu-map [jaccent-buffer]
	'("Add Accents in Buffer" . jaccent-buffer))
      (define-key jspell-menu-map [jmorph-word]
	'("Analyse Word Morphology" . jmorph-word))
      (define-key jspell-menu-map [jmorph-region]
	'("Analyse Morphology in Region" . jmorph-region))
      (define-key jspell-menu-map [jmorph-buffer]
	'("Analyse Morphology in Buffer" . jmorph-buffer))))

;;;###autoload
(if jspell-menu-map-needed
    (progn
      (define-key jspell-menu-map [jaccent-message]
	'("Add Accents in Message" . jaccent-message))
      (define-key jspell-menu-map [jspell-message]
	'("Check Message" . jspell-message))
      (define-key jspell-menu-map [jspell-help]
	;; use (x-popup-menu last-nonmenu-event(list "" jspell-help-list)) ?
	'("Help" . (lambda () (interactive) (describe-function 'jspell-help))))
      (put 'jspell-region 'menu-enable 'mark-active)
      (put 'jaccent-region 'menu-enable 'mark-active)
      (put 'jmorph-region 'menu-enable 'mark-active)
      (fset 'jspell-menu-map (symbol-value 'jspell-menu-map))))

;;; Xemacs version 19
(if (and (string-lessp "19" emacs-version)
	 (string-match "Lucid" emacs-version))
    (let ((dicts (cons (cons "default" nil) jspell-dictionary-alist))
	  (current-menubar (or current-menubar default-menubar))
	  (menu
	   '(["Help"		(describe-function 'jspell-help) t]
	     ;;["Help"		(popup-menu jspell-help-list)	t]
	     ["Check Message"	jspell-message			t]
	     ["Check Buffer"	jspell-buffer			t]
	     ["Check Word"	jspell-word			t]
	     ["Check Region"	jspell-region  (or (not zmacs-regions) (mark))]
	     ["Continue Check"	jspell-continue			t]
	     ["Add Accents in Message" jaccent-message          t]
	     ["Add Accents in Buffer" jaccent-buffer            t]
	     ["Add Accents in Region" jaccent-region (or (not zmacs-regions)
							 (mark))]
	     ["Analyse Word Morphology" jmorph-word             t]
	     ["Analyse Morphology in Region" jmorph-region
	                       (or (not zmacs-regions) (mark))  t]
	     ["Analyse Morphology in Buffer" jmorph-buffer      t]
	     ["Complete Word Frag"jspell-complete-word-interior-frag t]
	     ["Complete Word"	jspell-complete-word		t]
	     ["Kill Process"	jspell-kill-jspell		t]
	     "-"
	     ["Save Dictionary"	(jspell-pdict-save t t)		t]
	     ["Change Dictionary" jspell-change-dictionary	t]))
	  name)
      (while dicts
	(setq name (car (car dicts))
	      dicts (cdr dicts))
	(if (stringp name)
	    (setq menu (append menu
			       (list
				(vector (concat "Select " (capitalize name))
					(list 'jspell-change-dictionary name)
					t))))))
      (setq jspell-menu-lucid menu)
      (if current-menubar
	  (progn
	    (delete-menu-item '("Edit" "Spell")) ; in case already defined
	    (add-menu '("Edit") "Spell" jspell-menu-lucid)))))


;;; **********************************************************************
;;; The following are used by jspell, and should not be changed.
;;; **********************************************************************


(defvar jspell-tex-alist nil
  "Alist holding 8 bit accented characters and their tex-encoded
counterparts.")

(defun jspell-get-dictfile ()
  (nth 1 (assoc jspell-dictionary jspell-dictionary-alist)))
(defun jspell-get-casechars ()
  (concat "[" (nth 2 (assoc jspell-dictionary jspell-dictionary-alist)) "]" ))
(defun jspell-get-not-casechars ()
  (concat "[^" (nth 2 (assoc jspell-dictionary jspell-dictionary-alist)) "]" ))
(defun jspell-get-otherchars ()
  (nth 3 (assoc jspell-dictionary jspell-dictionary-alist)))
(defun jspell-get-many-otherchars-p ()
  (nth 4 (assoc jspell-dictionary jspell-dictionary-alist)))
(defun jspell-get-jspell-args ()
  (nth 5 (assoc jspell-dictionary jspell-dictionary-alist)))
(defun jspell-get-spell-specific ()
  (nth 6 (assoc jspell-dictionary jspell-dictionary-alist)))
(defun jspell-get-accent-specific ()
  (nth 7 (assoc jspell-dictionary jspell-dictionary-alist)))
(defun jspell-get-morph-specific ()
  (nth 8 (assoc jspell-dictionary jspell-dictionary-alist)))
(defun jspell-get-guess-specific ()
  (nth 9 (assoc jspell-dictionary jspell-dictionary-alist)))
(defun jspell-get-accented-chars ()
  (nth 10 (assoc jspell-dictionary jspell-dictionary-alist)))
(defun jspell-get-escaped-chars ()
  (nth 11 (assoc jspell-dictionary jspell-dictionary-alist)))


;;; This function may be used to prepare data for the conversion from
;;; 8 bit characters to TeX notation.
(defun make-tex-alist ()
  "Prepare assoc list for matching 8 bit characters with their TeX
equivalents."
  (let ((alist nil)
	(accented-chars (jspell-get-accented-chars))
	(tex-chars (jspell-get-escaped-chars)))
    (while (and accented-chars tex-chars)
      (setq alist (cons (cons (car accented-chars) (car tex-chars)) alist))
      (setq accented-chars (cdr accented-chars) tex-chars (cdr tex-chars)))
    (setq jspell-tex-alist (reverse alist))))

(defvar jspell-process nil
  "The process object for Jspell.")

(defvar jspell-aux-process nil
  "The process object for guessing process.")

(defvar current-program-name nil
  "The name of the spelling or accenting program currently running.")

(defvar jspell-pdict-modified-p nil
  "Non-nil means personal dictionary has modifications to be saved.")

;;; If you want to save the dictionary when quitting, must do so explicitly.
;;; When non-nil, the spell session is terminated.
;;; When numeric, contains cursor location in buffer, and cursor remains there.
(defvar jspell-quit nil)

(defvar jspell-filter nil
  "Output filter from piped calls to Jspell.")

(defvar jspell-filter-continue nil
  "Control variable for Jspell filter function.")

(defvar jspell-aux-filter nil
  "Output filter from piped calls to Jguess.")

(defvar jspell-aux-filter-continue nil
  "Control variable for Jguess filter function.")

(defvar jspell-process-directory nil
  "The directory where `jspell-process' was started.")

(defvar jspell-query-replace-marker (make-marker)
  "Marker for `query-replace' processing.")

(defvar jspell-checking-message nil
  "Non-nil when we're checking a mail message")

(defconst jspell-choices-buffer "*Choices*")

(defvar jspell-overlay nil "Overlay variable for Jspell highlighting.")

;;; *** Buffer Local Definitions ***

;;; This is the local dictionary to use.  When nil the default dictionary will
;;; be used.  Do not redefine default value or it will override the global!
(defvar jspell-local-dictionary nil
  "If non-nil, a dictionary to use for Jspell commands in this buffer.
The value must be a string dictionary name in `jspell-dictionary-alist'.
This variable becomes buffer-local when set in any fashion.

Setting jspell-local-dictionary to a value has the same effect as
calling \\[jspell-change-dictionary] with that value.  This variable
is automatically set when defined in the file with either
`jspell-dictionary-keyword' or the Local Variable syntax.")

(make-variable-buffer-local 'jspell-local-dictionary)

;; Use default directory, unless locally set.
(set-default 'jspell-local-dictionary nil)

(defconst jspell-words-keyword "LocalWords: "				      
  "The keyword for local oddly-spelled words to accept.
The keyword will be followed by any number of local word spellings.
There can be multiple of these keywords in the file.")

(defconst jspell-dictionary-keyword "Local JspellDict: "
  "The keyword for local dictionary definitions.
There should be only one dictionary keyword definition per file, and it
should be followed by a correct dictionary name in `jspell-dictionary-alist'.")

(defconst jspell-parsing-keyword "Local JspellParsing: "
  "The keyword for overriding default Jspell parsing.
Determined by the buffer's major mode and extended-character mode as well as
the default dictionary.

The above keyword string should be followed by `latex-mode' or
`nroff-mode' to put the current buffer into the desired parsing mode.

Extended character mode can be changed for this buffer by placing
a `~' followed by an extended-character mode -- such as `~.tex'.")

(defvar jspell-skip-sgml nil
  "Skips spell checking of SGML tags and entity references when non-nil.
This variable is set when major-mode is sgml-mode or html-mode.")

(defvar jspell-local-pdict jspell-personal-dictionary
  "A buffer local variable containing the current personal dictionary.
If non-nil, the value must be a string, which is a file name.

If you specify a personal dictionary for the current buffer which is
different from the current personal dictionary, the effect is similar
to calling \\[jspell-change-dictionary].  This variable is automatically
set when defined in the file with either `jspell-pdict-keyword' or the
local variable syntax.")

(make-variable-buffer-local 'jspell-local-pdict)

(defconst jspell-pdict-keyword "Local JspellPersDict: "
  "The keyword for defining buffer local dictionaries.")

(defvar jspell-buffer-local-name nil
  "Contains the buffer name if local word definitions were used.
Jspell is then restarted because the local words could conflict.")

(defvar jspell-parser 'use-mode-name
   "*Indicates whether jspell should parse the current buffer as TeX Code.
Special value `use-mode-name' tries to guess using the name of major-mode.
Default parser is 'nroff.
Currently the only other valid parser is 'tex.

You can set this variable in hooks in your init file -- eg:

(add-hook 'tex-mode-hook (function (lambda () (setq jspell-parser 'tex))))")

(defvar jspell-local-accepted-list nil
  "List of words that were accepted for the current session,
but should not be inserted into personal dictionary.")

(defvar jspell-local-insert-list nil
  "List of words that should be inserted into a personal dictionary.")

(defvar jmorph-local-insert-list nil
  "List of approximate morphological analyses of words chosen by the user.")

(defvar jspell-region-end (make-marker)
  "Marker that allows spelling continuations.")

(defvar jspell-check-only nil
  "If non-nil, `jspell-word' does not try to correct the word.")


;;; **********************************************************************
;;; **********************************************************************


(and (string-lessp "19" emacs-version)
     (not (boundp 'epoch::version))
     (defalias 'jspell 'jspell-buffer))

(defun format-morph (mformat order param-list)
  "Prepare a string with inflected word, lexeme, and tag.
param-list should be: inflected word, lexeme, adn tag.
mformat is a format specification string with 3 %s places for 3 strings.
order decides in which order word, lexeme, and tag appear in mformat,
e.g. when order is 2, 3, 1, then the first %s in mformat is lexeme,
the second one - tag, and the third one - the inflected word."
  (format mformat
	  (nth (1- (car order)) param-list)
	  (nth (1- (car (cdr order))) param-list)
	  (nth (1- (car (cdr (cdr order)))) param-list)))

;;;###autoload
(defun jspell-word (&optional following quietly continue)
  "Check spelling of word under or before the cursor.
If the word is not found in dictionary, display possible corrections
in a window allowing you to choose one.

With a prefix argument (or if CONTINUE is non-nil),
resume interrupted spell-checking of a buffer or region.

If optional argument FOLLOWING is non-nil or if `jspell-following-word'
is non-nil when called interactively, then the following word
\(rather than preceding\) is checked when the cursor is not over a word.
When the optional argument QUIETLY is non-nil or `jspell-quietly' is non-nil
when called interactively, non-corrective messages are suppressed.

Word syntax described by `jspell-dictionary-alist' (which see).

This will check or reload the dictionary.  Use \\[jspell-change-dictionary]
or \\[jspell-region] to update the Jspell process."
  (interactive (list nil nil current-prefix-arg))
  (setq jspell-program-name jspell-spelling-program)
  (jspell-or-accent-word following quietly continue))

(defun jaccent-word (&optional following quietly continue)
  "Restore diacritics in the word under or before the cursor."
  (interactive (list nil nil current-prefix-arg))
  (setq jspell-program-name jspell-accent-program)
  (jspell-or-accent-word following quietly continue))

(defun insert-morph (word-inf orig-word)
  "Insert morphological annotation near or around the word.
word-inf is the lexeme for the word with morphotactic categories,
orig-word is the inflected word."
  (setq jspell-debug-var (list "w" jspell-debug-var))
  (let* ((tag-pos (string-match jspell-morph-sep word-inf))
	 (bare-word (substring word-inf 0 tag-pos))
	 (morph-tag (substring word-inf (1+ tag-pos))))
    (setq jspell-debug-var (list "y" orig-word bare-word morph-tag)) ;; jd
    (insert 
     (format-morph jmorph-format jmorph-order
		   (list 
		    orig-word	; inflected word
		    bare-word      ; lexeme
		    morph-tag)))))  ; tag

;;;###autoload
(defun jmorph-word (&optional following quietly continue)
  "Perform morphological analysis on a word under cursor.
If there are more than one possible analyses, display possibilities
in a window allowing you to choose one.
The result of the analysis is inserted into the text.
The format used is determined with jmorph-format.

With a prefix argument (or if CONTINUE is non-nil),
resume interrupted spell-checking of a buffer or region.

If optional argument FOLLOWING is non-nil or if `jspell-following-word'
is non-nil when called interactively, then the following word
\(rather than preceding\) is checked when the cursor is not over a word.
When the optional argument QUIETLY is non-nil or `jspell-quietly' is non-nil
when called interactively, non-corrective messages are suppressed.

Word syntax described by `jspell-dictionary-alist' (which see).

This will check or reload the dictionary.  Use \\[jspell-change-dictionary]
or \\[jspell-region] to update the jspell process."
  (interactive (list nil nil current-prefix-arg))
  (setq jspell-program-name jspell-morphing-program)
  (setq jspell-doing-morphology t)
  (jspell-or-accent-word following quietly continue)
  (setq jspell-doing-morphology nil))

(defun jspell-or-accent-word (&optional following quietly continue)
  "Check spelling of word under or before the cursor.
If the word is not found in dictionary, display possible corrections
in a window allowing you to choose one.

With a prefix argument (or if CONTINUE is non-nil),
resume interrupted spell-checking of a buffer or region.

If optional argument FOLLOWING is non-nil or if `jspell-following-word'
is non-nil when called interactively, then the following word
\(rather than preceding\) is checked when the cursor is not over a word.
When the optional argument QUIETLY is non-nil or `jspell-quietly' is non-nil
when called interactively, non-corrective messages are suppressed.

Word syntax described by `jspell-dictionary-alist' (which see).

This will check or reload the dictionary.  Use \\[jspell-change-dictionary]
or \\[jspell-region] to update the Jspell process."
  (if continue
      (jspell-continue)
    (if (interactive-p)
	(setq following jspell-following-word
	      quietly jspell-quietly))
    (jspell-accept-buffer-local-defs)	; use the correct dictionary
    (let ((cursor-location (point))	; retain cursor location
	  (word (jspell-get-word following))
	  start end poss replace)
      ;; destructure return word info list.
      (setq start (car (cdr word))
	    end (car (cdr (cdr word)))
	    word (car word))

      ;; now check spelling of word.
      (or quietly
	  (message "Checking spelling of %s..."
		   (funcall jspell-format-word word)))
      (process-send-string jspell-process (concat word "\n"))
      ;;(setq jspell-debug-var word)
      ;; wait until jspell has processed word
      (while (progn
	       (accept-process-output jspell-process)
;;	       (setq jspell-debug-var (list jspell-filter jspell-debug-var))
;;	       (setq jspell-debug-var jspell-filter)
	       (string-match "\n" (car jspell-filter))))
      (if (listp jspell-filter)
	  (progn
	    (setq poss (jspell-parse-output (car jspell-filter)))
	    (if (and poss (listp poss) (not (car (cdr (cdr poss))))
		     jspell-doing-morphology guessing-available
		     jspell-aux-process)
		;; No morphotactic information available for the word
		;; Try approximate morphological analysis based on endings
		;; (guessing)
		(progn
		  ;; send the word to the guessing program
		  (process-send-string jspell-aux-process (concat word "\n"))
		  ;; wait until jguess has processed word
		  (while
		      (progn
			(accept-process-output jspell-aux-process)
			(string-match "\n" (car jspell-aux-filter))))
		  ;; if it found something, replace guess-list
;;		  (setq jspell-debug-var (car jspell-aux-filter))
		  (if (listp jspell-aux-filter)
		      (let (poss1)
			(setq poss1 (jspell-parse-output
				     (car jspell-aux-filter)))
;;			(setq jspell-debug-var poss1)
			(if (and poss1 (listp poss1) (car (cdr (cdr poss1)))
				 (listp (car (cdr (cdr poss1)))))
			    (setcar (cdr (cdr (cdr poss)))
				    (car (cdr (cdr poss1)))))))))))
      (setq jspell-debug-var poss)
      (cond ((eq poss t)
	     (or quietly
		 (message "%s is correct" (funcall jspell-format-word word))))
	    ((null poss) (message "Error in jspell process"))
	    (jspell-check-only		; called from jspell minor mode.
	     (beep))
	    (t				; prompt for correct word.
	     (save-window-excursion
	       (setq replace (jspell-command-loop
			      (car (cdr (cdr poss)))        ; miss
			      (car (cdr (cdr (cdr poss))))  ; guess
			      (car poss) start end)))       ; word start end
	     (cond ((equal 0 replace)   ; insert locally into buffer-local
					; dictionary
		    (jspell-add-per-file-word-list (car poss)))
		   (replace
		    (setq word (if (atom replace) replace (car replace))
			  cursor-location (+ (- (length word) (- end start))
					     cursor-location))
		    (if (and (not (equal word (car poss)))
			     (not jspell-doing-morphology))
			(progn
			  (delete-region start end)
			  (insert word)))
		    (if jspell-doing-morphology
			(progn
			  (delete-region start end)
			  (insert-morph word (car poss))))
		    (if (not (atom replace)) ; recheck spelling of replacement
			(progn
			  (goto-char cursor-location)
			  (jspell-word following quietly)))))
	     (if (get-buffer jspell-choices-buffer)
		 (kill-buffer jspell-choices-buffer))))
      (goto-char cursor-location)	; return to original location
      (jspell-pdict-save jspell-silently-savep)
      (if jspell-quit (setq jspell-quit nil)))))


(defun jspell-get-word (following &optional extra-otherchars)
  "Return the word for spell-checking according to jspell syntax.
If optional argument FOLLOWING is non-nil or if `jspell-following-word'
is non-nil when called interactively, then the following word
\(rather than preceding\) is checked when the cursor is not over a word.
Optional second argument contains otherchars that can be included in word
many times.

Word syntax described by `jspell-dictionary-alist' (which see)."
  (let* ((jspell-casechars (jspell-get-casechars))
	 (jspell-not-casechars (jspell-get-not-casechars))
	 (jspell-otherchars (jspell-get-otherchars))
	 (jspell-many-otherchars-p (jspell-get-many-otherchars-p))
	 (word-regexp (concat jspell-casechars
			      "+\\("
			      jspell-otherchars
			      "?"
			      (if extra-otherchars
				  (concat extra-otherchars "?"))
			      jspell-casechars
			      "+\\)"
			      (if (or jspell-many-otherchars-p
				      extra-otherchars)
				  "*" "?")))
	 did-it-once
	 start end word)
    ;; find the word
    (if (not (looking-at jspell-casechars))
	(if following
	    (re-search-forward jspell-casechars (point-max) t)
	  (re-search-backward jspell-casechars (point-min) t)))
    ;; move to front of word
    (re-search-backward jspell-not-casechars (point-min) 'start)
    (while (and (or (looking-at jspell-otherchars)
		    (and extra-otherchars (looking-at extra-otherchars)))
		(not (bobp))
		(or (not did-it-once)
		    jspell-many-otherchars-p))
      (if (and extra-otherchars (looking-at extra-otherchars))
	  (progn
	    (backward-char 1)
	    (if (looking-at jspell-casechars)
		(re-search-backward jspell-not-casechars (point-min) 'move)))
	(setq did-it-once t)
	(backward-char 1)
	(if (looking-at jspell-casechars)
	    (re-search-backward jspell-not-casechars (point-min) 'move)
	  (backward-char -1))))
    ;; Now mark the word and save to string.
    (or (re-search-forward word-regexp (point-max) t)
	(error "No word found to check!"))
    (setq start (match-beginning 0)
	  end (point)
	  word (buffer-substring start end))
    (list word start end)))


;;; Global jspell-pdict-modified-p is set by jspell-command-loop and
;;; tracks changes in the dictionary.  The global may either be
;;; a value or a list, whose value is the state of whether the
;;; dictionary needs to be saved.

(defun jspell-pdict-save (&optional no-query force-save)
  "Check to see if the personal dictionary has been modified.
If so, ask if it needs to be saved."
  (interactive (list jspell-silently-savep t))
  (if (and jspell-pdict-modified-p (listp jspell-pdict-modified-p))
      (setq jspell-pdict-modified-p (car jspell-pdict-modified-p)))
  (if (or jspell-pdict-modified-p force-save)
      (if (or no-query (y-or-n-p "Personal dictionary modified.  Save? "))
	  (progn
	    (let ((buffer (generate-new-buffer "*jspell-locall-insert-list*")))
	      (save-excursion
		(set-buffer buffer)
		(goto-char 0)
		(while jspell-local-insert-list
		  (insert (concat (car jspell-local-insert-list) "\n"))
		  (setq jspell-local-insert-list
			(cdr jspell-local-insert-list)))
		(append-to-file (point-min) (point-max)
				jspell-private-dictionary)))
	    ;; kill spelling process, so that the next check
	    ;;   will use new entries
	    (jspell-kill-jspell t)
	    (message "Personal dictionary saved."))))
  (if jmorph-private-dictionary
      (progn
	(let ((buffer (generate-new-buffer "*jmorph-local-insert-list*")))
	  (save-excursion
	    (set-buffer buffer)
	    (goto-char 0)
	    (while jmorph-local-insert-list
	      (insert (concat (car jmorph-local-insert-list) "\n"))
	      (setq jmorph-local-insert-list (cdr jmorph-local-insert-list)))
	    (append-to-file (point-min) (point-max)
			    jmorph-private-dictionary)))
	(setq jmorph-local-insert-list nil)))
  ;; unassert variable, even if not saved to avoid questioning.
  (setq jspell-pdict-modified-p nil)
  (setq jspell-local-insert-list nil))


(defun jspell-command-loop (miss guess word start end)
  "Display possible corrections from list MISS.
GUESS lists possibly valid affix construction of WORD.
Returns nil to keep word.
Returns 0 to insert locally into buffer-local dictionary.
Returns string for new chosen word.
Returns list for new replacement word (will be rechecked).
Highlights the word, which is assumed to run from START to END.
Global `jspell-pdict-modified-p' becomes a list where the only value
indicates whether the dictionary has been modified when option `a' or `i' is
used."
  (let ((textbuf (current-buffer))
	(count ?0)
	(line 2)
	(max-lines (- (window-height) 4)) ; assure 4 context lines.
	(choices miss)
	(window-min-height (min window-min-height
				jspell-choices-win-default-height))
	(command-characters '( ?  ?i ?a ?A ?r ?R ?? ?x ?X ?q ?l ?u ?m ))
	(skipped 0)
	char num result textwin highlighted)

    ;; setup the *Choices* buffer with valid data.
    (save-excursion
      (set-buffer (get-buffer-create jspell-choices-buffer))
      (setq mode-line-format (concat "--  %b  --  word: " word))
      (erase-buffer)
      (if guess
          (progn
            (insert "Approximate morphological analyses "
		    " for this word are shown below:\n")
;;	      (while guess
;;		(if (> (+ 4 (current-column) (length (car guess)))
;;		       (window-width))
;;		    (progn
;;		      (insert "\n\t")
;;		      (setq line (1+ line))))
;;		(insert (car guess) "    ")
;;		(setq guess (cdr guess)))
;;	      (insert "\nUse option `i' if this is a correct composition"
;;		      " from the derivative root.\n")
;;	      (setq line (+ line (if choices 3 2)))))
	    (setq choices guess miss guess)))
      (while (and choices
		  (< (if (> (+ 7 (current-column) (length (car choices))
			       (if (> count ?~) 3 0))
			    (window-width))
			 (progn
			   (insert "\n")
			   (setq line (1+ line)))
		       line)
		     max-lines))
	;; not so good if there are over 20 or 30 options, but then, if
	;; there are that many you don't want to scan them all anyway...
	(while (memq count command-characters) ; skip command characters.
	  (setq count (1+ count)
		skipped (1+ skipped)))
	(insert "(" count ") " (car choices) "  ")
	(setq choices (cdr choices)
	      count (1+ count)))
      (setq count (- count ?0 skipped)))

    ;; Assure word is visible
    (if guess
	(setq line (1+ line)))
    (if (not (pos-visible-in-window-p end))
	(sit-for 0))
    ;; Display choices for misspelled word.
    (let ((choices-window (get-buffer-window jspell-choices-buffer)))
      (if choices-window
	  (if (= line (window-height choices-window))
	      (select-window choices-window)
	    ;; *Choices* window changed size.  Adjust the choices window
	    ;; without scrolling the spelled window when possible
	    (let ((window-line (- line (window-height choices-window)))
		  (visible (progn (forward-line -1) (point))))
	      (if (< line jspell-choices-win-default-height)
		  (setq window-line (+ window-line
				       (- jspell-choices-win-default-height
					  line))))
	      (move-to-window-line 0)
	      (forward-line window-line)
	      (set-window-start (selected-window)
				(if (> (point) visible) visible (point)))
	      (goto-char end)
	      (select-window (previous-window)) ; *Choices* window
	      (enlarge-window window-line)))
	;; Overlay *Choices* window when it isn't showing
	(jspell-overlay-window (max line jspell-choices-win-default-height)))
      (switch-to-buffer jspell-choices-buffer)
      (goto-char (point-min)))

    (select-window (setq textwin (next-window)))

    ;; highlight word, protecting current buffer status
    (unwind-protect
	(progn
	  (if jspell-highlight-p
	      (jspell-highlight-spelling-error start end t))
	  ;; Loop until a valid choice is made.
	  (while
	      (eq
	       t
	       (setq
		result
		(progn
		  (undo-boundary)
		  (message (concat "C-h or ? for more options; SPC to leave "
				   "unchanged, Character to replace word"))
		  (let ((inhibit-quit t))
		    (setq char (if (fboundp 'read-char-exclusive)
				   (read-char-exclusive)
				 (read-char))
			  skipped 0)
		    (if (or quit-flag (= char ?\C-g)) ; C-g is like typing X
			(setq char ?X
			      quit-flag nil)))
		  ;; Adjust num to array offset skipping command characters.
		  (let ((com-chars command-characters))
		    (while com-chars
		      (if (and (> (car com-chars) ?0) (< (car com-chars) char))
			  (setq skipped (1+ skipped)))
		      (setq com-chars (cdr com-chars)))
		    (setq num (- char ?0 skipped)))

		  (cond
		   ((= char ? ) nil)	; accept word this time only
		   ((= char ?i)		; accept and insert word into pers dict
		    (setq jspell-local-insert-list
			  (cons word jspell-local-insert-list))
		    (setq jspell-pdict-modified-p '(t)) ; dictionary modified!
		    nil)
		   ((or (= char ?a) (= char ?A)) ; accept word without insert
		    (setq jspell-local-accepted-list
			  (cons word jspell-local-accepted-list))
		    (if (null jspell-pdict-modified-p)
			(setq jspell-pdict-modified-p
			      (list jspell-pdict-modified-p)))
		    (if (= char ?A) 0))	; return 0 for jspell-add buffer-local
		   ((or (= char ?r) (= char ?R)) ; type in replacement
		    (if (or (= char ?R) jspell-query-replace-choices)
			(list (read-string "Query-replacement for: " word) t)
		      (cons (read-string "Replacement for: " word) nil)))
		   ((or (= char ??) (= char help-char) (= char ?\C-h))
		    (jspell-help)
		    t)
		   ;; Quit and move point back.
		   ((= char ?x)
		    (jspell-pdict-save jspell-silently-savep)
		    (message "Exited spell-checking")
		    (setq jspell-quit t)
		    nil)
		   ;; Quit and preserve point.
		   ((= char ?X)
		    (jspell-pdict-save jspell-silently-savep)
		    (message "%s"
		     (substitute-command-keys
		      (concat "Spell-checking suspended;"
			      " use C-u \\[jspell-word] to resume")))
		    (setq jspell-quit (max (point-min)
					   (- (point) (length word))))
		    nil)
		   ((= char ?q)
		    (if (y-or-n-p "Really kill Jspell process? ")
			(progn
			  (jspell-kill-jspell t) ; terminate process.
			  (setq jspell-quit (or (not jspell-checking-message)
						(point))
				jspell-pdict-modified-p nil))
		      t))		; continue if they don't quit.
		   ((= char ?l)
		    (let ((new-word (read-string
				     "Lookup string (`*' is wildcard): "
				     word))
			  (new-line 2))
		      (if new-word
			  (progn
			    (save-excursion
			      (set-buffer (get-buffer-create
					   jspell-choices-buffer))
			      (erase-buffer)
			      (setq count ?0
				    skipped 0
				    mode-line-format (concat
						      "--  %b  --  word: "
						      new-word)
				    miss (lookup-words new-word)
				    choices miss)
			      (while (and choices ; adjust choices window.
					  (< (if (> (+ 7 (current-column)
						       (length (car choices))
						       (if (> count ?~) 3 0))
						    (window-width))
						 (progn
						   (insert "\n")
						   (setq new-line
							 (1+ new-line)))
					       new-line)
					     max-lines))
				(while (memq count command-characters)
				  (setq count (1+ count)
					skipped (1+ skipped)))
				(insert "(" count ") " (car choices) "  ")
				(setq choices (cdr choices)
				      count (1+ count)))
			      (setq count (- count ?0 skipped)))
			    (select-window (previous-window))
			    (if (and (/= new-line line)
				     (> (max line new-line)
					jspell-choices-win-default-height))
				(let* ((minh jspell-choices-win-default-height)
				       (gr-bl (if (< line minh) ; blanks
						  (- minh line)
						0))
				       (shr-bl (if (< new-line minh) ; blanks
						   (- minh new-line)
						 0)))
				  (if (> new-line line)
				      (enlarge-window (- new-line line gr-bl))
				    (shrink-window (- line new-line shr-bl)))
				  (setq line new-line)))
			    (select-window (next-window)))))
		    t)			; reselect from new choices
		   ((= char ?u)
		    (process-send-string jspell-process
					 (concat "*" (downcase word) "\n"))
		    (setq jspell-pdict-modified-p '(t)) ; dictionary modified!
		    nil)
		   ((= char ?m)		; type in what to insert
		    (process-send-string
		     jspell-process (concat "*" (read-string "Insert: " word)
					    "\n"))
		    (setq jspell-pdict-modified-p '(t))
		    (cons word nil))
		   ;; Select replacement by number
		   ((and (>= num 0) (< num count))
		    (if guess
			(progn
			  (setq jmorph-local-insert-list
				(cons (nth num miss) jmorph-local-insert-list))
			  (if jspell-query-replace-choices ; Query replace flag
			      (list (car jmorph-local-insert-list)
				    'query-replace)
			    (car jmorph-local-insert-list)))
		      (if jspell-query-replace-choices ; Query replace flag
			  (list (nth num miss) 'query-replace)
			(nth num miss))))
		   ((= char ?\C-l)
		    (redraw-display) t)
		   ((= char ?\C-r)
		    (save-window-excursion (recursive-edit)) t)
		   ((= char ?\C-z)
		    (funcall (key-binding "\C-z"))
		    t)
		   (t (ding) t))))))
	  result)
      ;; protected
      (if jspell-highlight-p		; unhighlight
	  (save-window-excursion
	    (select-window textwin)
	    (jspell-highlight-spelling-error start end))))))


;;;###autoload
(defun jspell-help ()
  "Display a list of the options available when a misspelling is encountered.

Selections are:

DIGIT: Replace the word with a digit offered in the *Choices* buffer.
SPC:   Accept word this time.
`i':   Accept word and insert into private dictionary.
`a':   Accept word for this session.
`A':   Accept word and place in `buffer-local dictionary'.
`r':   Replace word with typed-in value.  Rechecked.
`R':   Replace word with typed-in value. Query-replaced in buffer. Rechecked.
`?':   Show these commands.
`x':   Exit spelling buffer.  Move cursor to original point.
`X':   Exit spelling buffer.  Leaves cursor at the current point, and permits
        the aborted check to be completed later.
`q':   Quit spelling session (Kills jspell process).
`l':   Look up typed-in replacement in alternate dictionary.  Wildcards okay.
`u':   Like `i', but the word is lower-cased first.
`m':   Like `i', but allows one to include dictionary completion information.
`C-l':  redraws screen
`C-r':  recursive edit
`C-z':  suspend emacs or iconify frame"

  (let ((help-1 (concat "[r/R]eplace word; [a/A]ccept for this session; "
			"[i]nsert into private dictionary"))
	(help-2 (concat "[l]ook a word up in alternate dictionary;  "
			"e[x/X]it;  [q]uit session"))
	(help-3 (concat "[u]ncapitalized insert into dictionary.  "
			"Type 'C-h d jspell-help' for more help")))
    (save-window-excursion
      (if jspell-help-in-bufferp
	  (progn
	    (jspell-overlay-window 4)
	    (switch-to-buffer (get-buffer-create "*Jspell Help*"))
	    (insert (concat help-1 "\n" help-2 "\n" help-3))
	    (sit-for 5)
	    (kill-buffer "*Jspell Help*"))
	(select-window (minibuffer-window))
	;;(enlarge-window 2)
	(erase-buffer)
	(cond ((string-match "Lucid" emacs-version)
	       (message help-3)
	       (enlarge-window 1)
	       (message help-2)
	       (enlarge-window 1)
	       (message help-1)
	       (goto-char (point-min)))
	      (t
	       (if (string-lessp "19" emacs-version)
		   (message nil))
	       (enlarge-window 2)
	       ;; Make sure we display the minibuffer
	       ;; in this window, not some other.
	       (set-minibuffer-window (selected-window))
	       (insert (concat help-1 "\n" help-2 "\n" help-3))))
	(sit-for 5)
	(erase-buffer)))))


(defun lookup-words (word &optional lookup-dict)
  "Look up word in word-list dictionary.
A `*' serves as a wild card.  If no wild cards, `look' is used if it exists.
Otherwise the variable `jspell-grep-command' contains the command used to
search for the words (usually egrep).

Optional second argument contains the dictionary to use; the default is
`jspell-alternate-dictionary'."
  ;; We don't use the filter for this function, rather the result is written
  ;; into a buffer.  Hence there is no need to save the filter values.
  (if (null lookup-dict)
      (setq lookup-dict jspell-alternate-dictionary))

  (let* ((process-connection-type jspell-use-ptys-p)
	 (wild-p (string-match "\\*" word))
	 (look-p (and jspell-look-p	; Only use look for an exact match.
		      (or jspell-have-new-look (not wild-p))))
	 (jspell-grep-buffer (get-buffer-create "*Jspell-Temp*")) ; result buf
	 (prog (if look-p jspell-look-command jspell-grep-command))
	 (args (if look-p jspell-look-options jspell-grep-options))
	 status results loc)
    (unwind-protect
	(save-window-excursion
	  (message "Starting \"%s\" process..." (file-name-nondirectory prog))
	  (set-buffer jspell-grep-buffer)
	  (if look-p
	      nil
	    ;; convert * to .*
	    (insert "^" word "$")
	    (while (search-backward "*" nil t) (insert "."))
	    (setq word (buffer-string))
	    (erase-buffer))
	  (setq status (call-process prog nil t nil args word lookup-dict))
	  ;; grep returns status 1 and no output when word not found, which
	  ;; is a perfectly normal thing.
	  (if (stringp status)
	      (setq results (cons (format "error: %s exited with signal %s"
					  (file-name-nondirectory prog) status)
				  results))
	    ;; else collect words into `results' in FIFO order
	    (goto-char (point-max))
	    ;; assure we've ended with \n
	    (or (bobp) (= (preceding-char) ?\n) (insert ?\n))
	    (while (not (bobp))
	      (setq loc (point))
	      (forward-line -1)
	      (setq results (cons (buffer-substring (point) (1- loc))
				  results)))))
      ;; protected
      (kill-buffer jspell-grep-buffer)
      (if (and results (string-match ".+: " (car results)))
	  (error "%s error: %s" jspell-grep-command (car results))))
    results))


;;; "jspell-filter" is a list of output lines from the generating function.
;;;   Each full line (ending with \n) is a separate item on the list.
;;; "output" can contain multiple lines, part of a line, or both.
;;; "start" and "end" are used to keep bounds on lines when "output" contains
;;;   multiple lines.
;;; "jspell-filter-continue" is true when we have received only part of a
;;;   line as output from a generating function ("output" did not end with \n)
;;; THIS FUNCTION WILL FAIL IF THE PROCESS OUTPUT DOESN'T END WITH \n!
;;;   This is the case when a process dies or fails. The default behavior
;;;   in this case treats the next input received as fresh input.

(defun jspell-filter (process output)
  "Output filter function for jspell, grep, and look."
  (let ((start 0)
	(continue t)
	end)
    (while continue
      (setq end (string-match "\n" output start)) ; get text up to the newline.
      ;; If we get out of sync and jspell-filter-continue is asserted when we
      ;; are not continuing, treat the next item as a separate list.  When
      ;; jspell-filter-continue is asserted, jspell-filter *should* always be a
      ;; list!

      ;; Continue with same line (item)?
      (if (and jspell-filter-continue jspell-filter (listp jspell-filter))
	  ;; Yes.  Add it to the prev item
	  (setcar jspell-filter
		  (concat (car jspell-filter) (substring output start end)))
	;; No. This is a new line and item.
	(setq jspell-filter
	      (cons (substring output start end) jspell-filter)))
      (if (null end)
	  ;; We've completed reading the output, but didn't finish the line.
	  (setq jspell-filter-continue t continue nil)
	;; skip over newline, this line complete.
	(setq jspell-filter-continue nil end (1+ end))
	(if (= end (length output))	; No more lines in output
	    (setq continue nil)		;  so we can exit the filter.
	  (setq start end))))))		; else move start to next line of input

;;; "jspell-aux-filter" is a list of output lines from the generating function.
;;;   Each full line (ending with \n) is a separate item on the list.
;;; "output" can contain multiple lines, part of a line, or both.
;;; "start" and "end" are used to keep bounds on lines when "output" contains
;;;   multiple lines.
;;; "jspell-aux-filter-continue" is true when we have received only part of a
;;;   line as output from a generating function ("output" did not end with \n)
;;; THIS FUNCTION WILL FAIL IF THE PROCESS OUTPUT DOESN'T END WITH \n!
;;;   This is the case when a process dies or fails. The default behavior
;;;   in this case treats the next input received as fresh input.

(defun jspell-aux-filter (process output)
  "Output filter function for jguess."
  (let ((start 0)
	(continue t)
	end)
    (while continue
      (setq end (string-match "\n" output start)) ; get text up to the newline.
      ;; If we get out of sync and jspell-aux-filter-continue is asserted
      ;; when we are not continuing, treat the next item as a separate list.
      ;; When jspell-aux-filter-continue is asserted, jspell-aux-filter
      ;; *should* always be a list!

      ;; Continue with same line (item)?
      (if (and jspell-aux-filter-continue jspell-aux-filter
	       (listp jspell-aux-filter))
	  ;; Yes.  Add it to the prev item
	  (setcar jspell-aux-filter
		  (concat (car jspell-aux-filter)
			  (substring output start end)))
	;; No. This is a new line and item.
	(setq jspell-aux-filter
	      (cons (substring output start end) jspell-aux-filter)))
      (if (null end)
	  ;; We've completed reading the output, but didn't finish the line.
	  (setq jspell-aux-filter-continue t continue nil)
	;; skip over newline, this line complete.
	(setq jspell-aux-filter-continue nil end (1+ end))
	(if (= end (length output))	; No more lines in output
	    (setq continue nil)		;  so we can exit the filter.
	  (setq start end))))))		; else move start to next line of input


;;; This function destroys the mark location if it is in the word being
;;; highlighted.
(defun jspell-highlight-spelling-error-generic (start end &optional highlight)
  "Highlight the word from START to END with a kludge using `inverse-video'.
When the optional third arg HIGHLIGHT is set, the word is highlighted;
otherwise it is displayed normally."
  (let ((modified (buffer-modified-p))	; don't allow this fn to modify buffer
	(buffer-read-only nil)		; Allow highlighting read-only buffers.
	(text (buffer-substring start end)) ; Save highlight region
	(inhibit-quit t)		; inhibit interrupt processing here.
	(buffer-undo-list t))		; don't clutter the undo list.
    (delete-region start end)
    (insert-char ?  (- end start))	; minimize amount of redisplay
    (sit-for 0)				; update display
    (if highlight (setq inverse-video (not inverse-video))) ; toggle video
    (delete-region start end)		; delete whitespace
    (insert text)			; insert text in inverse video.
    (sit-for 0)				; update display showing inverse video.
    (if highlight (setq inverse-video (not inverse-video))) ; toggle video
    (set-buffer-modified-p modified)))	; don't modify if flag not set.


(defun jspell-highlight-spelling-error-lucid (start end &optional highlight)
  "Highlight the word from START to END using `isearch-highlight'.
When the optional third arg HIGHLIGHT is set, the word is highlighted,
otherwise it is displayed normally."
  (if highlight
      (isearch-highlight start end)
    (isearch-dehighlight t))
  ;;(sit-for 0)
  )


(defun jspell-highlight-spelling-error-overlay (start end &optional highlight)
  "Highlight the word from START to END using overlays.
When the optional third arg HIGHLIGHT is set, the word is highlighted
otherwise it is displayed normally.

The variable `jspell-highlight-face' selects the face to use for highlighting."
  (if highlight
      (progn
	(setq jspell-overlay (make-overlay start end))
	(overlay-put jspell-overlay 'face jspell-highlight-face))
    (delete-overlay jspell-overlay)))


(defun jspell-highlight-spelling-error (start end &optional highlight)
  (cond
   ((string-match "Lucid" emacs-version)
    (jspell-highlight-spelling-error-lucid start end highlight))
   ((and (string-lessp "19" emacs-version)
	 (featurep 'faces) window-system)
    (jspell-highlight-spelling-error-overlay start end highlight))
   (t (jspell-highlight-spelling-error-generic start end highlight))))


(defun jspell-overlay-window (height)
  "Create a window covering the top HEIGHT lines of the current window.
Ensure that the line above point is still visible but otherwise avoid
scrolling the current window.  Leave the new window selected."
  (save-excursion
    (let ((oldot (save-excursion (forward-line -1) (point)))
	  (top (save-excursion (move-to-window-line height) (point))))
      ;; If line above old point (line starting at olddot) would be
      ;; hidden by new window, scroll it to just below new win
      ;; otherwise set top line of other win so it doesn't scroll.
      (if (< oldot top) (setq top oldot))
      ;; NB: Lemacs 19.9 bug: If a window of size N (N includes the mode
      ;; line) is demanded, the last line is not visible.
      ;; At least this happens on AIX 3.2, lemacs w/ Motif, font 9x15.
      ;; So we increment the height for this case.
      (if (string-match "19\.9.*Lucid" (emacs-version))
	  (setq height (1+ height)))
      (split-window nil height)
      (set-window-start (next-window) top))))


;;; Should we add a compound word match return value?
(defun jspell-parse-output (output)
  "Parse the OUTPUT string from Jspell and return:
1: t for an exact match.
2: A string containing the root word for a match via suffix removal.
3: A list of possible correct spellings of the format:
   '(\"ORIGINAL-WORD\" OFFSET MISS-LIST)
   ORIGINAL-WORD is a string of the possibly misspelled word.
   OFFSET is an integer giving the line offset of the word.
   MISS-LIST and GUESS-LIST are possibly null lists of guesses and misses."
  (cond
   ((string= output "") t)		; for startup with pipes...
   ((string-match "\\*OK\\*" output) t)	; exact match
   (t					; need to process &, ?, and #'s
    (let ((original-word (substring output 0 (string-match ":" output)))
	  (cur-count 0)			; contains number of misses + guesses
	  miss-list offset)
      (setq output (substring output
			      (1+ (match-end 0)))) ; skip over misspelling
      (setq offset 0)
      (if (string-match "\\*not found\\*" output) ; No miss list.
	  (setq output nil))
      (while output
	(let ((end (string-match ", \\|\\($\\)" output))) ; end of miss.
	  (setq cur-count (1+ cur-count))
;;	  (debug)
	  (setq miss-list (cons (substring output 0 end) miss-list))
	  (if (match-end 1)		; True only when at end of line.
	      (setq output nil)		; no more misses
	    (setq output (substring output (+ end 2))))))
    (list original-word offset miss-list nil)))))




(defun jspell-init-process ()
  "Check status of Jspell process and start if necessary."
  (if (and jspell-process
	   (eq (process-status jspell-process) 'run)
	   ;; If we're using a personal dictionary, assure
	   ;; we're in the same default directory!
	   (equal current-program-name jspell-program-name)
	   (or (not jspell-personal-dictionary)
	       (equal jspell-process-directory default-directory)))
      (setq jspell-filter nil jspell-filter-continue nil)
    ;; may need to restart to select new personal dictionary.
    (jspell-kill-jspell t)
    (setq current-program-name jspell-program-name )
    (message "Starting new Jspell process...")
    (sit-for 0)
    ;;(check-jspell-version) deleted (jd)
    (setq jspell-process
	  (let ((process-connection-type jspell-use-ptys-p))
	    (apply 'start-process
		   "jspell" nil jspell-program-name
		   (let (args)
		     ;; Local dictionary becomes the global dictionary in use.
		     (if jspell-local-dictionary
			 (setq jspell-dictionary jspell-local-dictionary))
		     (setq args (jspell-get-jspell-args))
		     (if (and jspell-dictionary ; use specified dictionary
			      (not (equal jspell-program-name
					  jspell-morphing-program))
			      (not (equal jspell-program-name
					  jspell-guessing-program)))
			 (setq args
			       (append (list "-d" (jspell-get-dictfile))
				       args)))
		     (if (equal jspell-program-name jspell-accent-program)
			 (if (jspell-get-accent-specific)
			     (setq args
				   (append args
					   (jspell-get-accent-specific)))))
		     (if (and (equal jspell-program-name
				     jspell-spelling-program)
			      (jspell-get-spell-specific))
			 (setq args (append args (jspell-get-spell-specific))))
		     (if (and (equal jspell-program-name
				     jspell-morphing-program)
			      (jspell-get-morph-specific))
			 (setq args (append args (jspell-get-morph-specific))))
		     (if jspell-personal-dictionary ; use specified pers dict
			 (setq args
			       (append args
				       (list "-d"
					     (expand-file-name
					      jspell-personal-dictionary)))))
		     (setq args (append args jspell-extra-args))
		     args)))
	  jspell-filter nil
	  jspell-filter-continue nil
	  jspell-process-directory default-directory)
    (if (and (equal jspell-program-name jspell-morphing-program)
	     guessing-available jspell-guessing-program
	     (jspell-get-guess-specific))
	(progn
	    (setq jspell-aux-process
		  (let ((process-connection-type jspell-use-ptys-p))
		    (apply 'start-process "jspell-aux" nil jspell-guessing-program
			   (jspell-get-guess-specific)))
		  jspell-aux-filter nil
		  jspell-aux-filter-continue nil)
	    (set-process-filter jspell-aux-process 'jspell-aux-filter)))
    (set-process-filter jspell-process 'jspell-filter)
    (cond ((accept-process-output jspell-process 1)
	   ;; It must be an error message.  Show the user.
	   ;; But first wait to see if some more output is going to arrive.
	   ;; Otherwise we get cool errors like "Can't open ".
	   (sleep-for 1)
	   (accept-process-output)
	   (error "%s" (mapconcat 'identity jspell-filter "\n"))))
    (cond ((and jspell-aux-process
	   (accept-process-output jspell-aux-process 1))
	   ;; The same for guessing program
	  (sleep-for 1)
	  (accept-process-output)
	  (error "%s" (mapconcat 'identity jspell-aux-filter "\n"))))
    (setq jspell-filter nil)		; Discard error message
    (setq jspell-aux-filter nil)	; ditto
    (process-kill-without-query jspell-process)))

;;;###autoload
(defun jspell-kill-jspell (&optional no-error)
  "Kill current Jspell process (so that you may start a fresh one).
With NO-ERROR, just return non-nil if there was no Jspell running."
  (interactive)
  (if (not (and jspell-process
		(eq (process-status jspell-process) 'run)))
      (or no-error
	  (error "There is no jspell process running!"))
    (kill-process jspell-process)
    (setq jspell-process nil)
    (message "Jspell process killed")
    nil)
  (if (and jspell-aux-process
		(eq (process-status jspell-aux-process) 'run))
      (kill-process jspell-aux-process)
    (setq jspell-aux-process nil)
    (message "Jspell auxillary process killed")
    nil))


;;; jspell-change-dictionary is set in some people's hooks.  Maybe this should
;;;  call jspell-init-process rather than wait for a spell checking command?

;;;###autoload
(defun jspell-change-dictionary (dict &optional arg)
  "Change `jspell-dictionary' (q.v.) and kill old Jspell process.
A new one will be started as soon as necessary.

By just answering RET you can find out what the current dictionary is.

With prefix argument, set the default directory."
  (interactive
   (list (completing-read
	  "Use new dictionary (RET for current, SPC to complete): "
	  (cons (cons "default" nil) jspell-dictionary-alist) nil t)
	 current-prefix-arg))
  (if (equal dict "default") (setq dict nil))
  ;; This relies on completing-read's bug of returning "" for no match
  (cond ((equal dict "")
	 (message "Using %s dictionary"
		  (or jspell-local-dictionary jspell-dictionary "default")))
	((and (equal dict jspell-dictionary)
	      (or (null jspell-local-dictionary)
		  (equal dict jspell-local-dictionary)))
	 ;; Specified dictionary is the default already.  No-op
	 (and (interactive-p)
	      (message "No change, using %s dictionary" (or dict "default"))))
	(t				; reset dictionary!
	 (if (assoc dict jspell-dictionary-alist)
	     (progn
	       (if (or arg (null dict))	; set default dictionary
		   (setq jspell-dictionary dict))
	       (if (null arg)		; set local dictionary
		   (setq jspell-local-dictionary dict)))
	   (error "Illegal dictionary: %s" dict))
	 (jspell-kill-jspell t)
	 (message "(Next %sJspell command will use %s dictionary)"
		  (cond ((equal jspell-local-dictionary jspell-dictionary)
			 "")
			(arg "global ")
			(t "local "))
		  (or (if (or (equal jspell-local-dictionary jspell-dictionary)
			      (null arg))
			  jspell-local-dictionary
			jspell-dictionary)
		      "default")))))


;;; Spelling of comments are checked when jspell-check-comments is non-nil.

;;;###autoload
(defun jspell-region (reg-start reg-end)
  "Interactively check a region for spelling errors."
  (interactive "r")			; Don't flag errors on read-only bufs.
  (setq jspell-program-name jspell-spelling-program)
  (jspell-or-accent-region reg-start reg-end))

;;;###autoload
(defun jaccent-region (reg-start reg-end)
  "Interactively restore diacritics in a region."
  (interactive "r")			; Don't flag errors on read-only bufs.
  (setq jspell-program-name jspell-accent-program)
  (jspell-or-accent-region reg-start reg-end))


;;;###autoload
(defun jmorph-region (reg-start reg-end)
  "Interactively add morphotactic information to words in a region."
  (interactive "r")			; Don't flag errors on read-only bufs.
  (setq jspell-program-name jspell-morphing-program)
  (setq jspell-doing-morphology t)
  (jspell-or-accent-region reg-start reg-end)
  (setq jspell-doing-morphology nil))

(defun jspell-or-accent-region (reg-start reg-end)
  "Interactively check a region for spelling errors."
  (interactive "r")			; Don't flag errors on read-only bufs.
  (jspell-accept-buffer-local-defs)	; set up dictionary, local words, etc.
  (unwind-protect
      (save-excursion
	(message "Checking %s using %s dictionary..."
		 (if (and (= reg-start (point-min)) (= reg-end (point-max)))
		     (buffer-name) "region")
		 (or jspell-dictionary "default"))
	;; Returns cursor to original location.
	(save-window-excursion
	  (goto-char reg-start)
	  (let ((transient-mark-mode nil)
		ref-type)
	    (while (and (not jspell-quit) (< (point) reg-end))
	      (let ((start (point))
		    (offset-change 0)
		    (end (save-excursion (end-of-line) (min (point) reg-end)))
		    (jspell-casechars (jspell-get-casechars))
		    string)
		(cond			; LOOK AT THIS LINE AND SKIP OR PROCESS
		 ((eolp)		; END OF LINE, just go to next line.
		  (forward-char 1))
		 ((and (null jspell-check-comments) ; SKIPPING COMMENTS
		       comment-start	; skip comments that start on the line.
		       (search-forward comment-start end t)) ; or found here.
		  (if (= (- (point) start) (length comment-start))
		      ;; comment starts the line.  Skip entire line or region
		      (if (string= "" comment-end) ; skip to next line
			  (beginning-of-line 2)	; or jump to comment end.
			(search-forward comment-end reg-end 'limit))
		    ;; Comment later in line.  Check spelling before comment.
		    (let ((limit (- (point) (length comment-start))))
		      (goto-char (1- limit))
		      (if (looking-at "\\\\") ; "quoted" comment, don't skip
			  ;; quoted comment.  Skip over comment-start
			  (if (= start (1- limit))
			      (setq limit (+ limit (length comment-start)))
			    (setq limit (1- limit))))
		      (goto-char start)
		      ;; Only check when "casechars" or math before comment
		      (if (or (re-search-forward jspell-casechars limit t)
			      (re-search-forward "[][()$]" limit t))
			  (setq string
				(concat (buffer-substring start limit)
					"\n")))
		      (goto-char limit))))
		 ((looking-at "[---#@*+!%~^]") ; SKIP SPECIAL JSPELL CHARACTERS
		  (forward-char 1))
		 ((or (and jspell-skip-tib ; SKIP TIB REFERENCES OR SGML MARKUP
			   (re-search-forward jspell-tib-ref-beginning end t)
			   (setq ref-type 'tib))
		      (and jspell-skip-sgml
			   (re-search-forward "[<&]" end t)
			   (setq ref-type 'sgml)))
		  (if (or (and (eq 'tib ref-type) ; tib tag is 2 chars.
			       (= (- (point) 2) start))
			  (and (eq 'sgml ref-type) ; sgml skips 1 char.
			       (= (- (point) 1) start)))
		      ;; Skip to end of reference, not necessarily on this line
		      ;; Return an error if tib/sgml reference not found
		      (if (or
			   (and
			    (eq 'tib ref-type)
			    (not
			     (re-search-forward jspell-tib-ref-end reg-end t)))
			   (and (eq 'sgml ref-type)
				(not (re-search-forward "[>;]" reg-end t))))
			  (progn
			    (jspell-pdict-save jspell-silently-savep)
			    (ding)
			    (message
			     (concat
			      "Open tib or SGML command.  Fix buffer or set "
			      (if (eq 'tib ref-type)
				  "jspell-skip-tib"
				"jspell-skip-sgml")
			      " to nil"))
			    ;; keep cursor at error location
			    (setq jspell-quit (- (point) 2))))
		    ;; Check spelling between reference and start of the line.
		    (let ((limit (- (point) (if (eq 'tib ref-type) 2 1))))
		      (goto-char start)
		      (if (or (re-search-forward jspell-casechars limit t)
			      (re-search-forward "[][()$]" limit t))
			  (setq string
				(concat (buffer-substring start limit)
					"\n")))
		      (goto-char limit))))
		 ((or (re-search-forward jspell-casechars end t) ; TEXT EXISTS
		      (re-search-forward "[][()$]" end t)) ; or MATH COMMANDS
		  (setq string (concat (buffer-substring start end) "\n"))
		  (goto-char end))
		 (t (beginning-of-line 2))) ; EMPTY LINE, skip it.

		(setq end (point))	; "end" tracks end of region to check.

		(if string		; there is something to spell!
		    (let* ((jspell-casechars (jspell-get-casechars))
			   (jspell-not-casechars (jspell-get-not-casechars))
			   (jspell-otherchars (jspell-get-otherchars))
			   (jspell-many-otherchars-p
			    (jspell-get-many-otherchars-p))
			   (word-regexp (concat jspell-casechars
						"+\\("
						jspell-otherchars
						"?"
						jspell-casechars
						"+\\)"
						(if jspell-many-otherchars-p
						    "*" "?")))
			   (curr-offset 0)
			   offsets new-string new-start new-end
			   poss)
		      (while (string-match word-regexp string)
			(setq new-start (match-beginning 0)
			      new-end (match-end 0))
			(setq new-string (substring string
						    new-start
						    new-end))
			(setq curr-offset (+ new-start curr-offset)
			      string (substring string new-end))
			(setq offsets (cons curr-offset offsets))
			(setq curr-offset (+ (length new-string)
					     curr-offset))
			;; check if the word was accepted for this session
			;; or to be inserted into personal dictionary
			(if (or
			     (and jspell-local-accepted-list
				  (member new-string
					  jspell-local-accepted-list))
			     (and jspell-local-insert-list
				  (member new-string
					  jspell-local-insert-list)))
			    (setq offsets (cdr offsets)) ;; remove offset
			  ;; send string to spell process and get input.
			  (progn
			    ;;(setq jspell-debug-var new-string)
			    (setq new-string
				  (concat new-string "\n"))
			    (process-send-string jspell-process new-string)
			    (while (progn
				     (accept-process-output jspell-process)
				     (string-match "\n"
						   (car jspell-filter)))))))
		      ;; parse all inputs from the stream one word at a time.
		      ;; Place in FIFO order
		      (setq jspell-filter (nreverse jspell-filter))
		      (setq offsets (nreverse offsets))
		      (while (and (not jspell-quit) jspell-filter)
			(setq poss (jspell-parse-output (car jspell-filter)))
			(setq curr-offset (car offsets) offsets (cdr offsets))
			(if (and poss (listp poss))
			    (progn
			      (setcar (cdr poss)
				      (+ (car (cdr poss)) curr-offset))
			      (if (and (not (car (cdr (cdr poss))))
				       jspell-doing-morphology
				       guessing-available
				       jspell-aux-process)
				  ;; No morphotactic information available
				  ;; for the word - try approximate
				  ;; morphological analysis based on endings
				  ;; (guessing)
				  (progn
				    ;; send the word to the guessing program
				    (process-send-string
				     jspell-aux-process
				     (concat (car poss) "\n"))
				    ;; wait until jguess has processed word
				    (while
					(progn
					  (accept-process-output
					   jspell-aux-process)
					  (string-match
					   "\n" (car jspell-aux-filter))))
				    ;; if it found something,
				    ;; replace guess-list
				    (if (listp jspell-aux-filter)
					(let (poss1)
					  (setq poss1
					   (jspell-parse-output
					    (car jspell-aux-filter)))
					  (if (and
					       poss1 (listp poss1)
					       (car (cdr (cdr poss1)))
					       (listp
						(car (cdr (cdr poss1)))))
					      (setcar
					       (cdr (cdr (cdr poss)))
					       (car (cdr (cdr poss1)))))))))))
;;			(setq jspell-debug-var poss)
			(if (listp poss) ; spelling error occurred.
			    (let* ((word-start (+ start offset-change
						  (car (cdr poss))))
				   (word-end (+ word-start
						(length (car poss))))
				   replace)
			      (goto-char word-start)
			      ;; Adjust the horizontal scroll & point
			      (jspell-horiz-scroll)
			      (goto-char word-end)
			      (jspell-horiz-scroll)
			      (goto-char word-start)
			      (jspell-horiz-scroll)
			      (if (/= word-end
				      (progn
					(search-forward (car poss) word-end
							t)
					(point)))
				  ;; This occurs due to filter pipe problems
				  (error
				   (concat "Jspell misalignment: word "
					   "`%s' point %d; please retry")
				   (car poss) word-start))
			      (if (not (pos-visible-in-window-p))
				  (sit-for 0))
			      (if (and
				   (or
				    (and
				     jaccent-automatically
				     (equal jspell-program-name
					    jspell-accent-program))
				    (and
				     jmorph-automatically
				     jspell-doing-morphology))
				   (car (cdr (cdr poss))) ; found something
				   (not (cdr (car (cdr (cdr poss)))))) ; 1
				  ;; There is only one candidate for
				  ;; replacement. Replace without query.
				  (progn
				    (setq replace (car poss))
				    (delete-region word-start word-end)
				    (if jspell-doing-morphology
					(progn
					  (insert-morph (car
							 (car
							  (cdr (cdr poss))))
							replace)
					  (let ((change (+ (length
							    (car
							     (car
							      (cdr
							       (cdr poss)))))
							   (length
							    jmorph-format)
							   -7)))
					    (setq reg-end (+ reg-end change)
						  offset-change (+
								 offset-change
								 change)
						  end (+ end change))))
				      (progn
					(insert replace)
					(let ((change (- (length replace)
							 (length (car poss)))))
					  (setq reg-end (+ reg-end change)
						offset-change (+ offset-change
								 change)
						end (+ end change)))))
				    (setq replace nil))
				;; there are more candidates than 1,
				;; or there are no candidates,
				;; or there is exactly one, but the user
				;; does not wish automatic replacements
				(if jspell-keep-choices-win
				    (setq replace
					  (jspell-command-loop
					   (car (cdr (cdr poss)))
					   (car (cdr (cdr (cdr poss))))
					   (car poss) word-start word-end))
				  (save-window-excursion
				    (setq replace
					  (jspell-command-loop
					   (car (cdr (cdr poss)))
					   (car (cdr (cdr (cdr poss))))
					   (car poss)
					   word-start word-end)))))
			      ;; now replacement can be either:
			      ;; 1. nil    - the user wants to keep the word
			      ;;             or the word was replaced
			      ;;             automatically (nothing to be done
			      ;;             with the buffer).
			      ;; 2. 0      - the word found in buffer is to
			      ;;             be inserted into buffer-local
			      ;;             dictionary (so keep the word).
			      ;; 3. string - replacement chosen by user.
			      ;; 4. list   - the user entered replacement
			      ;;             to be rechecked.
			      (cond
			       ((and replace (listp replace))
				;; REPLACEMENT WORD entered.  Recheck line
				;; starting with the replacement word.
				(setq jspell-filter nil
				      string (buffer-substring word-start
							       word-end))
				(let ((change (- (length (car replace))
						 (length (car poss)))))
				  ;; adjust regions
				  (setq reg-end (+ reg-end change)
					offset-change (+ offset-change
							 change)))
				(if (not (equal (car replace) (car poss)))
				    (progn
				      (delete-region word-start word-end)
				      (insert (car replace))))
				;; I only need to recheck typed-in replacements
				(if (not (eq 'query-replace
					     (car (cdr replace))))
				    (backward-char (length (car replace))))
				(setq end (point)) ; reposition for recheck
				;; when second arg exists, query-replace, saving regions
				(if (car (cdr replace))
				    (unwind-protect
					(save-window-excursion
					  (set-marker
					   jspell-query-replace-marker reg-end)
					  ;; Assume case-replace &
					  ;; case-fold-search correct?
					  (query-replace string (car replace)
							 t))
				      (setq reg-end
					    (marker-position
					     jspell-query-replace-marker))
				      (set-marker jspell-query-replace-marker
						  nil))))

			       ((or (null replace)
				    (equal 0 replace)) ; ACCEPT/INSERT
				(if (equal 0 replace) ; BUFFER-LOCAL DICT ADD
				    (setq reg-end
					  (jspell-add-per-file-word-list
					   (car poss) reg-end)))
				;; This avoids pointing out the word that was
				;; just accepted (via 'i' or 'a') if it follows
				;; on the same line.
				;; Redo check following the accepted word.
				(if (and jspell-pdict-modified-p
					 (listp jspell-pdict-modified-p))
				    ;; Word accepted.  Recheck line.
				    (setq jspell-pdict-modified-p ; update flag
					  (car jspell-pdict-modified-p)
					  jspell-filter nil ; discontinue check
					  end word-start))) ; reposition loc.

			       (replace	; STRING REPLACEMENT for this word.
				(delete-region word-start word-end)
				(if jspell-doing-morphology
				    (progn
				      (insert-morph replace (car poss))
				      ;; The length of the original word
				      ;; is (length (car poss)). Since we
				      ;; keep the word it does not take part
				      ;; in calculations. The other parts
				      ;; are jmorph-format, lexeme, and tag.
				      ;; jmorph-format contains 3 %s, which
				      ;; are expanded to original word,
				      ;; lexeme and tag, so these 6 characters
				      ;; do not contribute to the length.
				      ;; The lexeme and the tag are contained
				      ;; in replace, where they are separated
				      ;; with a separator that must be omited
				      ;; in the calculations.
				      (let ((change (+ (length replace)
						       (length jmorph-format)
						       -7)))
					(setq reg-end (+ reg-end change)
					      offset-change (+ offset-change
							       change)
					      end (+ end change))))
				  (progn
				    (insert replace)
				    (let ((change (- (length replace)
						     (length (car poss)))))
				      (setq reg-end (+ reg-end change)
					    offset-change (+ offset-change
							     change)
					    end (+ end change)))))))
			      (if (not jspell-quit)
				  (message "Continuing spelling check using %s dictionary..."
					   (or jspell-dictionary "default")))
			      (sit-for 0)))
			;; finished with line!
			(setq jspell-filter (cdr jspell-filter)))))
	      (goto-char end)))))
    (not jspell-quit))
  ;; protected
  (if (get-buffer jspell-choices-buffer)
      (kill-buffer jspell-choices-buffer))
  (if jspell-quit
      (progn
	;; preserve or clear the region for jspell-continue.
	(if (not (numberp jspell-quit))
	    (set-marker jspell-region-end nil)
	  ;; Enable jspell-continue.
	  (set-marker jspell-region-end reg-end)
	  (goto-char jspell-quit))
	;; Check for aborting
	(if (and jspell-checking-message (numberp jspell-quit))
	    (progn
	      (setq jspell-quit nil)
	      (error "Message send aborted.")))
	(setq jspell-quit nil))
    (set-marker jspell-region-end nil)
    ;; Only save if successful exit.
    (jspell-pdict-save jspell-silently-savep)
    (message "Spell-checking done"))))



;;;###autoload
(defun jspell-buffer ()
  "Check the current buffer for spelling errors interactively."
  (interactive)
  (setq jspell-program-name jspell-spelling-program)
  (jspell-or-accent-region (point-min) (point-max)))

;;;###autoload
(defun jaccent-buffer ()
  "Restore diacritics interactively in the current buffer."
  (interactive)
  (setq jspell-program-name jspell-accent-program)
  (jspell-or-accent-region (point-min) (point-max)))


;;;###autoload
(defun jmorph-buffer ()
  "Add morphotactic annotations to words in the current buffer."
  (interactive)
  (setq jspell-program-name jspell-morphing-program)
  (setq jspell-doing-morphology t)
  (jspell-or-accent-region (point-min) (point-max))
  (setq jspell-doing-morphology nil))



;;;###autoload
(defun jspell-continue ()
  (interactive)
  "Continue a spelling session after making some changes."
  (if (not (marker-position jspell-region-end))
      (message "No session to continue.  Use 'X' command when checking!")
    (if (not (equal (marker-buffer jspell-region-end) (current-buffer)))
	(message "Must continue jspell from buffer %s"
		 (buffer-name (marker-buffer jspell-region-end)))
      (jspell-or-accent-region (point) (marker-position jspell-region-end)))))


;;; Horizontal scrolling
(defun jspell-horiz-scroll ()
  "Places point within the horizontal visibility of its window area."
  (if truncate-lines			; display truncating lines?
      ;; See if display needs to be scrolled.
      (let ((column (- (current-column) (max (window-hscroll) 1))))
	(if (and (< column 0) (> (window-hscroll) 0))
	    (scroll-right (max (- column) 10))
	  (if (>= column (- (window-width) 2))
	      (scroll-left (max (- column (window-width) -3) 10)))))))


;;; Interactive word completion.
;;; Forces "previous-word" processing.  Do we want to make this selectable?

;;;###autoload
(defun jspell-complete-word (&optional interior-frag)
  "Look up word before or under point in dictionary (see lookup-words command)
and try to complete it.  If optional INTERIOR-FRAG is non-nil then the word
may be a character sequence inside of a word.

Standard jspell choices are then available."
  (interactive "P")
  (let ((cursor-location (point))
	case-fold-search
	(word (jspell-get-word nil "\\*")) ; force "previous-word" processing.
	start end possibilities replacement)
    (setq start (car (cdr word))
	  end (car (cdr (cdr word)))
	  word (car word)
	  possibilities
	  (or (string= word "")		; Will give you every word
	      (lookup-words (concat (if interior-frag "*") word "*")
			    jspell-complete-word-dict)))
    (cond ((eq possibilities t)
	   (message "No word to complete"))
	  ((null possibilities)
	   (message "No match for \"%s\"" word))
	  (t				; There is a modification...
	   (cond			; Try and respect case of word.
	    ((string-match "^[^A-Z]+$" word)
	     (setq possibilities (mapcar 'downcase possibilities)))
	    ((string-match "^[^a-z]+$" word)
	     (setq possibilities (mapcar 'upcase possibilities)))
	    ((string-match "^[A-Z]" word)
	     (setq possibilities (mapcar 'capitalize possibilities))))
	   (save-window-excursion
	     (setq replacement
		   (jspell-command-loop possibilities nil word start end)))
	   (cond
	    ((equal 0 replacement)	; BUFFER-LOCAL ADDITION
	     (jspell-add-per-file-word-list word))
	    (replacement		; REPLACEMENT WORD
	     (delete-region start end)
	     (setq word (if (atom replacement) replacement (car replacement))
		   cursor-location (+ (- (length word) (- end start))
				      cursor-location))
	     (insert word)
	     (if (not (atom replacement)) ; recheck spelling of replacement.
		 (progn
		   (goto-char cursor-location)
		   (jspell-word nil t)))))
	   (if (get-buffer jspell-choices-buffer)
	       (kill-buffer jspell-choices-buffer))))
    (jspell-pdict-save jspell-silently-savep)
    (goto-char cursor-location)))


;;;###autoload
(defun jspell-complete-word-interior-frag ()
  "Completes word matching character sequence inside a word."
  (interactive)
  (jspell-complete-word t))


;;; **********************************************************************
;;; 			Jspell Minor Mode
;;; **********************************************************************

(defvar jspell-minor-mode nil
  "Non-nil if Jspell minor mode is enabled.")
;; Variable indicating that jspell minor mode is active.
(make-variable-buffer-local 'jspell-minor-mode)

(or (assq 'jspell-minor-mode minor-mode-alist)
    (setq minor-mode-alist
          (cons '(jspell-minor-mode " Spell") minor-mode-alist)))

(defvar jspell-minor-keymap
  (let ((map (make-sparse-keymap)))
    (define-key map " " 'jspell-minor-check)
    (define-key map "\r" 'jspell-minor-check)
    map)
  "Keymap used for Jspell minor mode.")

(or (not (boundp 'minor-mode-map-alist))
    (assoc 'jspell-minor-mode minor-mode-map-alist)
    (setq minor-mode-map-alist
          (cons (cons 'jspell-minor-mode jspell-minor-keymap)
                minor-mode-map-alist)))

;;;###autoload
(defun jspell-minor-mode (&optional arg)
  "Toggle Jspell minor mode.
With prefix arg, turn Jspell minor mode on iff arg is positive.
 
In Jspell minor mode, pressing SPC or RET
warns you if the previous word is incorrectly spelled."
  (interactive "P")
  (setq jspell-minor-mode
	(not (or (and (null arg) jspell-minor-mode)
		 (<= (prefix-numeric-value arg) 0))))
  (force-mode-line-update))
 
(defun jspell-minor-check ()
  ;; Check previous word then continue with the normal binding of this key.
  (interactive "*")
  (let ((jspell-minor-mode nil)
	(jspell-check-only t))
    (save-restriction
      (narrow-to-region (point-min) (point))
      (jspell-word nil t))
    (call-interactively (key-binding (this-command-keys)))))


;;; **********************************************************************
;;; 			Jspell Message
;;; **********************************************************************
;;; Original from D. Quinlan, E. Bradford, A. Albert, and M. Ernst


(defvar jspell-message-text-end
  (mapconcat (function identity)
	     '(
	       ;; Matches postscript files.
	       "^%!PS-Adobe-[123].0"
	       ;; Matches uuencoded text
	       "^begin [0-9][0-9][0-9] .*\nM.*\nM.*\nM"
	       ;; Matches shell files (esp. auto-decoding)
	       "^#! /bin/[ck]?sh"
	       ;; Matches context difference listing
	       "\\(diff -c .*\\)?\n\\*\\*\\* .*\n--- .*\n\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*\\*"
               ;; Matches reporter.el bug report
               "^current state:\n==============\n"
	       ;; Matches "----------------- cut here"
	       ;; and "------- Start of forwarded message"
	       "^[-=_]+\\s ?\\(cut here\\|Start of forwarded message\\)")
	     "\\|")
  "*End of text which will be checked in jspell-message.
If it is a string, limit at first occurrence of that regular expression.
Otherwise, it must be a function which is called to get the limit.")


(defvar jspell-message-start-skip
  (mapconcat (function identity)
	     '(
	       ;; Matches forwarded messages
	       "^---* Forwarded Message"
	       ;; Matches PGP Public Key block
	       "^---*BEGIN PGP [A-Z ]*--*"
	       )
	     "\\|")
  "Spelling is skipped inside these start/end groups by jspell-message.
Assumed that blocks are not mutually inclusive.")


(defvar jspell-message-end-skip
  (mapconcat (function identity)
	     '(
	       ;; Matches forwarded messages
	       "^--- End of Forwarded Message"
	       ;; Matches PGP Public Key block
	       "^---*END PGP [A-Z ]*--*"
	       )
	     "\\|")
  "Spelling is skipped inside these start/end groups by jspell-message.
Assumed that blocks are not mutually inclusive.")


;;;###autoload
(defun jspell-message ()
  "Check the spelling of a mail message or news post.
Don't check spelling of message headers except the Subject field.
Don't check included messages.

To abort spell checking of a message region and send the message anyway,
use the `x' command.  (Any subsequent regions will be checked.)
The `X' command aborts the message send so that you can edit the buffer.

To spell-check whenever a message is sent, include the appropriate lines
in your .emacs file:
   (add-hook 'message-send-hook 'jspell-message)
   (add-hook 'mail-send-hook  'jspell-message)
   (add-hook 'mh-before-send-letter-hook 'jspell-message)

You can bind this to the key C-c i in GNUS or mail by adding to
`news-reply-mode-hook' or `mail-mode-hook' the following lambda expression:
   (function (lambda () (local-set-key \"\\C-ci\" 'jspell-message)))"
  (interactive)
  (setq jspell-program-name jspell-spelling-program)
  (jspell-or-accent-message))

(defun jaccent-message ()
  "Restore diacritics in a message. See jspell-message."
  (interactive)
  (setq jspell-program-name jspell-accent-program)
  (jspell-or-accent-message))

(defun jspell-or-accent-message ()
  "Check the spelling of a mail message or news post.
Don't check spelling of message headers except the Subject field.
Don't check included messages.

To abort spell checking of a message region and send the message anyway,
use the `x' command.  (Any subsequent regions will be checked.)
The `X' command aborts the message send so that you can edit the buffer.

To spell-check whenever a message is sent, include the appropriate lines
in your .emacs file:
   (add-hook 'message-send-hook 'jspell-message)
   (add-hook 'mail-send-hook  'jspell-message)
   (add-hook 'mh-before-send-letter-hook 'jspell-message)

You can bind this to the key C-c i in GNUS or mail by adding to
`news-reply-mode-hook' or `mail-mode-hook' the following lambda expression:
   (function (lambda () (local-set-key \"\\C-ci\" 'jspell-message)))"
  (save-excursion
    (goto-char (point-min))
    (let* ((internal-messagep (save-excursion
				(re-search-forward
				 (concat "^"
					 (regexp-quote mail-header-separator)
					 "$")
				 nil t)))
	   (limit (copy-marker
		   (cond
		    ((not jspell-message-text-end) (point-max))
		    ((char-or-string-p jspell-message-text-end)
		     (if (re-search-forward jspell-message-text-end nil t)
			 (match-beginning 0)
		       (point-max)))
		    (t (min (point-max) (funcall jspell-message-text-end))))))
	   (cite-regexp			;Prefix of inserted text
	    (cond
	     ((featurep 'supercite)	; sc 3.0
	      (concat "\\(" (sc-cite-regexp) "\\)" "\\|"
		      (jspell-non-empty-string sc-reference-tag-string)))
	     ((featurep 'sc)		; sc 2.3
	      (concat "\\(" sc-cite-regexp "\\)" "\\|"
		      (jspell-non-empty-string sc-reference-tag-string)))
	     ((equal major-mode 'news-reply-mode) ;GNUS 4 & below
	      (concat "In article <" "\\|"
		      (if mail-yank-prefix
			  (jspell-non-empty-string mail-yank-prefix)
			"^   \\|^\t")))
	     ((equal major-mode 'message-mode) ;GNUS 5
	      (concat ".*@.* writes:$" "\\|"
		      (if mail-yank-prefix
			  (jspell-non-empty-string mail-yank-prefix)
			"^   \\|^\t")))
	     ((equal major-mode 'mh-letter-mode) ; mh mail message
	      (jspell-non-empty-string mh-ins-buf-prefix))
	     ((not internal-messagep)	; Assume n sent us this message.
	      (concat "In [a-zA-Z.]+ you write:" "\\|"
		      "In <[^,;&+=]+> [^,;&+=]+ writes:" "\\|"
		      " *> *"))
	     ((boundp 'vm-included-text-prefix) ; VM mail message
	      (concat "[^,;&+=]+ writes:" "\\|"
		      (jspell-non-empty-string vm-included-text-prefix)))
	     (mail-yank-prefix		; vanilla mail message.
	      (jspell-non-empty-string mail-yank-prefix))
	     (t "^   \\|^\t")))
	   (cite-regexp-start (concat "^[ \t]*$\\|" cite-regexp))
	   (cite-regexp-end   (concat "^\\(" cite-regexp "\\)"))
	   (old-case-fold-search case-fold-search)
	   (case-fold-search t)
	   (jspell-checking-message t))
      (goto-char (point-min))
      ;; Skip header fields except Subject: without Re:'s
      ;;(search-forward mail-header-separator nil t)
      (while (if internal-messagep
		 (< (point) internal-messagep)
	       (and (looking-at "[a-zA-Z---]+:\\|\t\\| ")
		    (not (eobp))))
	(if (looking-at "Subject: *")	; Spell check new subject fields
	    (progn
	      (goto-char (match-end 0))
	      (if (and (not (looking-at ".*Re\\>"))
		       (not (looking-at "\\[")))
		  (let ((case-fold-search old-case-fold-search))
		    (jspell-or-accent-region (point)
				   (progn
				     (end-of-line)
				     (while (looking-at "\n[ \t]")
				       (end-of-line 2))
				     (point)))))))
	(forward-line 1))
      (setq case-fold-search nil)
      ;; Skip mail header, particularly for non-english languages.
      (if (looking-at (concat (regexp-quote mail-header-separator) "$"))
	  (forward-line 1))
      (while (< (point) limit)
	;; Skip across text cited from other messages.
	(while (and (looking-at cite-regexp-start)
		    (< (point) limit)
		    (zerop (forward-line 1))))

	(if (< (point) limit)
	    (let* ((start (point))
		   ;; Check the next batch of lines that *aren't* cited.
		   (end-c (and (re-search-forward cite-regexp-end limit 'end)
			       (match-beginning 0)))
		   ;; Skip a block of included text.
		   (end-fwd (and (goto-char start)
				 (re-search-forward jspell-message-start-skip
						    limit 'end)
				 (progn (beginning-of-line)
					(point))))
		   (end (or (and end-c end-fwd (min end-c end-fwd))
			    end-c end-fwd
			    ;; default to limit of text.
			    (marker-position limit))))
	      (goto-char start)
	      (jspell-or-accent-region start end)
	      (if (and end-fwd (= end end-fwd))
		  (progn
		    (goto-char end)
		    (re-search-forward jspell-message-end-skip limit 'end))
		(goto-char end)))))
      (set-marker limit nil))))


(defun jspell-non-empty-string (string)
  (if (or (not string) (string-equal string ""))
      "\\'\\`" ; An unmatchable string if string is null.
    (regexp-quote string)))


;;; **********************************************************************
;;; 			Buffer Local Functions
;;; **********************************************************************


(defun jspell-accept-buffer-local-defs ()
  "Load all buffer-local information, restarting jspell when necessary."
  (jspell-buffer-local-dict)		; May kill jspell-process.
  (jspell-buffer-local-words)		; Will initialize jspell-process.
  (jspell-buffer-local-parsing))


(defun jspell-buffer-local-parsing ()
  "Place Jspell into parsing mode for this buffer.
Overrides the default parsing mode.
Includes latex/nroff modes and extended character mode."
  ;; (jspell-init-process) must already be called.
  ;; Hard-wire test for SGML & HTML mode.
  (setq jspell-skip-sgml (memq major-mode '(sgml-mode html-mode)))
  ;; Set buffer-local parsing mode and extended character mode, if specified.
  )

;;; Can kill the current jspell process

(defun jspell-buffer-local-dict ()
  "Initializes local dictionary.
When a dictionary is defined in the buffer (see variable
`jspell-dictionary-keyword'), it will override the local setting
from \\[jspell-change-dictionary].
Both should not be used to define a buffer-local dictionary."
  (save-excursion
    (goto-char (point-min))
    (let (end)
      ;; Override the local variable definition.
      ;; Uses last valid definition.
      (while (search-forward jspell-dictionary-keyword nil t)
	(setq end (save-excursion (end-of-line) (point)))
	(if (re-search-forward " *\\([^ \"]+\\)" end t)
	    (setq jspell-local-dictionary
		  (buffer-substring (match-beginning 1) (match-end 1)))))
      (goto-char (point-min))
      (while (search-forward jspell-pdict-keyword nil t)
	(setq end (save-excursion (end-of-line) (point)))
	(if (re-search-forward " *\\([^ \"]+\\)" end t)
	    (setq jspell-local-pdict
		  (buffer-substring (match-beginning 1) (match-end 1)))))))
  ;; Reload if new personal dictionary defined.
  (if (and jspell-local-pdict
	   (not (equal jspell-local-pdict jspell-personal-dictionary)))
      (progn
	(jspell-kill-jspell t)
	(setq jspell-personal-dictionary jspell-local-pdict)))
  ;; Reload if new dictionary defined.
  (if (and jspell-local-dictionary
	   (not (equal jspell-local-dictionary jspell-dictionary)))
      (jspell-change-dictionary jspell-local-dictionary)))


(defun jspell-buffer-local-words ()
  "Loads the buffer-local dictionary in the current buffer."
  (if (and jspell-buffer-local-name
	   (not (equal jspell-buffer-local-name (buffer-name))))
      (progn
	(jspell-kill-jspell t)
	(setq jspell-buffer-local-name nil)))
  (jspell-init-process)
  (save-excursion
    (goto-char (point-min))
    (while (search-forward jspell-words-keyword nil t)
      (or jspell-buffer-local-name
	  (setq jspell-buffer-local-name (buffer-name)))
      (let ((end (save-excursion (end-of-line) (point)))
	    string)
	;; buffer-local words separated by a space, and can contain
	;; any character other than a space.
	(while (re-search-forward " *\\([^ ]+\\)" end t)
	  (setq string (buffer-substring (match-beginning 1) (match-end 1)))
	  (setq jspell-local-accepted-list
		(cons (concat string "\n") jspell-local-accepted-list)))))))


;;; returns optionally adjusted region-end-point.

(defun jspell-add-per-file-word-list (word &optional reg-end)
  "Adds new word to the per-file word list."
  (or jspell-buffer-local-name
      (setq jspell-buffer-local-name (buffer-name)))
  (if (null reg-end)
      (setq reg-end 0))
  (save-excursion
    (goto-char (point-min))
    (let (case-fold-search line-okay search done string)
      (while (not done)
	(setq search (search-forward jspell-words-keyword nil 'move)
	      line-okay (< (+ (length word) 1 ; 1 for space after word..
			      (progn (end-of-line) (current-column)))
			   80))
	(if (or (and search line-okay)
		(null search))
	    (progn
	      (setq done t)
	      (if (null search)
		  (progn
		    (open-line 1)
		    (setq string (concat comment-start " "
					 jspell-words-keyword))
		    ;; in case the keyword is in the middle of the file....
		    (if (> reg-end (point))
			(setq reg-end (+ reg-end (length string))))
		    (insert string)
		    (if (and comment-end (not (equal "" comment-end)))
			(save-excursion
			  (open-line 1)
			  (forward-line 1)
			  (insert comment-end)))))
	      (if (> reg-end (point))
		  (setq reg-end (+ 1 reg-end (length word))))
	      (insert (concat " " word)))))))
  reg-end)


(defconst jspell-version "2.37 -- Tue Jun 13 12:05:28 EDT 1995")

(provide 'jspell)


;;; LOCAL VARIABLES AND BUFFER-LOCAL VALUE EXAMPLES.

;;; Local Variable options:
;;; mode: name(-mode)
;;; eval: expression
;;; local-variable: value

;;; The following sets the buffer local dictionary to english!

;;; Local Variables:
;;; mode: emacs-lisp
;;; comment-column: 40
;;; jspell-local-dictionary: "english"
;;; End:


;;; MORE EXAMPLES OF JSPELL BUFFER-LOCAL VALUES

;;; The following places this file in nroff parsing and extended char modes.
;;; Local JspellParsing: nroff-mode ~nroff
;;; Change JspellDict to JspellDict: to enable the following line.
;;; Local JspellDict english
;;; Change JspellPersDict to JspellPersDict: to enable the following line.
;;; Local JspellPersDict ~/.jspell_lisp
;;; The following were automatically generated by jspell using the 'A' command:
; LocalWords:  jspell jspell-highlight-p jspell-check-comments query-replace
; LocalWords:  jspell-query-replace-choices jspell-skip-tib non-nil tib
; LocalWords:  regexps jspell-tib-ref-beginning jspell-tib-ref-end

;; jspell.el ends here
