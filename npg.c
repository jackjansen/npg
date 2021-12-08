#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#undef putchar
extern putchar();
/*
 * newpg - newer page program
 *
 * author : jack jansen, 1980.
 */

/* constants */

/***********************************************************/
/*							   */
/*   compile time options. Remove /* on beginning of each  */
/*   line to effectuate option.				   */
/* #define DEBUG 1			/* debug mode	   */
/* #define SIMTERM 1			/* simple ttytype  */
#define BERTERM 1			/* berkely ttys    */
/* NOTE : npg uses CL,CM,CO,LI,CE and the		   */
/*        non-standard AO (Aux on) and AE (Aux off).	   */
/***********************************************************/
#ifndef SIMTERM
#ifndef BERTERM
#define NOTERM 1
#endif
#endif

#ifdef NOTERM
/* very simple terminal interface. recognizes one type of
 * display terminals, and non-display terminals.
 */
#define clrseq "\033E\200\200\200"	/* clear seq for uB,mB,B100 */
#define homseq "\033H\200\200\200"	/* home seq for uB,mB,B100 */
#define cllseq "\033K\200\200\200"	/* clr line seq for uB,mB,B100 */
#define lboseq "\033H\200\200\200\033A"	/* left lower corner */
#define aonseq ""			/* aux on line */
#define aofseq ""			/* aux off line */

#else

/* variables for BERTERM and SIMTERM interface */
char *clrseq,*cllseq,*homseq,*lboseq,*aonseq,*aofseq,*CM,*UP;

#ifdef BERTERM
char terminf[512];			/* string for stroing ca strings*/
#endif
#endif

#ifdef SIMTERM
#define TERMFILE "/etc/termcap"
#endif

#define BUFLEN 1024			/* size of file buffers.NOTE: */
					/* !! should be power of two */
#define ADRMSK (BUFLEN-1)		/* mask for file addresses */
#define NBUF   4			/* number of file buffers */
#define D_SSIZE 20			/* default screen size */
#define D_LLEN  78			/* line length */
#define NALLOC  8			/* number of linfos at once */

/* type definitions */

#define TRUE   1
#define FALSE  0
#define logical char
struct linfo {
	struct linfo *next;		/* pointer to info for next 8 lines */
	long diska;			/* disk address for first line here */
	char llen[8];			/* length of these 8 lines */
	};

/* terminal interface */

#ifdef SIMTERM
struct {
	int n1,n2,n3;
	int t_width,t_length;
	int ca,cl,aa,au,ho,ll;
	int u1,u2,u3;
	char t_str[490];
} tdesc;

#endif

char *term;				/* terminal name */

/* inline functions */
#define outstr(s) { register char *xxx; for(xxx = s;*xxx;)putc(*xxx++,stdout); }
#ifdef BERTERM
#define clear() tputs(clrseq,1,putchar)
#define loleft() tputs(tgoto(CM,0,o_ssize),1,putchar)
#define home() tputs(tgoto(CM,0,0),1,putchar)
#else
#define clear() outstr(clrseq)
#define home() outstr(homseq)
#define loleft() outstr(lboseq)
#endif
#define clrlin() outstr(cllseq)
#define auxon()  outstr(aonseq)
#define auxoff() outstr(aofseq)

/* routines */

extern char *getenv();
extern char *malloc();
    /* variables */

