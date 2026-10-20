/* ------------------------------------------------------------ */
/*
HTTrack Website Copier, Offline Browser for Windows and Unix
Copyright (C) 1998-2017 Xavier Roche and other contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Important notes:

- We hereby ask people using this source NOT to use it in purpose of grabbing
emails addresses, or collecting any other private information on persons.
This would disgrace our work, and spoil the many hours we spent on it.

Please visit our Website: http://www.httrack.com
*/

/* ------------------------------------------------------------ */
/* File: htsparse.c parser                                      */
/*       html/javascript/css parser                             */
/*       and other parser routines                              */
/* Author: Xavier Roche                                         */
/* ------------------------------------------------------------ */

/* Internal engine bytecode */
#define HTS_INTERNAL_BYTECODE

#include <fcntl.h>
#include <ctype.h>

/* File defs */
#include "htscore.h"

/* specific definitions */
#include "htsbase.h"
#include "htsnet.h"
#include "htsbauth.h"
#include "htsmd5.h"
#include "htsindex.h"
#include "htscharset.h"
#include "htsencoding.h"

/* external modules */
#include "htsmodules.h"

// htswrap_add
#include "htswrap.h"

// parser
#include "htsparse.h"
#include "htsback.h"

// arrays
#include "htsarrays.h"

//net
#include "htsbasenet.h"


/** Append bytes to the output buffer up to the pointer 'html'. **/
#define HT_add_adr do { \
  const size_t sz_ = html - lastsaved; \
  if (sz_ != 0) { \
    TypedArrayAppend(output_buffer, lastsaved, sz_); \
    lastsaved = html; \
  } \
} while(0)

/** Append to the output buffer the string 'A'. **/
#define HT_ADD(A) TypedArrayAppend(output_buffer, A, strlen(A))

/** Append to the output buffer the string 'A', html-escaped. **/
#define HT_ADD_HTMLESCAPED_ANY(A, FUNCTION) do { \
  const char *const str_ = (A); \
  size_t size_; \
  /* &amp; is the maximum expansion */ \
  TypedArrayEnsureRoom(output_buffer, strlen(str_) * 5 + 1024); \
  size_ = FUNCTION(str_, &TypedArrayTail(output_buffer), \
                   TypedArrayRoom(output_buffer)); \
  TypedArraySize(output_buffer) += size_; \
} while(0)

/** Append to the output buffer the string 'A', html-escaped for &. **/
#define HT_ADD_HTMLESCAPED(A) HT_ADD_HTMLESCAPED_ANY(A, escape_for_html_print)

/**
 * Append to the output buffer the string 'A', html-escaped for & and 
 * high chars.
 **/
#define HT_ADD_HTMLESCAPED_FULL(A) HT_ADD_HTMLESCAPED_ANY(A, escape_for_html_print_full)

// does nothing
#define XH_uninit do {} while(0)

#define HT_ADD_FOP


#define _FILTERS     (*opt->filters.filters)
#define _FILTERS_PTR (opt->filters.filptr)
#define _ROBOTS      ((robots_wizard*)opt->robotsptr)

/* Apply current *adr character for the script automate */
#define AUTOMATE_LOOKUP_CURRENT_ADR() do { \
  if (inscript) { \
  int new_state_pos; \
  new_state_pos=inscript_state[inscript_state_pos][(unsigned char)*html]; \
  if (new_state_pos < 0) { \
  new_state_pos=inscript_state[inscript_state_pos][INSCRIPT_DEFAULT]; \
  } \
  assertf(new_state_pos >= 0); \
  assertf(new_state_pos*sizeof(inscript_state[0]) < sizeof(inscript_state)); \
  inscript_state_pos=new_state_pos; \
  } \
} while(0)

/* Increment current pointer to 'steps' characters, modifying automate if necessary */
#define INCREMENT_CURRENT_ADR(steps) do { \
  int steps__ = (int) ( steps ); \
  while(steps__ > 0) { \
  html++; \
  AUTOMATE_LOOKUP_CURRENT_ADR(); \
  steps__ --; \
  } \
} while(0)

