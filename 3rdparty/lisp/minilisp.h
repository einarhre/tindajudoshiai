#ifndef _MINILISP_H_
#define _MINILISP_H_

extern int lisp_print_script;

int lisp_init(int argc, char **argv);
char *lisp_exe(char *code);

void lisp_set_competitor(int who, int ix,
			 const char *first, const char *last,
			 const char *club, const char *country,
			 const char *regcategory, const char *category,
			 const char *compid,
			 const char *comment, const char *coachid,
			 int birthyear, int belt,
			 int weight, int seeding, int clubseeding, int gender, int flags);

void lisp_set_match(int cat, int num, int c1, int c2,
		    int s1, int s2, int p1, int p2,
		    int mt, int com, int t, int g, int f,
		    int ft, int fn, int d, int l, int r);
void lisp_set_comp_points(int ix, int wins, int pts, int res);


#endif