jmp_buf delreturn;			/* longjmp struct for DEL */
char helpfn[] = "/user2/geert/doc/npg:help?"; /* name of helpfile */
#define HNRPOS 25			/* position of helpnr in helpfn */
char *fname;				/* pointer to filename */
char fnbuf[64];				/* area to read filename */
char **oargv,**gargv;			/* old and current argv */
int oargc,gargc;			/* old and current argc */
logical otherf;				/* true if other file paged*/
char sstring[256];			/* the search-string */
int sstrlen;				/* the length of the sstring */
logical anchor;				/* TRUE if string started with ^ */
logical endfhit;			/* true if end of file hit.*/
struct linfo *first,*ffree,*llinfo;	/* line list, free list, last line */
logical badline;			/* set true if bad linenumber given */
long goodline;				/* good line if badline = 1 */
logical lastknown;			/* set true if last line is known */
long lkline,lkladr;			/* last known line& its address */
logical badseek;			/* true if bad seek */
long lkadr;				/* first unknown addr in file */
int curllen;				/* length of current line */
long curline;				/* current line number */
char cback = 0;				/* character pushed back */
logical o_nr = TRUE ;			/* print line numbers option */
logical o_under = FALSE;		/* true if header should be under*/
logical o_header = TRUE ; 		/* print header line */
logical o_vdu = TRUE ;			/* true if visual display used */
logical o_list = FALSE ;		/* true if ctrl characters expansion */
int o_ssize = D_SSIZE;			/* # of lines displayed per page */
int o_llen = D_LLEN;			/* #of chars used per line */
logical o_aux = FALSE;			/* true if aux port on */
#ifdef DEBUG
logical o_debug = FALSE;		/* true if debug on */
#endif
long lastlp;				/* linenr of last line printed */
long lastprd;				/* last position read on file*/
logical printit;			/* true if current screen is to be printed */
logical stopped,warning,errmes;		/* true if 'q' given, warning given, error */
char lastc;				/* set to last char read */
char buffer[NBUF][BUFLEN];		/* the file buffers */
int bflen[NBUF];			/* the length of info inthe buffer */
long bfadr[NBUF];			/* the fileaddress of the buffer */
int bfage[NBUF];			/* gives the age of the buffer */
int age;				/* current age */
int curbuf;				/* number of current buffer */
char *curbpos,*curbend;			/* current buffer pos/end */
int pfd;				/* fd of paged file */
char shcmd[80];				/* used to store shell cmd */

/*main program */

main(argc,argv) int argc; char **argv;
{
register int i;
register char *t,*s;

    oargc = gargc = argc;
    oargv = gargv = argv;
    setbuf(stdout,malloc(BUFSIZ));
    setbuf(stdin,malloc(BUFSIZ));
    s = (stdin)->_ptr;
    t = getenv("NPG_STUP");
    if( t != NULL )
	while(*s++ = *t++);
     else s++;
    *--s = '\n';
    (stdin)->_cnt = s - (stdin)->_ptr+1;
    (stdin)->_file = (stdout)->_file;
    term = getenv("TERM");
    newterm(term);
    if( o_vdu ) clear();
    next(TRUE);
    process();
    if( o_vdu ) loleft();
    outstr("Goodbye...\n");
    fclose(stdout);
    exit(0);
}

/* routines */

struct linfo *newlinf() {
/* newlinf returns a new lie info pointer */
register struct linfo *rv,*arr;
register int i;

    if(ffree == NULL ){
	arr = (struct linfo *)malloc(NALLOC* sizeof *ffree);
	if( arr == NULL ) { error("Out of core. Use 'b' to continue.\n");
		longjmp(delreturn);
	}
	ffree = arr;
	for(i=0;i<NALLOC-1;i++) arr[i].next = &(arr[i+1]);
	arr[NALLOC-1].next = NULL;
    }
    rv = ffree;
    ffree = rv->next;
    rv->next = NULL;
    return(rv);
}

error(mes) char *mes; {
/* print an error message */
    if( o_vdu ) { home(); 
	putc('\n',stdout);
	putc('\n',stdout);
	clrlin();
    }
    printf("ERROR:%s",mes);
    errmes = TRUE;
}

logical rdopt() {
/* rdopt returns TRUE for +, FALSE for - */
register char c;
    lastc = c = getchar();
    if( c == '+' ) return(TRUE);
    if( c != '-' ) error("Option should be '+' or '-'\n");
    return( FALSE );
}