/* Main parser */
int htsparse(htsmoduleStruct * str, htsmoduleStructExtended * stre) {
  char catbuff[CATBUFF_SIZE];

  /* Load engine variables */
  //ENGINE_LOAD_CONTEXT();
#define heap_(N) str->opt->liens[N]

  {
    char *cAddr = stre->r_->adr;
    int cSize = (int) stre->r_->size;

    hts_log_print(str->opt, LOG_DEBUG, "engine: preprocess-html: %s%s", heap_(*str->ptr_)->adr,
                  heap_(*str->ptr_)->fil);
    if (RUN_CALLBACK4(str->opt, preprocess, &cAddr, &cSize, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil) == 1) {
      stre->r_->adr = cAddr;
      stre->r_->size = cSize;
    }
  }
  
  // Si HTML pas ok, on retourne
  if (!RUN_CALLBACK4(str->opt, check_html, stre->r_->adr, (int) stre->r_->size, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil)) {
    
    //ENGINE_SAVE_CONTEXT();
    
    return 0;
  }

  FILE *fp = NULL;                  // fichier écrit localement
  const char *html = stre->r_->adr;        // pointeur (on parcours)
  const char *lastsaved;            // adresse du dernier octet sauvé + 1

  hts_log_print(str->opt, LOG_DEBUG, "scanning file %s%s (%s)..", heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil,
                heap_(*str->ptr_)->sav);

  /* Hack to avoid NULL char problems with C syntax */
  /* Yes, some bogus HTML pages can embed null chars
  and therefore can not be properly handled if this hack is not done
  */
  if (stre->r_->adr != NULL) {
    size_t i;
    for(i = 0 ; i < (size_t) stre->r_->size ; i++) {
      if (stre->r_->adr[i] == '\0') {
        stre->r_->adr[i] = ' ';
      }
    }
  }

  // Indexing!
#if HTS_MAKE_KEYWORD_INDEX
  if (str->opt->kindex) {
    if (index_keyword(stre->r_->adr, stre->r_->size, stre->r_->contenttype, heap_(*str->ptr_)->sav,
         StringBuff(str->opt->path_html_utf8))) {
      hts_log_print(str->opt, LOG_DEBUG, "indexing file..done");
    } else {
      hts_log_print(str->opt, LOG_DEBUG, "indexing file..error!");
    }
  }
#endif

  // Now, parsing
  if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {      // récupérer les html sur disque
    // créer le fichier html local
    HT_ADD_FOP;               // écrire peu à peu le fichier
  }

  if (*stre->error_)
    goto error;
  
  // output HTML
  TypedArray(char) output_buffer = EMPTY_TYPED_ARRAY;

  time_t user_interact_timestamp = 0;
  int detect_title = 0;     // détection  du title
  int back_add_stats = str->opt->state.back_add_stats;

  const char *in_media = NULL;    // in other media type (real media and so..)
  int intag = 0;            // on est dans un tag
  int incomment = 0;        // dans un <!--
  int inscript = 0;         // dans un scipt pour applets javascript)
  int inscript_locked = 0;  // in locked script (ie. js file)
  signed char inscript_state[10][257];
  typedef enum {
    INSCRIPT_START = 0,
    INSCRIPT_ANTISLASH,
    INSCRIPT_INQUOTE,
    INSCRIPT_INQUOTE2,
    INSCRIPT_SLASH,
    INSCRIPT_SLASHSLASH,
    INSCRIPT_COMMENT,
    INSCRIPT_COMMENT2,
    INSCRIPT_ANTISLASH_IN_QUOTE,
    INSCRIPT_ANTISLASH_IN_QUOTE2,
    INSCRIPT_DEFAULT = 256
  } INSCRIPT;
  INSCRIPT inscript_state_pos = INSCRIPT_START;
  const char *inscript_name = NULL;       // script tag name
  int inscript_tag = 0;     // on est dans un <body onLoad="... terminé par >
  char inscript_tag_lastc = '\0';

  // terminaison (" ou ') du "<body onLoad=.."
  int inscriptgen = 0;      // on est dans un code générant, ex après obj.write("..

  //int inscript_check_comments=0, inscript_in_comments=0;    // javascript comments
  char scriptgen_q = '\0';  // caractère faisant office de guillemet (' ou ")

  //int no_esc_utf=0;      // ne pas echapper chars > 127
  int nofollow = 0;         // ne pas scanner

  //
  int parseall_lastc = '\0';        // dernier caractère parsé pour parseall

  //int parseall_incomment=0;   // dans un /* */ (exemple: a = /* URL */ "img.gif";)
  //
  const char *intag_start = html;
  const char *intag_name = NULL;
  const char *intag_startattr = NULL;
  int intag_start_valid = 0;
  int intag_ctype = 0;

  //
  int emited_footer = 0;    // emitted footer comment tag(s) count

  //
  int parent_relative = 0;  // the parent is the base path (.js, .css..)

  lastsaved = html;

  /* Initialize script automate for comments, quotes.. */
  memset(inscript_state, 0xff, sizeof(inscript_state));
  inscript_state[INSCRIPT_START][INSCRIPT_DEFAULT] = INSCRIPT_START;        /* by default, stay in START */
  inscript_state[INSCRIPT_START]['\\'] = INSCRIPT_ANTISLASH;        /* #1: \ escapes the next character whatever it is */
  inscript_state[INSCRIPT_ANTISLASH][INSCRIPT_DEFAULT] = INSCRIPT_START;
  inscript_state[INSCRIPT_START]['\''] = INSCRIPT_INQUOTE;  /* #2: ' opens quote and only ' returns to 0 */
  inscript_state[INSCRIPT_INQUOTE][INSCRIPT_DEFAULT] = INSCRIPT_INQUOTE;
  inscript_state[INSCRIPT_INQUOTE]['\''] = INSCRIPT_START;
  inscript_state[INSCRIPT_INQUOTE]['\\'] = INSCRIPT_ANTISLASH_IN_QUOTE;
  inscript_state[INSCRIPT_START]['\"'] = INSCRIPT_INQUOTE2; /* #3: " opens double-quote and only " returns to 0 */
  inscript_state[INSCRIPT_INQUOTE2][INSCRIPT_DEFAULT] = INSCRIPT_INQUOTE2;
  inscript_state[INSCRIPT_INQUOTE2]['\"'] = INSCRIPT_START;
  inscript_state[INSCRIPT_INQUOTE2]['\\'] = INSCRIPT_ANTISLASH_IN_QUOTE2;
  inscript_state[INSCRIPT_START]['/'] = INSCRIPT_SLASH;     /* #4: / state, default to #0 */
  inscript_state[INSCRIPT_SLASH][INSCRIPT_DEFAULT] = INSCRIPT_START;
  inscript_state[INSCRIPT_SLASH]['/'] = INSCRIPT_SLASHSLASH;        /* #5: // with only LF to escape */
  inscript_state[INSCRIPT_SLASHSLASH][INSCRIPT_DEFAULT] =
    INSCRIPT_SLASHSLASH;
  inscript_state[INSCRIPT_SLASHSLASH]['\n'] = INSCRIPT_START;
  inscript_state[INSCRIPT_SLASH]['*'] = INSCRIPT_COMMENT;   /* #6: / * with only * / to escape */
  inscript_state[INSCRIPT_COMMENT][INSCRIPT_DEFAULT] = INSCRIPT_COMMENT;
  inscript_state[INSCRIPT_COMMENT]['*'] = INSCRIPT_COMMENT2;        /* #7: closing comments */
  inscript_state[INSCRIPT_COMMENT2][INSCRIPT_DEFAULT] = INSCRIPT_COMMENT;
  inscript_state[INSCRIPT_COMMENT2]['/'] = INSCRIPT_START;
  inscript_state[INSCRIPT_COMMENT2]['*'] = INSCRIPT_COMMENT2;
  inscript_state[INSCRIPT_ANTISLASH_IN_QUOTE][INSCRIPT_DEFAULT] = INSCRIPT_INQUOTE; /* #8: escape in '' */
  inscript_state[INSCRIPT_ANTISLASH_IN_QUOTE2][INSCRIPT_DEFAULT] = INSCRIPT_INQUOTE2;       /* #9: escape in "" */

  /* Primary list or URLs */
  if (*str->ptr_ == 0) {
    intag = 1;
    intag_start_valid = 0;
    intag_name = NULL;
  }
  /* Check is the file is a .js file */
  else
    if (
        (compare_mime(str->opt, stre->r_->contenttype, str->url_file, "application/x-javascript") != 0)
      || (compare_mime(str->opt, stre->r_->contenttype, str->url_file, "text/css") != 0) // but also css ?
    ) {                     /* JavaScript js file */
    inscript = 1;
    inscript_locked = 1;    /* Don't exit js space upon </script> */
    if (str->opt->parsedebug) {
      HT_ADD("<@@ inscript @@>");
    }
    inscript_name = "script";
    intag = 1;              // because après <script> on y est .. - pas utile
    intag_start_valid = 0;  // OUI car nous sommes dans du code, plus dans du "vrai" tag
    hts_log_print(str->opt, LOG_DEBUG, "note: this file is a javascript file");
    // for javascript only
    if (compare_mime(str->opt, stre->r_->contenttype, str->url_file, "application/x-javascript") != 0) {
      // all links must be checked against parent, not this link
      if (heap_(*str->ptr_)->precedent != 0) {
        parent_relative = 1;
      }
    }
  }
  /* Or a real audio */
  else if (compare_mime(str->opt, stre->r_->contenttype, str->url_file, "audio/x-pn-realaudio") != 0) { /* realaudio link file */
    inscript = intag = 0;
    inscript_name = "media";
    intag_start_valid = 0;
    in_media = "LNK";       // real media! -> links
  }
  /* Or a m3u playlist */
  else if (compare_mime(str->opt, stre->r_->contenttype, str->url_file, "audio/x-mpegurl") != 0) {      /* mp3 link file */
    inscript = intag = 0;
    inscript_name = "media";
    intag_start_valid = 0;
    in_media = "LNK";       // m3u! -> links
  } else if (compare_mime(str->opt, stre->r_->contenttype, str->url_file, "application/x-authorware-map") != 0) {       /* macromedia aam file */
    inscript = intag = 0;
    inscript_name = "media";
    intag_start_valid = 0;
    in_media = "AAM";       // aam
  }
  /* Or a RSS file */
  else if (compare_mime(str->opt, stre->r_->contenttype, str->url_file, "text/xml") != 0
      || compare_mime(str->opt, stre->r_->contenttype, str->url_file, "application/xml") != 0) {
    if (strstr(html, "http://purl.org/rss/") != NULL)        // Hmm, this is a bit lame ; will have to cleanup
    {                       /* RSS file */
      inscript = intag = 0;
      intag_start_valid = 0;
      in_media = NULL;      // regular XML
    } else {                // cancel: write all
      html = stre->r_->adr + stre->r_->size;
      { // HT_add_adr;
        if ( (str->opt->getmode & HTS_DOWNLOAD_HTML) != 0 && *str->ptr_ > 0 ) {
          const size_t sz_ = html - lastsaved;
          if (sz_ != 0) {
            TypedArrayAppend(output_buffer, lastsaved, sz_);
            lastsaved = html;
          }
        }
      }
      lastsaved = html;
    }
  }
  // Detect UTF8 format
  //if (is_unicode_utf8(r->adr, (unsigned int) r->size) == 1) {
  //  no_esc_utf=1;
  //} else {
  //  no_esc_utf=0;
  //}

  // Hack to prevent any problems with ram files of other files
  *(stre->r_->adr + stre->r_->size) = '\0';

  // ------------------------------------------------------------
  // analyser ce qu'il y a en mémoire (fichier html)
  // on scanne les balises
  // ------------------------------------------------------------
  str->opt->state._hts_in_html_done = 0; // 0% scannés
  str->opt->state._hts_in_html_parsing = 1;      // flag pour indiquer un parsing

  stre->base[0] = '\0';           // effacer base-href
  do {
    int p = 0;
    int valid_p = 0;        // force to take p even if == 0
    int ending_p = '\0';    // ending quote?
    int archivetag_p = 0;   // avoid multiple-archives with commas
    int unquoted_script = 0;
    INSCRIPT inscript_state_pos_prev = inscript_state_pos;

    *stre->error_ = 0;

    /* Break if we are done yet */
    if (html - stre->r_->adr >= stre->r_->size)
      break;

    /*
       index.html built here
     */
    // Construction index.html (sommaire)
    // Avant de tester les a href,
    // Ici on teste si l'on doit construire l'index vers le(s) site(s) miroir(s)
    if (!*stre->makeindex_done_) {  // autoriation d'écrire un index
      if (!detect_title) {
        if (str->opt->depth == heap_(*str->ptr_)->depth) {      // on note toujours les premiers liens
          if (!in_media) {
            if (str->opt->makeindex && (*str->ptr_ > 0)) {
              if (str->opt->getmode & HTS_DOWNLOAD_HTML) {       // autorisation d'écrire
                p = strfield(html, "title");
                if (p) {
                  if (*(html - 1) == '/')
                    p = 0;  // /title
                } else {
                  if (strfield(html, "/html"))
                    p = -1; // noter, mais sans titre
                  else if (strfield(html, "body"))
                    p = -1; // noter, mais sans titre
                  else if (html - stre->r_->adr >= stre->r_->size - 1)
                    p = -1; // noter, mais sans titre
                  else if (html - stre->r_->adr >= stre->r_->size - 2)     // we got to hurry
                    p = -1; // xxc xxc xxc
                }
              } else
                p = 0;

              if (p) {      // ok center
                char INDEXfile[255];
                snprintf(INDEXfile, 255, "%s%s", StringBuff(str->opt->path_html_utf8), "index.html");
                if (*stre->makeindex_fp_ == NULL) {
                  file_notify(str->opt, "", "", INDEXfile, 1, 1, 0);
                  verif_backblue(str->opt, StringBuff(str->opt->path_html_utf8));     // générer gif
                  *stre->makeindex_fp_ =
                    filecreate(&str->opt->state.strc, INDEXfile);
                  if (*stre->makeindex_fp_ != NULL) {

                    // Header
                    fprintf(*stre->makeindex_fp_, stre->template_header_,
                            "<!-- Mirror and index made by HTTrack Website Copier/"
                            HTTRACK_VERSION " " HTTRACK_AFF_AUTHORS " -->");

                  } else
                    *stre->makeindex_done_ = -1;    // fait, erreur
                }

                if (*stre->makeindex_fp_ != NULL) {
                  char BIGSTK tempo[HTS_URLMAXSIZE];
                  char BIGSTK s[HTS_URLMAXSIZE];
                  char *a = NULL;
                  char *b = NULL;

                  s[0] = '\0';
                  if (p > 0) {
                    a = strchr(html, '>');
                    if (a != NULL) {
                      a++;
                      while(is_space(*a))
                        a++;        // sauter espaces & co
                      b = strchr(a, '<');   // prochain tag
                    }
                  }
                  if (lienrelatif(tempo, heap_(*str->ptr_)->sav, INDEXfile) == 0) {
                    detect_title = 1;       // ok détecté pour cette page!
                    (*stre->makeindex_links_)++;      // un de plus
                    strcpybuff(stre->makeindex_firstlink_, tempo);
                    //

                    /* Hack */
                    if (str->opt->mimehtml) {
                      strcpybuff(stre->makeindex_firstlink_,
                                 "cid:primary/primary");
                    }

                    if ((b == a) || (a == NULL) || (b == NULL)) {   // pas de titre
                      strcpybuff(s, tempo);
                    } else if ((b - a) < 256) {
                      b--;
                      while(is_space(*b))
                        b--;
                      strncpy(s, a, b - a + 1);
                      *(s + (b - a) + 1) = '\0';
                    }

                    // Decode title with encoding
                    if (str->page_charset_ != NULL
                        && *str->page_charset_ != '\0') {
                      char *const sUtf =
                        hts_convertStringToUTF8(s, strlen(s), str->page_charset_);
                      if (sUtf != NULL) {
                        strcpy(s, sUtf);
                        free(sUtf);
                      }
                    }

                    // Body
                    inplace_escape_uri_utf(tempo, sizeof(tempo));
                    fprintf(*stre->makeindex_fp_, stre->template_body_, tempo, s);
                  }
                }
              }
            }
          }

        } else if (heap_(*str->ptr_)->depth < str->opt->depth) {        // on a sauté level1+1 et level1
          /// FIN
          char INDEXfile[255];
          snprintf(INDEXfile, 255, "%s%s", StringBuff(str->opt->path_html_utf8), "index.html");
          if (!stre->makeindex_done_) {
            if (*stre->makeindex_fp_) {
              char BIGSTK tempo[1024];
              if (*stre->makeindex_links_ == 1) {
                char BIGSTK link_escaped[HTS_URLMAXSIZE];
                escape_uri_utf(stre->makeindex_firstlink_, link_escaped, sizeof(link_escaped));
                snprintf(tempo,sizeof(tempo),"<meta HTTP-EQUIV=\"Refresh\" CONTENT=\"0; URL=%s\">"CRLF,link_escaped);
              } else {
                tempo[0]='\0';
              }
              
              fprintf(*stre->makeindex_fp_,stre->template_footer_,
                "<!-- Mirror and index made by HTTrack Website Copier/"HTTRACK_VERSION" "HTTRACK_AFF_AUTHORS" -->",
                tempo
              );
              fflush(*stre->makeindex_fp_);
              fclose(*stre->makeindex_fp_);  /* à ne pas oublier sinon on passe une nuit blanche */
              *stre->makeindex_fp_=NULL;
              usercommand(str->opt,0,NULL,INDEXfile,"primary","primary");
            }
          }
          *stre->makeindex_done_=1;    /* ok c'est fait */  \
        }
      }                     // if (opt->makeindex)
    }
    // FIN Construction index.html (sommaire)
    /*
       end -- index.html built here
     */

    /* Parse */
    if ((*html == '<')       /* No starting tag */
        &&(!inscript)       /* Not in (java)script */
        &&(!incomment)      /* Not in comment (<!--) */
        &&(!in_media)       /* Not in media */
      ) {
      intag = 1;
      intag_ctype = 0;
      //parseall_incomment=0;
      //inquote=0;  // effacer quote
      intag_start = html;
      for(intag_name = html + 1; is_realspace(*intag_name); intag_name++) ;
      intag_start_valid = 1;
      stre->codebase[0] = '\0';   // effacer éventuel codebase

      /* Meta ? */
      if (check_tag(intag_start, "meta")) {
        int pos;

        // <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
        if ((pos = rech_tageq_all(html, "http-equiv"))) {
          const char *token = NULL;
          int len = rech_endtoken(html + pos, &token);

          if (len > 0) {
            if (strfield(token, "content-type")) {
              intag_ctype = 1;
              //NOPE-we do not convert the whole page actually
              //intag_start[1] = 'X';
            } else if (strfield(token, "refresh")) {
              intag_ctype = 2;
            }
          }
        }
      }

      if (str->opt->getmode & HTS_DOWNLOAD_HTML) {       // sauver html
        p = 0;
        switch (emited_footer) {
        case 0:
          // We are looking for the first head so that we can declare the HTTP-headers charset early
          // Emit as soon as we see the first <head>, <meta>, or <body> tag.
          // FIXME: we currently emit the tag BEFORE the <head> tag, actually, which is not clean
          if ((p = strfield(html, "<head>")) != 0
              || ((p = strfield(html, "<head")) != 0 && isspace(html[p]))
              || (p = strfield(html, "<body>")) != 0
              || ((p = strfield(html, "<body")) != 0 && isspace(html[p]))
              || ((p = strfield(html, "<meta")) != 0 && isspace(html[p]))
            ) {
            emited_footer++;
          } else {
            p = 0;
          }
          break;
        case 1:
          // And the closing comment info tag
          if ((p = strfield(html, "</html")) != 0) {
            emited_footer++;
          } else {
            p = 0;
          }
          break;
        default:
          p = 0;
          break;
        }

        if (p != 0) {
          const char *eol = "\n";

          if (strchr(stre->r_->adr, '\r'))
            eol = "\r\n";
          if (StringNotEmpty(str->opt->footer) || str->opt->urlmode != 4) {   /* != preserve */
            if (StringNotEmpty(str->opt->footer)) {
              char BIGSTK tempo[1024 + HTS_URLMAXSIZE];
              char gmttime[256];

              tempo[0] = '\0';
              time_gmt_rfc822(gmttime);
              strcatbuff(tempo, eol);
              snprintf(tempo + strlen(tempo), sizeof(tempo) - strlen(tempo),
                      StringBuff(str->opt->footer),
                      jump_identification_const(heap_(*str->ptr_)->adr), heap_(*str->ptr_)->fil, gmttime,
                      HTTRACK_VERSIONID);
              strcatbuff(tempo, eol);
              //fwrite(tempo,1,strlen(tempo),fp);
              HT_ADD(tempo);
            }
            // Emit charset ?
            if (emited_footer == 1 && strnotempty(stre->r_->charset)) {
              HT_ADD
                ("<!-- Added by HTTrack --><meta http-equiv=\"content-type\" content=\"text/html;charset=");
              HT_ADD(stre->r_->charset);
              HT_ADD("\" /><!-- /Added by HTTrack -->");
              HT_ADD(eol);
            }
          }
        }
      }
      // éliminer les <!-- (commentaires) : intag dévalidé
      if (*(html + 1) == '!')
        if (*(html + 2) == '-')
          if (*(html + 3) == '-') {
            intag = 0;
            incomment = 1;
            intag_start_valid = 0;
          }

    } else if ((*html == '>')        /* ending tag */
               &&((!inscript && !in_media) || (inscript_tag))       /* and in tag (or in script) */
      ) {
      if (inscript_tag) {
        inscript_tag = inscript = 0;
        intag = 0;
        incomment = 0;
        intag_start_valid = 0;
        intag_name = NULL;
        if (str->opt->parsedebug) {
          HT_ADD("<@@ /inscript @@>");
        }
      } else if (!incomment) {
        intag = 0;          //inquote=0;

        // entrée dans du javascript?
        // on parse ICI car il se peut qu'on ait eu a parser les src=.. dedans
        //if (!inscript) {  // sinon on est dans un obj.write("..
        if ((intag_start_valid) && (check_tag(intag_start, "script")
                                    || check_tag(intag_start, "style")
            )
          ) {
          const char *a = intag_start;    // <

          // ** while(is_realspace(*(--a)));
          if (*a == '<') {  // sûr que c'est un tag?
            if (check_tag(intag_start, "script"))
              inscript_name = "script";
            else
              inscript_name = "style";
            inscript = 1;
            inscript_state_pos = INSCRIPT_START;
            intag = 1;      // because après <script> on y est .. - pas utile
            intag_start_valid = 0;  // OUI car nous sommes dans du code, plus dans du "vrai" tag
            if (str->opt->parsedebug) {
              HT_ADD("<@@ inscript @@>");
            }
          }
        }
      } else {              /* end of comment? */
        // vérifier fermeture correcte
        if ((*(html - 1) == '-') && (*(html - 2) == '-')) {
          intag = 0;
          incomment = 0;
          intag_start_valid = 0;
          intag_name = NULL;
        }
#if GT_ENDS_COMMENT
        /* wrong comment ending */
        else {
          /* check if correct ending does not exists
             <!-- foo > example <!-- bar > is sometimes accepted by browsers
             when no --> is used somewhere else.. darn those browsers are dirty
           */
          if (!strstr(html, "-->")) {
            intag = 0;
            incomment = 0;
            intag_start_valid = 0;
            intag_name = NULL;
          }
        }
#endif
      }
      //}
    }
    //else if (*adr==34) {
    //  inquote=(inquote?0:1);
    //}
    else if (intag || inscript || in_media) {       // nous sommes dans un tag/commentaire, tester si on recoit un tag
      int p_type = 0;
      int p_nocatch = 0;
      int p_searchMETAURL = 0;      // chercher ..URL=<url>
      int add_class = 0;    // ajouter .class
      int add_class_dots_to_patch = 0;      // number of '.' in code="x.y.z<realname>"
      const char *p_flush = NULL;

      // ------------------------------------------------------------
      // parsing évolé
      // ------------------------------------------------------------
      if (((isalpha((unsigned char) *html)) || (*html == '/') || (inscript) || (in_media) || (inscriptgen))) {        // sinon pas la peine de tester..

        /* caractère de terminaison pour "miniparsing" javascript=.. ?
           (ex: <a href="javascript:()" action="foo"> ) */
        if (inscript_tag) {
          if (inscript_tag_lastc) {
            if (*html == inscript_tag_lastc) {
              /* sortir */
              inscript_tag = inscript = 0;
              incomment = 0;
              if (str->opt->parsedebug) {
                HT_ADD("<@@ /inscript @@>");
              }
            }
          }
        }

        /* automate */
        AUTOMATE_LOOKUP_CURRENT_ADR();

        // Note:
        // Certaines pages ne respectent pas le html
        // notamment les guillements ne sont pas fixés
        // Nous sommes dans un tag, donc on peut faire un test plus
        // large pour pouvoi prendre en compte ces particularités

        // à vérifier: ACTION, CODEBASE, VRML

        if (in_media) {
          if (strcmp(in_media, "LNK") == 0) {       // real media
            p = 0;
            valid_p = 1;
          } else if (strcmp(in_media, "AAM") == 0) {        // AAM
            if (is_space((unsigned char) html[0])
                && !is_space((unsigned char) html[1])) {
              const char *a = html + 1;
              int n = 0;
              int ok = 0;
              int dot = 0;

              while(n < HTS_URLMAXSIZE && a[n] != '\0'
                    && (!is_space((unsigned char) a[n]) || !(ok = 1))
                ) {
                if (a[n] == '.') {
                  dot = n;
                }
                n++;
              }
              if (ok && dot > 0) {
                char BIGSTK tmp[HTS_URLMAXSIZE + 2];

                tmp[0] = '\0';
                strncat(tmp, a + dot + 1, n - dot - 1);
                if (is_knowntype(str->opt, tmp) || ishtml_ext(tmp) != -1) {
                  html++;
                  p = 0;
                  valid_p = 1;
                  unquoted_script = 1;
                }
              }
            }
          }
        } else if (*str->ptr_ > 0) {       /* pas première page 0 (primary) */
          p = 0;            // saut pour le nom de fichier: adresse nom fichier=adr+p

          // ------------------------------
          // détection d'écriture JavaScript.
          // osons les obj.write et les obj.href=.. ! osons!
          // note: inscript==1 donc on sautera après les \"
          if (inscript) {
            if (inscriptgen) {      // on est déja dans un objet générant..
              if (*html == scriptgen_q) {    // fermeture des " ou '
                if (*(html - 1) != '\\') {   // non
                  inscriptgen = 0;  // ok parsing terminé
                }
              }
            } else {
              const char *a = NULL;
              char check_this_fking_line = 0;       // parsing code javascript..
              char must_be_terminated = 0;  // caractère obligatoire de terminaison!
              int token_size;

              if (!(token_size = strfield(html, ".writeln")))        // détection ...objet.write[ln]("code html")...
                token_size = strfield(html, ".write");
              if (token_size) {
                a = html + token_size;
                while(is_realspace(*a))
                  a++;      // sauter espaces
                if (*a == '(') {    // début parenthèse
                  check_this_fking_line = 2;        // à parser!
                  must_be_terminated = ')';
                  a++;      // sauter (
                }
              }
              // euhh ??? ???
              /* else if (strfield(adr,".href")) {  // détection ...objet.href="...
                 a=adr+5;
                 while(is_realspace(*a)) a++; // sauter espaces
                 if (*a=='=') {  // ohh un égal
                 check_this_fking_line=1;  // à noter!
                 must_be_terminated=';';   // et si t'as oublié le ; tu sais pas coder
                 a++;   // sauter =
                 }

                 } */

              // on a un truc du genre instruction"code généré" dont on parse le code
              if (check_this_fking_line) {
                while(is_realspace(*a))
                  a++;
                if ((*a == '\'') || (*a == '"')) {  // départ de '' ou ""
                  const char *b;

                  scriptgen_q = *a; // quote
                  b = a + 1;        // départ de la chaîne
                  // vérifier forme ("code") et pas ("code"+var), ingérable
                  do {
                    if (*a == scriptgen_q && *(a - 1) != '\\')      // quote non slash
                      break;        // sortie
                    else if (*a == '\n' && *(a - 1) != '\\'   /* LF and no continue (\) character */
                             && (*(a - 1) != '\r' || *(a - 2) != '\\'))     /* and not CRLF and no .. */
                      break;
                    else
                      a++;  // caractère suivant
                  } while((a - b) < HTS_URLMAXSIZE);
                  if (*a == scriptgen_q) {  // fin du quote
                    a++;
                    while(is_realspace(*a))
                      a++;
                    if (*a == must_be_terminated) { // parenthèse fermante: ("..")

                      // bon, on doit parser une ligne javascript
                      // 1) si check.. ==1 alors c'est un nom de fichier direct, donc
                      // on fixe p sur le saut nécessaire pour atteindre le nom du fichier
                      // et le moteur se débrouillera ensuite tout seul comme un grand
                      // 2) si check==2 c'est un peu plus tordu car là on génére du
                      // code html au sein de code javascript au sein de code html
                      // dans ce cas on doit fixer un flag à un puis ensuite dans la boucle
                      // on devra parser les instructions standard comme <a href etc
                      // NOTE: le code javascript autogénéré n'est pas pris en compte!!
                      // (et ne marche pas dans 50% des cas de toute facon!)
                      if (check_this_fking_line == 1) {
                        p = (int) (b - html);        // calculer saut!
                      } else {
                        inscriptgen = 1;    // SCRIPTGEN actif
                        html = b;    // jump
                      }

                      if ((str->opt->debug > 1) && (str->opt->log != NULL)) {
                        char str_[512];

                        str_[0] = '\0';
                        strncatbuff(str_, b, minimum((int) (a - b + 1), 32));
                        hts_log_print(str->opt, LOG_DEBUG,
                                      "active code (%s) detected in javascript: %s",
                                      (check_this_fking_line ==
                                       2) ? "parse" : "pickup", str_);
                      }
                    }

                  }

                }

              }
            }
          }
          // fin detection code générant javascript vers html
          // ------------------------------

          // analyse proprement dite, A HREF=.. etc..
          if (!p) {
            // si dans un tag, et pas dans un script - sauf si on analyse un obj.write("..
            if ((intag && (!inscript)) || inscriptgen) {
              if ((*(html - 1) == '<') || (is_space(*(html - 1)))) {  // <tag < tag etc
                // <A HREF=.. pour les liens HTML
                p = rech_tageq(html, "href");
                if (p) {    // href.. tester si c'est une bas href!
                  if ((intag_start_valid) && check_tag(intag_start, "base")) {      // oui!
                    // ** note: base href et codebase ne font pas bon ménage..
                    p_type = 2;     // c'est un chemin
                  }
                }

                /* Tags supplémentaires à vérifier (<img src=..> etc) */
                if (p == 0) {
                  int i = 0;

                  while((p == 0) && (strnotempty(hts_detect[i]))) {
                    p = rech_tageq(html, hts_detect[i]);
                    if (p) {
                      /* This is a temporary hack to avoid archive=foo.jar,bar.jar .. */
                      if (strcmp(hts_detect[i], "archive") == 0) {
                        archivetag_p = 1;
                      }
                    }
                    i++;
                  }
                }

                /* Tags supplémentaires en début à vérifier (<object .. hotspot1=..> etc) */
                if (p == 0) {
                  int i = 0;

                  while((p == 0) && (strnotempty(hts_detectbeg[i]))) {
                    p = rech_tageqbegdigits(html, hts_detectbeg[i]);
                    i++;
                  }
                }

                /* Tags supplémentaires à vérifier : URL=.. */
                if (p == 0) {
                  int i = 0;

                  while((p == 0) && (strnotempty(hts_detectURL[i]))) {
                    p = rech_tageq(html, hts_detectURL[i]);
                    i++;
                  }
                  if (p) {
                    if (intag_ctype == 1) {
                      p = 0;
#if 0
                      //if ((pos=rech_tageq(html, "content"))) {
                      char temp[256];
                      char *token = NULL;
                      int len = rech_endtoken(html + pos, &token);

                      if (len > 0 && len < sizeof(temp) - 2) {
                        char *chpos;

                        temp[0] = '\0';
                        strncat(temp, token, len);
                        if ((chpos = strstr(temp, "charset"))
                            && (chpos = strchr(chpos, '='))
                          ) {
                          chpos++;
                          while(is_space(*chpos))
                            chpod++;
                          //chpos
                        }
                      }
#endif
                    }
                    // <META HTTP-EQUIV="Refresh" CONTENT="3;URL=http://www.example.com">
                    else if (intag_ctype == 2) {
                      p_searchMETAURL = 1;
                    } else {
                      p = 0;        /* cancel */
                    }
                  }

                }

                /* Tags supplémentaires à vérifier, mais à ne pas capturer */
                if (p == 0) {
                  int i = 0;

                  while((p == 0) && (strnotempty(hts_detectandleave[i]))) {
                    p = rech_tageq(html, hts_detectandleave[i]);
                    i++;
                  }
                  if (p)
                    p_nocatch = 1;  /* ne pas rechercher */
                }

                /* Evénements */
                if (p == 0 && !inscript     /* we don't want events inside document.write */
                  ) {
                  int i = 0;

                  /* détection onLoad etc */
                  while((p == 0) && (strnotempty(hts_detect_js[i]))) {
                    p = rech_tageq(html, hts_detect_js[i]);
                    i++;
                  }
                  /* non détecté - détecter également les onXxxxx= */
                  if (p == 0) {
                    if ((*html == 'o') && (*(html + 1) == 'n')
                        && isUpperLetter(*(html + 2))) {
                      p = 0;
                      while(isalpha((unsigned char) html[p]) && (p < 64))
                        p++;
                      if (p < 64) {
                        while(is_space(html[p]))
                          p++;
                        if (html[p] == '=')
                          p++;
                        else
                          p = 0;
                      } else
                        p = 0;
                    }
                  }
                  /* OK, événement repéré */
                  if (p) {
                    inscript_tag_lastc = *(html + p);        /* à attendre à la fin */
                    html += p /*+ 1*/;   /* saut */
                    /*
                       On est désormais dans du code javascript
                     */
                    inscript_name = "";
                    inscript = inscript_tag = 1;
                    inscript_state_pos = INSCRIPT_START;
                    if (str->opt->parsedebug) {
                      HT_ADD("<@@ inscript @@>");
                    }
                  }
                  p = 0;    /* quoi qu'il arrive, ne rien démarrer ici */
                }
                // <APPLET CODE=.. pour les applet java.. [CODEBASE (chemin..) à faire]
                if (p == 0) {
                  p = rech_tageq(html, "code");
                  if (p) {
                    if ((intag_start_valid) && check_tag(intag_start, "applet")) {  // dans un <applet !
                      p_type = -1;  // juste le nom de fichier+dossier, écire avant codebase
                      add_class = 1;        // ajouter .class au besoin

                      // vérifier qu'il n'y a pas de codebase APRES
                      // sinon on swappe les deux.
                      // pas très propre mais c'est ce qu'il y a de plus simple à faire!!

                      {
                        const char *a;

                        a = html;
                        while((*a) && (*a != '>')
                              && (!rech_tageq(a, "codebase")))
                          a++;
                        if (rech_tageq(a, "codebase")) {    // banzai! codebase=
                          char *b;

                          b = strchr(a, '>');
                          if (b != NULL) {
                            if (b - html < 1000) { // au total < 1Ko
                              char BIGSTK tempo[HTS_URLMAXSIZE];
                              const size_t offset = html - stre->r_->adr;
                              char *const modify = &stre->r_->adr[offset];
                              assertf(modify == html);

                              tempo[0] = '\0';
                              strncatbuff(tempo, a, b - a);
                              strcatbuff(tempo, " ");
                              strncatbuff(tempo, html, a - html - 1);
                              // éventuellement remplire par des espaces pour avoir juste la taille
                              while(strlen(tempo) < (size_t) (b - html))
                                strcatbuff(tempo, " ");
                              // pas d'erreur?
                              if (strlen(tempo) == b - html) {
                                strncpy(modify, tempo, strlen(tempo)); // PAS d'octet nul à la fin!
                                p = 0;      // DEVALIDER!!
                                p_type = 0;
                                add_class = 0;
                              }
                            }
                          }
                        }
                      }

                    }
                  }
                }
                // liens à patcher mais pas à charger (ex: codebase)
                if (p == 0) {       // note: si non chargé (ex: ignorer .class) patché tout de même
                  p = rech_tageq(html, "codebase");
                  if (p) {
                    if ((intag_start_valid) && check_tag(intag_start, "applet")) {  // dans un <applet !
                      p_type = -2;
                    } else
                      p = -1;       // ne plus chercher
                  }
                }

                // Meta tags pour robots
                if (p == 0) {
                  if (str->opt->robots) {
                    if ((intag_start_valid)
                        && check_tag(intag_start, "meta")) {
                      if (rech_tageq(html, "name")) {        // name=robots.txt
                        char tempo[1100];
                        char *a;

                        tempo[0] = '\0';
                        a = strchr(html, '>');
#if DEBUG_ROBOTS
                        printf("robots.txt meta tag detected\n");
#endif
                        if (a) {
                          if (a - html < 999) {
                            strncatbuff(tempo, html, a - html);
                            if (strstrcase(tempo, "content")) {
                              if (strstrcase(tempo, "robots")) {
                                if (strstrcase(tempo, "nofollow")) {
#if DEBUG_ROBOTS
                                  printf
                                    ("robots.txt meta tag: nofollow in %s%s\n",
                                     urladr(), urlfil());
#endif
                                  nofollow = 1;     // NE PLUS suivre liens dans cette page
                                  hts_log_print(str->opt, LOG_WARNING,
                                                "Link %s%s not scanned (follow robots meta tag)",
                                                heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil);
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
                // entrée dans une applet javascript
                /*if (!inscript) {  // sinon on est dans un obj.write("..
                   if (p==0)
                   if (rech_sampletag(html,"script"))
                   if (check_tag(intag_start,"script")) {
                   inscript=1;
                   }
                   } */

                // Ici on procède à une analyse du code javascript pour tenter de récupérer
                // certains fichiers évidents.
                // C'est devenu obligatoire vu le nombre de pages qui intègrent
                // des images réactives par exemple
              }
            } else if (inscript) {

#if 0
              /* Check // javascript comments */
              if (*html == 10 || *html == 13) {
                inscript_check_comments = 1;
                inscript_in_comments = 0;
              } else if (inscript_check_comments) {
                if (!is_realspace(*html)) {
                  inscript_check_comments = 0;
                  if (html[0] == '/' && html[1] == '/') {
                    inscript_in_comments = 1;
                  }
                }
              }
#endif

              /* Parse */
              assertf(inscript_name != NULL);
              if (*html == '/'
                  &&
                  ((strfield(html, "/script")
                    && strfield(inscript_name, "script"))
                   || (strfield(html, "/style")
                       && strfield(inscript_name, "style"))
                  )
                  && inscript_locked == 0) {
                const char *a = html;

                //while(is_realspace(*(--a)));
                while(is_realspace(*a))
                  a--;
                a--;
                if (*a == '<') {    // sûr que c'est un tag?
                  inscript = 0;
                  if (str->opt->parsedebug) {
                    HT_ADD("<@@ /inscript @@>");
                  }
                }
              } else if (inscript_state_pos ==
                         INSCRIPT_START /*!inscript_in_comments */ ) {
                /*
                   Script Analyzing - different types supported:
                   foo="url"
                   foo("url") or foo(url)
                   foo "url"
                 */
                char expected = '=';        // caractère attendu après
                const char *expected_end = ";";
                int can_avoid_quotes = 0;
                char quotes_replacement = '\0';
                int ensure_not_mime = 0;

                if (inscript_tag)
                  expected_end = ";\"\'";   // voir a href="javascript:doc.location='foo'"

                /* Can we parse javascript ? */
                if ((str->opt->parsejava & HTSPARSE_NO_JAVASCRIPT) == 0) {
                  int nc;

                  nc = strfield(html, ".src");       // nom.src="image";
                  if (!nc && inscript_tag && inscript_tag_lastc == *(html - 1))
                    nc = strfield(html, "src");       // onXXX='src="image";'
                  if (!nc)
                    nc = strfield(html, ".location");        // document.location="doc"
                  if (!nc)
                    nc = strfield(html, ":location");        // javascript:location="doc"
                  if (!nc) {        // location="doc"
                    if ((nc = strfield(html, "location"))
                        && !isspace(*(html - 1))
                      )
                      nc = 0;
                  }
                  if (!nc)
                    nc = strfield(html, ".href");    // document.location="doc"
                  if (!nc)
                    if ((nc = strfield(html, ".open"))) {    // window.open("doc",..
                      expected = '(';       // parenthèse
                      expected_end = "),";  // fin: virgule ou parenthèse
                      ensure_not_mime = 1;  //* ensure the url is not a mime type */
                    }
                  if (!nc)
                    if ((nc = strfield(html, ".replace"))) { // window.replace("url")
                      expected = '(';       // parenthèse
                      expected_end = ")";   // fin: parenthèse
                    }
                  if (!nc)
                    if ((nc = strfield(html, ".link"))) {    // window.link("url")
                      expected = '(';       // parenthèse
                      expected_end = ")";   // fin: parenthèse
                    }
                  if (!nc && (nc = strfield(html, "url")) && (!isalnum(*(html - 1))) && *(html - 1) != '_') {  // url(url)
                    expected = '('; // parenthèse
                    expected_end = ")";     // fin: parenthèse
                    can_avoid_quotes = 1;
                    quotes_replacement = ')';
                  }
                  if (!nc)
                    if ((nc = strfield(html, "import"))) {   // import "url"
                      if (is_space(*(html + nc))) {
                        expected = 0;       // no char expected
                      } else
                        nc = 0;
                    }
                  if (nc) {
                    const char *a;

                    a = html + nc;
                    while(is_realspace(*a))
                      a++;
                    if ((*a == expected) || (!expected)) {
                      if (expected)
                        a++;
                      while(is_realspace(*a))
                        a++;
                      if ((*a == 34) || (*a == '\'') || (can_avoid_quotes)) {
                        const char *b, *c;
                        int ndelim = 1;

                        if ((*a == 34) || (*a == '\''))
                          a++;
                        else
                          ndelim = 0;
                        b = a;
                        if (ndelim) {
                          while((*b != 34) && (*b != '\'') && (*b != '\0'))
                            b++;
                        } else {
                          while((*b != quotes_replacement) && (*b != '\0'))
                            b++;
                        }
                        c = b--;
                        c += ndelim;
                        while(*c == ' ')
                          c++;
                        if ((strchr(expected_end, *c)) || (*c == '\n')
                            || (*c == '\r')) {
                          c -= (ndelim + 1);
                          if ((int) (c - a + 1)) {
                            if (ensure_not_mime) {
                              int i = 0;

                              while(a != NULL && hts_main_mime[i] != NULL
                                    && hts_main_mime[i][0] != '\0') {
                                int pp;

                                if ((pp = strfield(a, hts_main_mime[i]))
                                    && a[pp] == '/') {
                                  a = NULL;
                                }
                                i++;
                              }
                            }
                            // Check for bogus links (Vasiliy)
                            if (a != NULL) {
                              const size_t size = c - a + 1;
                              int i;
                              int first = 1;

                              for(i = 0; i < size; i++) {
                                // Suspicious (in code ?), abort.
                                if (a[i] == ',' || a[i] == ';') {
                                  if (first) {
                                    a = NULL;
                                    break;
                                  }
                                }
                                // Suspicious, abort.
                                else if (a[i] == '"' || a[i] == '\''
                                         || a[i] == '\t' || a[i] == '\r'
                                         || a[i] == '\n') {
                                  a = NULL;
                                  break;
                                } else if (a[i] != ' ') {
                                  first = 0;
                                }
                              }
                            }
                            if (a != NULL) {
                              if ((str->opt->debug > 1) && (str->opt->log != NULL)) {
                                char str_[512];

                                str_[0] = '\0';
                                strncatbuff(str_, a,
                                            minimum((int) (c - a + 1), 32));
                                hts_log_print(str->opt, LOG_DEBUG,
                                              "link detected in javascript: %s",
                                              str_);
                              }
                              p = (int) (a - html);  // p non nul: TRAITER CHAINE COMME FICHIER
                              if (can_avoid_quotes) {
                                ending_p = quotes_replacement;
                              }
                            }
                          }
                        }

                      }
                    }
                  }

                }
                /* HTSPARSE_NO_JAVASCRIPT */
              }
            }
          }

        } else {            // ptr == 0
          //p=rech_tageq(adr,"primary");    // lien primaire, yeah
          p = 0;            // No stupid tag anymore, raw link
          valid_p = 1;      // Valid even if p==0
          while((html[p] == '\r') || (html[p] == '\n'))
            p++;
          //can_avoid_quotes=1;
          ending_p = '\r';
        }

      } else if (isspace((unsigned char) *html)) {
        intag_startattr = html + 1;  // attribute in tag (for dirty parsing)
      }

      // ------------------------------------------------------------
      // dernier recours - parsing "sale" : détection systématique des .gif, etc.
      // risque: générer de faux fichiers parazites
      // fix: ne parse plus dans les commentaires
      // ------------------------------------------------------------
      if (str->opt->parseall && (str->opt->parsejava & HTSPARSE_NO_AGGRESSIVE) == 0 && (*str->ptr_ > 0) && (!in_media) /* && (!inscript_in_comments) */ ) {  // option parsing "brut"
        //int incomment_justquit=0;
        if (!is_realspace(*html)) {
          int noparse = 0;

          // Gestion des /* */
#if 0
          if (inscript) {
            if (parseall_incomment) {
              if ((*html == '/') && (*(html - 1) == '*'))
                parseall_incomment = 0;
              incomment_justquit = 1;       // ne pas noter dernier caractère
            } else {
              if ((*html == '/') && (*(html + 1) == '*'))
                parseall_incomment = 1;
            }
          } else
            parseall_incomment = 0;
#endif
          /* ensure automate state  0 (not in comments, quotes..) */
          if (inscript
              && (inscript_state_pos != INSCRIPT_INQUOTE
                  && inscript_state_pos != INSCRIPT_INQUOTE2)) {
            noparse = 1;
          }

          /* vérifier que l'on est pas dans un <!-- --> pur */
          if ((!intag) && (incomment) && (!inscript))
            noparse = 1;    /* commentaire */

          // recherche d'URLs
          if (!noparse) {
            //if ((!parseall_incomment) && (!noparse)) {
            if (!p) {       // non déja trouvé
              if (html != stre->r_->adr) {  // >1 caractère
                // scanner les chaines
                if ((*html == '\"') || (*html == '\'')) {     // "xx.gif" 'xx.gif'
                  if (strchr("=(,", parseall_lastc)) {      // exemple: a="img.gif.. (handles comments)
                    const char *a = html;
                    char stop = *html;       // " ou '
                    int count = 0;

                    // sauter caractères
                    a++;
                    // copier
                    while((*a) && (*a != '\'') && (*a != '\"')
                          && (count < HTS_URLMAXSIZE)) {
                      count++;
                      a++;
                    }

                    // ok chaine terminée par " ou '
                    if ((*a == stop) && (count < HTS_URLMAXSIZE)
                        && (count > 0)) {
                      char c;

                      //char* aend;
                      //
                      //aend=a;     // sauver début
                      a++;
                      while(is_taborspace(*a))
                        a++;
                      c = *a;
                      if (strchr("),;>/+\r\n", c)) {        // exemple: ..img.gif";
                        // le / est pour funct("img.gif" /* URL */);
                        char BIGSTK tempo[HTS_URLMAXSIZE];
                        char type[256];
                        int url_ok = 0;     // url valide?

                        tempo[0] = '\0';
                        type[0] = '\0';
                        //
                        strncatbuff(tempo, html + 1, count);
                        //
                        if ((!strchr(tempo, ' ')) || inscript) {    // espace dedans: méfiance! (sauf dans code javascript)
                          int invalid_url = 0;

                          // escape
                          unescape_amp(tempo);

                          // Couper au # ou ? éventuel
                          {
                            char *aa = strchr(tempo, '#');

                            if (aa)
                              *aa = '\0';
                            aa = strchr(tempo, '?');
                            if (aa)
                              *aa = '\0';
                          }

                          // vérifier qu'il n'y a pas de caractères spéciaux
                          if (!strnotempty(tempo))
                            invalid_url = 1;
                          else if (strchr(tempo, '*')
                                   || strchr(tempo, '<')
                                   || strchr(tempo, '>')
                                   || strchr(tempo, ',')    /* list of files ? */
                                   ||strchr(tempo, '\"')    /* potential parsing bug */
                                   ||strchr(tempo, '\'')    /* potential parsing bug */
                            )
                            invalid_url = 1;
                          else if (tempo[0] == '.' && isalnum(tempo[1]))    // ".gif"
                            invalid_url = 1;

                          /* non invalide? */
                          if (!invalid_url) {
                            // Un plus à la fin? Alors ne pas prendre sauf si extension ("/toto.html#"+tag)
                            if (c != '+') { // PAS de plus à la fin
#if 0
                              char *a;
#endif
                              // "Comparisons of scheme names MUST be case-insensitive" (RFC2616)
                              if ((strfield(tempo, "http:"))
                                  || (strfield(tempo, "ftp:"))
#if HTS_USEOPENSSL
                                  || (strfield(tempo, "https:")
                                  )
#endif
                                )   // ok pas de problème
                                url_ok = 1;
                              else if (tempo[strlen(tempo) - 1] == '/') {   // un slash: ok..
                                if (inscript)       // sinon si pas javascript, méfiance (répertoire style base?)
                                  url_ok = 1;
                              }
#if 0
                              else if ((a = strchr(tempo, '/'))) {  // un slash: ok..
                                if (inscript) {     // sinon si pas javascript, méfiance (style "text/css")
                                  if (strchr(a + 1, '/'))   // un seul / : abandon (STYLE type='text/css')
                                    if (!strchr(tempo, ' '))        // avoid spaces (too dangerous for comments)
                                      url_ok = 1;
                                }
                              }
#endif
                            }
                            // Prendre si extension reconnue
                            if (!url_ok) {
                              get_httptype(str->opt, type, tempo, 0);
                              if (strnotempty(type))        // type reconnu!
                                url_ok = 1;
                              else if (is_dyntype(get_ext(OPT_GET_BUFF(str->opt), OPT_GET_BUFF_SIZE(str->opt), tempo)))       // reconnu php,cgi,asp..
                                url_ok = 1;
                              // MAIS pas les foobar@aol.com !!
                              if (strchr(tempo, '@'))
                                url_ok = 0;
                            }
                            //
                            // Ok, cela pourrait être une URL
                            if (url_ok) {

                              // Check if not fodbidden tag (id,name..)
                              if (intag_start_valid) {
                                if (intag_start)
                                  if (intag_startattr)
                                    if (intag)
                                      if (!inscript)
                                        if (!incomment) {
                                          int i = 0, nop = 0;

                                          while((nop == 0)
                                                &&
                                                (strnotempty
                                                 (hts_nodetect[i]))) {
                                            nop =
                                              rech_tageq(intag_startattr,
                                                         hts_nodetect[i]);
                                            i++;
                                          }
                                          // Forbidden tag
                                          if (nop) {
                                            url_ok = 0;
                                            hts_log_print(str->opt, LOG_DEBUG,
                                                          "dirty parsing: bad tag avoided: %s",
                                                          hts_nodetect[i - 1]);
                                          }
                                        }
                              }

                              // Accepter URL, on la traitera comme une URL normale!!
                              if (url_ok) {
                                valid_p = 1;
                                p = 0;
                              }

                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }               // p == 0

          }                 // not in comment

          // plus dans un commentaire
          if (inscript_state_pos == INSCRIPT_START
              && inscript_state_pos_prev == INSCRIPT_START) {
            parseall_lastc = *html;  // caractère avant le prochain
          }

        }                   // if realspace
      }                     // if parseall

      // ------------------------------------------------------------
      // p!=0 : on a repéré un éventuel lien
      // ------------------------------------------------------------
      //
      if ((p > 0) || (valid_p)) {   // on a repéré un lien
        //int lien_valide=0;
        const char *eadr = NULL;  /* fin de l'URL */

        //char* quote_adr=NULL;     /* adresse du ? dans l'adresse */
        int ok = 1;
        char quote = '\0';
        int quoteinscript = 0;
        int noquote = 0;
        const char *tag_attr_start = html;

        // si nofollow ou un stop a été déclenché, réécrire tous les liens en externe
        if ((nofollow)
            || (str->opt->state.stop
                && /* force follow not to lose previous cache data */
                !str->opt->is_update)
          )
          p_nocatch = 1;

        // écrire codebase avant, flusher avant code
        if ((p_type == -1) || (p_type == -2)) {
          if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
            HT_add_adr;     // refresh
          }
          lastsaved = html;  // dernier écrit+1
        }
        // sauter espaces
        // adr+=p;
        INCREMENT_CURRENT_ADR(p);
        while((is_space(*html)
               || (inscriptgen && html[0] == '\\' && is_space(html[1])
               )
              )
              && quote == '\0') {
          if (!quote)
            if ((*html == '\"') || (*html == '\'')) {
              quote = *html; // on doit attendre cela à la fin
              if (inscriptgen && *(html - 1) == '\\') {
                quoteinscript = 1;  /* will wait for \" */
              }
            }
          // puis quitter
          // html++;    // sauter les espaces, "" et cie
          INCREMENT_CURRENT_ADR(1);
        }

        /* Stop at \n (LF) if primary links or link lists */
        if (*str->ptr_ == 0 || (in_media && strcmp(in_media, "LNK") == 0))
          quote = '\n';
        /* s'arrêter que ce soit un ' ou un " : pour document.write('<img src="foo'+a); par exemple! */
        else if (inscript && !unquoted_script)
          noquote = 1;

        // sauter éventuel \" ou \' javascript
        if (inscript) {     // on est dans un obj.write("..
          if (*html == '\\') {
            if ((*(html + 1) == '\'') || (*(html + 1) == '"')) {      // \" ou \'
              // html+=2;    // sauter
              INCREMENT_CURRENT_ADR(2);
            }
          }
        }
        // sauter content="1;URL=http://..
        if (p_searchMETAURL) {
          int l = 0;

          while((html + l + 4 < stre->r_->adr + stre->r_->size)
                && (!strfield(html + l, "URL="))
                && (l < 128))
            l++;
          if (!strfield(html + l, "URL="))
            ok = -1;
          else
            html += (l + 4);
        }

        /* éviter les javascript:document.location=.. : les parser, plutôt */
        if (ok != -1) {
          if (strfield(html, "javascript:")
              && !inscript  /* we don't want to parse 'javascript:' inside document.write inside scripts */
            ) {
            ok = -1;
            /*
               On est désormais dans du code javascript
             */
            inscript_name = "";
            inscript_tag = inscript = 1;
            inscript_state_pos = INSCRIPT_START;
            inscript_tag_lastc = quote;     /* à attendre à la fin */
            if (str->opt->parsedebug) {
              HT_ADD("<@@ inscript @@>");
            }
          }
        }

        if (p_type == 1) {
          if (*html == '#') {
            html++;          // sauter # pour usemap etc
          }
        }
        eadr = html;

        // ne pas flusher après code si on doit écrire le codebase avant!
        if ((p_type != -1) && (p_type != 2) && (p_type != -2)) {
          if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
            HT_add_adr;     // refresh
          }
          lastsaved = html;  // dernier écrit+1
          // après on écrira soit les données initiales,
          // soir une URL/lien modifié!
        } else if (p_type == -1)
          p_flush = html;    // flusher jusqu'à adr ensuite

        if (ok != -1) {     // continuer
          // découper le lien
          do {
            if ((unsigned char) *eadr < 32) {   // caractère de contrôle (ou \0)
              if (!is_space(*eadr))
                ok = 0;
            }
            if (eadr - html > HTS_URLMAXSIZE)    // ** trop long, >HTS_URLMAXSIZE caractères (on prévoit HTS_URLMAXSIZE autres pour path)
              ok = -1;      // ne pas traiter ce lien

            if (ok > 0) {
              //if (*eadr!=' ') {
              if (is_space(*eadr)) {        // guillemets,CR, etc
                if ((*eadr == quote && (!quoteinscript || *(eadr - 1) == '\\'))     // end quote
                    || (noquote && (*eadr == '\"' || *eadr == '\''))        // end at any quote
                    || (!noquote && quote == '\0' && is_realspace(*eadr))   // unquoted href
                  )         // si pas d'attente de quote spéciale ou si quote atteinte
                  ok = 0;
              } else if (ending_p && (*eadr == ending_p))
                ok = 0;
              else {
                switch (*eadr) {
                case '>':
                  if (!quote) {
                    if (!inscript && !in_media) {
                      intag = 0;    // PLUS dans un tag!
                      intag_start_valid = 0;
                      intag_name = NULL;
                    }
                    ok = 0;
                  }
                  break;
                  /*case '<': */
                case '#':
                  if (*(eadr - 1) != '&')   // &#40;
                    ok = 0;
                  break;
                  // case '?': non!
                case '\\':
                  if (inscript)
                    ok = 0;
                  break;    // \" ou \' point d'arrêt
                case '?':  /*quote_adr=adr; */
                  break;    // noter position query
                }
              }
              //}
            }
            eadr++;
          } while(ok == 1);

          // Empty link detected
          if (eadr - html <= 1) {        // link empty
            ok = -1;        // No
            if (*html != '#') {      // Not empty+unique #
              if (eadr - html == 1) {    // 1=link empty with delim (end_adr-start_adr)
                if (quote) {
                  if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
                    HT_ADD("#");    // We add this for a <href="">
                  }
                }
              }
            }
          }
          // This is a dirty and horrible hack to avoid parsing an Adobe GoLive bogus tag
          if (strfield(html, "(Empty Reference!)")) {
            ok = -1;        // No
          }

        }

        if (ok == 0) {      // tester un lien
          char BIGSTK lien[HTS_URLMAXSIZE];
          int meme_adresse = 0;     // 0 par défaut pour primary

          //char *copie_de_adr=html;
          //char* p;

          // construire lien (découpage)
          if (eadr - html - 1 < HTS_URLMAXSIZE) {        // pas trop long?
            strncpy(lien, html, eadr - html - 1);
            lien[eadr - html - 1] = '\0';
            //printf("link: %s\n",lien);
            // supprimer les espaces
            while((lien[strlen(lien) - 1] == ' ') && (strnotempty(lien)))
              lien[strlen(lien) - 1] = '\0';

          } else
            lien[0] = '\0'; // erreur

          // ------------------------------------------------------
          // Lien repéré et extrait
          if (strnotempty(lien) > 0) {      // construction du lien
            lien_adrfilsave afs;
            int forbidden_url = -1; // lien non interdit (mais non autorisé..)
            int just_test_it = 0;   // mode de test des liens
            int set_prio_to = 0;    // pour capture de page isolée
            int import_done = 0;    // lien importé (ne pas scanner ensuite *à priori*)

            //
            afs.af.adr[0] = '\0';
            afs.af.fil[0] = '\0';
            afs.save[0] = '\0';
            //
            // 0: autorisé
            // 1: interdit (patcher tout de même adresse)

            hts_log_print(str->opt, LOG_DEBUG, "link detected in html (tag): %s",
                          lien);

            // external check
            if (!RUN_CALLBACK1(str->opt, linkdetected, lien)
                || !RUN_CALLBACK2(str->opt, linkdetected2, lien, intag_start)) {
              *stre->error_ = 1;    // erreur
              hts_log_print(str->opt, LOG_ERROR,
                            "Link %s refused by external wrapper", lien);
            }
#if HTS_STRIP_DOUBLE_SLASH
            // supprimer les // en / (sauf pour http://)
            if (opt->urlhack) {
              char *a, *p, *q;
              int done = 0;

              a = strchr(lien, ':');        // http://
              if (a) {
                a++;
                while(*a == '/')
                  a++;      // position après http://
              } else {
                a = lien;   // début
                while(*a == '/')
                  a++;      // position après http://
              }
              q = strchr(a, '?');   // ne pas traiter après '?'
              if (!q)
                q = a + strlen(a) - 1;
              while((p = strstr(a, "//")) && (!done)) {     // remplacer // par /
                if (p > q) {    // après le ? (toto.cgi?param=1//2.3)
                  done = 1; // stopper
                } else {
                  char BIGSTK tempo[HTS_URLMAXSIZE];

                  tempo[0] = '\0';
                  strncatbuff(tempo, a, p - a);
                  strcatbuff(tempo, p + 1);
                  strcpybuff(a, tempo);     // recopier
                }
              }
            }
#endif

            // purger espaces de début et fin, CR,LF résiduels
            // (IMG SRC="foo.<\n><\t>gif<\t>")
            {
              char *a = lien;
              size_t llen;

              // strip ending spaces
              llen = (*a != '\0') ? strlen(a) : 0;
              while(llen > 0 && is_realspace(lien[llen - 1])) {
                a[--llen] = '\0';
              }
              //  skip leading ones
              while(is_realspace(*a))
                a++;
              // strip cr, lf, tab inside URL
              llen = 0;
              while(*a) {
                if (*a != '\n' && *a != '\r' && *a != '\t') {
                  lien[llen++] = *a;
                }
                a++;
              }
              lien[llen] = '\0';
            }

            // commas are forbidden
            if (archivetag_p) {
              if (strchr(lien, ',')) {
                *stre->error_ = 1;  // erreur
                hts_log_print(str->opt, LOG_DEBUG,
                              "link rejected (multiple-archive) %s", lien);
              }
            }

            /* Unescape/escape %20 and other &nbsp; */
            {
              // Note: always true (iso-8859-1 as default)
              const char *const charset = str->page_charset_;
              const int hasCharset = charset != NULL
                && *charset != '\0';
              char BIGSTK query[HTS_URLMAXSIZE];

              // cut query string
              {
                char *const a = strchr(lien, '?');
                if (a != NULL) {
                  strcpybuff(query, a);
                  *a = '\0';
                } else {
                  query[0] = '\0';
                }
              }

              // Unescape %XX, but not yet high-chars (supposedly encoded with UTF-8)
              strcpybuff(lien,
                unescape_http_unharm(catbuff, sizeof(catbuff), lien, 1 | 2));     /* note: '%' is still escaped */

              // Force to encode non-printable chars (should never happend)
              escape_remove_control(lien);
              
              // charset conversion for the URI filename,
              // and not already UTF-8
              // (note: not for the query string!)
              if (hasCharset && !hts_isCharsetUTF8(charset)) {
                char *const s = hts_convertStringToUTF8(lien, strlen(lien), charset);
                if (s != NULL) {
                  hts_log_print(str->opt, LOG_DEBUG,
                    "engine: save-name: '%s' charset conversion from '%s' to '%s'",
                    charset, lien, s);
                  strcpybuff(lien, s);
                  free(s);
                }
              }

              // decode URI entities with UTF-8 charset
              if (hts_unescapeEntities(lien, lien, strlen(lien) + 1) != 0) {
                hts_log_print(str->opt, LOG_WARNING,
                  "could not decode URI '%s' with charset '%s'", lien, charset);
              }

              // decode query string entities with page charset
              if (hasCharset) {
                if (hts_unescapeEntitiesWithCharset(query,
                                                    query, strlen(query) + 1,
                                                    charset) != 0) {
                    hts_log_print(str->opt, LOG_WARNING,
                      "could not decode query string '%s' with charset '%s'", query, charset);
                }
              }

              // Decode remaining %XX high characters with UTF-8
              // but only when this leads to valid UTF-8.
              // Otherwise, leave them unescaped.
              if (hts_unescapeUrlSpecial(lien, catbuff, sizeof(catbuff),
                                         UNESCAPE_URL_NO_ASCII) == 0) {
                strcpybuff(lien, catbuff);
              } else {
                hts_log_print(str->opt, LOG_WARNING,
                  "could not URL-decode string '%s'", lien);
              }

              // we need to encode query string non-ascii chars,
              // leaving the encoding as-is (unlike the file part)
              // and copy back query
              append_escape_check_url(query, lien, sizeof(lien));
            }

            // convertir les éventuels \ en des / pour éviter des problèmes de reconnaissance!
            {
              char *a;

              for(a = jump_identification(lien); *a != '\0' && *a != '?';
                  a++) {
                if (*a == '\\') {
                  *a = '/';
                }
              }
            }

            // supprimer le(s) ./
            while((lien[0] == '.') && (lien[1] == '/')) {
              char BIGSTK tempo[HTS_URLMAXSIZE];

              strncpy(tempo, lien + 2, sizeof(lien) - 2);
              //strcpybuff(tempo, lien + /* ./ */ 2); dangeureux vu que lien + 2 degenere en *char
              strcpybuff(lien, tempo);
            }
            if (strnotempty(lien) == 0)     // sauf si plus de nom de fichier
              strcpybuff(lien, "./");

            // vérifie les /~machin -> /~machin/
            // supposition dangereuse?
            // OUI!!
#if HTS_TILDE_SLASH
            if (lien[strlen(lien) - 1] != '/') {
              char *a = lien + strlen(lien) - 1;

              // éviter aussi index~1.html
              while(a > lien && (*a != '~') && (*a != '/')
                    && (*a != '.'))
                a--;
              if (*a == '~') {
                strcatbuff(lien, "/");      // ajouter slash
              }
            }
#endif

            // APPLET CODE="mixer.MixerApplet.class" --> APPLET CODE="mixer/MixerApplet.class"
            // yes, this is dirty
            // but I'm so lazzy..
            // and besides the java "code" convention is really a pain in html code
            if (p_type == -1) {
              char *a = strrchr(lien, '.');

              add_class_dots_to_patch = 0;
              if (a) {
                char *b;

                do {
                  b = strchr(lien, '.');
                  if ((b != a) && (b)) {
                    add_class_dots_to_patch++;
                    *b = '/';
                  }
                } while((b != a) && (b));
              }
            }
            // éliminer les éventuels :80 (port par défaut!)
            if (link_has_authority(lien)) {
              char *a;

              a = strstr(lien, "//");       // "//" authority
              if (a)
                a += 2;
              else
                a = lien;
              // while((*a) && (*a!='/') && (*a!=':')) a++;
              a = jump_toport(a);
              if (a) {      // port
                int port = 0;
                int defport = 80;
                char *b = a + 1;

#if HTS_USEOPENSSL
                // FIXME
                //if (strfield(adr, "https:")) {
                //}
#endif
                while(isdigit((unsigned char) *b)) {
                  port *= 10;
                  port += (int) (*b - '0');
                  b++;
                }
                if (port == defport) {      // port 80, default - c'est débile
                  char BIGSTK tempo[HTS_URLMAXSIZE];

                  tempo[0] = '\0';
                  strncatbuff(tempo, lien, a - lien);
                  strcatbuff(tempo, a + 3); // sauter :80
                  strcpybuff(lien, tempo);
                }
              }
            }
            // filtrer les parazites (mailto & cie)
            /*
               if (strfield(lien,"mailto:")) {  // ne pas traiter
               error=1;
               } else if (strfield(lien,"news:")) {  // ne pas traiter
               error=1;
               }
             */

            // vérifier que l'on ne doit pas ajouter de .class
            if (!*stre->error_) {
              if (add_class) {
                char *a = lien + strlen(lien) - 1;

                while((a > lien) && (*a != '/') && (*a != '.'))
                  a--;
                if (*a != '.')
                  strcatbuff(lien, ".class");       // ajouter .class
                else if (!strfield2(a, ".class"))
                  strcatbuff(lien, ".class");       // idem
              }
            }
            // si c'est un chemin, alors vérifier (toto/toto.html -> http://www/toto/)
            if (!*stre->error_) {
              hts_log_print(str->opt, LOG_DEBUG, "position link check %s", lien);

              if ((p_type == 2) || (p_type == -2)) {        // code ou codebase
                // Vérifier les codebase=applet (au lieu de applet/)
                if (p_type == -2) { // codebase
                  if (strnotempty(lien)) {
                    if (lien[strlen(lien) - 1] != '/') {     // pas répertoire
                      strcatbuff(lien, "/");
                    }
                  }
                }

                /* base has always authority */
                if (p_type == 2 && !link_has_authority(lien)) {
                  char BIGSTK tmp[HTS_URLMAXSIZE];

                  strcpybuff(tmp, "http://");
                  strcatbuff(tmp, lien);
                  strcpybuff(lien, tmp);
                }

                /* only one ending / (bug on some pages) */
                if (strlen(lien) > 2) {
                  size_t len = strlen(lien);

                  while(len > 1 && lien[len - 1] == '/' && lien[len - 2] == '/')    /* double // (bug) */
                    lien[--len] = '\0';
                }
                // copier nom host si besoin est
                if (!link_has_authority(lien)) {    // pas de http://
                  lien_adrfil af2;   // ** euh ident_url_relatif??

                  if (ident_url_relatif(lien, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, &af2) < 0) {
                    *stre->error_ = 1;
                  } else {
                    strcpybuff(lien, "http://");
                    strcatbuff(lien, af2.adr);
                    if (*af2.fil != '/')
                      strcatbuff(lien, "/");
                    strcatbuff(lien, af2.fil);
                    {
                      char *a;

                      a = lien + strlen(lien) - 1;
                      while((*a) && (*a != '/') && (a > lien))
                        a--;
                      if (*a == '/') {
                        *(a + 1) = '\0';
                      }
                    }
                    //char BIGSTK tempo[HTS_URLMAXSIZE];
                    //strcpybuff(tempo,"http://");
                    //strcatbuff(tempo,urladr());    // host
                    //if (*lien!='/')
                    //  strcatbuff(tempo,"/");
                    //strcatbuff(tempo,lien);
                    //strcpybuff(lien,tempo);
                  }
                }

                if (!*stre->error_) {       // pas d'erreur?
                  if (p_type == 2) {        // code ET PAS codebase
                    char *a = lien + strlen(lien) - 1;
                    char *start_of_filename = jump_identification(lien);

                    if (start_of_filename != NULL
                        && (start_of_filename =
                            strchr(start_of_filename, '/')) != NULL)
                      start_of_filename++;
                    if (start_of_filename == NULL)
                      strcatbuff(lien, "/");
                    while((a > lien) && (*a) && (*a != '/'))
                      a--;
                    if (*a == '/') {        // ok on a repéré le dernier /
                      if (start_of_filename != NULL
                          && a + 1 >= start_of_filename) {
                        *(a + 1) = '\0';    // couper
                      }
                    } else {
                      *lien = '\0'; // éliminer
                      *stre->error_ = 1;    // erreur, ne pas poursuivre
                    }
                  }
                  // stocker base ou codebase?
                  switch (p_type) {
                  case 2:{
                      //if (*lien!='/') strcatbuff(base,"/");
                      strcpybuff(stre->base, lien);
                    }
                    break;  // base
                  case -2:{
                      //if (*lien!='/') strcatbuff(codebase,"/");
                      strcpybuff(stre->codebase, lien);
                    }
                    break;  // base
                  }

                  hts_log_print(str->opt, LOG_DEBUG,
                                "code/codebase link %s base %s", lien,
                                stre->base);
                  //printf("base code: %s - %s\n",lien,base);
                }

              } else {
                char *_base;

                if (p_type == -1)   // code (applet)
                  _base = stre->codebase;
                else
                  _base = stre->base;

                // ajouter chemin de base href..
                if (strnotempty(_base)) {   // considérer base
                  if (!link_has_authority(lien)) {  // non absolue
                    if (*lien != '/') {     // non absolu sur le site (/)
                      if ((strlen(_base) + strlen(lien)) < HTS_URLMAXSIZE) {
                        // mailto: and co: do NOT add base
                        if (ident_url_relatif
                            (lien, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, &afs.af) >= 0) {
                          char BIGSTK tempo[HTS_URLMAXSIZE];

                          // base est absolue
                          strcpybuff(tempo, _base);
                          strcatbuff(tempo,
                                     lien + ((*lien == '/') ? 1 : 0));
                          strcpybuff(lien, tempo);  // patcher en considérant base
                          // ** vérifier que ../ fonctionne (ne doit pas arriver mais bon..)

                          hts_log_print(str->opt, LOG_DEBUG,
                                        "link modified with code/codebase %s",
                                        lien);
                        }
                      } else {
                        *stre->error_ = 1;  // erreur
                        hts_log_print(str->opt, LOG_ERROR,
                                      "Link %s too long with base href",
                                      lien);
                      }
                    } else {
                      lien_adrfil baseaf;
                      if (ident_url_absolute(_base, &baseaf) >= 0) {
                        if ((strlen(baseaf.adr) + strlen(lien)) < HTS_URLMAXSIZE) {
                          char BIGSTK tempo[HTS_URLMAXSIZE];

                          // base est absolue
                          tempo[0] = '\0';
                          if (!link_has_authority(baseaf.adr)) {
                            strcatbuff(tempo, "http://");
                          }
                          strcatbuff(tempo, baseaf.adr);
                          strcatbuff(tempo, lien);
                          strcpybuff(lien, tempo);  // patcher en considérant base

                          hts_log_print(str->opt, LOG_DEBUG,
                                        "link modified with code/codebase %s",
                                        lien);
                        } else {
                          *stre->error_ = 1;        // erreur
                          hts_log_print(str->opt, LOG_ERROR,
                                        "Link %s too long with base href",
                                        lien);
                        }
                      }
                    }
                  }
                }

              }
            }
            // transformer lien quelconque (http, relatif, etc) en une adresse
            // et un chemin+fichier (adr,fil)
            if (!*stre->error_) {
              int reponse;
              char*reladr, *relfil;
              reladr = parent_relative? str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->adr : str->opt->liens[*str->ptr_]->adr;
              relfil = parent_relative? str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->fil : str->opt->liens[*str->ptr_]->fil;
              hts_log_print(str->opt, LOG_DEBUG,
                            "build relative link %s with %s%s", lien,
                            reladr, relfil);
              if ((reponse =
                   ident_url_relatif(lien, reladr, relfil,
                                     &afs.af)) < 0) {
                afs.af.adr[0] = '\0';      // erreur
                if (reponse == -2) {
                  hts_log_print(str->opt, LOG_WARNING,
                                "Link %s not caught (unknown protocol)",
                                lien);
                } else {
                  hts_log_print(str->opt, LOG_DEBUG,
                                "ident_url_relatif failed for %s with %s%s",
                                lien, reladr, relfil);
                }
              } else {
                hts_log_print(str->opt, LOG_DEBUG,
                              "built relative link %s with %s%s -> %s%s",
                              lien, reladr, relfil, afs.af.adr,
                              afs.af.fil);
              }
            } else {
              hts_log_print(str->opt, LOG_DEBUG,
                            "link %s not build, error detected before",
                            lien);
              afs.af.adr[0] = '\0';
            }

            // Le lien doit juste être réécrit, mais ne doit pas générer un lien
            // exemple: <FORM ACTION="url_cgi">
            if (p_nocatch) {
              forbidden_url = 1;    // interdire récupération du lien
              hts_log_print(str->opt, LOG_DEBUG, "link forced external at %s%s",
                            afs.af.adr, afs.af.fil);
            }
            // Tester si un lien doit être accepté ou refusé (wizard)
            // forbidden_url=1 : lien refusé
            // forbidden_url=0 : lien accepté
            //if ((ptr>0) && (p_type!=2) && (p_type!=-2)) {    // tester autorisations?
            if ((p_type != 2) && (p_type != -2)) {  // tester autorisations?
              if (!p_nocatch) {
                if (afs.af.adr[0] != '\0') {
                  hts_log_print(str->opt, LOG_DEBUG,
                                "wizard link test at %s%s..", afs.af.adr, afs.af.fil);
                  forbidden_url =
                    hts_acceptlink(str->opt, *str->ptr_, afs.af.adr, afs.af.fil,
                                   intag_name ? intag_name : NULL,
                                   intag_name ? tag_attr_start : NULL,
                                   &set_prio_to, &just_test_it);
                  hts_log_print(str->opt, LOG_DEBUG,
                                "result for wizard link test: %d",
                                forbidden_url);
                }
              }
            }
            // calculer meme_adresse
            meme_adresse =
              strfield2(jump_identification_const(afs.af.adr),
                        jump_identification_const(heap_(*str->ptr_)->adr));

            // Début partie sauvegarde

            // ici on forme le nom du fichier à sauver, et on patche l'URL
            if (afs.af.adr[0] != '\0') {
              // savename(): simplifier les ../ et autres joyeusetés
              int r_sv = 0;

              // En cas de moved, adresse première
              lien_adrfil former;

              //
              afs.save[0] = '\0';
              former.adr[0] = '\0';
              former.fil[0] = '\0';
              //

              // nom du chemin à sauver si on doit le calculer
              // note: url_savename peut décider de tester le lien si il le trouve
              // suspect, et modifier alors adr et fil
              // dans ce cas on aura une référence directe au lieu des traditionnels
              // moved en cascade (impossible à reproduire à priori en local, lorsque des fichiers
              // gif sont impliqués par exemple)
              if ((p_type != 2) && (p_type != -2)) {        // pas base href ou codebase
                if (forbidden_url != 1) {
                  char BIGSTK last_adr[HTS_URLMAXSIZE];

                  /* Calc */
                  last_adr[0] = '\0';
                  //char last_fil[HTS_URLMAXSIZE]="";
                  strcpybuff(last_adr, afs.af.adr);        // ancienne adresse
                  //strcpybuff(last_fil,fil);    // ancien chemin
                  r_sv =
                    url_savename(&afs, &former, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, str->opt,
                                 str->sback, str->cache, stre->hash_, *str->ptr_,
                                 str->numero_passe, NULL);
                  if (strcmp(jump_identification_const(last_adr),
                    jump_identification_const(afs.af.adr)) != 0) {       // a changé

                    // 2e test si moved

                    // Tester si un lien doit être accepté ou refusé (wizard)
                    // forbidden_url=1 : lien refusé
                    // forbidden_url=0 : lien accepté
                    if ((*str->ptr_ > 0) && (p_type != 2) && (p_type != -2)) {     // tester autorisations?
                      if (!p_nocatch) {
                        if (afs.af.adr[0] != '\0') {
                          hts_log_print(str->opt, LOG_DEBUG,
                                        "wizard moved link retest at %s%s..",
                                        afs.af.adr, afs.af.fil);
                          forbidden_url =
                            hts_acceptlink(str->opt, *str->ptr_, afs.af.adr, afs.af.fil,
                                           intag_name ? intag_name : NULL,
                                           intag_name ? tag_attr_start :
                                           NULL, &set_prio_to,
                                           &just_test_it);
                          hts_log_print(str->opt, LOG_DEBUG,
                                        "result for wizard moved link retest: %d",
                                        forbidden_url);
                        }
                      }
                    }
                    //import_done=1;    // c'est un import!
                    meme_adresse = 0;       // on a changé
                  }
                } else {
                  strcpybuff(afs.save, "");     // dummy
                }
              }
              // resolve unresolved type
              if (r_sv != -1 && p_type != 2 && p_type != -2
                  && forbidden_url == 0 && IS_DELAYED_EXT(afs.save)
                ) {
                time_t t;

                // pas d'erreur, on continue
                r_sv =
                  hts_wait_delayed(str, &afs, heap_(*str->ptr_)->adr,
                                   heap_(*str->ptr_)->fil, &former,
                                   &forbidden_url);

                /* User interaction, because hts_wait_delayed can be slow.. (3.43) */
                t = time(NULL);
                if (user_interact_timestamp == 0
                    || t - user_interact_timestamp > 0) {
                  user_interact_timestamp = t;
                  //ENGINE_SAVE_CONTEXT();
                  {
                    hts_mirror_process_user_interaction(str, stre);
                  }
                  //ENGINE_SET_CONTEXT();
                }
              }
              // record!
              if (r_sv != -1) {     // pas d'erreur, on continue
                /* log */
                if ((str->opt->debug > 1) && (str->opt->log != NULL)) {
                  if (forbidden_url != 1) { // le lien va être chargé
                    if ((p_type == 2) || (p_type == -2)) {  // base href ou codebase, pas un lien
                      hts_log_print(str->opt, LOG_DEBUG, "Code/Codebase: %s%s",
                                    afs.af.adr, afs.af.fil);
                    } else if ((str->opt->getmode & HTS_DOWNLOAD_NON_HTML_LATER) == 0) {
                      hts_log_print(str->opt, LOG_DEBUG, "Record: %s%s -> %s",
                                    afs.af.adr, afs.af.fil, afs.save);
                    } else {
                      if (!ishtml(str->opt, afs.af.fil))
                        hts_log_print(str->opt, LOG_DEBUG,
                                      "Record after: %s%s -> %s", afs.af.adr, afs.af.fil,
                                      afs.save);
                      else
                        hts_log_print(str->opt, LOG_DEBUG, "Record: %s%s -> %s",
                                      afs.af.adr, afs.af.fil, afs.save);
                    }
                  } else
                    hts_log_print(str->opt, LOG_DEBUG, "External: %s%s", afs.af.adr,
                                  afs.af.fil);
                }
                /* FIN log */

                // écrire lien
                if ((p_type == 2) || (p_type == -2)) {      // base href ou codebase, sauter
                  lastsaved = eadr - 1 + 1; // sauter "
                }
                /* */
                else if (str->opt->urlmode == 0) {       // URL absolue dans tous les cas
                  if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {    // ecrire les html
                    if (!link_has_authority(afs.af.adr)) {
                      HT_ADD("http://");
                    } else {
                      char *aut = strstr(afs.af.adr, "//");

                      if (aut) {
                        char tmp[256];

                        tmp[0] = '\0';
                        strncatbuff(tmp, afs.af.adr, aut - afs.af.adr);   // scheme
                        HT_ADD(tmp);        // Protocol
                        HT_ADD("//");
                      }
                    }

                    if (!str->opt->passprivacy) {
                      HT_ADD_HTMLESCAPED(jump_protocol_const(afs.af.adr));       // Password
                    } else {
                      HT_ADD_HTMLESCAPED(jump_identification_const(afs.af.adr)); // No Password
                    }
                    if (afs.af.fil[0] != '/')
                      HT_ADD("/");
                    HT_ADD_HTMLESCAPED(afs.af.fil);
                  }
                  lastsaved = eadr - 1;     // dernier écrit+1 (enfin euh apres on fait un ++ alors hein)
                  /* */
                } else if (str->opt->urlmode == 4) {     // ne rien faire!
                  /* */
                  /* leave the link 'as is' */
                  /* Sinon, dépend de interne/externe */
                } else if (forbidden_url == 1) {    // le lien ne sera pas chargé, référence externe!
                  if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
                    if (p_type != -1) {     // pas que le nom de fichier (pas classe java)
                      if (!str->opt->external) {
                        if (!link_has_authority(afs.af.adr)) {
                          HT_ADD("http://");
                          if (!str->opt->passprivacy) {
                            HT_ADD_HTMLESCAPED(afs.af.adr);        // Password
                          } else {
                            HT_ADD_HTMLESCAPED(jump_identification_const(afs.af.adr));   // No Password
                          }
                          if (afs.af.fil[0] != '/')
                            HT_ADD("/");
                          HT_ADD_HTMLESCAPED(afs.af.fil);
                        } else {
                          char *aut = strstr(afs.af.adr, "//");

                          if (aut) {
                            char tmp[256];

                            tmp[0] = '\0';
                            strncatbuff(tmp, afs.af.adr, (aut - afs.af.adr));       // scheme
                            HT_ADD(tmp);    // Protocol
                            HT_ADD("//");
                            if (!str->opt->passprivacy) {
                              HT_ADD_HTMLESCAPED(jump_protocol_const(afs.af.adr));       // Password
                            } else {
                              HT_ADD_HTMLESCAPED(jump_identification_const(afs.af.adr)); // No Password
                            }
                            if (afs.af.fil[0] != '/')
                              HT_ADD("/");
                            HT_ADD_HTMLESCAPED(afs.af.fil);
                          }
                        }
                        //
                      } else {      // fichier/page externe, mais on veut générer une erreur
                        //
                        int patch_it = 0;
                        int add_url = 0;
                        const char *cat_name = NULL;
                        const char *cat_data = NULL;
                        int cat_nb = 0;
                        int cat_data_len = 0;

                        // ajouter lien external
                        switch ((link_has_authority(afs.af.adr)) ? 1
                                : ((afs.af.fil[strlen(afs.af.fil) - 1] ==
                                    '/') ? 1 : (ishtml(str->opt, afs.af.fil)))) {
                        case 1:
                        case -2:   // html ou répertoire
                          if (str->opt->getmode & HTS_DOWNLOAD_HTML) {   // sauver html
                            patch_it = 1;   // redirect
                            add_url = 1;    // avec link?
                            cat_name = "external.html";
                            cat_nb = 0;
                            cat_data = HTS_DATA_UNKNOWN_HTML;
                            cat_data_len = HTS_DATA_UNKNOWN_HTML_LEN;
                          }
                          break;
                        default:   // inconnu
                          // asp, cgi..
                          if ((strfield2
                               (afs.af.fil + max(0, strlen(afs.af.fil) - 4),
                                ".gif"))
                              ||
                              (strfield2
                               (afs.af.fil + max(0, strlen(afs.af.fil) - 4),
                                ".jpg"))
                              ||
                              (strfield2
                               (afs.af.fil + max(0, strlen(afs.af.fil) - 4),
                                ".xbm"))
                              /*|| (ishtml(opt,fil)!=0) */
                            ) {
                            patch_it = 1;   // redirect
                            add_url = 1;    // avec link aussi
                            cat_name = "external.gif";
                            cat_nb = 1;
                            cat_data = HTS_DATA_UNKNOWN_GIF;
                            cat_data_len = HTS_DATA_UNKNOWN_GIF_LEN;
                          } else {  /* if (is_dyntype(get_ext(fil))) */

                            patch_it = 1;   // redirect
                            add_url = 1;    // avec link?
                            cat_name = "external.html";
                            cat_nb = 0;
                            cat_data = HTS_DATA_UNKNOWN_HTML;
                            cat_data_len = HTS_DATA_UNKNOWN_HTML_LEN;
                          }
                          break;
                        }   // html,gif

                        if (patch_it) {
                          char BIGSTK save[HTS_URLMAXSIZE];
                          char BIGSTK tempo[HTS_URLMAXSIZE];
                          char*relsav;
                          relsav = parent_relative? str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->sav : str->opt->liens[*str->ptr_]->sav;

                          strcpybuff(save, StringBuff(str->opt->path_html_utf8));
                          strcatbuff(save, cat_name);
                          if (lienrelatif(tempo, save, relsav) == 0) {
                            /* Never escape high-chars (we don't know the encoding!!) */
                            inplace_escape_uri_utf(tempo, sizeof(tempo));  // escape with %xx
                            //if (!no_esc_utf)
                            //  escape_uri(tempo);     // escape with %xx
                            //else
                            //  escape_uri_utf(tempo);     // escape with %xx
                            HT_ADD_HTMLESCAPED(tempo);      // page externe
                            if (add_url) {
                              HT_ADD("?link=");     // page externe

                              // same as above
                              if (!link_has_authority(afs.af.adr)) {
                                HT_ADD("http://");
                                if (!str->opt->passprivacy) {
                                  HT_ADD_HTMLESCAPED(afs.af.adr);  // Password
                                } else {
                                  HT_ADD_HTMLESCAPED(jump_identification_const(afs.af.adr));     // No Password
                                }
                                if (afs.af.fil[0] != '/')
                                  HT_ADD("/");
                                HT_ADD_HTMLESCAPED(afs.af.fil);
                              } else {
                                char *aut = strstr(afs.af.adr, "//");

                                if (aut) {
                                  char tmp[256];

                                  tmp[0] = '\0';
                                  strncatbuff(tmp, afs.af.adr, (aut - afs.af.adr) + 2);     // scheme
                                  HT_ADD(tmp);
                                  if (!str->opt->passprivacy) {
                                    HT_ADD_HTMLESCAPED(jump_protocol_const(afs.af.adr)); // Password
                                  } else {
                                    HT_ADD_HTMLESCAPED(jump_identification_const(afs.af.adr));   // No Password
                                  }
                                  if (afs.af.fil[0] != '/')
                                    HT_ADD("/");
                                  HT_ADD_HTMLESCAPED(afs.af.fil);
                                }
                              }
                              //

                            }
                          }
                          // écrire fichier?
                          if (verif_external(str->opt, cat_nb, 1)) {
                            char CATname[255];
                            snprintf(CATname, 255, "%s%s", StringBuff(str->opt->path_html_utf8), cat_name);
                            FILE *fpc =
                              filecreate(&str->opt->state.strc, CATname);
                            if (fpc) {
                              if (cat_data_len == 0) {      // texte
                                verif_backblue(str->opt,
                                               StringBuff(str->opt->
                                                          path_html_utf8));
                                fprintf(fpc, "%s%s",
                                        "<!-- Created by HTTrack Website Copier/"
                                        HTTRACK_VERSION " "
                                        HTTRACK_AFF_AUTHORS " -->" LF,
                                        cat_data);
                              } else {      // data
                                fwrite(cat_data, cat_data_len, 1, fpc);
                              }
                              fclose(fpc);
                              usercommand(str->opt, 0, NULL, CATname, "", "");
                            }
                          }
                        } else {    // écrire normalement le nom de fichier
                          HT_ADD("http://");
                          if (!str->opt->passprivacy) {
                            HT_ADD_HTMLESCAPED(afs.af.adr);        // Password
                          } else {
                            HT_ADD_HTMLESCAPED(jump_identification_const(afs.af.adr));   // No Password
                          }
                          if (afs.af.fil[0] != '/')
                            HT_ADD("/");
                          HT_ADD_HTMLESCAPED(afs.af.fil);
                        }   // patcher?
                      }     // external
                    } else {        // que le nom de fichier (classe java)
                      // en gros recopie de plus bas: copier codebase et base
                      if (p_flush) {
                        char BIGSTK tempo[HTS_URLMAXSIZE];      // <-- ajouté
                        char BIGSTK tempo_pat[HTS_URLMAXSIZE];

                        // Calculer chemin
                        tempo_pat[0] = '\0';
                        strcpybuff(tempo, afs.af.fil);     // <-- ajouté
                        {
                          char *a = strrchr(tempo, '/');

                          // Example: we converted code="x.y.z.foo.class" into "x/y/z/foo.class"
                          // we have to do the contrary now
                          if (add_class_dots_to_patch > 0) {
                            while((add_class_dots_to_patch > 0) && (a)) {
                              *a = '.';     // convert "false" java / into .
                              add_class_dots_to_patch--;
                              a = strrchr(tempo, '/');
                            }
                            // if add_class_dots_to_patch, this is because there is a problem!!
                            if (add_class_dots_to_patch) {
                              hts_log_print(str->opt, LOG_WARNING,
                                            "Error: can not rewind java path %s, check html code",
                                            tempo);
                            }
                          }
                          // Cut path/filename
                          if (a) {
                            char BIGSTK tempo2[HTS_URLMAXSIZE];

                            strcpybuff(tempo2, a + 1);      // FICHIER
                            strncatbuff(tempo_pat, tempo, (a - tempo) + 1);   // chemin
                            strcpybuff(tempo, tempo2);      // fichier
                          }
                        }

                        // érire codebase="chemin"
                        if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
                          char BIGSTK tempo4[HTS_URLMAXSIZE];

                          tempo4[0] = '\0';

                          if (strnotempty(tempo_pat)) {
                            HT_ADD("codebase=\"http://");
                            if (!str->opt->passprivacy) {
                              HT_ADD_HTMLESCAPED(afs.af.adr);      // Password
                            } else {
                              HT_ADD_HTMLESCAPED(jump_identification_const(afs.af.adr)); // No Password
                            }
                            if (*tempo_pat != '/')
                              HT_ADD("/");
                            HT_ADD(tempo_pat);
                            HT_ADD("\" ");
                          }

                          strncatbuff(tempo4, lastsaved, p_flush - lastsaved);
                          HT_ADD(tempo4);   // refresh code="
                          HT_ADD(tempo);
                        }
                      }
                    }
                  }
                  lastsaved = eadr - 1;
                }
                /*
                   else if (opt->urlmode==1) {    // ABSOLU, c'est le cas le moins courant
                   //  NE FONCTIONNE PAS!!  (et est inutile)
                   if ((opt->getmode & HTS_DOWNLOAD_HTML) && (ptr>0)) {    // ecrire les html
                   // écrire le lien modifié, absolu
                   HT_ADD("file:");
                   if (*save=='/')
                   HT_ADD(save+1)
                   else
                   HT_ADD(save)
                   }
                   lastsaved=eadr-1;    // dernier écrit+1 (enfin euh apres on fait un ++ alors hein)
                   }
                 */
                else if (str->opt->mimehtml) {
                  char BIGSTK cid[HTS_URLMAXSIZE * 3];

                  HT_ADD("cid:");
                  make_content_id(afs.af.adr, afs.af.fil, cid, sizeof(cid));
                  HT_ADD_HTMLESCAPED(cid);
                  lastsaved = eadr - 1;     // dernier écrit+1 (enfin euh apres on fait un ++ alors hein)
                } else if (str->opt->urlmode == 3) {     // URI absolue /
                  if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {    // ecrire les html
                    HT_ADD_HTMLESCAPED(afs.af.fil);
                  }
                  lastsaved = eadr - 1;     // dernier écrit+1 (enfin euh apres on fait un ++ alors hein)
                } else if (str->opt->urlmode == 5) {     // transparent proxy URL
                  char BIGSTK tempo[HTS_URLMAXSIZE];
                  const char *uri;
                  int i;
                  char *pos;

                  if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {    // ecrire les html
                    if (!link_has_authority(afs.af.adr)) {
                      HT_ADD("http://");
                    } else {
                      char *aut = strstr(afs.af.adr, "//");

                      if (aut) {
                        char tmp[256];

                        tmp[0] = '\0';
                        strncatbuff(tmp, afs.af.adr, (aut - afs.af.adr));   // scheme
                        HT_ADD(tmp);        // Protocol
                        HT_ADD("//");
                      }
                    }

                    // filename is taken as URI (ex: "C:\My Website\www.example.com\foo4242.html)
                    uri = afs.save;

                    // .. after stripping the path prefix (ex: "www.example.com\foo4242.html)
                    if (strnotempty(StringBuff(str->opt->path_html_utf8))) {
                      uri += StringLength(str->opt->path_html_utf8);
                      for(; uri[0] == '/' || uri[0] == '\\'; uri++) ;
                    }
                    // and replacing all \ by / (ex: "www.example.com/foo4242.html)
                    strcpybuff(tempo, uri);
                    for(i = 0; tempo[i] != '\0'; i++) {
                      if (tempo[i] == '\\') {
                        tempo[i] = '/';
                      }
                    }

                    // put original query string if any (ex: "www.example.com/foo4242.html?q=45)
                    pos = strchr(afs.af.fil, '?');
                    if (pos != NULL) {
                      strcatbuff(tempo, pos);
                    }
                    // write it
                    HT_ADD_HTMLESCAPED(tempo);
                  }
                  lastsaved = eadr - 1;     // dernier écrit+1 (enfin euh apres on fait un ++ alors hein)
                } else if (str->opt->urlmode == 2) {     // RELATIF
                  char BIGSTK tempo[HTS_URLMAXSIZE];
                  char*relsav;
                  relsav = parent_relative? str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->sav : str->opt->liens[*str->ptr_]->sav;

                  tempo[0] = '\0';
                  // calculer le lien relatif

                  if (lienrelatif(tempo, afs.save, relsav) == 0) {
                    if (!in_media) {        // In media (such as real audio): don't patch
                      /* Never escape high-chars (we don't know the encoding!!) */
                      inplace_escape_uri_utf(tempo, sizeof(tempo));

                      //if (!no_esc_utf)
                      //  escape_uri(tempo);     // escape with %xx
                      //else {
                      //  /* No escaping at all - remaining upper chars will be escaped below */
                      //  /* FIXME - Should be done in all local cases */
                      //  //x_escape_html(tempo);
                      //  //escape_uri_utf(tempo);     // FIXME - escape with %xx
                      //  //escape_uri(tempo);     // escape with %xx
                      //}
                    }
                    hts_log_print(str->opt, LOG_DEBUG,
                                  "relative link at %s build with %s and %s: %s",
                                  afs.af.adr, afs.save, relsav, tempo);

                    // lien applet (code) - il faut placer un codebase avant
                    if (p_type == -1) {     // que le nom de fichier

                      if (p_flush) {
                        char BIGSTK tempo_pat[HTS_URLMAXSIZE];

                        tempo_pat[0] = '\0';
                        {
                          char *a = strrchr(tempo, '/');

                          // Example: we converted code="x.y.z.foo.class" into "x/y/z/foo.class"
                          // we have to do the contrary now
                          if (add_class_dots_to_patch > 0) {
                            while((add_class_dots_to_patch > 0) && (a)) {
                              *a = '.';     // convert "false" java / into .
                              add_class_dots_to_patch--;
                              a = strrchr(tempo, '/');
                            }
                            // if add_class_dots_to_patch, this is because there is a problem!!
                            if (add_class_dots_to_patch) {
                              hts_log_print(str->opt, LOG_WARNING,
                                            "Error: can not rewind java path %s, check html code",
                                            tempo);
                            }
                          }

                          if (a) {
                            char BIGSTK tempo2[HTS_URLMAXSIZE];

                            strcpybuff(tempo2, a + 1);
                            strncatbuff(tempo_pat, tempo, a - tempo + 1);   // chemin
                            strcpybuff(tempo, tempo2);      // fichier
                          }
                        }

                        // érire codebase="chemin"
                        if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
                          char BIGSTK tempo4[HTS_URLMAXSIZE];

                          tempo4[0] = '\0';

                          if (strnotempty(tempo_pat)) {
                            HT_ADD("codebase=\"");
                            HT_ADD_HTMLESCAPED(tempo_pat);
                            HT_ADD("\" ");
                          }

                          strncatbuff(tempo4, lastsaved, p_flush - lastsaved);
                          HT_ADD(tempo4);   // refresh code="
                        }
                      }
                      //lastsaved=adr;    // dernier écrit+1
                    }

                    if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
                      // convert to local codepage - NOT, already converted into %NN, and passed to the remote server so we do not have anything to do
                      //if (str->page_charset_ != NULL && *str->page_charset_ != '\0') {
                      //  char *const local_save = hts_convertStringFromUTF8(tempo, strlen(tempo), str->page_charset_);
                      //  if (local_save != NULL) {
                      //    strcpybuff(tempo, local_save);
                      //    free(local_save);
                      //  } else {
                      //    if ((opt->debug>1) && (opt->log!=NULL)) {
                      //      fprintf(opt->log, "Warning: could not build local charset representation of '%s' in '%s'"LF, tempo, str->page_charset_);
                      //    }
                      //  }
                      //}

                      // écrire le lien modifié, relatif
                      // Note: escape all chars, even >127 (no UTF)
                      HT_ADD_HTMLESCAPED_FULL(tempo);

                      // Add query-string, for informational purpose only
                      // Useless, because all parameters-pages are saved into different targets
                      if (str->opt->includequery) {
                        char *a = strchr(lien, '?');

                        if (a) {
                          HT_ADD_HTMLESCAPED(a);
                        }
                      }
                    }
                    lastsaved = eadr - 1;   // dernier écrit+1 (enfin euh apres on fait un ++ alors hein)
                  } else {
                    hts_log_print(str->opt, LOG_WARNING,
                                  "Error building relative link %s and %s",
                                  afs.save, relsav);
                  }
                }           // sinon le lien sera écrit normalement

#if 0
                if (fexist(save)) { // le fichier existe..
                  adr[0] = '\0';
                  //if ((opt->debug>0) && (opt->log!=NULL)) {
                  hts_log_print(opt, LOG_WARNING,
                                "Link has already been written on disk, cancelled: %s",
                                save);
                }
#endif

                /* Security check */
                if (strlen(afs.save) >= HTS_URLMAXSIZE) {
                  afs.af.adr[0] = '\0';
                  hts_log_print(str->opt, LOG_WARNING, "Link is too long: %s",
                                afs.save);
                }

                if ((afs.af.adr[0] != '\0') && (p_type != 2) && (p_type != -2) && (forbidden_url != 1)) {  // si le fichier n'existe pas, ajouter à la liste
                  // n'y a-t-il pas trop de liens?
                  if (str->opt->maxlink > 0 && str->opt->lien_tot + 1 >= str->opt->maxlink) {       // trop de liens!
                    printf("PANIC! : Too many URLs : >%d [%d]\n", str->opt->lien_tot,
                           __LINE__);
                    hts_log_print(str->opt, LOG_PANIC, "Too many URLs, giving up..(>%d)",
                                  str->opt->maxlink);
                    hts_log_print(str->opt, LOG_INFO,
                                  "To avoid that: use #L option for more links (example: -#L1000000)");
                    if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
                      if (fp) {
                        fclose(fp);
                        fp = NULL;
                      }
                    }
                    XH_uninit;      // désallocation mémoire & buffers
                    return -1;
                  } else {  // noter le lien sur la listes des liens à charger
                    int pass_fix, dejafait = 0;

                    // Calculer la priorité de ce lien
                    if ((str->opt->getmode & HTS_DOWNLOAD_NON_HTML_LATER) == 0) {  // traiter html après
                      pass_fix = 0;
                    } else {        // vérifier que ce n'est pas un !html
                      if (!ishtml(str->opt, afs.af.fil))
                        pass_fix = 1;       // priorité inférieure (traiter après)
                      else
                        pass_fix = max(0, str->numero_passe);    // priorité normale
                    }

                    /* If the file seems to be an html file, get depth-1 */
                    /*
                       if (strnotempty(save)) {
                       if (ishtml(opt,save) == 1) {
                       // descore_prio = 2;
                       } else {
                       // descore_prio = 1;
                       }
                       }
                     */

                    // vérifier que le lien n'a pas déja été noté
                    // si c'est le cas, alors il faut s'assurer que la priorité associée
                    // au fichier est la plus grande des deux priorités
                    //
                    // On part de la fin et on essaye de se presser (économise temps machine)
                    {
                      int i = hash_read(stre->hash_, afs.save, NULL, 0);   // lecture type 0 (sav)

                      if (i >= 0) {
                        if ((str->opt->debug > 1) && (str->opt->log != NULL)) {
                          if (strcmp(afs.af.adr, str->opt->liens[i]->adr) != 0
                              || strcmp(afs.af.fil, str->opt->liens[i]->fil) != 0) {
                            hts_log_print(str->opt, LOG_DEBUG,
                                          "merging similar links %s%s and %s%s",
                                          afs.af.adr, afs.af.fil, str->opt->liens[i]->adr,
                                          str->opt->liens[i]->fil);
                          }
                        }
                        str->opt->liens[i]->depth =
                          maximum(str->opt->liens[i]->depth, str->opt->liens[*str->ptr_]->depth - 1);
                        dejafait = 1;
                      }
                    }

                    // le lien n'a jamais été créé.
                    // cette fois ci, on le crée!
                    if (!dejafait) {
                      //
                      // >>>> CREER LE LIEN <<<<
                      //
                      // enregistrer lien à charger
                      //heap_top()->adr[0]=heap_top()->fil[0]=heap_top()->sav[0]='\0';
                      // même adresse: l'objet père est l'objet père de l'actuel

                      // DEBUT ROBOTS.TXT AJOUT
                      if (!just_test_it) {
                        if ((!strfield(afs.af.adr, "ftp://"))      // non ftp
                            && (!strfield(afs.af.adr, "file://"))
                          ) {       // non file
                          if (str->opt->robots) {        // récupérer robots
                            if (ishtml(str->opt, afs.af.fil) != 0) {    // pas la peine pour des fichiers isolés
                              if (checkrobots(str->opt->robotsptr, afs.af.adr, "") != -1) {    // robots.txt ?
                                checkrobots_set(str->opt->robotsptr, afs.af.adr, "");  // ajouter entrée vide
                                if (checkrobots(str->opt->robotsptr, afs.af.adr, "") == -1) {  // robots.txt ?
                                  // enregistrer robots.txt (MACRO)
                                  if (!hts_record_link(str->opt, afs.af.adr, "/robots.txt", "", "", "", NULL)) {
                                    if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
                                      if (fp) {
                                        fclose(fp);
                                        fp = NULL;
                                      }
                                    }
                                    XH_uninit;      // désallocation mémoire & buffers
                                    return -1;
                                  }
                                  str->opt->liens[str->opt->lien_tot - 1]->testmode = 0;    // pas mode test
                                  str->opt->liens[str->opt->lien_tot - 1]->link_import = 0; // pas mode import
                                  str->opt->liens[str->opt->lien_tot - 1]->premier = str->opt->lien_tot - 1;
                                  str->opt->liens[str->opt->lien_tot - 1]->precedent = *str->ptr_;
                                  str->opt->liens[str->opt->lien_tot - 1]->depth = 0;
                                  str->opt->liens[str->opt->lien_tot - 1]->pass2 = max(0, str->numero_passe);
                                  str->opt->liens[str->opt->lien_tot - 1]->retry = 0;
#if DEBUG_ROBOTS
                                  printf
                                    ("robots.txt: added file robots.txt for %s\n",
                                     afs.af.adr);
#endif
                                  hts_log_print(str->opt, LOG_DEBUG,
                                                "robots.txt added at %s",
                                                afs.af.adr);
                                } else {
                                  hts_log_print(str->opt, LOG_ERROR,
                                                "Unexpected robots.txt error at %d",
                                                __LINE__);
                                }
                              }
                            }
                          }
                        }
                      }
                      // FIN ROBOTS.TXT AJOUT

                      // enregistrer
                      if (!hts_record_link(str->opt, afs.af.adr, afs.af.fil, afs.save,
                                           former.adr, former.fil, stre->codebase)) {
                        if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
                          if (fp) {
                            fclose(fp);
                            fp = NULL;
                          }
                        }
                        XH_uninit;  // désallocation mémoire & buffers
                        return -1;
                      }
                      // mode test?
                      if (!just_test_it)
                        str->opt->liens[str->opt->lien_tot - 1]->testmode = 0;      // pas mode test
                      else
                        str->opt->liens[str->opt->lien_tot - 1]->testmode = 1;      // mode test
                      if (!import_done)
                        str->opt->liens[str->opt->lien_tot - 1]->link_import = 0;   // pas mode import
                      else
                        str->opt->liens[str->opt->lien_tot - 1]->link_import = 1;   // mode import
                      // écrire autres paramètres de la structure-lien
                      if ((meme_adresse) && (!import_done)
                          && (heap_(*str->ptr_)->premier != 0))
                        str->opt->liens[str->opt->lien_tot - 1]->premier = heap_(*str->ptr_)->premier;
                      else  // sinon l'objet père est le précédent lui même
                        str->opt->liens[str->opt->lien_tot - 1]->premier = str->opt->lien_tot - 1;
                      // heap_top()->premier=ptr;

                      str->opt->liens[str->opt->lien_tot - 1]->precedent = *str->ptr_;
                      // noter la priorité
                      if (!set_prio_to)
                        str->opt->liens[str->opt->lien_tot - 1]->depth = heap_(*str->ptr_)->depth - 1;
                      else
                        str->opt->liens[str->opt->lien_tot - 1]->depth = max(0, min(heap_(*str->ptr_)->depth - 1, set_prio_to - 1));       // PRIORITE NULLE (catch page)
                      // noter pass
                      str->opt->liens[str->opt->lien_tot - 1]->pass2 = pass_fix;
                      str->opt->liens[str->opt->lien_tot - 1]->retry = str->opt->retry;

                      //strcpybuff(heap_top()->adr,adr);
                      //strcpybuff(heap_top()->fil,fil);
                      //strcpybuff(heap_top()->sav,save);
                      if (!just_test_it) {
                        hts_log_print(str->opt, LOG_DEBUG,
                                      "OK, NOTE: %s%s -> %s",
                                      str->opt->liens[str->opt->lien_tot - 1]->adr,
                                      str->opt->liens[str->opt->lien_tot - 1]->fil,
                                      str->opt->liens[str->opt->lien_tot - 1]->sav);
                      } else {
                        hts_log_print(str->opt, LOG_DEBUG, "OK, TEST: %s%s",
                                      str->opt->liens[str->opt->lien_tot - 1]->adr,
                                      str->opt->liens[str->opt->lien_tot - 1]->fil);
                      }

                    } else {        // if !dejafait
                      hts_log_print(str->opt, LOG_DEBUG,
                                    "link has already been recorded, cancelled: %s",
                                    afs.save);

                    }

                  }         // si pas trop de liens
                }           // si adr[0]!='\0'

              }             // if adr[0]!='\0'

            }               // if adr[0]!='\0'

          }                 // if strlen(lien)>0

        }                   // if ok==0

        assertf(eadr - html >= 0);   // Should not go back
        if (eadr > html) {
          INCREMENT_CURRENT_ADR(eadr - 1 - html);
        }
        // adr=eadr-1;  // ** sauter

        /* We skipped bytes and skip the " : reset state */
        /*if (inscript) {
           inscript_state_pos = INSCRIPT_START;
           } */

      }                     // if (p)

    }                       // si '<' ou '>'

    // plus loin
    html++;                  // automate will be checked next loop

    /* Otimization: if we are scanning in HTML data (not in tag or script),
       then jump to the next starting tag */
    if (*str->ptr_ > 0) {
      if ((!intag)          /* Not in tag */
          &&(!inscript)     /* Not in (java)script */
          &&(!in_media)     /* Not in media */
          &&(!incomment)    /* Not in comment (<!--) */
          &&(!inscript_tag) /* Not in tag with script inside */
        ) {
        /* Not at the end */
        if (html - stre->r_->adr < stre->r_->size) {
          /* Not on a starting tag yet */
          if (*html != '<') {
            /* strchr does not well behave with null chrs.. */
            /* char* adr_next = strchr(adr,'<'); */
            const char *adr_next = html;

            while(*adr_next != '<' && (adr_next - stre->r_->adr) < stre->r_->size) {
              adr_next++;
            }
            /* Jump to near end (index hack) */
            if (!adr_next || *adr_next != '<') {
              if (html - stre->r_->adr < stre->r_->size - 4
                  && stre->r_->size > 4
                ) {
                html = stre->r_->adr + stre->r_->size - 2;
              }
            } else {
              html = adr_next;
            }
          }
        }
      }
    }
    // ----------
    // écrire peu à peu
    if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0))
      HT_add_adr;
    lastsaved = html;        // dernier écrit+1
    // ----------

    // Checks
    if (back_add_stats != str->opt->state.back_add_stats) {
      back_add_stats = str->opt->state.back_add_stats;

      // Check max time
      if (!back_checkmirror(str->opt)) {
        html = stre->r_->adr + stre->r_->size;
      }
    }
    // pour les stats du shell si parsing trop long
    if (stre->r_->size)
      str->opt->state._hts_in_html_done =
        (100 * ((int) (html - stre->r_->adr))) / (int) (stre->r_->size);
    if (str->opt->state._hts_in_html_poll) {
      str->opt->state._hts_in_html_poll = 0;
      // temps à attendre, et remplir autant que l'on peut le cache (backing)
      back_wait(str->sback, str->opt, str->cache, HTS_STAT.stat_timestart);
      back_fillmax(str->sback, str->opt, str->cache, *str->ptr_, str->numero_passe);

      // Transfer rate
      engine_stats();

      // Refresh various stats
      HTS_STAT.stat_nsocket = back_nsoc(str->sback);
      HTS_STAT.stat_errors = fspc(str->opt, NULL, "error");
      HTS_STAT.stat_warnings = fspc(str->opt, NULL, "warning");
      HTS_STAT.stat_infos = fspc(str->opt, NULL, "info");
      HTS_STAT.nbk = backlinks_done(str->sback, str->opt->liens, str->opt->lien_tot, *str->ptr_);
      HTS_STAT.nb = back_transferred(HTS_STAT.stat_bytes, str->sback);

      if (!RUN_CALLBACK7
          (str->opt, loop, str->sback->lnk, str->sback->count, 0, *str->ptr_, str->opt->lien_tot,
           (int) (time_local() - HTS_STAT.stat_timestart), &HTS_STAT)) {
        hts_log_print(str->opt, LOG_ERROR, "Exit requested by shell or user");
        *stre->exit_xh_ = 1;        // exit requested
        XH_uninit;
        return -1;
        //adr = r->adr + r->size;  // exit
      } else if (str->opt->state._hts_cancel == 1) {
        // adr = r->adr + r->size;  // exit
        nofollow = 1;       // moins violent
        str->opt->state._hts_cancel = 0;
      }

    }
    // refresh the backing system each 2 seconds
    if (engine_stats()) {
      back_wait(str->sback, str->opt, str->cache, HTS_STAT.stat_timestart);
      back_fillmax(str->sback, str->opt, str->cache, *str->ptr_, str->numero_passe);
    }
  } while(html - stre->r_->adr < stre->r_->size);

  str->opt->state._hts_in_html_parsing = 0;      // flag
  str->opt->state._hts_cancel = 0;       // pas de cancel

  if ((str->opt->getmode & HTS_DOWNLOAD_HTML) && (*str->ptr_ > 0)) {
    {
      char *cAddr = TypedArrayElts(output_buffer);
      int cSize = (int) TypedArraySize(output_buffer);

      hts_log_print(str->opt, LOG_DEBUG, "engine: postprocess-html: %s%s",
                    heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil);
      if (RUN_CALLBACK4(str->opt, postprocess, &cAddr, &cSize, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil) == 1) {
        if (cAddr != TypedArrayElts(output_buffer)) {
          hts_log_print(str->opt, LOG_DEBUG,
            "engine: postprocess-html: callback modified data, applying %d bytes", cSize);
          TypedArraySize(output_buffer) = 0;
          TypedArrayAppend(output_buffer, cAddr, cSize);
        }
      }
    }

    /* Flush and save to disk */
                 // achever
    int ok=0;
    if (TypedArraySize(output_buffer) != 0) {
      const size_t ht_len = TypedArraySize(output_buffer);
      const char *const ht_buff = TypedArrayElts(output_buffer);
      char digest[32+2];
      off_t fsize_old = fsize(fconv(OPT_GET_BUFF(str->opt),OPT_GET_BUFF_SIZE(str->opt),heap_(*str->ptr_)->sav));
      digest[0] = '\0';
      domd5mem(TypedArrayElts(output_buffer), ht_len, digest, 1);
      if (fsize_old == (off_t) ht_len) {
        int mlen = 0;
        char* mbuff;
        cache_readdata(str->cache,"//[HTML-MD5]//",heap_(*str->ptr_)->sav,&mbuff,&mlen);
        if (mlen)
          mbuff[mlen]='\0';
        if ((mlen == 32) && (strcmp(((mbuff!=NULL)?mbuff:""),digest)==0)) {
          ok=1;
          hts_log_print(str->opt, LOG_DEBUG, "File not re-written (md5): %s",heap_(*str->ptr_)->sav);
        } else {
          ok=0;
        }
      }
      if (!ok) {
        file_notify(str->opt,heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, heap_(*str->ptr_)->sav, 1, 1, stre->r_->notmodified);
        fp=filecreate(&str->opt->state.strc, heap_(*str->ptr_)->sav);
        if (fp) {
          if (ht_len>0) {
          if (fwrite(ht_buff,1,ht_len,fp) != ht_len) {
            int fcheck;
            if ((fcheck=check_fatal_io_errno())) {
              str->opt->state.exit_xh=-1;
            }
            if (str->opt->log) {
              hts_log_print(str->opt, LOG_ERROR | LOG_ERRNO, "Unable to write HTML file %s", heap_(*str->ptr_)->sav);
              if (fcheck) {
                hts_log_print(str->opt, LOG_ERROR, "* * Fatal write error, giving up");
              }
            }
          }
          }
          fclose(fp); fp=NULL;
          if (strnotempty(stre->r_->lastmodified))
          set_filetime_rfc822(heap_(*str->ptr_)->sav,stre->r_->lastmodified);
        } else {
          int fcheck;
          if ((fcheck=check_fatal_io_errno())) {
            hts_log_print(str->opt, LOG_ERROR, "Mirror aborted: disk full or filesystem problems");
            str->opt->state.exit_xh=-1;
          }
          hts_log_print(str->opt, LOG_ERROR | LOG_ERRNO, "Unable to save file %s", heap_(*str->ptr_)->sav);
          if (fcheck) {
            hts_log_print(str->opt, LOG_ERROR, "* * Fatal write error, giving up");
          }
        }
      } else {
        file_notify(str->opt,heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, heap_(*str->ptr_)->sav, 0, 0, stre->r_->notmodified);
        filenote(&str->opt->state.strc, heap_(*str->ptr_)->sav,NULL);
      }
      if (str->cache->ndx)
        cache_writedata(str->cache->ndx,str->cache->dat,"//[HTML-MD5]//",heap_(*str->ptr_)->sav,digest,(int)strlen(digest));
    }
    TypedArrayFree(output_buffer);
      
  }
  //
  //
  //
  error:;

  if (str->opt->getmode & HTS_DOWNLOAD_HTML) {
    if (fp) {
      fclose(fp);
      fp = NULL;
    }
  }
  // sauver fichier
  //structcheck(savename());
  //filesave(opt,r->adr,r->size,savename());

  /* Apply changes */
  //ENGINE_SAVE_CONTEXT();

  return 0;
}

/*
Check 301, 302, .. statuscodes (moved)
*/
int hts_mirror_check_moved(htsmoduleStruct * str,
                           htsmoduleStructExtended * stre) {
  /* Load engine variables */
  //ENGINE_LOAD_CONTEXT();

  char*adr, *fil, *sav;
  adr = str->opt->liens[*str->ptr_]->adr;
  fil = str->opt->liens[*str->ptr_]->fil;
  sav = str->opt->liens[*str->ptr_]->sav;
  // DEBUT rattrapage des 301,302,307..
  // ------------------------------------------------------------
  if (!*stre->error_) {
    ////////{
    // on a chargé un fichier en plus
    // if (!error) stat_loaded+=r.size;

    // ------------------------------------------------------------
    // Rattrapage des 301,302,307 (moved) et 412,416 - les 304 le sont dans le backing 
    // ------------------------------------------------------------
    if (HTTP_IS_REDIRECT(stre->r_->statuscode)) {
      //if (r->adr!=NULL) {   // adr==null si fichier direct. [catch: davename normalement si cgi]
      //int i=0;
      // char* p;

      hts_log_print(str->opt, LOG_WARNING, "\"%s\" for %s%s", stre->r_->msg, adr, fil);

      {
        char BIGSTK mov_url[HTS_URLMAXSIZE];
        lien_adrfilsave savedmoved;
        lien_adrfil *const moved = &savedmoved.af;
        int get_it = 0;         // ne pas prendre le fichier à la même adresse par défaut
        int reponse = 0;

        mov_url[0] = '\0';
        moved->adr[0] = '\0';
        moved->fil[0] = '\0';
        savedmoved.save[0] = '\0';
        //

        strcpybuff(mov_url, stre->r_->location);

        // url qque -> adresse+fichier
        if ((reponse =
             ident_url_relatif(mov_url, adr, fil, moved)) >= 0) {
          int set_prio_to = 0;  // pas de priotité fixéd par wizard

          // check whether URLHack is harmless or not
          if (str->opt->urlhack) {
            char BIGSTK n_adr[HTS_URLMAXSIZE], n_fil[HTS_URLMAXSIZE];
            char BIGSTK pn_adr[HTS_URLMAXSIZE], pn_fil[HTS_URLMAXSIZE];

            n_adr[0] = n_fil[0] = '\0';
            (void) adr_normalized(moved->adr, n_adr);
            (void) fil_normalized(moved->fil, n_fil);
            (void) adr_normalized(adr, pn_adr);
            (void) fil_normalized(fil, pn_fil);
            if (strcasecmp(n_adr, pn_adr) == 0
                && strcasecmp(n_fil, pn_fil) == 0) {
              hts_log_print(str->opt, LOG_WARNING,
                            "Redirected link is identical because of 'URL Hack' option: %s%s and %s%s",
                            adr, fil, moved->adr, moved->fil);
            }
          }
          //if (ident_url_absolute(mov_url,moved->adr,moved->fil)!=-1) {    // ok URL reconnue
          // c'est (en gros) la même URL..
          // si c'est un problème de casse dans le host c'est que le serveur est buggé
          // ("RFC says.." : host name IS case insensitive)
          if ((strfield2(moved->adr, adr) != 0) && (strfield2(moved->fil, fil) != 0)) { // identique à casse près
            // on tourne en rond
            if (strcmp(moved->fil, fil) == 0) {
              *stre->error_ = 1;
              get_it = -1;      // ne rien faire
              hts_log_print(str->opt, LOG_WARNING,
                            "Can not bear crazy server (%s) for %s%s", stre->r_->msg,
                            adr, fil);
            } else {            // mauvaise casse, effacer entrée dans la pile et rejouer une fois
              get_it = 1;
            }
          } else {              // adresse différente
            if (ishtml(str->opt, mov_url) == 0) {    // pas même adresse MAIS c'est un fichier non html (pas de page moved possible)
              // -> on prend à cette adresse, le lien sera enregistré avec lien_record() (hash)
              hts_log_print(str->opt, LOG_DEBUG,
                            "wizard link test for moved file at %s%s..",
                            moved->adr, moved->fil);
              // accepté?
              if (hts_acceptlink(str->opt, *str->ptr_, moved->adr, moved->fil, NULL, NULL, &set_prio_to, NULL) != 1) {   /* nouvelle adresse non refusée ? */
                get_it = 1;
                hts_log_print(str->opt, LOG_DEBUG, "moved link accepted: %s%s",
                              moved->adr, moved->fil);
              }
            }                   /* sinon traité normalement */
          }

          //if ((strfield2(moved->adr,urladr())!=0) && (strfield2(moved->fil,urlfil())!=0)) {  // identique à casse près
          if (get_it == 1) {
            // court-circuiter le reste du traitement
            // et reculer pour mieux sauter
            hts_log_print(str->opt, LOG_WARNING,
                          "Warning moved treated for %s%s (real one is %s%s)",
                          adr, fil, moved->adr, moved->fil);
            // canceller lien actuel
            *stre->error_ = 1;
            hts_invalidate_link(str->opt, *str->ptr_);  // invalidate hashtable entry
            // noter NOUVEAU lien
            //xxc xxc
            //  set_prio_to=0+1;  // protection if the moved URL is an html page!!
            //xxc xxc
            {
              // calculer lien et éventuellement modifier addresse/fichier
              if (url_savename(&savedmoved, NULL,
                   str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->adr,
                   str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->fil, str->opt,
                   str->sback, str->cache, stre->hash_, *str->ptr_, str->numero_passe, NULL) != -1) {
                if (hash_read(stre->hash_, savedmoved.save, NULL, HASH_STRUCT_FILENAME) < 0) {   // n'existe pas déja
                  // enregistrer lien avec SAV IDENTIQUE
                  if (hts_record_link(str->opt, moved->adr, moved->fil, str->opt->liens[*str->ptr_]->sav, "", "", NULL)) {
                    // mode test?
                    str->opt->liens[str->opt->lien_tot - 1]->testmode = str->opt->liens[*str->ptr_]->testmode;
                    str->opt->liens[str->opt->lien_tot - 1]->link_import = 0;   // mode normal
                    if (!set_prio_to)
                      str->opt->liens[str->opt->lien_tot - 1]->depth = str->opt->liens[*str->ptr_]->depth;
                    else
                      str->opt->liens[str->opt->lien_tot - 1]->depth = max(0, min(set_prio_to - 1, str->opt->liens[*str->ptr_]->depth)); // PRIORITE NULLE (catch page)
                    str->opt->liens[str->opt->lien_tot - 1]->pass2 =
                      max(str->opt->liens[*str->ptr_]->pass2, str->numero_passe);
                    str->opt->liens[str->opt->lien_tot - 1]->retry = str->opt->liens[*str->ptr_]->retry;
                    str->opt->liens[str->opt->lien_tot - 1]->premier = str->opt->liens[*str->ptr_]->premier;
                    str->opt->liens[str->opt->lien_tot - 1]->precedent = str->opt->liens[*str->ptr_]->precedent;
                  } else {      // oups erreur, plus de mémoire!!
                    XH_uninit;  // désallocation mémoire & buffers
                    return 0;
                  }
                } else {
                  hts_log_print(str->opt, LOG_INFO,
                                "moving %s to an existing file %s",
                                str->opt->liens[*str->ptr_]->fil, fil);
                }

              }
            }

            //printf("-> %s %s %s\n",liens[lien_tot-1]->adr,liens[lien_tot-1]->fil,liens[lien_tot-1]->sav);

            // note métaphysique: il se peut qu'il y ait un index.html et un INDEX.HTML
            // sous DOS ca marche pas très bien... mais comme je suis génial url_savename()
            // est à même de régler ce problème
          }
        }                       // ident_url_xx

        if (get_it == 0) {      // adresse vraiment différente et potentiellement en html (pas de possibilité de bouger la page tel quel à cause des <img src..> et cie)
          const size_t rn_size = 8192;
          char *const rn = (char *) malloct(rn_size);
          if (rn != NULL) {
            hts_log_print(str->opt, LOG_WARNING, "File has moved from %s%s to %s",
                          adr, fil, mov_url);
            if (!str->opt->mimehtml) {
              inplace_escape_uri(mov_url, sizeof(mov_url));
            } else {
              char BIGSTK cid[HTS_URLMAXSIZE];
              make_content_id(moved->adr, moved->fil, cid, sizeof(cid));
              strcpybuff(mov_url, "cid:");
              strcatbuff(mov_url, cid);
            }
            // On prépare une page qui sautera immédiatement sur la bonne URL
            // Le scanner re-changera, ensuite, cette URL, pour la mirrorer!
            snprintf(rn, rn_size,
              "<HTML>" CRLF
              "<!-- Created by HTTrack Website Copier/" HTTRACK_VERSION " " HTTRACK_AFF_AUTHORS " -->" CRLF
              "<HEAD>" CRLF 
              "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html;charset=UTF-8\">"
              "<META HTTP-EQUIV=\"Refresh\" CONTENT=\"0; URL=%s\">"
              "<TITLE>Page has moved</TITLE>" CRLF 
              "</HEAD>" CRLF
              "<BODY>" CRLF
              "<A HREF=\"%s\"><h3>Click here...</h3></A>" CRLF
              "</BODY>" CRLF
              "<!-- Created by HTTrack Website Copier/" HTTRACK_VERSION " " HTTRACK_AFF_AUTHORS " -->" CRLF
              "</HTML>" CRLF,
              mov_url, mov_url);

            // changer la page
            if (stre->r_->adr) {
              freet(stre->r_->adr);
            }
            stre->r_->adr = rn;
            stre->r_->size = strlen(stre->r_->adr);
            strcpybuff(stre->r_->contenttype, "text/html");
          }
        }                       // get_it==0

      }                         // bloc
      // erreur HTTP (ex: 404, not found)
    } else if ((stre->r_->statuscode == HTTP_PRECONDITION_FAILED)
               || (stre->r_->statuscode == HTTP_REQUESTED_RANGE_NOT_SATISFIABLE)
      ) {                       // Precondition Failed, c'est à dire pour nous redemander TOUT le fichier
      if (fexist_utf8(str->opt->liens[*str->ptr_]->sav)) {
        remove(str->opt->liens[*str->ptr_]->sav);        // Eliminer
      } else {
        hts_log_print(str->opt, LOG_WARNING,
                      "Unexpected 412/416 error (%s) for %s%s, '%s' could not be found on disk",
                      stre->r_->msg, adr, fil,
                      str->opt->liens[*str->ptr_]->sav != NULL ? str->opt->liens[*str->ptr_]->sav : "");
      }
      if (!fexist_utf8(str->opt->liens[*str->ptr_]->sav)) {    // Bien éliminé? (sinon on boucle..)
#if HDEBUG
        printf("Partial content NOT up-to-date, reget all file for %s\n",
               heap(ptr)->sav);
#endif
        hts_log_print(str->opt, LOG_DEBUG, "Partial file reget (%s) for %s%s",
                      stre->r_->msg, adr, fil);
        // enregistrer le MEME lien
        if (hts_record_link(str->opt, str->opt->liens[*str->ptr_]->adr, str->opt->liens[*str->ptr_]->fil, str->opt->liens[*str->ptr_]->sav, "", "", NULL)) {
          str->opt->liens[str->opt->lien_tot - 1]->testmode = str->opt->liens[*str->ptr_]->testmode;   // mode test?
          str->opt->liens[str->opt->lien_tot - 1]->link_import = 0;   // pas mode import
          str->opt->liens[str->opt->lien_tot - 1]->depth = str->opt->liens[*str->ptr_]->depth;
          str->opt->liens[str->opt->lien_tot - 1]->pass2 = max(str->opt->liens[*str->ptr_]->pass2, str->numero_passe);
          str->opt->liens[str->opt->lien_tot - 1]->retry = str->opt->liens[*str->ptr_]->retry;
          str->opt->liens[str->opt->lien_tot - 1]->premier = str->opt->liens[*str->ptr_]->premier;
          str->opt->liens[str->opt->lien_tot - 1]->precedent = *str->ptr_;
          //
          // canceller lien actuel
          *stre->error_ = 1;
          hts_invalidate_link(str->opt, *str->ptr_);  // invalidate hashtable entry
          //
        } else {              // oups erreur, plus de mémoire!!
          XH_uninit;          // désallocation mémoire & buffers
          return 0;
        }
      } else {
        hts_log_print(str->opt, LOG_ERROR, "Can not remove old file %s", fil);
        *stre->error_ = 1;
      }

      // Error ?
      if (*stre->error_) {
        if (!str->opt->errpage) {
          if (stre->r_->adr) {         // désalloc
            freet(stre->r_->adr);
          }
        }
      }
    } else if (stre->r_->statuscode != HTTP_OK) {
      int can_retry = 0;

      // cas où l'on peut reessayer
      switch (stre->r_->statuscode) {
        //case -1: can_retry=1; break;
      case STATUSCODE_TIMEOUT:
        if (str->opt->hostcontrol) { // timeout et retry épuisés
          if ((str->opt->hostcontrol & 1) && (str->opt->liens[*str->ptr_]->retry <= 0)) {
            hts_log_print(str->opt, LOG_DEBUG, "Link banned: %s%s", adr, fil);
            host_ban(str->opt, *str->ptr_, str->sback, jump_identification_const(adr));
            hts_log_print(str->opt, LOG_DEBUG,
                          "Info: previous log - link banned: %s%s", adr, fil);
          } else
            can_retry = 1;
        } else
          can_retry = 1;
        break;
      case STATUSCODE_SLOW:
        if ((str->opt->hostcontrol) && (str->opt->liens[*str->ptr_]->retry <= 0)) {   // too slow
          if (str->opt->hostcontrol & 2) {
            hts_log_print(str->opt, LOG_DEBUG, "Link banned: %s%s", adr, fil);
            host_ban(str->opt, *str->ptr_, str->sback, jump_identification_const(adr));
            hts_log_print(str->opt, LOG_DEBUG,
                          "Info: previous log - link banned: %s%s", adr, fil);
          } else
            can_retry = 1;
        } else
          can_retry = 1;
        break;
      case STATUSCODE_CONNERROR:       // connect closed
        can_retry = 1;
        break;
      case STATUSCODE_NON_FATAL:       // other (non fatal) error
        can_retry = 1;
        break;
      case STATUSCODE_SSL_HANDSHAKE:   // bad SSL handskake
        can_retry = 1;
        break;
      case 408:
      case 409:
      case 500:
      case 502:
      case 504:
        can_retry = 1;
        break;
      }

      if (strcmp(str->opt->liens[*str->ptr_]->fil, "/primary") != 0) {   // no primary (internal page 0)
        if ((str->opt->liens[*str->ptr_]->retry <= 0) || (!can_retry)) { // retry épuisés (ou retry impossible)
          if ((str->opt->retry > 0) && (can_retry)) {
            hts_log_print(str->opt, LOG_ERROR,
                          "\"%s\" (%d) after %d retries at link %s%s (from %s%s)",
                          stre->r_->msg, stre->r_->statuscode, str->opt->retry, adr, fil,
                          str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->adr,
                          str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->fil);
          } else {
            if (stre->r_->statuscode == STATUSCODE_TEST_OK) {  // test OK
              hts_log_print(str->opt, LOG_INFO, "Test OK at link %s%s (from %s%s)",
                            adr, fil, str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->adr,
                            str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->fil);
            } else {
              if (strcmp(fil, "/robots.txt")) {      // ne pas afficher d'infos sur robots.txt par défaut
                hts_log_print(str->opt, LOG_ERROR,
                              "\"%s\" (%d) at link %s%s (from %s%s)", stre->r_->msg,
                              stre->r_->statuscode, adr, fil,
                              str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->adr,
                              str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->fil);
              } else {
                hts_log_print(str->opt, LOG_DEBUG, "No robots.txt rules at %s", adr);
              }
            }
          }

          // NO error in trop level
          // due to the "no connection -> previous restored" hack
          // This prevent the engine from wiping all data if the website has been deleted (or moved)
          // since last time (which is quite annoying)
          if (str->opt->liens[*str->ptr_]->precedent != 0) {
            // ici on teste si on doit enregistrer la page tout de même
            if (str->opt->errpage) {
              *stre->store_errpage_ = 1;
            }
          } else {
            if (strcmp(fil, "/robots.txt") != 0) {
              /*
                 This is an error caused by a link entered by the user
                 That is, link(s) entered by user are invalid (404, 500, connect error, proxy error->.)
                 If all links entered are invalid, the session failed and we will attempt to restore
                 the previous one
                 Example: Try to update a website which has been deleted remotely: this may delete
                 the website locally, which is really not desired (especially if the website disappeared!)
                 With this hack, the engine won't wipe local files (how clever)
               */
              HTS_STAT.stat_errors_front++;
            }
          }

        } else {                // retry!!
          hts_log_print(str->opt, LOG_NOTICE,
                        "Retry after error %d (%s) at link %s%s (from %s%s)",
                        stre->r_->statuscode, stre->r_->msg, adr, fil,
                        str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->adr,
                        str->opt->liens[str->opt->liens[*str->ptr_]->precedent]->fil);
          // redemander fichier
          if (hts_record_link(str->opt, adr, fil, sav, "", "", stre->codebase)) {
            str->opt->liens[str->opt->lien_tot - 1]->testmode = str->opt->liens[*str->ptr_]->testmode;   // mode test?
            str->opt->liens[str->opt->lien_tot - 1]->link_import = 0;   // pas mode import
            str->opt->liens[str->opt->lien_tot - 1]->depth = str->opt->liens[*str->ptr_]->depth;
            str->opt->liens[str->opt->lien_tot - 1]->pass2 = max(str->opt->liens[*str->ptr_]->pass2, str->numero_passe);
            str->opt->liens[str->opt->lien_tot - 1]->retry = str->opt->liens[*str->ptr_]->retry - 1;     // moins 1 retry!
            str->opt->liens[str->opt->lien_tot - 1]->premier = str->opt->liens[*str->ptr_]->premier;
            str->opt->liens[str->opt->lien_tot - 1]->precedent = str->opt->liens[*str->ptr_]->precedent;
          } else {              // oups erreur, plus de mémoire!!
            return 0;
          }
        }
      } else {
        hts_log_print(str->opt, LOG_DEBUG, "Info: no robots.txt at %s%s", adr, fil);
      }
      if (!*stre->store_errpage_) {
        if (stre->r_->adr) {           // désalloc
          freet(stre->r_->adr);
        }
        *stre->error_ = 1;              // erreur!
      }
      // otherwise, consider this is not an error
    }
    // FIN rattrapage des 301,302,307..
    // ------------------------------------------------------------

  }                             // if !error

  /* Apply changes */
  //ENGINE_SAVE_CONTEXT();

  return 0;

}

/*
  Process pause, link adding..
*/
void hts_mirror_process_user_interaction(const htsmoduleStruct * str,
                                         const htsmoduleStructExtended * stre) {
  int b;

  /* Load engine variables */
  //ENGINE_LOAD_CONTEXT();

#if BDEBUG==1
  printf("\nBack test..\n");
#endif

  // pause/lock files
  {
    int do_pause = 0;

    // user pause lockfile : create hts-paused.lock --> HTTrack will be paused
    char lockfile[255];
    char pausefile[255];
    snprintf(lockfile, 255, "%s%s", str->opt->path_log.buffer_, "hts-stop.lock");
    snprintf(pausefile, 255, "%s%s", str->opt->path_log.buffer_, "hts-paused.lock");
    if (fexist(lockfile)) {
      // remove lockfile
      remove(lockfile);
      if (!fexist(lockfile)) {
        do_pause = 1;
      }
    }
    // after receving N bytes, pause
    if (str->opt->fragment > 0) {
      if ((HTS_STAT.stat_bytes - *stre->stat_fragment_) > str->opt->fragment) {
        do_pause = 1;
      }
    }
    // pause?
    if (do_pause) {
      hts_log_print(str->opt, LOG_INFO, "engine: pause requested..");
      while(back_nsoc(str->sback) > 0) {     // attendre fin des transferts
        back_wait(str->sback, str->opt, str->cache, HTS_STAT.stat_timestart);
        Sleep(200);
        {
          back_wait(str->sback, str->opt, str->cache, HTS_STAT.stat_timestart);

          // Transfer rate
          engine_stats();

          // Refresh various stats
          HTS_STAT.stat_nsocket = back_nsoc(str->sback);
          HTS_STAT.stat_errors = fspc(str->opt, NULL, "error");
          HTS_STAT.stat_warnings = fspc(str->opt, NULL, "warning");
          HTS_STAT.stat_infos = fspc(str->opt, NULL, "info");
          HTS_STAT.nbk = backlinks_done(str->sback, str->opt->liens, str->opt->lien_tot, *str->ptr_);
          HTS_STAT.nb = back_transferred(HTS_STAT.stat_bytes, str->sback);

          b = 0;
          if (!RUN_CALLBACK7
              (str->opt, loop, str->sback->lnk, str->sback->count, b, *str->ptr_, str->opt->lien_tot,
               (int) (time_local() - HTS_STAT.stat_timestart), &HTS_STAT)
              || !back_checkmirror(str->opt)) {
            hts_log_print(str->opt, LOG_ERROR, "Exit requested by shell or user");
            *stre->exit_xh_ = 1;        // exit requested
            XH_uninit;
            return;
          }
        }
      }
      // On désalloue le buffer d'enregistrement des chemins créée, au cas où pendant la pause
      // l'utilisateur ferait un rm -r après avoir effectué un tar
      // structcheck_init(1);
      {
        FILE *fp =
          fopen(pausefile, "wb");
        if (fp) {
          fspc(NULL, fp, "info");       // dater
          fprintf(fp,
                  "Pause" LF "HTTrack is paused after retreiving " LLintP
                  " bytes" LF "Delete this file to continue the mirror->.." LF
                  "" LF "", (LLint) HTS_STAT.stat_bytes);
          fclose(fp);
        }
      }
      *stre->stat_fragment_ = HTS_STAT.stat_bytes;
      /* Info for wrappers */
      hts_log_print(str->opt, LOG_INFO, "engine: pause: %s", pausefile);
      RUN_CALLBACK1(str->opt, pause, pausefile);
    }
    //
  }
  // end of pause/lock files

  // changement dans les préférences
  if (str->opt->state._hts_addurl) {
    lien_adrfilsave add;

    while(*str->opt->state._hts_addurl) {
      char BIGSTK add_url[HTS_URLMAXSIZE];

      add.af.adr[0] = add.af.fil[0] = add_url[0] = '\0';
      if (!link_has_authority(*str->opt->state._hts_addurl))
        strcpybuff(add_url, "http://"); // ajouter http://
      strcatbuff(add_url, *str->opt->state._hts_addurl);
      if (ident_url_absolute(add_url, &add.af) >= 0) {
        // ----Ajout----

        // calculer lien et éventuellement modifier addresse/fichier
        if (url_savename
            (&add, NULL, NULL, NULL, str->opt, str->sback, str->cache, stre->hash_, *str->ptr_, str->numero_passe, NULL) != -1) {
          if (hash_read(stre->hash_, add.save, NULL, HASH_STRUCT_FILENAME) < 0) { // n'existe pas déja
            // enregistrer lien
            if (hts_record_link(str->opt, add.af.adr, add.af.fil, add.save, "", "", NULL)) {
              httrackp*opt = str->opt;
              str->opt->liens[str->opt->lien_tot - 1]->testmode = 0;    // mode test?
              str->opt->liens[str->opt->lien_tot - 1]->link_import = 0; // mode normal
              str->opt->liens[str->opt->lien_tot - 1]->depth = opt->depth;
              str->opt->liens[str->opt->lien_tot - 1]->pass2 = max(0, str->numero_passe);
              str->opt->liens[str->opt->lien_tot - 1]->retry = opt->retry;
              str->opt->liens[str->opt->lien_tot - 1]->premier = str->opt->lien_tot - 1;
              str->opt->liens[str->opt->lien_tot - 1]->precedent = str->opt->lien_tot - 1;
              //
              hts_log_print(opt, LOG_INFO, "Link added by user: %s%s", add.af.adr,
                            add.af.fil);
              //
            } else {            // oups erreur, plus de mémoire!!
              XH_uninit;        // désallocation mémoire & buffers
              return;
            }
          } else {
            hts_log_print(str->opt, LOG_NOTICE,
                          "Existing link %s%s not added after user request",
                          add.af.adr, add.af.fil);
          }

        }
      } else {
        hts_log_print(str->opt, LOG_ERROR, "Error during URL decoding for %s",
                      add_url);
      }
      // ----Fin Ajout----
      str->opt->state._hts_addurl++; // suivante
    }
    str->opt->state._hts_addurl = NULL;      // libérer _hts_addurl
  }
  // si une pause a été demandée
  if (str->opt->state._hts_setpause
      || back_pluggable_sockets_strict(str->sback, str->opt) <= 0) {
    // index du lien actuel
    lien_url*url = str->opt->liens[*str->ptr_];
    b = back_index(str->opt, str->sback, url->adr, url->fil, url->sav);
    int prev = str->opt->state._hts_in_html_parsing;

    if (b < 0)
      b = 0;                    // forcer pour les stats
    while(str->opt->state._hts_setpause || back_pluggable_sockets_strict(str->sback, str->opt) <= 0) { // on fait la pause..
      str->opt->state._hts_in_html_parsing = 6;
      back_wait(str->sback, str->opt, str->cache, HTS_STAT.stat_timestart);

      // Transfer rate
      engine_stats();

      // Refresh various stats
      HTS_STAT.stat_nsocket = back_nsoc(str->sback);
      HTS_STAT.stat_errors = fspc(str->opt, NULL, "error");
      HTS_STAT.stat_warnings = fspc(str->opt, NULL, "warning");
      HTS_STAT.stat_infos = fspc(str->opt, NULL, "info");
      HTS_STAT.nbk = backlinks_done(str->sback, str->opt->liens, str->opt->lien_tot, *str->ptr_);
      HTS_STAT.nb = back_transferred(HTS_STAT.stat_bytes, str->sback);

      if (!RUN_CALLBACK7
          (str->opt, loop, str->sback->lnk, str->sback->count, b, *str->ptr_, str->opt->lien_tot,
           (int) (time_local() - HTS_STAT.stat_timestart), &HTS_STAT)) {
        hts_log_print(str->opt, LOG_ERROR, "Exit requested by shell or user");
        *stre->exit_xh_ = 1;    // exit requested
        XH_uninit;
        return;
      }
      Sleep(100);               // pause
    }
    str->opt->state._hts_in_html_parsing = prev;
  }
  //ENGINE_SAVE_CONTEXT();
  return;
}

/*
Wait for next file and
check 301, 302, .. statuscodes (moved)
*/
int hts_mirror_wait_for_next_file(htsmoduleStruct * str,
                                  htsmoduleStructExtended * stre) {
  /* Load engine variables */
  //ENGINE_DEFINE_CONTEXT();
  int b;
  int n;

  /* This is not supposed to hapen. */
  //lien_url* heap_ = str->opt->liens[*str->ptr_];
#define heap_(N) str->opt->liens[N]
  if (heap_(*str->ptr_)->pass2 == -1) {
    hts_log_print(str->opt, LOG_WARNING, "Link is already ready %s%s", heap_(*str->ptr_)->adr,
                  heap_(*str->ptr_)->fil);
  }

  /* User interaction */
  //ENGINE_SAVE_CONTEXT();
  {
    hts_mirror_process_user_interaction(str, stre);
  }
  //ENGINE_SET_CONTEXT();

  /* Done while processing user interactions ? */
  if (heap_(*str->ptr_)->pass2 == -1) {
    hts_log_print(str->opt, LOG_DEBUG, "Link is now ready %s%s", heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil);
    // We are ready
    return 2;                   // goto jump_if_done;
  }
  // si le fichier n'est pas en backing, le mettre..
  if (!back_exist(str->sback, str->opt, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, heap_(*str->ptr_)->sav)) {
#if BDEBUG==1
    printf("crash backing: %s%s\n", heap(ptr)->adr, heap(ptr)->fil);
#endif
    if (back_add
        (str->sback, str->opt, str->cache, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, heap_(*str->ptr_)->sav,
         str->opt->liens[heap_(*str->ptr_)->precedent]->adr, str->opt->liens[heap_(*str->ptr_)->precedent]->fil,
         heap_(*str->ptr_)->testmode) == -1) {
      printf("PANIC! : Crash adding error, unexpected error found.. [%d]\n",
             __LINE__);
#if BDEBUG==1
      printf("error while crash adding\n");
#endif
      hts_log_print(str->opt, LOG_ERROR, "Unexpected backing error for %s%s", heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil);

    }
  }
#if BDEBUG==1
  printf("test number of socks\n");
#endif

  // ajouter autant de socket qu'on peut ajouter
  n = str->opt->maxsoc - back_nsoc(str->sback);
#if BDEBUG==1
  printf("%d sockets available for backing\n", n);
#endif

  if ((n > 0) && (!str->opt->state._hts_setpause)) { // si sockets libre et pas en pause, ajouter
    // remplir autant que l'on peut le cache (backing)
    back_fillmax(str->sback, str->opt, str->cache, *str->ptr_, str->numero_passe);
  }
  // index du lien actuel
  {
    // ------------------------------------------------------------
    // attendre que le fichier actuel soit prêt - BOUCLE D'ATTENTE
    do {
      /* User interaction */
      //ENGINE_SAVE_CONTEXT();
      {
        hts_mirror_process_user_interaction(str, stre);
      }
      //ENGINE_SET_CONTEXT();

      // index du lien actuel
      b = back_index(str->opt, str->sback, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, heap_(*str->ptr_)->sav);
#if BDEBUG==1
      printf("back index %d, waiting\n", b);
#endif
      // Continue to the loop if link still present
      if (b < 0)
        break;

      // Receive data
      if (str->sback->lnk[b].status > 0)
        back_wait(str->sback, str->opt, str->cache, HTS_STAT.stat_timestart);

      // Continue to the loop if link still present
      b = back_index(str->opt, str->sback, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, heap_(*str->ptr_)->sav);
      if (b < 0)
        break;

      // Stop the mirror
      if (!back_checkmirror(str->opt)) {
        hts_log_print(str->opt, LOG_ERROR, "Exit requested by shell or user");
        *stre->exit_xh_ = 1;    // exit requested
        XH_uninit;
        return 0;
      }
      // And fill the backing stack
      if (str->sback->lnk[b].status > 0)
        back_fillmax(str->sback, str->opt, str->cache, *str->ptr_, str->numero_passe);

      // Continue to the loop if link still present
      b = back_index(str->opt, str->sback, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil, heap_(*str->ptr_)->sav);
      if (b < 0)
        break;

      // autres occupations de HTTrack: statistiques, boucle d'attente, etc.
      if ((str->opt->makestat) || (str->opt->maketrack)) {
        TStamp l = time_local();

        if ((int) (l - stre->makestat_time) >= 60) {
          if (stre->makestat_fp != NULL) {
            fspc(NULL, stre->makestat_fp, "info");
            fprintf(stre->makestat_fp,
                    "Rate= %d (/" LLintP ") \11NewLinks= %d (/%d)" LF,
                    (int) ((HTS_STAT.HTS_TOTAL_RECV -
                            *stre->makestat_total_) / (l - stre->makestat_time)),
                    (LLint) HTS_STAT.HTS_TOTAL_RECV,
                    (int) str->opt->lien_tot - *stre->makestat_lnk_, (int) str->opt->lien_tot);
            fflush(stre->makestat_fp);
            *stre->makestat_total_ = HTS_STAT.HTS_TOTAL_RECV;
            *stre->makestat_lnk_ = str->opt->lien_tot - 1;
          }
          if (stre->maketrack_fp != NULL) {
            int i;

            fspc(NULL, stre->maketrack_fp, "info");
            fprintf(stre->maketrack_fp, LF);
            for(i = 0; i < str->sback->count; i++) {
              back_info(str->sback, i, 3, stre->maketrack_fp);
            }
            fprintf(stre->maketrack_fp, LF);
            fflush(stre->maketrack_fp);

          }
          stre->makestat_time = l;
        }
      }

      /* cancel links */
      {
        int i;
        char *s;

        while((s = hts_cancel_file_pop(str->opt)) != NULL) {
          if (strnotempty(s)) { // fichier à canceller
            for(i = 0; i < str->sback->count; i++) {
              if ((str->sback->lnk[i].status > 0)) {
                if (strcmp(str->sback->lnk[i].url_sav, s) == 0) {  // ok trouvé
                  if (str->sback->lnk[i].status != 1000) {
#if HTS_DEBUG_CLOSESOCK
                    DEBUG_W("user cancel: deletehttp\n");
#endif
                    if (str->sback->lnk[i].r.soc != INVALID_SOCKET)
                      deletehttp(&str->sback->lnk[i].r);
                    str->sback->lnk[i].r.soc = INVALID_SOCKET;
                    str->sback->lnk[i].r.statuscode = STATUSCODE_INVALID;
                    strcpybuff(str->sback->lnk[i].r.msg, "Cancelled by User");
                    str->sback->lnk[i].status = 0; // terminé
                    back_set_finished(str->sback, i);
                  } else        // cancel ftp.. flag à 1
                    str->sback->lnk[i].stop_ftp = 1;
                }
              }
            }
            s[0] = '\0';
          }
          freet(s);
        }

        // Transfer rate
        engine_stats();

        // Refresh various stats
        HTS_STAT.stat_nsocket = back_nsoc(str->sback);
        HTS_STAT.stat_errors = fspc(str->opt, NULL, "error");
        HTS_STAT.stat_warnings = fspc(str->opt, NULL, "warning");
        HTS_STAT.stat_infos = fspc(str->opt, NULL, "info");
        HTS_STAT.nbk = backlinks_done(str->sback, str->opt->liens, str->opt->lien_tot, *str->ptr_);
        HTS_STAT.nb = back_transferred(HTS_STAT.stat_bytes, str->sback);

        if (!RUN_CALLBACK7
            (str->opt, loop, str->sback->lnk, str->sback->count, b, *str->ptr_, str->opt->lien_tot,
             (int) (time_local() - HTS_STAT.stat_timestart), &HTS_STAT)) {
          hts_log_print(str->opt, LOG_ERROR, "Exit requested by shell or user");
          *stre->exit_xh_ = 1;  // exit requested
          XH_uninit;
          return 0;
        }

      }

#if HTS_POLL
      if ((str->opt->shell) || (str->opt->keyboard) || (str->opt->verbosedisplay)
          || (!str->opt->quiet)) {
        TStamp tl;

        *stre->info_shell_ = 1;

        /* Toggle with ENTER */
        if (!str->opt->quiet) {
          if (check_stdin()) {
            char com[256];

            linput(stdin, com, 200);
            if (str->opt->verbosedisplay == 2)
              str->opt->verbosedisplay = 1;
            else
              str->opt->verbosedisplay = 2;
            /* Info for wrappers */
            hts_log_print(str->opt, LOG_INFO, "engine: change-options");
            RUN_CALLBACK0(str->opt, chopt);
          }
        }

        tl = time_local();

        // générer un message d'infos sur l'état actuel
        if (str->opt->shell) {       // si shell
          if ((tl - *stre->last_info_shell_) > 0) {     // toute les 1 sec
            FILE *fp = stdout;
            int a = 0;

            *stre->last_info_shell_ = tl;
            char autopsyfile[255];
            char isalivefile[255];
            snprintf(autopsyfile, 255, "%s%s", StringBuff(str->opt->path_log), "hts-autopsy");
            snprintf(isalivefile, 255, "%s%s", StringBuff(str->opt->path_log), "hts-isalive");
            if (fexist(autopsyfile)) { // débuggage: teste si le robot est vivant
              // (oui je sais un robot vivant.. mais bon.. il a le droit de vivre lui aussi)
              // (libérons les robots esclaves de l'internet!)
              remove(autopsyfile);
              fp =
                fopen(isalivefile, "wb");
              a = 1;
            }
            if ((*stre->info_shell_) || a) {
              int i, j;

              fprintf(fp, "TIME %d" LF, (int) (tl - HTS_STAT.stat_timestart));
              fprintf(fp, "TOTAL %d" LF, (int) HTS_STAT.stat_bytes);
              fprintf(fp, "RATE %d" LF,
                      (int) (HTS_STAT.HTS_TOTAL_RECV /
                             (tl - HTS_STAT.stat_timestart)));
              fprintf(fp, "SOCKET %d" LF, back_nsoc(str->sback));
              fprintf(fp, "LINK %d" LF, str->opt->lien_tot);
              {
                LLint mem = 0;

                for(i = 0; i < str->sback->count; i++)
                  if (str->sback->lnk[i].r.adr != NULL)
                    mem += str->sback->lnk[i].r.size;
                fprintf(fp, "INMEM " LLintP "" LF, (LLint) mem);
              }
              for(j = 0; j < 2; j++) {  // passes pour ready et wait
                for(i = 0; i < str->sback->count; i++) {
                  back_info(str->sback, i, j + 1, stdout);   // maketrack_fp a la place de stdout ?? // **
                }
              }
              fprintf(fp, LF);
              if (a)
                fclose(fp);
              io_flush;
            }
          }
        }                       // si shell

      }                         // si shell ou keyboard (option)
      //
#endif
    } while((b >= 0) && (str->sback->lnk[max(b, 0)].status > 0));

    // If link not found on the stack, it's because it has already been downloaded
    // in background
    // Then, skip it and go to the next one
    if (b < 0) {
      hts_log_print(str->opt, LOG_DEBUG,
                    "link #%d is ready, no more on the stack, skipping: %s%s..",
                    *str->ptr_, heap_(*str->ptr_)->adr, heap_(*str->ptr_)->fil);

      // prochain lien
      // ptr++;

      return 2;                 // goto jump_if_done;

    }
#if 0
    /* FIXME - finalized HAS NO MORE THIS MEANING */
    /* link put in cache by the backing system for memory spare - reclaim */
    else if (back[b].finalized) {
      assertf(back[b].r.adr == NULL);
      /* read file in cache */
      back[b].r =
        cache_read_ro(opt, cache, back[b].url_adr, back[b].url_fil,
                      back[b].url_sav, back[b].location_buffer);
      /* ensure correct location buffer set */
      back[b].r.location = back[b].location_buffer;
      if (back[b].r.statuscode == STATUSCODE_INVALID) {
        hts_log_print(opt, LOG_ERROR,
                      "Unexpected error: %s%s not found anymore in cache",
                      back[b].url_adr, back[b].url_fil);
      } else {
        hts_log_print(opt, LOG_DEBUG, "reclaim file %s%s (%d)", back[b].url_adr,
                      back[b].url_fil, back[b].r.statuscode);
      }
    }
#endif

    if (!str->opt->verbosedisplay) {
      if (!str->opt->quiet) {
        static int roll = 0;    /* static: ok */

        roll = (roll + 1) % 4;
//        printf("%c\x0d", ("/-\\|")[roll]);
        printf("\b%c", ("/-\\|")[roll]);
        fflush(stdout);
      }
    } else if (str->opt->verbosedisplay == 1) {
      if (b >= 0) {
        if (str->sback->lnk[b].r.statuscode == HTTP_OK)
          printf("%d/%d: %s%s (" LLintP " bytes) - OK\33[K\r", *str->ptr_, str->opt->lien_tot,
                 str->sback->lnk[b].url_adr, str->sback->lnk[b].url_fil, (LLint) str->sback->lnk[b].r.size);
        else
          printf("%d/%d: %s%s (" LLintP " bytes) - %d\33[K\r", *str->ptr_, str->opt->lien_tot,
                 str->sback->lnk[b].url_adr, str->sback->lnk[b].url_fil, (LLint) str->sback->lnk[b].r.size,
                 str->sback->lnk[b].r.statuscode);
      } else {
        hts_log_print(str->opt, LOG_ERROR, "Link disappeared");
      }
      fflush(stdout);
    }
    //}

    // ------------------------------------------------------------
    // Vérificateur d'intégrité
#if DEBUG_CHECKINT
    _CHECKINT(&back[b], "Retour de back_wait, après le while") {
      int i;

      for(i = 0; i < back_max; i++) {
        char si[256];

        sprintf(si, "Test global après back_wait, index %d", i);
        _CHECKINT(&back[i], si)
      }
    }
#endif

    // copier structure réponse htsblk
    if (b >= 0) {
      memcpy(stre->r_, &(str->sback->lnk[b].r), sizeof(htsblk));
      stre->r_->location = stre->loc_; // ne PAS copier location!! adresse, pas de buffer
      if (str->sback->lnk[b].r.location)
        strcpybuff(stre->r_->location, str->sback->lnk[b].r.location);
      str->sback->lnk[b].r.adr = NULL;     // ne pas faire de desalloc ensuite

      // libérer emplacement backing
      back_maydelete(str->opt, str->cache, str->sback, b);
    }
    // débug graphique
#if BDEBUG==2
    {
      char s[12];
      int i = 0;

      _GOTOXY(1, 1);
      printf("Rate=%d B/sec\n",
             (int) (HTS_STAT.HTS_TOTAL_RECV /
                    (time_local() - HTS_STAT.stat_timestart)));
      while(i < minimum(back_max, 160)) {
        if (back[i].status > 0) {
          sprintf(s, "%d", back[i].r.size);
        } else if (back[i].status == STATUS_READY) {
          strcpybuff(s, "ENDED");
        } else
          strcpybuff(s, "   -   ");
        while(strlen(s) < 8)
          strcatbuff(s, " ");
        printf("%s", s);
        io_flush;
        i++;
      }
    }
#endif

#if BDEBUG==1
    printf("statuscode=%d with %s / msg=%s\n", r->statuscode, r->contenttype,
           r->msg);
#endif

  }

  //ENGINE_SAVE_CONTEXT();
  return 0;
  
#undef heap_
}

/* Wait for delayed types */
int hts_wait_delayed(htsmoduleStruct * str, lien_adrfilsave *afs,
                     char *parent_adr, char *parent_fil, lien_adrfil *former,
                     int *forbidden_url) {
  //ENGINE_LOAD_CONTEXT_BASE();
  hash_struct *const hash = str->hashptr;

  int in_error = 0;
  LLint in_error_size = 0;
  char in_error_msg[32];

  // resolve unresolved type
  if (str->opt->savename_delayed != 0 && *forbidden_url == 0 && IS_DELAYED_EXT(afs->save)
      && !str->opt->state.stop) {
    int loops;
    int continue_loop;

    hts_log_print(str->opt, LOG_DEBUG, "Waiting for type to be known: %s%s", afs->af.adr,
                  afs->af.fil);

    /* Follow while type is unknown and redirects occurs */
    for(loops = 0, continue_loop = 1;
        IS_DELAYED_EXT(afs->save) && continue_loop && loops < 7; loops++) {
      continue_loop = 0;

      /*
         Wait for an available slot 
       */
      int prev = str->opt->state._hts_in_html_parsing;
      while(back_pluggable_sockets_strict(str->sback, str->opt) <= 0) {
        str->opt->state._hts_in_html_parsing = 6;
        /* Wait .. */
        back_wait(str->sback,str->opt,str->cache,0);
        /* Transfer rate */
        engine_stats();
        /* Refresh various stats */
        HTS_STAT.stat_nsocket=back_nsoc(str->sback);
        HTS_STAT.stat_errors=fspc(str->opt,NULL,"error");
        HTS_STAT.stat_warnings=fspc(str->opt,NULL,"warning");
        HTS_STAT.stat_infos=fspc(str->opt,NULL,"info");
        HTS_STAT.nbk=backlinks_done(str->sback,str->opt->liens,str->opt->lien_tot,*str->ptr_);
        HTS_STAT.nb=back_transferred(HTS_STAT.stat_bytes,str->sback);
        /* Check */
        if (!RUN_CALLBACK7(str->opt, loop, str->sback->lnk, str->sback->count, -1,*str->ptr_,str->opt->lien_tot,(int) (time_local()-HTS_STAT.stat_timestart),&HTS_STAT)) {
          return -1;
        }
      }
      str->opt->state._hts_in_html_parsing = prev;


      /* We can lookup directly in the cache to speedup this mess */
      if (str->opt->delayed_cached) {
        lien_back back;

        memset(&back, 0, sizeof(back));
        back.r = cache_read(str->opt, str->cache, afs->af.adr, afs->af.fil, NULL, NULL);  // test uniquement
        if (back.r.statuscode == HTTP_OK && strnotempty(back.r.contenttype)) {  // cache found, and aswer is 'OK'
          hts_log_print(str->opt, LOG_DEBUG,
                        "Direct type lookup in cache (-%%D1): %s",
                        back.r.contenttype);

          /* Recompute filename with MIME type */
          afs->save[0] = '\0';
          lien_url* heap_ = str->opt->liens[*str->ptr_];
          url_savename(afs, former, heap_->adr,
                       heap_->fil, str->opt, str->sback, str->cache,
                       hash, *str->ptr_, str->numero_passe, &back);

          /* Recompute authorization with MIME type */
          {
            int new_forbidden_url =
              hts_acceptmime(str->opt, *str->ptr_, afs->af.adr, afs->af.fil, back.r.contenttype);
            if (new_forbidden_url != -1) {
              hts_log_print(str->opt, LOG_DEBUG, "result for wizard mime test: %d",
                            new_forbidden_url);
              if (new_forbidden_url == 1) {
                *forbidden_url = new_forbidden_url;
                hts_log_print(str->opt, LOG_DEBUG,
                              "link forbidden because of MIME types restrictions: %s%s",
                              afs->af.adr, afs->af.fil);
                break;          // exit loop
              }
            }
          }

          /* And exit loop */
          break;
        }
      }

      /* Check if the file was recorded already (necessary for redirects) */
      if (hash_read(hash, afs->save, NULL, HASH_STRUCT_FILENAME) >= 0) {
        if (loops == 0) {       /* Should not happend */
          hts_log_print(str->opt, LOG_ERROR,
                        "Duplicate entry in hts_wait_delayed() cancelled: %s%s -> %s",
                        afs->af.adr, afs->af.fil, afs->save);
        }
        /* Exit loop (we're done) */
        continue_loop = 0;
        break;
      }

      /* Add in backing (back_index() will respond correctly) */
      if (back_add_if_not_exists
          (str->sback, str->opt, str->cache, afs->af.adr, afs->af.fil, afs->save, parent_adr, parent_fil,
           0) != -1) {
        int b;

        b = back_index(str->opt, str->sback, afs->af.adr, afs->af.fil, afs->save);
        if (b < 0) {
          printf("PANIC! : Crash adding error, unexpected error found.. [%d]\n",
                 __LINE__);
          XH_uninit;            // désallocation mémoire & buffers
          return -1;
        }

        /* We added the link before the parser recorded it -- the background download MUST NOT clean silently this entry! (Petr Gajdusek) */
        str->sback->lnk[b].early_add = 1;

        /* Cache read failed because file does not exist (bad delayed name!)
           Just re-add with the correct name, as we know the MIME now!
         */
        if (str->sback->lnk[b].r.statuscode == STATUSCODE_INVALID && str->sback->lnk[b].r.adr == NULL) {
          lien_back delayed_back;

          //char BIGSTK delayed_ctype[128];
          // delayed_ctype[0] = '\0';
          // strncatbuff(delayed_ctype, back[b].r.contenttype, sizeof(delayed_ctype) - 1);    // copier content-type
          back_copy_static(&str->sback->lnk[b], &delayed_back);

          /* Delete entry */
          str->sback->lnk[b].r.statuscode = 0;  /* TEMPORARY INVESTIGATE WHY WE FETCHED A SOCKET HERE */
          back_maydelete(str->opt, str->cache, str->sback, b);    // cancel
          b = -1;

          /* Recompute filename with MIME type */
          afs->save[0] = '\0';
          lien_url* heap_ = str->opt->liens[*str->ptr_];
          url_savename(afs, former, heap_->adr,
                       heap_->fil, str->opt, str->sback, str->cache,
                       hash, *str->ptr_, str->numero_passe, &delayed_back);

          /* Recompute authorization with MIME type */
          {
            int new_forbidden_url =
              hts_acceptmime(str->opt, *str->ptr_, afs->af.adr, afs->af.fil, delayed_back.r.contenttype);
            if (new_forbidden_url != -1) {
              hts_log_print(str->opt, LOG_DEBUG, "result for wizard mime test: %d",
                            *forbidden_url);
              if (new_forbidden_url == 1) {
                *forbidden_url = new_forbidden_url;
                hts_log_print(str->opt, LOG_DEBUG,
                              "link forbidden because of MIME types restrictions: %s%s",
                              afs->af.adr, afs->af.fil);
                break;          // exit loop
              }
            }
          }

          /* Re-Add wiht correct type */
          if (back_add_if_not_exists
              (str->sback, str->opt, str->cache, afs->af.adr, afs->af.fil, afs->save, parent_adr, parent_fil,
               0) != -1) {
            b = back_index(str->opt, str->sback, afs->af.adr, afs->af.fil, afs->save);
          }
          if (b < 0) {
            printf
              ("PANIC! : Crash adding error, unexpected error found.. [%d]\n",
               __LINE__);
            XH_uninit;          // désallocation mémoire & buffers
            return -1;
          }

          /* We added the link before the parser recorded it -- the background download MUST NOT clean silently this entry! (Petr Gajdusek) */
          str->sback->lnk[b].early_add = 1;

          hts_log_print(str->opt, LOG_DEBUG,
                        "Type immediately loaded from cache: %s",
                        delayed_back.r.contenttype);
        }

        /* Wait for headers to be received */
        if (b >= 0) {
          back_set_locked(str->sback, b);    // Locked entry
        }
        do {
          if (b < 0)
            break;

          // temps à attendre, et remplir autant que l'on peut le cache (backing)
          if (str->sback->lnk[b].status > 0) {
            back_wait(str->sback, str->opt, str->cache, 0);
          }
          if (*str->ptr_ >= 0) {
            back_fillmax(str->sback, str->opt, str->cache, *str->ptr_, str->numero_passe);
          }
          // on est obligé d'appeler le shell pour le refresh..
          {

            // Transfer rate
            engine_stats();

            // Refresh various stats
            HTS_STAT.stat_nsocket = back_nsoc(str->sback);
            HTS_STAT.stat_errors = fspc(str->opt, NULL, "error");
            HTS_STAT.stat_warnings = fspc(str->opt, NULL, "warning");
            HTS_STAT.stat_infos = fspc(str->opt, NULL, "info");
            HTS_STAT.nbk = backlinks_done(str->sback, str->opt->liens, str->opt->lien_tot, *str->ptr_);
            HTS_STAT.nb = back_transferred(HTS_STAT.stat_bytes, str->sback);

            if (!RUN_CALLBACK7
                (str->opt, loop, str->sback->lnk, str->sback->count, b, *str->ptr_, str->opt->lien_tot,
                 (int) (time_local() - HTS_STAT.stat_timestart), &HTS_STAT)) {
              return -1;
            } else if (str->opt->state._hts_cancel || !back_checkmirror(str->opt)) {      // cancel 2 ou 1 (cancel parsing)
              back_delete(str->opt, str->cache, str->sback, b);        // cancel test
              break;
            }
          }
        } while(
                 /* dns/connect/request */
                 (str->sback->lnk[b].status >= 99 && str->sback->lnk[b].status <= 101)
                 ||
                 /* For redirects, wait for request to be terminated */
                 (HTTP_IS_REDIRECT(str->sback->lnk[b].r.statuscode) && str->sback->lnk[b].status > 0)
                 ||
                 /* Same for errors */
                 (HTTP_IS_ERROR(str->sback->lnk[b].r.statuscode) && str->sback->lnk[b].status > 0)
          );
        if (b >= 0) {
          back_set_unlocked(str->sback, b);  // Unlocked entry
        }
        /* ready (chunked) or ready (regular download) or ready (completed) */

        // Note: filename NOT in hashtable yet - liens_record will do it, with the correct ext!
        if (b >= 0) {
          lien_back delayed_back;

          //char BIGSTK delayed_ctype[128];
          //delayed_ctype[0] = '\0';
          //strncatbuff(delayed_ctype, back[b].r.contenttype, sizeof(delayed_ctype) - 1);    // copier content-type
          back_copy_static(&str->sback->lnk[b], &delayed_back);

          /* Error */
          if (HTTP_IS_ERROR(str->sback->lnk[b].r.statuscode)) {
            /* seen as in error */
            in_error = str->sback->lnk[b].r.statuscode;
            in_error_msg[0] = 0;
            strncat(in_error_msg, str->sback->lnk[b].r.msg, sizeof(in_error_msg) - 1);
            in_error_size = str->sback->lnk[b].r.totalsize;
            /* don't break, even with "don't take error pages" switch, because we need to process the slot anyway (and cache the error) */
          }
          /* Moved! */
          else if (HTTP_IS_REDIRECT(str->sback->lnk[b].r.statuscode)) {
            char BIGSTK mov_url[HTS_URLMAXSIZE];

            mov_url[0] = '\0';
            strcpybuff(mov_url, str->sback->lnk[b].r.location);    // copier URL

            /* Remove (temporarily created) file if it was created */
            UNLINK(fconv(OPT_GET_BUFF(str->opt), OPT_GET_BUFF_SIZE(str->opt), str->sback->lnk[b].url_sav));

            /* Remove slot! */
            if (str->sback->lnk[b].status == STATUS_READY) {
              back_maydelete(str->opt, str->cache, str->sback, b);
            } else {            /* should not happend */
              back_delete(str->opt, str->cache, str->sback, b);
            }
            b = -1;

            /* Handle redirect */
            if (strnotempty(mov_url)) {   // location existe!
              lien_adrfil moved;
              moved.adr[0] = moved.fil[0] = '\0';
              //
              if (ident_url_relatif(mov_url, afs->af.adr, afs->af.fil, &moved) >= 0) {
                hts_log_print(str->opt, LOG_DEBUG,
                              "Redirect while resolving type: %s%s -> %s%s",
                              afs->af.adr, afs->af.fil, moved.adr, moved.fil);
                // si non bouclage sur soi même, ou si test avec GET non testé
                if (strcmp(moved.adr, afs->af.adr) != 0 || strcmp(moved.fil, afs->af.fil) != 0) {

                  // recopier former->adr/fil?
                  if (former != NULL) {
                    if (strnotempty(former->adr) == 0) { // Pas déja noté
                      strcpybuff(former->adr, afs->af.adr);
                      strcpybuff(former->fil, afs->af.fil);
                    }
                  }
                  // check explicit forbidden - don't follow 3xx in this case
                  {
                    int set_prio_to = 0;

                    if (hts_acceptlink(str->opt, *str->ptr_, moved.adr, moved.fil, NULL, NULL, &set_prio_to, NULL) == 1) {     /* forbidden */
                      /* Note: the cache 'cached_tests' system will remember this error, and we'll only issue ONE request */
                      *forbidden_url = 1;       /* Forbidden! */
                      hts_log_print(str->opt, LOG_DEBUG,
                                    "link forbidden because of redirect beyond the mirror scope at %s%s -> %s%s",
                                    afs->af.adr, afs->af.fil, moved.adr, moved.fil);
                      strcpybuff(afs->af.adr, moved.adr);
                      strcpybuff(afs->af.fil, moved.fil);
                      mov_url[0] = '\0';
                      break;
                    }
                  }

                  // ftp: stop!
                  if (strfield(mov_url, "ftp://")) {
                    strcpybuff(afs->af.adr, moved.adr);
                    strcpybuff(afs->af.fil, moved.fil);
                    break;
                  }

                  /* ok, continue */
                  strcpybuff(afs->af.adr, moved.adr);
                  strcpybuff(afs->af.fil, moved.fil);
                  continue_loop = 1;

                  /* Recompute filename for hash lookup */
                  afs->save[0] = '\0';
                  lien_url*heap_ = str->opt->liens[*str->ptr_];
                  url_savename(afs, former, heap_->adr, heap_->fil,
                               str->opt, str->sback, str->cache, hash, *str->ptr_, str->numero_passe,
                               &delayed_back);
                } else {
                  hts_log_print(str->opt, LOG_WARNING,
                                "Unable to test %s%s (loop to same filename)",
                                afs->af.adr, afs->af.fil);
                }               // loop to same location
              }                 // ident_url_relatif()
            }                   // location
          }                     // redirect
          hts_log_print(str->opt, LOG_DEBUG, "Final type for %s%s: '%s'", afs->af.adr, afs->af.fil,
                        delayed_back.r.contenttype);

          /* If we are done, do additional checks with final type and authorizations */
          if (!continue_loop) {
            /* Recompute filename with MIME type */
            afs->save[0] = '\0';
            lien_url*heap_ = str->opt->liens[*str->ptr_];
            url_savename(afs, former,
                         heap_->adr, heap_->fil, str->opt,
                         str->sback, str->cache, hash, *str->ptr_, str->numero_passe, &delayed_back);

            /* Recompute authorization with MIME type */
            {
              int new_forbidden_url =
                hts_acceptmime(str->opt, *str->ptr_, afs->af.adr, afs->af.fil, delayed_back.r.contenttype);
              if (new_forbidden_url != -1) {
                hts_log_print(str->opt, LOG_DEBUG, "result for wizard mime test: %d",
                              *forbidden_url);
                if (new_forbidden_url == 1) {
                  *forbidden_url = new_forbidden_url;
                  hts_log_print(str->opt, LOG_DEBUG,
                                "link forbidden because of MIME types restrictions: %s%s",
                                afs->af.adr, afs->af.fil);
                  break;        // exit loop
                }
              }
            }
          }

          /* Still have a back reference */
          if (b >= 0) {
            /* Finalize now as we have the type */
            if (str->sback->lnk[b].status == STATUS_READY) {
              if (!str->sback->lnk[b].finalized) {
                hts_log_print(str->opt, LOG_TRACE, "finalizing as we have the type");
                back_finalize(str->opt, str->cache, str->sback, b);
              }
            }
            /* Patch destination filename for direct-to-disk mode */
            strcpybuff(str->sback->lnk[b].url_sav, afs->save);
          }

        }                       // b >= 0
      } else {
        printf("PANIC! : Crash adding error, unexpected error found.. [%d]\n",
               __LINE__);
        XH_uninit;              // désallocation mémoire & buffers
        return -1;
      }

    }                           // while(IS_DELAYED_EXT(save))

    if (in_error != 0) {
      /* 'no error page' selected or file discarded by size rules! */
      if (!str->opt->errpage || (in_error == STATUSCODE_TOO_BIG)) {
        /* Note: the cache 'cached_tests' system will remember this error, and we'll only issue ONE request */
#if 0
        /* No (3.43) - don't do that. We must not post-exclude an authorized link, because this will prevent the cache
           system from processing it, leading to refetch it endlessly. Just accept it, and handle the error as
           usual during parsing.
         */
        *forbidden_url = 1;     /* Forbidden! */
#endif
        if (in_error == STATUSCODE_TOO_BIG) {
          hts_log_print(str->opt, LOG_INFO,
                        "link not taken because of its size (%d bytes) at %s%s",
                        (int) in_error_size, afs->af.adr, afs->af.fil);
        } else {
          hts_log_print(str->opt, LOG_INFO,
                        "link not taken because of error (%d '%s') at %s%s",
                        in_error, in_error_msg, afs->af.adr, afs->af.fil);
        }
      }
    }
    // error
    if (*forbidden_url != 1 && IS_DELAYED_EXT(afs->save)) {
      *forbidden_url = 1;
      if (in_error) {
        hts_log_print(str->opt, LOG_WARNING,
                      "link in error (%d '%s'), type unknown, aborting: %s%s",
                      in_error, in_error_msg, afs->af.adr, afs->af.fil);
      } else {
        hts_log_print(str->opt, LOG_WARNING,
                      "link is probably looping, type unknown, aborting: %s%s",
                      afs->af.adr, afs->af.fil);
      }
    }

  }                             // delayed type check ?

  return 0;
}