next(ftime) logical ftime; {
/* next starts reading the ext file. ftime is true the first time */
register char c;
register char *nfn;
register int i;
int nfd;

    lastc = 0;
    if( !ftime ) {
	c = getchar();
	if(c == 'r') {
	    gargc = oargc;
	    gargv = oargv;
	    if( o_vdu){ home(); clrlin(); }
	    printf("%d more files to page.\n",oargc-1);
	    return;
	} else if( c == '?' ) {
	    if( o_vdu ) loleft();
	    for(i = 1; i < oargc; i++){
		putc(' ',stdout);
		if( !otherf && i == oargc - gargc ) putc('[',stdout);
		outstr(oargv[i]);
		if( !otherf && i == oargc - gargc ) putc(']',stdout);
		putc(' ',stdout);
	    }
	    if( otherf ) printf(". [%s]",fnbuf);
	    putc('\n',stdout);
	    return;
	} else if( c != ' ' && c != '\n' && c != '=' ) ungetc(c,stdin);
    }
    if( c != '\n' && !ftime && c != ',') {
	scanf("%[^ ,\n]",fnbuf);
	otherf = TRUE;
	c = getchar();
	nfn = fnbuf;
    } else {
	otherf = FALSE;
	gargc--;
	gargv++;
	if( gargc <= 0) if( ftime ){
			    nfd = 0;
			    goto okee;
			} else {
			    error("No more files to page.\n");
			    gargc++;
			    gargv--;
			    lastc = c;
			    return;
	}
	if( gargv[0][0] == '-' && gargv[0][1] == '\0' ) {
		nfd = 0;
		goto okee;
	}
	nfn = gargv[0];
    }
    lastc = c;
    if( o_vdu ) clear();
    printf("Paging file %s", nfn);
    if(gargc > 1)
	printf(", %d more files to page",gargc - 1);
    printf(".\n");
    if( ( nfd = open(nfn,0)) < 0){ error("Cannot open file.\n");
				   if( nfn == fnbuf ) {
					fnbuf[0] = '?';
					fnbuf[1] = '\0';
					nfd = -1;
				   }
				   if(!ftime)return;
				 }
okee:
    if( pfd != 0 )close(pfd);
    pfd = nfd;
    fname = nfn;
    cback = 0;
    if( pfd == 0 ) fname = "Standard Input";
    if( llinfo != NULL ) {
	llinfo -> next = ffree;
	ffree = first;
    }
    first = NULL;
    llinfo = NULL;
    lastknown = FALSE;
    lastprd = lkline = lkladr = lkadr = lastlp = 0L;
    for(i = 0;i < NBUF; i++) bfadr[i] = -1L;
    curbpos = curbend;
    curline = 0L;
}

process() {
/* process reads user commands and calls command to execute them */
register char c;
logical cmddone;
int delcatch();

	lastc = ' ';
	cmddone = TRUE;
	warning = FALSE;
	signal( SIGINT , delcatch);
	while ( !stopped) {
		if( lastc == '\n' ) {
			setjmp(delreturn);
			if( printit ) printpg(curline);
			if( o_vdu && o_under ) {
			   loleft();
			} else if( o_vdu ) {
			    home();
			    putc('\n',stdout);
			    clrlin();
			}
			putc('>',stdout);
			fflush(stdout);
			cmddone = FALSE;
		}
		printit = FALSE;
		errmes = FALSE;
		while ((( c = getchar()) == ' ' ) || ( c == '|' ) || ( c == ','));
		lastc = c;
		if( c == '\n'&& !cmddone )
		    { printit = TRUE; curline = lastlp +1L; }
		   else { cmddone = TRUE; command(c); }
		if( errmes&& lastc != '\n') {
		    outstr("Remainder line skipped.\n");
		    while( (lastc = getchar()) != '\n');
		} else putc('\n',stdout);
		if( stopped && !warning && gargc > 1) {
			if(o_vdu) { home();putc('\n',stdout);putc('\n',stdout);}
			outstr(" warning : not all files paged yet\n");
			while((lastc = getchar()) != '\n');
			warning = TRUE;
			stopped = FALSE;
		}
		   else if( !stopped) warning = FALSE;
	}
}

shell() {
/* shell calls the shell */
int i,j;
char c;
int delcatch();
	signal( SIGINT , SIG_IGN);
	lastc = c = getchar();
	if(c != '!' && c != ' '&& c != '\n') {
	    ungetc(c,stdin);
	    scanf("%[^\n]",shcmd);
	}
	if( o_vdu ) loleft();
	fflush(stdout);
	if( (i = fork()) == 0) {
		if( o_vdu ) loleft();
		close(0);dup(1);
		signal(SIGINT, SIG_DFL);
		if( c == ' ' || c == '\n') execl("/bin/sh","-",0);
		execl("/bin/sh","sh","-c",shcmd,0);
		error(" Cannot execute shell.\n");
	} else if( i > 0) while( i!= wait(&j) );
	    else error(" Cannot fork. Try again.\n");
	if( o_vdu ) { home(); clrlin(); }
	signal( SIGINT , delcatch );
}
help(nr) char nr; {
/* help prints one of the help bulletins. */
register char *c;
register int l,fd;

	helpfn[HNRPOS] = nr;
	if( ( fd=open(helpfn,0) ) < 0) {
		error("Cannot open helpfile.\n");
		return;
	}
	if( o_vdu )clear();
	putc('\n',stdout);putc('\n',stdout);
	fflush(stdout);
	l = newbuf();
	bfadr[l] = -1L;
	c = buffer[l];
	while( l = read(fd,c,BUFLEN) ) { write(1,c,l); if( l < BUFLEN) break; }
	close(fd);
}

newbuf() {
/* newbuf returns the number of a free buffer */
register int i,j;

	j = 0;
	for(i=0;i<NBUF;i++){
		if(bfadr[i] == -1L){j=i;break; }
		if( bfage[i] < bfage[j]) j = i;
	}
	age++;
	bfage[j] = age;
	return(j);
}

getbuf(adr) long adr; {
/* getbuf loads the block containing 'adr' in core */
register int b,l;

	l = 0;
	b = newbuf();
	if( pfd != 0 ) lseek(pfd,adr&~ADRMSK,0);
	badseek = pfd == 0 && (adr & ~ADRMSK) != lastprd;
	bfadr[b] = adr &~ ADRMSK;
	if( !badseek ){
	     l = read(pfd,buffer[b],BUFLEN);
	     lastprd = bfadr[b] + l;
	}
	bflen[b] = l;
	endfhit = l == 0 || badseek;
#ifdef DEBUG
if(o_debug)printf("GETBUF:buffer = %d,adr = %ld,len =%d.\n",
		  b,bfadr[b],bflen[b]);
#endif
	return (b);
}

setpos(adr) long adr; {
/* setpos sets the 'readc' pointer to'adr', and loads the block(if necc.) */
register int b,i;
register int l;

	badseek = endfhit = FALSE;
	for(i = 0; i<NBUF; i++)
	   if( bfadr[i] != -1L && bfadr[i] <= adr &&bfadr[i]+bflen[i] > adr) {
		b = i;
		goto found;
	    } else if( bfadr[i] == (adr & ~ADRMSK) && lastprd == adr ){
		l = read(pfd,buffer[i]+bflen[i],BUFLEN-bflen[i]);
		bflen[i] += l;
		lastprd += l;
		b = i;
		goto found;
	    }
	b = getbuf(adr);
found:
#ifdef DEBUG
if(o_debug)printf("SETPOS:found adr %ld in buffer %d.\n",adr,b);
#endif
	if( endfhit ) { curbpos = curbend = 0;
			bfadr[b] = -1L;
			return;
	}
	curbuf = b;
	i = adr & ADRMSK;
	curbpos = buffer[b] + i;
	curbend = buffer[b] + bflen[b];
}


readc() {
/* read next char from file */
	if( cback ) { register char c;
			 c = cback;
			 cback = 0;
			 return(c);
	}
	if(curbpos >= curbend)
	    setpos(bfadr[curbuf] + bflen[curbuf] );
	if(curbpos >= curbend) return(EOF);
	return( *curbpos++);
}

logical compare(s1,s2) char *s1,*s2;
/* compare returns TRUE if s1 == beginning of s2 */
{
register char *p,*q;

    p = s1;
    for(q = s2; *p && *q && *p == *q ; p++ , q++ );;
    return( *p == '\0');
}

long search() {
/* search searches for the current string expression */
long cclin;
register int llen;
register int ich;
register char *p;
char linbuf[255];
long findadr();

    setpos( findadr( cclin = curline+1L) );
    if(badline)setpos(findadr(cclin=1L));
    while( cclin != curline){
	llen = 2;
	for( p=linbuf;
	    (*p = ich = readc()) != '\n'&&ich != EOF && llen < 255;
	    p++)llen++;
	    *p++ = '\0';
	if( ich == EOF ) {
	    setpos( findadr( cclin = 1L) );
	    continue;
	}
	for(p = linbuf; llen>= sstrlen; p++){
	    llen--;
	    if(compare(sstring,p))return(cclin);
	    if(anchor)break;
	}
	cclin++;
    }
    return(curline);
}

compile(strtc) char strtc;
/* compile compiles a search string, and calls search to search it */
{
register int c;
register char *p;
long newadr;
register logical escaped;

    if( (c = getchar()) != '\n' && c != strtc) {
	if( c == '^') { 
	    anchor = TRUE;
	    c = getchar();
	} else anchor = FALSE;
	p = sstring;
	do {
	    if( escaped = c == '\\' ) c = getchar();
	    *p++ = c;
	    c = getchar();
	} while ( c != strtc && c != EOF && c != '\n');
	if( ! escaped && p[-1] == '$' ) p[-1] = '\n';
	*p = 0;
	sstrlen = p - sstring;
    }
    newadr = search();
    if( newadr == curline ) error("String not found.\n");
	else printit = TRUE;
    curline = newadr;
    if( c == '\n') lastc = '\n'; else lastc = getchar();
}

command(c) char c; {
/* execute command */
long adr;
long prvpage();
register int i;

switch (c) {
case '\n':	/* ignore */
	return;
#ifdef DEBUG
case 'd':
	o_debug = rdopt();
	return;
case 'B':
	for(i = 0; i < NBUF; i++){
	    printf("Buffer %d : \n",i);
	    printf("	File address = %ld.\n",bfadr[i]);
	    printf("	Core address = 0%o.\n",buffer[i]);
	    printf("	Age = %d.\n",bfage[i]);
	    printf("	Length = %d.\n",bflen[i]);
	}
	printf("Current buffer = %d.\n",curbuf);
	printf("Start = 0%o, length = 0%o.\n",curbpos,curbend);;
	lastc = getchar();
	return;
#endif
case '/':	/* search command */
	compile(c);
	return; /* printit is set in compile */
case 'u':
	o_under = rdopt();
	return;
case 'f':
	o_header = rdopt();
	return;
case '$':
	if( lastknown ) curline = lkline;
	    else curline = 017777777777L;
	findadr(curline);
	if( badline ) curline = goodline;
	curline = prvpage(curline)+5L;
	lastc = getchar();
	break;
case '!':	/* execute shell command */
	shell(); return;
case '.':	/* print current page */
case 'p':
	lastc = getchar();
	break;
case '?':
case 'h':	/* help command */
	lastc = c = getchar();
	if( '0' <= c && c <= '9') help(c) ; else help('0');
	return;
case 'n':	/* next command */
	next(FALSE);
	return;
case 'b':	/* set pseudo-first block */
	if( first != NULL ) {
		error(" Block setting not allowed after printing.\n");
		return;
	}
	scanf("%ld",&lkadr);
	if((lastc = getchar()) == 'b') {
	    lkadr = lkadr << 9;
	    lastc = getchar();
	}
	return;
case '-':	/* - cmd or -nnn cmd */
	if((c=getchar()) >= '0' && c <='9') {
		ungetc(c,stdin);
		if( scanf("%ld",&adr) <= 0) {
			error("Bad line offset.\n");
			return;
		}
		curline = curline - adr;
		lastc = getchar();
	} else {
		curline = prvpage(curline);
		lastc = c;
	}
	break;
case '+':	/* + cmd or +nnn cmd */
	if((c = getchar()) >= '0' && c <= '9') {
		ungetc(c,stdin);
		if( scanf("%ld",&adr) <= 0) {
			error("Bad line offset.\n");
			return;
		}
		curline = curline + adr;
		lastc = getchar();
	} else {
		curline = lastlp + 1L;
		lastc = c;
	}
	break;
case 'l':	/* l+ or l- */
	o_list = rdopt();
	return;
case 't':	/* t termname */
	scanf(" %[^\n,]",term);
	newterm(term);
	lastc = getchar();
	return;
case 'a':	/* aux-on or aux-off */
	o_aux = rdopt();
	return;
case 'w':
	if( scanf("%d",&o_llen ) <= 0) {
	    error("Error in screen width.\n");
	    o_llen = D_LLEN;
	}
	return;
case '#':	/* #+ or #- */
case 'r':	/* r+ or r- */
	o_nr = rdopt();
	return;
case '\0':	/* control-d */
case 'q':	/* quit */
	stopped = TRUE;
	return;
case 's':	/* snnn , set screen size */
	if( scanf("%d",&o_ssize) <= 0) {
		error("Error in screen size.\n");
		o_ssize = D_SSIZE;
	}
	return;
default:	/* nnn cmd */
	if( '0' <= c && c <= '9') {
		ungetc(c,stdin);
		if( scanf("%ld",&curline) <= 0)
		    error(" Error in line number.\n");
		lastc = getchar();
		break;
	} else { error("Unknown command : ");printf("'%c'.",c);}
	return;
}
printit = TRUE;
}


printpg(line) long line; {
/* printpg prints a page starting with line */
register char c;
register int cpos;
long bloka;
long fline;
int slno;
long findadr();

	if( o_vdu) {
	    clear();
	    if( !o_under )  putc('\n',stdout);
	    putc('\n',stdout);
	    slno = 2;
	} else slno = 0;

	if( pfd < 0 ) { error("No current page file.\n");
			return;
	}
	setpos( bloka = findadr(line) );
	if( badline ) curline = line = goodline;
	fline = line;
	bloka = bloka>>9;
	if( o_aux ) auxon();
	while( slno < o_ssize) {
	    if(slno + (curllen + o_llen-1) / o_llen >= o_ssize) {
		line--;
		break;
	    }
	    if( o_nr) printf("%6ld\t",line);
	    if( badseek ) {
		outstr("	*** *** Cannot seek on pipe *** ***\n");
		break;
	    }
	    cpos = o_nr ? 8 : 0;
	    c = ' ';
	    while ( c != '\n' && c != EOF ){
		c = readc();
		if((c & 0177) >= ' ' || !o_list)
		    {
			if( c < ' ' && c != '\b' && c != '\t' && c != '\n')putc('?',stdout);
			else if( c != '\n' ) putc(c,stdout);
		    }
		else switch(c & 0177) {
		case '\t' : outstr("-\b>"); break;
		case '\b' : outstr("-\b<"); break;
		case '\n' : putc('$',stdout); break;
		case '\r' : outstr("\\r"); break;
		case '\0' : outstr("\\0"); break;
		default   : printf("\\0%o%o",(c & 070)>>3 , c & 7);
			    cpos += 3;
			    break;
		}
		if( !o_list && c == '\t') cpos = (cpos+8)&~07;
		    else cpos++;
		if( cpos >= o_llen && c != '\n'
		    && c != EOF && (cback = readc() ) != '\n' ) {
			outstr("\\\n");
			slno++;
			if( o_nr ) putc('\t',stdout);
			cpos = o_nr ? 8 : 0;
		}
	    }
	    if( slno < o_ssize) {
		line++;
		setpos(findadr(line));
		if( badline ) {
		    if( o_aux ) auxoff();
		    while(slno++ < o_ssize-1) {
			putc('\n',stdout);
			slno++;
		    }
		    outstr("\t\t** ** END OF FILE ** **");
		    slno++;
		    line--;
		}
		putc('\n',stdout);
		slno++;
	    }
	}
	if( o_aux ) auxoff();
	if( o_header ) {
	    if( o_vdu && !o_under) home();
	    if( o_vdu && o_under ) putc('\n',stdout);
	    printf(" Paging file %s, lines %ld thru %ld, block %ld\n",
	    	fname,fline,line,bloka);
	}
	if( line > fline+1L ) lastlp = line-1L;
		else lastlp = line;
}

long findadr(line) long line; {
/* findadr returns the diskadr belonging to line line */
register int i,j;
register struct linfo *clp;
long hl;
int lastlen;

	badline = FALSE;
	if( line <= 0L ) {
	    goodline=line = 1L;
	    badline = TRUE;
	}
	if( line > lkline && !lastknown)
	{
	 register char *bptr;

	    setpos(lkadr);
	    while( line > lkline & !lastknown) {
	     register int c;

		clp = newlinf();
		if( first != NULL ) {
		    llinfo->next = clp;
		    llinfo = clp;
		} else llinfo = first = clp;
		clp->diska = lkadr;
		clp->next  = NULL;
		for(i=0;i<8;i++){
		    j = 0;
		    do {
			c = readc();
			lkadr++;
			j++;
		    } while ( c != EOF && c != '\n' && j != 255);
		    clp->llen[i] = j;
		    lkline++;
#ifdef DEBUG
if(o_debug)printf("FINDADR:Line %ld,length %d.\n",lkline,j);
#endif
		    if( c == EOF ){
			lkladr = lkadr-lastlen;
			lkline--;
			lastknown = TRUE;
			break;
		    }
		    lastlen = j;
		}
	    }
	}
	if( line > lkline) {
	     badline = 1;
	     goodline = lkline;
	     return(lkladr-1L);
	 }
	line--;
	clp = first;
	i = line >> 3;
	while( i--)
	    if( clp != NULL ) clp=clp->next;
	hl = clp-> diska;
	for(i = 0; i<(line&7);i++)hl += ((int)clp->llen[i] & 0377 );
	curllen = ((int)clp->llen[line&7] & 0377 );
	return(hl);
}

#ifdef BERTERM
newterm(name) char *name;
{
 char tinfo[1024];
 char *tp;

    if( tgetent(tinfo,name) <= 0 ){
	error("Unknown terminalname.\n");
	return;
    }
    tp = terminf;
    o_ssize = tgetnum("li")-1;
    o_llen  = tgetnum("co")-2;
    clrseq  = tp ; tgetstr("cl",&tp); *tp++ = '\0';
    cllseq  = tp ; tgetstr("ce",&tp); *tp++ = '\0';
    aonseq  = tp ; tgetstr("ao",&tp); *tp++ = '\0';
    aofseq  = tp ; tgetstr("ae",&tp); *tp++ = '\0';
    UP      = tp ; tgetstr("up",&tp); *tp++ = '\0';
    CM      = tp ; tgetstr("cm",&tp); *tp++ = '\0';
    o_vdu = *clrseq && *CM ? TRUE : FALSE;
}
#endif

#ifdef SIMTERM
newterm(name) char *name;
{ int fd;

    fd = open(TERMFILE,0);
    if( fd < 0){ error("Cannot open terminal file.\n");
		 o_vdu = FALSE;
		 return;
		}
    while( read(fd,tdesc,sizeof tdesc) >= sizeof tdesc){
	if( compare(name,&tdesc.t_str[tdesc.n1]) ||
	    compare(name,&tdesc.t_str[tdesc.n2]) ||
	    compare(name,&tdesc.t_str[tdesc.n3]) ) {
		clrseq = &tdesc.t_str[tdesc.ca];
		homseq = &tdesc.t_str[tdesc.ho];
		cllseq = &tdesc.t_str[tdesc.cl];
		lboseq = &tdesc.t_str[tdesc.ll];
		aonseq = &tdesc.t_str[tdesc.aa];
		aofseq = &tdesc.t_str[tdesc.au];
		if( tdesc.t_length != 0 ) o_ssize = tdesc.t_length;
		if( tdesc.t_width != 0 ) o_llen = tdesc.t_length;
		o_vdu = ( clrseq && homseq &&
			  cllseq && lboseq ) ? TRUE : FALSE;
		return;
	}
    }
    error("Unknown terminal type.\n");
    o_vdu = FALSE;
}
#endif
#ifdef NOTERM
newterm(name) char *name;
{
    o_vdu = compare(name,"minibee") ||
	    compare(name,"MiniBee") ||
	    compare(name,"B-100") ||
	    compare(name,"microbee") ||
	    compare(name,"MicroBee") ||
	    compare(name,"b100");
}
#endif

delcatch() {

    error("Interrupt.\n");
    signal( SIGINT , delcatch );
    printit = FALSE;
    lastc = '\n';
    (stdin)->_cnt = 0;
    longjmp(delreturn);
}

long prvpage(line) long line; {
 register struct linfo *clp;
 register int i,nlin;

    nlin = 2;
    while(line > 0) {
	clp = first;
	i = line>>3;
	while( i-- ) if( clp != NULL ) clp=clp->next;
	if( clp == NULL ) return(line);
	i = (line & 07) + 1;
	while( --i>=0 ) {
	    nlin += ( (((int)clp->llen[i])&0377)+o_llen+10)/o_llen;
	    if( nlin >= o_ssize ) return((long) (line & ~07) + i + 1);
	}
	line = (line & ~07 ) -1;
    }
    return((long) 1);
}
