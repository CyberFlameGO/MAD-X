#include "madx.h"

// private types

struct val_mic {
	double before[2];
	double after[2];
};

struct id_mic {
	int id_ttb;
	int enable;
	struct val_mic val;
	struct node* p_node;
	struct id_mic *next;
	struct id_mic *previous;
};

struct id_mic2 {
	int id_ttb[2];
	int enable;
	struct val_mic val;
	struct node* p_node;
	struct node* p_node_s1;
	struct node* p_node_s2;
	struct id_mic2 *next;
	struct id_mic2 *previous;
};

struct orb_cor {
	double qx0;
	double qy0;
	double units;
	struct id_mic *cor_table;
	struct id_mic *mon_table;
};

struct orb_cor2 {
	double qx0;
	double qy0;
	double units;
	struct id_mic2 *cor_table;
	struct id_mic2 *mon_table;
};

// forward declarations

static void correct_correct(struct in_cmd*);
static void correct_usemonitor(struct in_cmd*);
static void correct_usekick(struct in_cmd*);
static void correct_putorbit(struct in_cmd*);
static void correct_getorbit(struct in_cmd*); /* empty */
static void correct_option(struct in_cmd*);
static void correct_readcorr(struct in_cmd*);
static void correct_setcorr(struct in_cmd*);

static void correct_correct1(struct in_cmd*);
static int pro_correct_getactive(int ip, int *nm, int *nx, int *nc,
		double *corvec, double *monvec, char *conm);
static void pro_correct_write_results(double *monvec, double *resvec,
		double *corvec, int *nx, int *nc, int *nm, int imon, int icor, int ip);
static void pro_correct_fill_mon_table(int ip, char *name, double old,
		double new_);
static void pro_correct_fill_corr_table(int ip, char *name, double old,
		double new_);
static void pro_correct_make_mon_table(void);
static void pro_correct_make_corr_table(void);
static double* pro_correct_response_line(int ip, int nc, int nm);
static double* pro_correct_response_ring(int ip, int nc, int nm);
static int pro_correct_filter(int iplane, double sigcut);
static void pro_correct_write_cocu_table(void);
static void pro_correct_prtwiss(void);
static int pro_correct_getcorrs(struct in_cmd*);
static int pro_correct_getorbit_ext(struct in_cmd*);
static int pro_correct_getorbit(struct in_cmd*);
static int pro_correct_gettables(int iplane, struct in_cmd*);
static int pro_correct_getcommands(struct in_cmd*);
// static void    pro_correct_option(struct in_cmd*);

static void correct_correct2(struct in_cmd*);
static void pro_correct2_fill_mon_table(int ip, char *name, double old,
		double new_);
static void pro_correct2_fill_corr_table(int b, int ip, char *name, double old,
		double new_);
static void pro_correct2_make_mon_table(void);
static void pro_correct2_make_corr_table(void);
static double* pro_correct2_response_ring(int ip, int nc, int nm);
static int pro_correct2_getactive(int ip, int *nm, int *nx, int *nc,
		double *corvec, double *monvec, char *conm);
static int pro_correct2_getcorrs(struct in_cmd*);
static int pro_correct2_getorbit(struct in_cmd*);
static int pro_correct2_gettables(int iplane, struct in_cmd*);
static void pro_correct2_write_results(double *monvec, double *resvec,
		double *corvec, int *nx, int *nc, int *nm, int imon, int icor, int ip);

static void fill_orbit_table(struct table* t_out, struct table* t_in);

// private interface

static double crms(double *r, int m) {
	double xave = { 0.0 };
	double xrms = { 0.0 };
	int i;

	for (i = 0; i < m; i++) {
		xave = xave + r[i];
	}
	xave = xave / m;
	for (i = 0; i < m; i++) {
		xrms = xrms + (xave - r[i]) * (xave - r[i]);
	}
	xrms = sqrt(xrms / m);

	return (xrms);
}

static double cprp(double *r, int m) {
	double xhi = { -9999. };
	double xlo = { 9999. };
	double xptp = { 0.0 };
	int i;

	for (i = 0; i < m; i++) {
		if (r[i] < xlo) xlo = r[i];
		if (r[i] > xhi) xhi = r[i];
	}
	xptp = xhi - xlo;

	return (xptp);
}

static double copk(double *r, int m) {
	double xpk = { -9999. };
	int i;

	for (i = 0; i < m; i++) {
		if (fabs(r[i]) > xpk) xpk = fabs(r[i]);
	}

	return (xpk);
}

static int c_micit(double *dmat, char *conm, double *monvec, double *corvec,
	double *resvec, int *nx, float rms, int imon, int icor, int niter) {
	const char *rout_name = "c_micit";
	int *ny;
	int ifail;
	float *ax, *cinx, *xinx, *resx;
	float *rho, *ptop, *rmss, *xrms, *xptp, *xiter;

	/* allocate auxiliary vectors used by correction algorithms */
	ny = mycalloc_atomic("c_micit_ny" , icor , sizeof *ny);
	ax = mycalloc_atomic("c_micit_ax" , imon*icor, sizeof *ax);
	cinx = mycalloc_atomic("c_micit_cinx" , icor , sizeof *cinx);
	xinx = mycalloc_atomic("c_micit_xinx" , imon , sizeof *xinx);
	resx = mycalloc_atomic("c_micit_resx" , imon , sizeof *resx);
	rho = mycalloc_atomic("c_micit_rho" , 3*icor , sizeof *rho);
	ptop = mycalloc_atomic("c_micit_ptop" , icor , sizeof *ptop);
	rmss = mycalloc_atomic("c_micit_rmss" , icor , sizeof *rmss);
	xrms = mycalloc_atomic("c_micit_xrms" , icor , sizeof *xrms);
	xptp = mycalloc_atomic("c_micit_xptp" , icor , sizeof *xptp);
	xiter = mycalloc_atomic("c_micit_xiter", icor , sizeof *xiter);

	micit_(dmat, conm, monvec, corvec, resvec, nx, &rms, &imon, &icor, &niter,
			ny, ax, cinx, xinx, resx, rho, ptop, rmss, xrms, xptp, xiter,
			&ifail);

	myfree(rout_name, ny);
	myfree(rout_name, ax);
	myfree(rout_name, cinx);
	myfree(rout_name, xinx);
	myfree(rout_name, resx);
	myfree(rout_name, rho);
	myfree(rout_name, ptop);
	myfree(rout_name, rmss);
	myfree(rout_name, xrms);
	myfree(rout_name, xptp);
	myfree(rout_name, xiter);

	return (ifail);
}

static void c_haveit(double *dmat, double *monvec, double *corvec,
		double *resvec, int *nx, int imon, int icor) {
	const char *rout_name = "c_haveit";
	double *cb, *xmeas, *xres, *y, *z, *xd;

	cb = mycalloc_atomic("c_haveit_cb" , icor , sizeof *cb);
	xmeas = mycalloc_atomic("c_haveit_xmeas", imon , sizeof *xmeas);
	xres = mycalloc_atomic("c_haveit_xres" , imon , sizeof *xres);
	y = mycalloc_atomic("c_haveit_y" , icor*imon , sizeof *y);
	z = mycalloc_atomic("c_haveit_z" , icor*icor , sizeof *z);
	xd = mycalloc_atomic("c_haveit_xd" , icor , sizeof *xd);

	haveit_(dmat, monvec, corvec, resvec, nx, &imon, &icor, cb, xmeas, xres, y,
			z, xd);

	myfree(rout_name, cb);
	myfree(rout_name, xmeas);
	myfree(rout_name, xres);
	myfree(rout_name, y);
	myfree(rout_name, z);myfree(rout_name, xd);
}

static int c_svddec(double *dmat, int imon, int icor, int *sing, double *sngcut,
		double *sngval)
{
	const char *rout_name = "c_svddev";
	int flag;
	int dbg;

	double *s, *u, *v, *w, *ut, *vt, *wt;
	double *ws, *wv;
	int *sw;

	s = mycalloc_atomic("c_svddec_s" , icor*imon, sizeof *s);
	u = mycalloc_atomic("c_svddec_u" , icor*imon, sizeof *u);
	v = mycalloc_atomic("c_svddec_v" , icor*imon, sizeof *v);
	w = mycalloc_atomic("c_svddec_w" , icor*imon, sizeof *w);
	ut = mycalloc_atomic("c_svddec_ut", icor*imon, sizeof *ut);
	vt = mycalloc_atomic("c_svddec_vt", icor*imon, sizeof *vt);
	wt = mycalloc_atomic("c_svddec_wt", icor*imon, sizeof *wt);
	ws = mycalloc_atomic("c_svddec_ws", icor , sizeof *ws);
	wv = mycalloc_atomic("c_svddec_wv", icor , sizeof *wv);
	sw = mycalloc_atomic("c_svddec_sw", icor , sizeof *sw);

	dbg = debug_correct_opt;

	if (imon >= icor)
		svddec_m_(dmat, s, u, v, w, ut, vt, wt, ws, wv, sw, sngcut, sngval,
				&imon, &icor, &flag, sing, &dbg);
	else
		svddec_c_(dmat, s, u, v, w, ut, vt, wt, ws, wv, sw, sngcut, sngval,
				&imon, &icor, &flag, sing, &dbg);

	myfree(rout_name, s);
	myfree(rout_name, u);
	myfree(rout_name, v);
	myfree(rout_name, w);
	myfree(rout_name, ut);
	myfree(rout_name, vt);
	myfree(rout_name, wt);
	myfree(rout_name, ws);
	myfree(rout_name, wv);
	myfree(rout_name, sw);

	return flag;
}

static int c_svdcorr(double *dmat, double *xin, double *cor, double *res,
		int *nx, int imon, int icor) {
	const char *rout_name = "c_svdcorr";
	int flag;
	int dbg;

	double *s, *u, *v, *w, *ut, *vt, *wt;
	double *xa, *xb, *xp, *wv, *ws;
	int *sw;

	s = mycalloc_atomic("c_svdcorr_s" , icor*imon, sizeof *s);
	u = mycalloc_atomic("c_svdcorr_u" , icor*imon, sizeof *u);
	v = mycalloc_atomic("c_svdcorr_v" , icor*imon, sizeof *v);
	w = mycalloc_atomic("c_svdcorr_w" , icor*imon, sizeof *w);
	ut = mycalloc_atomic("c_svdcorr_ut", icor*imon, sizeof *ut);
	vt = mycalloc_atomic("c_svdcorr_vt", icor*imon, sizeof *vt);
	wt = mycalloc_atomic("c_svdcorr_wt", icor*imon, sizeof *wt);

	xa = mycalloc_atomic("c_svdcorr_xa", imon, sizeof *xa);
	xb = mycalloc_atomic("c_svdcorr_xb", imon, sizeof *xb);
	xp = mycalloc_atomic("c_svdcorr_xp", imon, sizeof *xp);
	ws = mycalloc_atomic("c_svdcorr_xp", icor, sizeof *ws);
	wv = mycalloc_atomic("c_svdcorr_xp", icor, sizeof *wv);

	sw = mycalloc_atomic("c_svdcorr_sw", icor, sizeof *sw);

	dbg = debug_correct_opt;

	if (imon >= icor)
		svdcorr_m_(dmat, s, u, v, w, ut, vt, wt, xin, cor, res, xa, xb, xp, ws,
				wv, sw, nx, &imon, &icor, &flag, &dbg);
	else
		svdcorr_c_(dmat, s, u, v, w, ut, vt, wt, xin, cor, res, xa, xb, xp, ws,
				wv, sw, nx, &imon, &icor, &flag, &dbg);

	myfree(rout_name, s);
	myfree(rout_name, u);
	myfree(rout_name, v);
	myfree(rout_name, w);
	myfree(rout_name, ut);
	myfree(rout_name, vt);
	myfree(rout_name, wt);
	myfree(rout_name, sw);
	myfree(rout_name, xa);
	myfree(rout_name, xb);
	myfree(rout_name, xp);
	myfree(rout_name, ws);
	myfree(rout_name, wv);

	return flag;
}

static void fill_orbit_table(struct table* t_out, struct table* t_in)
/* fills a table with orbit values at monitor positions */
{
	int i, j, pos;
	t_out->curr = 0;
	for (i = 0; i < t_in->curr; i++) {
		if (strstr(t_in->s_cols[1][i], "monitor")) {
			for (j = 0; j < t_out->num_cols; j++) {
				if ((pos = name_list_pos(t_out->columns->names[j],
						t_in->columns)) > -1) {
					if (t_out->columns->inform[j] < 3)
						t_out->d_cols[j][t_out->curr] = t_in->d_cols[pos][i];
					else
						t_out->s_cols[j][t_out->curr] = tmpbuff(
								t_in->s_cols[pos][i]);
				} else {
					if (t_out->columns->inform[j] < 3)
						t_out->d_cols[j][t_out->curr] = zero;
					else
						t_out->s_cols[j][t_out->curr] = tmpbuff(blank);
				}
			}
			t_out->curr++;
		}
	}
}

/* revert to old version after Thys Risselada's fix of Micado */

static void correct_setcorr(struct in_cmd* cmd) {
	/* read the correctors from named table and stores
	 them in the nodes of the sequence at
	 "->chkick" and "->cvkick". Subsequent Twiss will
	 use them correctly.
	 ===> Must be preceded by a call to "read_table"
	 ===> (unless table exists in memory !)
	 ===> Watch out, does not yet take care of existing corrector
	 ===> settings already present in sequence
	 ===> Uses table with name specified in parameter: table=
	 */

	int i, ix;

	struct node *ndexe;
	struct node *nextnode;

	char name[NAME_L];
	char slname[NAME_L];

	char nname[NAME_L];
	char slnname[NAME_L];

	char* namtab;
//  int      t1;

	double xnew, ynew;

	/* set up pointers to current sequence for later use */
	struct sequence* mysequ = current_sequ;
	nextnode = mysequ->ex_start;
	ndexe = mysequ->ex_end;

	/* printf("Pointers: %d %d %d\n",mysequ,nextnode,ndexe); */

	if ((namtab = command_par_string("table", cmd->clone)) != NULL ) {
		printf("Want to use named table: %s\n", namtab);
		if (name_list_pos(namtab, table_register->names) > -1) { // (t1 = not used
			printf("The table ==> %s <=== was found \n", namtab);
		} else {
			/* fatal_error("Corrector table requested, but not existing:",namtab); */
			/* exit(-77); */
			printf("No such corrector table in memory: %s\n", namtab);
		}

	} else {
		if (get_option("debug")) {
			printf("No table name requested\n");
			printf("Use default name\n");
		}
		strcpy(namtab, "corr");
	}

	i = 1;
	ix = 0;
	while (ix == 0) {
		ix = string_from_table_row(namtab, "name", &i, name);
		ix = double_from_table_row(namtab, "px.correction", &i, &xnew);
		ix = double_from_table_row(namtab, "py.correction", &i, &ynew);
		if (ix == 0) {
			stolower(name);
			strcpy(slname, strip(name));
			supp_tb(slname);

			/* printf("corrs: %s %d %e %e %e %e\n",name,ix,xold,yold,xnew,ynew); */
			nextnode = mysequ->ex_start;
			while (nextnode != ndexe) {
				stolower(name);
				strcpy(slname, strip(name));
				supp_tb(slname);

				strcpy(nname, nextnode->name);
				stolower(nname);
				strcpy(slnname, strip(nname));
				supp_tb(slnname);

				/* printf("seq and input (0): %s %d %s %d\n", nname,strlen(nname),  name,strlen(name));
				 printf("seq d in (2): %s %d %s %d\n",slnname,strlen(slnname),slname,strlen(slname)); */

				if (strcmp(slname, slnname) == 0) {
					/*
					 printf("Corrector selection found: %s, %s %d\n",lname,nextnode->name,nextnode->sel_err);
					 printf("corrs: %s %d %e %e %e %e\n",name,ix,xold,yold,xnew,ynew);
					 printf("corrs in sequence: %s %e %e\n",nextnode->name,nextnode->chkick,nextnode->cvkick);
					 */
					nextnode->chkick += xnew;
					nextnode->cvkick += ynew;
					/*
					 printf("corrs in sequence: %s %e %e\n",nextnode->name,nextnode->chkick,nextnode->cvkick);
					 */
					nextnode = ndexe;
				} else {
					nextnode = nextnode->next;
				}
			}

		}
		i++;
	}
	return;
}

static void correct_readcorr(struct in_cmd* cmd) {
	/* read the correctors from table "corr" and stores
	 them in the nodes of the sequence at
	 "->chkick" and "->cvkick". Subsequent Twiss will
	 use them correctly.
	 ===> Must be preceded by a call to "read_table"
	 ===> Watch out, does not yet take care of existing corrector
	 ===> settings already present in sequence
	 ===> Always uses table with name "corr", will change ...
	 */

	int i, ix;

	struct node *ndexe;
	struct node *nextnode;

	char name[NAME_L];
	char lname[NAME_L];
	char slname[NAME_L];
	char* uslname;

	char nname[NAME_L];
	char lnname[NAME_L];
	char slnname[NAME_L];
	char* uslnname;

	double xnew, ynew;

	/* set up pointers to current sequence for later use */
	struct sequence* mysequ = current_sequ;
	nextnode = mysequ->ex_start;
	ndexe = mysequ->ex_end;

	/* printf("Pointers: %d %d %d\n",mysequ,nextnode,ndexe); */

	(void) cmd;
	i = 1;
	ix = 0;
	while (ix == 0) {
		ix = string_from_table_row("corr", "name", &i, name);
		ix = double_from_table_row("corr", "px.correction", &i, &xnew);
		ix = double_from_table_row("corr", "py.correction", &i, &ynew);
		if (ix == 0) {
			/* printf("corrs: %s %d %e %e %e %e\n",name,ix,xold,yold,xnew,ynew); */
			nextnode = mysequ->ex_start;
			while (nextnode != ndexe) {
				strcpy(lname, name);
				stolower(lname);
				strcpy(slname, strip(lname));
				uslname = supp_tb(slname);

				strcpy(nname, nextnode->name);
				strcpy(lnname, nname);
				stolower(lnname);
				strcpy(slnname, strip(lnname));
				uslnname = supp_tb(slnname);

				/* printf("seq and input (0): %s %d %s %d\n", nname,strlen(nname),  name,strlen(name));
				 printf("seq d in (1): %s %d %s %d\n",lnname,strlen(lnname),lname,strlen(lname));
				 printf("seq d in (2): %s %d %s %d\n",slnname,strlen(slnname),slname,strlen(slname));
				 printf("seq d in (3): %s %d %s %d\n",uslnname,strlen(uslnname),uslname,strlen(uslname));
				 printf("compare: %s %d %s %d \n",uslname,strlen(uslname),uslnname,strlen(uslnname)); */

				if (strcmp(uslname, uslnname) == 0) {
					/*
					 printf("Corrector selection found: %s, %s %d\n",lname,nextnode->name,nextnode->sel_err);
					 printf("corrs: %s %d %e %e %e %e\n",name,ix,xold,yold,xnew,ynew);
					 printf("corrs in sequence: %s %e %e\n",nextnode->name,nextnode->chkick,nextnode->cvkick);
					 */
					nextnode->chkick += xnew;
					nextnode->cvkick += ynew;
					/*
					 printf("corrs in sequence: %s %e %e\n",nextnode->name,nextnode->chkick,nextnode->cvkick);
					 */
					nextnode = ndexe;
				} else {
					nextnode = nextnode->next;
				}
			}

		}
		i++;
	}
}

static void correct_correct2(struct in_cmd* cmd)
/* Steering routine for orbit corrections of two beams */
{
	const char *rout_name = "correct_correct2";

	/*
	 struct name_list* spos = sequences->list;
	 struct table *twb1;
	 struct table *twb2;
	 int idrop;
	 int pos;
	 struct timeb tp;
	 int sflag, svdflg;
	 double  sigcut;
	 */

	int ix, im, ip; // , it; not used
	int i, j, nnnseq; // ,err // not used
	int imon, icor;
	int ncorr, nmon;
	int niter;

	int twism;
	int ifail;
	float rms;
	double rrms;
	double tmp1, tmp2, tmp3, tmp4;
	char *clist, *mlist; /* file names for monitor and corrector output */
	char clist1[100], clist2[100]; /* file names for corrector output ring 1 and ring 2 */
	double *dmat = { NULL }; /* response matrix, double precision */
	double *corvec, *monvec; /* vectors to hold measured orbit and correctors */
	double *resvec; /* vector to hold corrected orbit */
	char *conm; /* vector to hold corrector names (for MICADO) */
//  int     *sing;  // not used /* array to store pointer to singular correctors */
	static int *nm, *nx, *nc;
	struct id_mic2 *c;
	/*
	 struct id_mic2   *m;
	 */

	int debug;

	debug = get_option("debug");


	/* If only Twiss summary is required prepare and write it */
	// Jun 26, 2013 8:06:33 PM ghislain : moved up from **twiss summary**
	if ((twism = command_par_value("twissum", cmd->clone)) > 0) {
		if (ftdata == NULL ) {
			if ((ftdata = fopen("twiss.summ", "w")) == NULL )
				exit(99);
		}
		j = 1;
		if ((nnnseq = get_variable("n")) == 0) {
			nnnseq = twism;
		}
		double_from_table_row("summ", "xcomax", &j, &tmp1); // err = not used
		double_from_table_row("summ", "xcorms", &j, &tmp2); // err = not used
		double_from_table_row("summ", "ycomax", &j, &tmp3); // err = not used
		double_from_table_row("summ", "ycorms", &j, &tmp4); // err = not used
		fprintf(ftdata, " T: %d %e %e %e %e\n", nnnseq, tmp1, tmp2, tmp3, tmp4);
		printf("TWISSUM: Data from twiss summary written to twiss.summ; aborting correction\n");
		return; // abort the correction here
	}

	strcpy(clist1, "\0");
	strcpy(clist2, "\0");

	printf("for two beams orbit corrections ...\n");
	ip = pro_correct_getcommands(cmd);
	im = pro_correct2_gettables(ip, cmd);
	ncorr = im % 10000;
	nmon = im / 10000;
	printf("%d monitors and %d correctors found in input\n", nmon, ncorr);
	if (nmon == 0) {
		printf("No monitor found in input, no correction done\n");
		return;
	}
	if (ncorr == 0) {
		printf("No corrector found in input, no correction done\n");
		return;
	}

	/* For debugging set output buffer to zero */
	if (get_option("debug"))
		setbuf(stdout, NULL );

	/* Prepare file descriptors for the output */
	if (command_par_value("resout", cmd->clone) > 0) {
		if (fddata == NULL ) {
			if ((fddata = fopen("corr.out", "w")) == NULL )
				exit(99);
		}
		if (fcdata == NULL ) {
			if ((fcdata = fopen("stren.out", "w")) == NULL )
				exit(99);
		}
		if (fgdata == NULL ) {
			if ((fgdata = fopen("plot.orb", "w")) == NULL )
				exit(99);
		}
	}


	/* allocate vectors used by correction algorithms */
	nx = mycalloc_atomic("correct_correct2_nx", ncorr, sizeof *nx);
	nc = mycalloc_atomic("correct_correct2_nc", ncorr, sizeof *nc);
	nm = mycalloc_atomic("correct_correct2_nm", nmon , sizeof *nm);
	// sing   = mycalloc_atomic("correct_correct2_sing", ncorr*2, sizeof *sing); // not used

	corvec = mycalloc_atomic("correct_correct2_corvec", ncorr , sizeof *corvec);
	monvec = mycalloc_atomic("correct_correct2_monvec", nmon , sizeof *monvec);
	resvec = mycalloc_atomic("correct_correct2_resvec", nmon , sizeof *resvec);
	conm = mycalloc_atomic("correct_correct2_conm" , ncorr*16, sizeof *conm);

	/* get original settings of correctors from input Twiss-table */
	pro_correct2_getcorrs(cmd); // it = not used
	/* get input orbit, default is from input Twiss-table */
	pro_correct2_getorbit(cmd);  // it = not used

	/* find and prepare enabled correctors and monitors, may be repeated */
	ix = pro_correct2_getactive(ip, nm, nx, nc, corvec, monvec, conm);
	icor = ix % 10000;
	imon = ix / 10000;
	printf("%d monitors and %d correctors enabled\n", imon, icor);

	if (get_option("debug")) {
		for (i = 0; i < icor; i++)
			printf("C: %d %d \n", nx[i], nc[i]);
		for (i = 0; i < imon; i++)
			printf("M: %d %e \n", nm[i], monvec[i]);
	}

	if (strcmp("ring", command_par_string("flag", cmd->clone)) == 0) {
		if (dmat != NULL )
			myfree(rout_name, dmat);
		/* icor and imon used to set up correct matrix size !! */
		dmat = pro_correct2_response_ring(ip, icor, imon);
	} else {
		printf("INVALID MACHINE TYPE\n");
		exit(-1);
	}

	/* MICADO correction, get desired number of correctors from command */
	corrl = command_par_value("corrlim", cmd->clone);
	set_variable("corrlim", &corrl);

	if (strcmp("micado", command_par_string("mode", cmd->clone)) == 0) {
		printf("enter MICADO correction ...\n");

		niter = command_par_value("ncorr", cmd->clone);
		if (niter == 0) {
			printf("Requested %d correctors (\?\?\?) set to %d\n", niter, icor);
			niter = icor;
		} else if (niter < 0) {
			printf("Requested %d correctors (\?\?\?) set to 0\n", niter);
			niter = 0;
		} else if (niter > icor) {
			printf("Fewer correctors available than requested by ncorr\n");
			printf("you want %d,  you get %d\n", niter, icor);
			printf("ncorr reset to %d\n", icor);
			niter = icor;
		}

		// 2013-Jun-24  10:57:43  ghislain: FIXME why the multiplication by 1000 ?
		rms = 1000.0 * command_par_value("error", cmd->clone);
		/*frs       micit_(dmat,monvec,corvec,resvec,nx,&rms,&imon,&icor,&niter); */
		/* printf("Time before micado:  %-6.3f\n",fextim());  */
		ifail = c_micit(dmat, conm, monvec, corvec, resvec, nx, rms, imon, icor,
				niter);

		printf("Back from micado %d\n", ifail);
		if (ifail != 0) {
			printf("MICADO correction completed with error code %d\n\n", ifail);
			warning("MICADO back with error", ", no correction done");
		}

		rrms = crms(monvec, imon);
		printf("RMS before %e\n", rrms);
		rrms = crms(resvec, imon);
		printf("RMS after  %e\n", rrms);

		if (fgdata != NULL ) {
			for (i = 0; i < nmon; i++) {
				fprintf(fgdata, "%e %e \n", monvec[i], resvec[i]);
			}
		}
		/*
		 for (i=0; i<nmon; i++) {
		 printf("monvec: %d %e \n",i,monvec[i]);
		 }
		 printf("\n");
		 for (i=0; i<nmon; i++) {
		 printf("resvec: %d %e \n",i,resvec[i]);
		 }
		 m = correct_orbit12->mon_table;
		 for (i=0; i<nmon; i++) {
		 printf("resvec: %s %e \n",m[nm[i]].p_node->name,resvec[i]);
		 }
		 printf("\n");
		 for (i=0; i<ncorr; i++) {
		 printf("corvec: %d %e \n",i,corvec[i]);
		 }
		 printf("\n");
		 */

		c = correct_orbit12->cor_table;
		for (i = 0; i < icor; i++) {
			printf("%s %e\n", c[nc[i]].p_node->name, corvec[nx[i] - 1]);
		}
		printf("\n");

		/* printf("Time after micado:  %-6.3f\n",fextim());   */
		if (ifail != 0) {
			printf("MICADO correction completed with error code %d\n\n", ifail);
			warning("MICADO back with error", ", no correction done");
		}
		if (ifail == 0) {
			pro_correct2_write_results(monvec, resvec, corvec, nx, nc, nm, imon,
					icor, ip);
		}
	}

	/* write corrector output to tfs table */
	if ((clist = command_par_string("clist", cmd->clone)) != NULL ) {
		strcat(clist1, clist);
		strcat(clist1, "_1");
		strcat(clist2, clist);
		strcat(clist2, "_2");
		out_table("corr1", corr_table1, clist1);
		out_table("corr2", corr_table2, clist2);
	}

	/* write monitor output to tfs table */
	if ((mlist = command_par_string("mlist", cmd->clone)) != NULL ) {
		out_table("mon", mon_table, mlist);
	}

	/* Clean up at the end of the module */
	myfree(rout_name, nm);
	myfree(rout_name, dmat);
	myfree(rout_name, nx);
	myfree(rout_name, nc);
	myfree(rout_name, corvec);
	myfree(rout_name, monvec);
	myfree(rout_name, resvec);myfree(rout_name, conm);
}

static int pro_correct2_gettables(int iplane, struct in_cmd* cmd) {
	const char *rout_name = "pro_correct2_gettables";

	struct id_mic2 *cor_l1, *cor_l2;
	struct id_mic2 *mon_l1, *mon_l2;
	struct id_mic2 *cor_l12, *mon_l12;
	struct id_mic2 *prt;

	// struct table *ttb; // not used

	struct table *b1 = NULL;
	struct table *b2 = NULL;

	char* orbtab1;
	char* orbtab2;

	int t1, t2;
	int ebl1, ebl2;

	int j, k;

	int cntm1 = 0;
	int cntc1 = 0;
	int cntm2 = 0;
	int cntc2 = 0;
	int cntm12 = 0;
	int cntc12 = 0;

	double ounits;

	/*
	 static char atm[6][4] = {"hmon","vmon","moni","hkic","vkic","kick"};
	 */

	/* Get access to tables, for orbit and model the default is twiss_table */
// Jun 25, 2013 3:28:07 PM ghislain : FIXME - beam1tab and beam2tab not documented
	if ((orbtab1 = command_par_string("beam1tab", cmd->clone)) != NULL ) {
		printf("Want to use orbit from: %s\n", orbtab1);
		if ((t1 = name_list_pos(orbtab1, table_register->names)) > -1) {
			b1 = table_register->tables[t1];
		} else {
			fatal_error("Beam 1 ORBIT table requested, but not provided:",
					orbtab1);
		}
	} else {
		// Jun 25, 2013 2:41:26 PM ghislain : FIXME - ??? empty else statement
    }

	if ((orbtab2 = command_par_string("beam2tab", cmd->clone)) != NULL ) {
		printf("Want to use orbit from: %s\n", orbtab2);
		if ((t2 = name_list_pos(orbtab2, table_register->names)) > -1) {
			b2 = table_register->tables[t2];
		} else {
			fatal_error("Beam 2 ORBIT table requested, but not provided:",
					orbtab2);
		}
	} else {
		// Jun 25, 2013 2:41:01 PM ghislain : FIXME - ??? empty else statement
	}

	/* store as globals for later use */
	if ((b1 != NULL) && (b2 != NULL)){
		twiss_table_beam1 = b1;
		twiss_table_beam2 = b2;
	} else {
		fatal_error("Beam 1 and 2 orbit tables not found:",orbtab1);
	}

	/* reserve space for orbit correction structures */
	if (correct_orbit12 == NULL )
		correct_orbit12 =
				mycalloc("pro_correct2_gettables", 1, sizeof *correct_orbit12);

	if (correct_orbit12->cor_table != NULL )
		myfree(rout_name, correct_orbit12->cor_table);
	if (correct_orbit12->mon_table != NULL )
		myfree(rout_name, correct_orbit12->mon_table);

	correct_orbit12->cor_table =
			mycalloc("pro_correct2_gettables_cor",5200, sizeof *correct_orbit12->cor_table);
	correct_orbit12->mon_table =
			mycalloc("pro_correct2_gettables_mon",5200, sizeof *correct_orbit12->mon_table);

	/* orbit table available, get units, if defined */
	if ((ounits = command_par_value("units", cmd->clone)) > 0)
		correct_orbit12->units = ounits;
	else
		correct_orbit12->units = 1.0;

	// ttb = model_table; // not used
	/* no more need, we have b1 and b2 as pointers .. */

	correct_orbit12->mon_table->previous = NULL;
	correct_orbit12->mon_table->next = NULL;
	correct_orbit12->cor_table->previous = NULL;
	correct_orbit12->cor_table->next = NULL;

	mon_l1 = correct_orbit12->mon_table;
	cor_l1 = correct_orbit12->cor_table;

	for (j = 0; j < b1->curr; j++) {
		if ((strncmp(atm[iplane - 1], b1->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atm[2], b1->p_nodes[j]->base_name, 4) == 0)) {
			/*    printf("1m: %s %ld\n", b1->p_nodes[j]->name, strstr(".b2", b1->p_nodes[j]->name)); */
			if (strstr(b1->p_nodes[j]->name, ".b1") != NULL ) {
				mon_l1->id_ttb[0] = j;
				mon_l1->id_ttb[1] = -1;
				mon_l1->enable = b1->p_nodes[j]->enable;
				mon_l1->p_node = b1->p_nodes[j];
				mon_l1->next = mon_l1;
				mon_l1->next++;
				mon_l1++;
				cntm1++;
			} else {
				/*      printf("Removed: %s\n",b1->p_nodes[j]->name); */
			}
		}
		if ((strncmp(atc[iplane - 1], b1->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atc[2], b1->p_nodes[j]->base_name, 4) == 0)) {
			/*    printf("1c: %s %ld\n", b1->p_nodes[j]->name, b1->p_nodes[j]->name); */
			if (strstr(b1->p_nodes[j]->name, ".b1") != NULL ) {
				cor_l1->id_ttb[0] = j;
				cor_l1->id_ttb[1] = -1;
				cor_l1->enable = b1->p_nodes[j]->enable;
				cor_l1->p_node = b1->p_nodes[j];
				cor_l1->p_node_s1 = b1->p_nodes[j];
				cor_l1->p_node_s2 = NULL;
				// 2013-Jun-24  12:09:17  ghislain: corzero option of correct command is not documented!!!
				if (command_par_value("corzero", cmd->clone) > 0) {
					if (iplane == 1)
						cor_l1->p_node_s1->chkick = 0.0;
					if (iplane == 2)
						cor_l1->p_node_s1->cvkick = 0.0;
				}
				cor_l1->next = cor_l1;
				cor_l1->next++;
				cor_l1++;
				cntc1++;
			} else {
				/*      printf("Removed: %s\n",b1->p_nodes[j]->name); */
			}
		}
	}

	mon_l2 = mon_l1;
	cor_l2 = cor_l1;
	for (j = 0; j < b2->curr; j++) {
		if ((strncmp(atm[iplane - 1], b2->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atm[2], b2->p_nodes[j]->base_name, 4) == 0)) {
			/*    printf("2m: %s %ld\n", b2->p_nodes[j]->name, b2->p_nodes[j]->name); */
			if (strstr(b2->p_nodes[j]->name, ".b2") != NULL ) {
				mon_l2->id_ttb[0] = -1;
				mon_l2->id_ttb[1] = j;
				mon_l2->enable = b2->p_nodes[j]->enable;
				mon_l2->p_node = b2->p_nodes[j];
				mon_l2->next = mon_l2;
				mon_l2->next++;
				mon_l2++;
				cntm2++;
			} else {
				/*      printf("Removed: %s\n",b2->p_nodes[j]->name); */
			}
		}
		if ((strncmp(atc[iplane - 1], b2->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atc[2], b2->p_nodes[j]->base_name, 4) == 0)) {
			/*    printf("2c: %s %ld\n", b2->p_nodes[j]->name, b2->p_nodes[j]->name); */
			if (strstr(b2->p_nodes[j]->name, ".b2") != NULL ) {
				cor_l2->id_ttb[0] = -1;
				cor_l2->id_ttb[1] = j;
				cor_l2->enable = b2->p_nodes[j]->enable;
				cor_l2->p_node = b2->p_nodes[j];
				cor_l2->p_node_s2 = b2->p_nodes[j];
				cor_l2->p_node_s1 = NULL;
				// 2013-Jun-24  12:09:17  ghislain: corzero option of correct command is not documented!!!
				if (command_par_value("corzero", cmd->clone) > 0) {
					if (iplane == 1)
						cor_l2->p_node_s2->chkick = 0.0;
					if (iplane == 2)
						cor_l2->p_node_s2->cvkick = 0.0;
				}
				cor_l2->next = cor_l2;
				cor_l2->next++;
				cor_l2++;
				cntc2++;
			} else {
				/*      printf("Removed: %s\n",b2->p_nodes[j]->name); */
			}
		}
	}

	mon_l12 = mon_l2;
	cor_l12 = cor_l2;
	for (j = 0; j < b1->curr; j++) {
		if ((strncmp(atm[iplane - 1], b1->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atm[2], b1->p_nodes[j]->base_name, 4) == 0)) {
			/*    printf("12m: %s \n", b1->p_nodes[j]->name); */
			if ((strstr(b1->p_nodes[j]->name, ".b1") == NULL )&&
					(strstr(b1->p_nodes[j]->name,".b2") == NULL)){
				mon_l12->id_ttb[0] = j;
				for (k=0; k < b2->curr; k++) {
					if(strcmp(b2->p_nodes[k]->name,b1->p_nodes[j]->name) == 0) {
						mon_l12->id_ttb[1] = k;
					}
				}
				mon_l12->enable = b1->p_nodes[j]->enable;
				mon_l12->p_node = b1->p_nodes[j];
				mon_l12->next = mon_l12;
				mon_l12->next++; mon_l12++;
				cntm12++;
			} else {
				/*      printf("Removed: %s\n",b1->p_nodes[j]->name); */
			}
		}
		if((strncmp(atc[iplane-1],b1->p_nodes[j]->base_name,4) == 0) ||
				(strncmp(atc[2], b1->p_nodes[j]->base_name,4) == 0)) {
			/*    printf("12c: %s \n", b1->p_nodes[j]->name);     */
			if((strstr(b1->p_nodes[j]->name,".b1") == NULL) &&
					(strstr(b1->p_nodes[j]->name,".b2") == NULL)) {
				cor_l12->id_ttb[0] = j;
				for (k=0; k < b2->curr; k++) {
					if(strcmp(b2->p_nodes[k]->name,b1->p_nodes[j]->name) == 0) {
						cor_l12->id_ttb[1] = k;
					}
				}
				cor_l12->p_node = b1->p_nodes[j];
				cor_l12->p_node_s1 = b1->p_nodes[cor_l12->id_ttb[0]];
				cor_l12->p_node_s2 = b2->p_nodes[cor_l12->id_ttb[1]];
				ebl1 = b1->p_nodes[cor_l12->id_ttb[0]]->enable;
				ebl2 = b2->p_nodes[cor_l12->id_ttb[1]]->enable;
				cor_l12->enable = ebl1*ebl2;
				// 2013-Jun-24  12:09:17  ghislain: corzero option of correct command is not documented!!!
				if(command_par_value("corzero",cmd->clone) > 0) {
					if(iplane == 1) cor_l12->p_node_s1->chkick = 0.0;
					if(iplane == 2) cor_l12->p_node_s1->cvkick = 0.0;
					if(iplane == 1) cor_l12->p_node_s2->chkick = 0.0;
					if(iplane == 2) cor_l12->p_node_s2->cvkick = 0.0;
				}
				cor_l12->next = cor_l12;
				cor_l12->next++; cor_l12++;
				cntc12++;
			} else {
				/*      printf("Removed: %s\n",b1->p_nodes[j]->name); */
			}
		}
	}
	/* terminate linked list   */
	mon_l12--;
	mon_l12->next = NULL;
	cor_l12--;
	cor_l12->next = NULL;

	printf("mons and corrs (beam 1)   : %ld %ld\n", (long int) cntm1,
			(long int) cntc1);
	printf("mons and corrs (beam 2)   : %ld %ld\n", (long int) cntm2,
			(long int) cntc2);
	printf("mons and corrs (beam 1+2) : %ld %ld\n", (long int) cntm12,
			(long int) cntc12);

	if (get_option("debug")) {
		prt = correct_orbit12->mon_table;
		while (prt != NULL ) {
			printf("Monitors beam12: %s %ld %ld\n", prt->p_node->name,
					(long int) prt->id_ttb[0], (long int) prt->id_ttb[1]);
			prt = prt->next;
		}

		prt = correct_orbit12->cor_table;
		while (prt != NULL ) {
			printf("Correctors beam12: %s %ld %ld\n", prt->p_node->name,
					(long int) prt->id_ttb[0], (long int) prt->id_ttb[1]);
			prt = prt->next;
		}
	}

	/*
	 prt = correct_orbit12->cor_table;
	 while(prt != NULL) {
	 printf("Correctors beam12: %s %ld %ld\n",prt->p_node->name,prt->id_ttb[0],prt->id_ttb[1]);
	 for (j=0; j < b2->curr; j++) {
	 if(strcmp(b2->p_nodes[j]->name,prt->p_node->name) == 0) {
	 prt->id_ttb[1] = j;
	 printf("matched correctors beam12: %s %ld %ld\n",prt->p_node->name,prt->id_ttb[0],prt->id_ttb[1]);
	 }
	 }
	 prt = prt->next;

	 }
	 */

	if (corr_table1 == NULL ) {
		corr_table1 = make_table("corr1", "corr1", corr_table_cols,
				corr_table_types, 15000);
		add_to_table_list(corr_table1, table_register);
	}
	if (corr_table2 == NULL ) {
		corr_table2 = make_table("corr2", "corr2", corr_table_cols,
				corr_table_types, 15000);
		add_to_table_list(corr_table2, table_register);
	}
	pro_correct2_make_corr_table();

	if (mon_table == NULL ) {
		mon_table = make_table("mon", "mon", mon_table_cols, mon_table_types,
				15000);
		add_to_table_list(mon_table, table_register);
		pro_correct2_make_mon_table();
	}

	return 10000 * (cntm1 + cntm2 + cntm12) + cntc1 + cntc2 + cntc12;
}

static int pro_correct2_getorbit(struct in_cmd* cmd) {
	struct name_list* nl;
	int i;
	int pos;

	struct id_mic2 *m; /* access to tables for monitors and correctors */
	double **da1;
	double **da2;
	double xlimit;

	char strx[40];
	char stry[40];

	int posx, posy, pospx, pospy;

	da1 = twiss_table_beam1->d_cols;
	da2 = twiss_table_beam2->d_cols;

	nl = cmd->clone->par_names;

	m = correct_orbit12->mon_table;

	strcpy(strx, "x");
	strcpy(stry, "y");

	if ((posx = name_list_pos(strx, twiss_table_beam1->columns)) < 0) {
		fatal_error("orbit x not found in input table", ", MAD-X terminates ");
	}
	if ((posy = name_list_pos(stry, twiss_table_beam1->columns)) < 0) {
		fatal_error("orbit y not found in input table", ", MAD-X terminates ");
	}
	if (get_option("debug")) {
		if ((pospx = name_list_pos("px", twiss_table_beam1->columns)) < 0) {
			warning("orbit px not found in input table", ", MAD-X continues ");
		}
		if ((pospy = name_list_pos("py", twiss_table_beam1->columns)) < 0) {
			warning("orbit py not found in input table", ", MAD-X continues ");
		}
		printf("====c1===>  %d %d %d %d \n", posx, posy, pospx, pospy);
	}

	while (m) {

		/* If correction to target orbit, subtract the wanted orbit ... */
		if (m->id_ttb[0] > 0) {
			m->val.before[0] = m->p_node->other_bv * da1[9][m->id_ttb[0]];
			m->val.before[1] = m->p_node->other_bv * da1[11][m->id_ttb[0]];
			m->val.before[0] = m->p_node->other_bv * da1[9][m->id_ttb[0]]
					* 1000.;
			m->val.before[1] = m->p_node->other_bv * da1[11][m->id_ttb[0]]
					* 1000.;
		} else if (m->id_ttb[1] > 0) {
			m->val.before[0] = m->p_node->other_bv * da2[9][m->id_ttb[1]];
			m->val.before[1] = m->p_node->other_bv * da2[11][m->id_ttb[1]];
			m->val.before[0] = m->p_node->other_bv * da2[9][m->id_ttb[1]]
					* 1000.;
			m->val.before[1] = m->p_node->other_bv * da2[11][m->id_ttb[1]]
					* 1000.;
		} else {
			printf("BIG SHIT .... \n");
			exit(-10);
		}

		pos = name_list_pos("monon", nl);
		if (nl->inform[pos] > 0) {
			xlimit = command_par_value("monon", cmd->clone);
			if (frndm() > xlimit) {
				m->enable = 0;
				printf("Monitor %s disabled\n", m->p_node->name);
			}
		}
		if (get_option("debug")) {
			printf("m-list: %d %d %s %s\n", m->id_ttb[0], m->id_ttb[1],
					m->p_node->name, m->p_node->base_name);
			printf("initial reading: %e %e\n\n", m->val.before[0],
					m->val.before[1]);
		}
		/*
		 */
		m = m->next;
	};
	i = 0;
	return (i);
}

static int pro_correct2_getcorrs(struct in_cmd* cmd) {
	int debug;
	struct id_mic2 *c; /* access to tables for monitors and correctors */

	(void) cmd;

	debug = get_option("debug");

	c = correct_orbit12->cor_table;
	while (c) {
		if (c->id_ttb[0] > 0) {
			c->val.before[0] = c->p_node_s1->chkick * 1000.;
			c->val.before[1] = c->p_node_s1->cvkick * 1000.;
		} else if (c->id_ttb[1] > 0) {
			c->val.before[0] = c->p_node_s2->chkick * 1000.;
			c->val.before[1] = c->p_node_s2->cvkick * 1000.;
		}

		if (debug) {
			printf("c-list: %d %d %s %s\n", c->id_ttb[0], c->id_ttb[1],
					c->p_node->name, c->p_node->base_name);
			printf("initial strengths: %e %e\n", c->val.before[0],
					c->val.before[1]);
		}

		c = c->next;
	};

	return (0);
}

static int pro_correct2_getactive(int ip, int *nm, int *nx, int *nc,
		double *corvec, double *monvec, char *conm) {
	int imon, icor;
	int imona, icora;
	struct id_mic2 *m, *c;

	m = correct_orbit12->mon_table;
	imon = 0;
	imona = 0;
	while (m) {
		if (get_option("debug")) {
			printf("from list: %d %d %s %s\n", m->id_ttb[0], m->id_ttb[1],
					m->p_node->name, m->p_node->base_name);
			printf("orbit readings: %d %f %f\n", ip, m->val.before[0],
					m->val.before[1]);
		}
		if (m->enable == 1) {
			monvec[imon] = m->val.before[ip - 1];
			nm[imon] = imona;
			imon++;
		}
		imona++;
		m = m->next;
	};

	c = correct_orbit12->cor_table;
	icor = 0;
	icora = 0;
	while (c) {
		if (get_option("debug")) {
			printf("from list: %d %d %d %s %s\n", c->enable, c->id_ttb[0],
					c->id_ttb[1], c->p_node->name, c->p_node->base_name);
			printf("kicker readings: %f %f\n", c->val.before[0],
					c->val.before[1]);
		}
		if (c->enable == 1) {
			corvec[icor] = c->val.before[ip - 1];
			nx[icor] = icora;
			nc[icor] = icora;
			strcpy(conm, c->p_node->name);
			conm += 16;
			/*          printf("nc: %d %d \n",icor,nc[icor]); */
			icor++;
		}
		icora++;
		c = c->next;
	};
	return (10000 * imon + icor);
}

static double* pro_correct2_response_ring(int ip, int nc, int nm) {
	int ic, im;
	struct id_mic2 *m, *c; /* access to tables for monitors and correctors */

	double **da1;
	double **da2;
	double bx_c, by_c, pix_c, piy_c;
	double bx_m, by_m, pix_m, piy_m;
	double qx0, qy0;
	double respx1, respy1;
	double respx, respy;
	double *dmat;
	int *imat;
	int mp;
	int i_zero, i_one;
	int icb;
	int i, j;

	int debug;

	debug = get_option("debug");

	setbuf(stdout, (char *) 0);

	ic = 0;
	im = 0;
	i_zero = 0;
	i_one = 1;

	da1 = twiss_table_beam1->d_cols;
	da2 = twiss_table_beam2->d_cols;

	dmat = mycalloc_atomic("pro_correct2_response_ring", nc*nm, sizeof *dmat);
	imat = mycalloc_atomic("pro_correct2_response_ring", nc*nm, sizeof *imat);

	/* initialize imat: */
	for (i = 0; i < nc; i++) {
		for (j = 0; j < nm; j++) {
			setupi_(&i_zero, imat, &j, &i, &nm, &nc);
		}
	}

	c = correct_orbit12->cor_table;
	ic = 0;

	while (c) {
		if (debug)
			printf("corrector flag: %d\n", c->enable);

		if (c->enable == 1) {

			for (icb = 0; icb < 2; icb++) { // correcting for two beamsm respectively 0 and 1
				if (c->id_ttb[icb] > 0) {

					if (icb == 0) { // beam1
						correct_orbit12->qx0 = da1[5][twiss_table_beam1->curr
								- 1];
						correct_orbit12->qy0 = da1[8][twiss_table_beam1->curr
								- 1];
						qx0 = correct_orbit12->qx0;
						qy0 = correct_orbit12->qy0;
						if (c->id_ttb[icb] > 0) {
							bx_c = da1[3][c->id_ttb[icb]];
							by_c = da1[6][c->id_ttb[icb]];
							pix_c = da1[5][c->id_ttb[icb]];
							piy_c = da1[8][c->id_ttb[icb]];
						} else {
							bx_c = 0.0;
							by_c = 0.0;
							pix_c = 0.0;
							piy_c = 0.0;
						}
					} else { // beam2
						correct_orbit12->qx0 = da2[5][twiss_table_beam2->curr
								- 1];
						correct_orbit12->qy0 = da2[8][twiss_table_beam2->curr
								- 1];
						qx0 = correct_orbit12->qx0;
						qy0 = correct_orbit12->qy0;
						if (c->id_ttb[icb] > 0) {
							bx_c = da2[3][c->id_ttb[icb]];
							by_c = da2[6][c->id_ttb[icb]];
							pix_c = da2[5][c->id_ttb[icb]];
							piy_c = da2[8][c->id_ttb[icb]];
						} else {
							bx_c = 0.0;
							by_c = 0.0;
							pix_c = 0.0;
							piy_c = 0.0;
						}
					}

					m = correct_orbit12->mon_table;
					im = 0;
					while (m) {
						if (debug)
							printf("monitor flag: %d\n", m->enable);

						if (m->enable == 1) {
							if ((m->id_ttb[icb] > 0) && (c->id_ttb[icb] > 0)) {
								if (m->id_ttb[icb] > 0) {
									if (icb == 0) {
										mp = m->id_ttb[icb];
										bx_m = da1[3][mp];
										by_m = da1[6][mp];
										pix_m = da1[5][mp];
										piy_m = da1[8][mp];
									} else {
										mp = m->id_ttb[icb];
										bx_m = da2[3][mp];
										by_m = da2[6][mp];
										pix_m = da2[5][mp];
										piy_m = da2[8][mp];
									}
								} else {
									bx_m = 0.0;
									by_m = 0.0;
									pix_m = 0.0;
									piy_m = 0.0;
								}

								respx = 0.0;
								respy = 0.0;

								/*  print Twiss parameters ... */
								if (debug) {
									printf("%s %d %e %e %e %e -- %s %e %e %e %e\n",
											c->p_node->name, icb, bx_c, by_c,
											pix_c, piy_c, m->p_node->name, bx_m,
											by_m, pix_m, piy_m);
								}

								if (ip == 1) { // x plane
									respx1 = cos((fabs(pix_m - pix_c) * twopi) - qx0 * pi);
									respx = respx1 * sqrt(bx_m * bx_c) / (2.0 * sin(pi * qx0));
//  2013-06-25 ghislain     		 if(icb != 0) { respx = respx; }
									setup_(&respx, dmat, &im, &ic, &nm, &nc);
								} else if (ip == 2) { // y plane
									respy1 = cos((fabs(piy_m - piy_c) * twopi) - qy0 * pi);
									respy = respy1 * sqrt(by_m * by_c) / (2.0 * sin(pi * qy0));
//   2013-06-25 ghislain 			 if(icb != 0) { respy = respy; }
									setup_(&respy, dmat, &im, &ic, &nm, &nc);
								}
								if ((fabs(respy) > 0.000006) || (fabs(respx) > 0.000006)) {
									if (debug) printf("true %d %d", ic, im);
									setupi_(&i_one, imat, &im, &ic, &nm, &nc);
								} else {
									if (debug) printf("false ");
									setupi_(&i_zero, imat, &im, &ic, &nm, &nc);
								}
								if (debug) printf("Response:  %d %d %e %e %e \n",
										ic, im, respx, respy, fabs(respy));
							}
							im++;
						}
						m = m->next;
					}
				}
			}
			ic++;
		}
		c = c->next;
	}

	if (debug) {
		primat_(imat, &nm, &nc);
		prdmat_(dmat, &nm, &nc);
		printf("\n");
		printf("\n");
	}

	myfree("pro_correct2_response_ring", imat);

	return dmat;
}

static void pro_correct2_write_results(double *monvec, double *resvec,
		double *corvec, int *nx, int *nc, int *nm, int imon, int icor, int ip) {
	/*                                              */
	/* Writes a summary of the correction           */
	/* Writes correctors strengths into sequences   */
	/* Fills TFS tables for correctors and monitors */
	/* Fills the 'stren.out' output                 */
	/* Makes various prints on request              */
	/*                                              */
	int i;
	int rst;
	double corrm;
	struct id_mic2 *m, *c; /* access to tables for monitors and correctors */

	m = correct_orbit12->mon_table;
	c = correct_orbit12->cor_table;

	if (fddata != NULL ) {
		rst = get_variable("n");
		fprintf(fddata, "%d %d %e %e %e %e %e %e\n", ip, rst,
				cprp(monvec, imon), cprp(resvec, imon), crms(monvec, imon),
				crms(resvec, imon), copk(monvec, imon), copk(resvec, imon));
	}

	if (print_correct_opt > 0) {
		printf("CORRECTION SUMMARY:   \n\n");
		printf("rms before correction: %f mm\nrms after correction:  %f mm\n\n",
				crms(monvec, imon), crms(resvec, imon));
		printf("ptp before correction: %f mm\nptp after correction:  %f mm\n\n",
				cprp(monvec, imon), cprp(resvec, imon));
	}

	if (print_correct_opt > 1) {
		printf("Monitor:  Before:     After:    Difference:\n");
		printf("           (mm)        (mm)         (mm)   \n");
	}

	for (i = 0; i < imon; i++) {
		if (print_correct_opt > 1) {
			printf("%s   %-4.3f     %-4.3f     %-4.3f\n", m[nm[i]].p_node->name,
					monvec[i], resvec[i], resvec[i] - monvec[i]);
		}
		m[nm[i]].val.after[ip - 1] = resvec[i];
		pro_correct2_fill_mon_table(ip, m[nm[i]].p_node->name, monvec[i],
				resvec[i]);
	}

	corrm = copk(corvec, icor);

	printf("Max strength: %e should be less than %e\n", corrm, corrl);
	if (corrm > corrl) {
		printf(
				"++++++ warning: maximum corrector strength larger than limit\n");
	}
	set_variable("corrmax", &corrm);

	if (print_correct_opt > 1) {
		printf("Max strength: %e\n", copk(corvec, icor));
		printf("Corrector:  Before:     After:    Difference:\n");
		printf("             (mrad)     (mrad)       (mrad)  \n");
	}

	for (i = 0; i < icor; i++) { /* loop over all correctors */

		c[nc[i]].val.after[ip - 1] = corvec[nx[i] - 1];
		if (print_correct_opt > 1) {
			printf("%s %-3.6f %-3.6f %-3.6f\n", c[nc[i]].p_node->name,
					c[nc[i]].val.before[ip - 1],
					corvec[nx[i] - 1] + c[nc[i]].val.before[ip - 1],
					corvec[nx[i] - 1]);
		}

		if (ip == 1) {
			/* Fill horizontal corrections for beam 1  */
			if (c[nc[i]].id_ttb[0] > 0) {
				c[nc[i]].p_node_s1->chkick += c[nc[i]].p_node_s1->other_bv
						* 0.001 * corvec[nx[i] - 1];
				pro_correct2_fill_corr_table(0, ip, c[nc[i]].p_node->name,
						c[nc[i]].val.before[ip - 1] * 0.001,
						c[nc[i]].p_node_s1->chkick);
/* ???                                    c[nc[i]].p_node_s1->other_bv*0.001*corvec[nx[i]-1]); */
				if (fcdata != NULL ) {
					fprintf(fcdata, "[1] %s = %e;\n", c[nc[i]].p_node->name,
							c[nc[i]].p_node_s1->other_bv * 0.001
									* corvec[nx[i] - 1]);
				}
			}
			/* Fill horizontal corrections for beam 2  */
			if (c[nc[i]].id_ttb[1] > 0) {
				c[nc[i]].p_node_s2->chkick += 0.001 * corvec[nx[i] - 1];
				pro_correct2_fill_corr_table(1, ip, c[nc[i]].p_node->name,
						c[nc[i]].val.before[ip - 1] * 0.001,
						c[nc[i]].p_node_s2->chkick);
/* ???                                   c[nc[i]].p_node_s2->other_bv*0.001*corvec[nx[i]-1]); */
				if (fcdata != NULL ) {
					fprintf(fcdata, "[2] %s = %e;\n", c[nc[i]].p_node->name,
							0.001 * corvec[nx[i] - 1]);
				}
			}

		} else if (ip == 2) {
			/* Fill vertical corrections for beam 1  */
			if (c[nc[i]].id_ttb[0] > 0) {
				c[nc[i]].p_node_s1->cvkick += c[nc[i]].p_node_s1->other_bv
						* 0.001 * corvec[nx[i] - 1];
				pro_correct2_fill_corr_table(0, ip, c[nc[i]].p_node->name,
						c[nc[i]].val.before[ip - 1] * 0.001,
						c[nc[i]].p_node_s1->cvkick);
/* ???                                   c[nc[i]].p_node_s1->other_bv*0.001*corvec[nx[i]-1]); */
				if (fcdata != NULL ) {
					fprintf(fcdata, "[1] %s = %e;\n", c[nc[i]].p_node->name,
							c[nc[i]].p_node_s1->other_bv * 0.001
									* corvec[nx[i] - 1]);
				}
			}
			if (c[nc[i]].id_ttb[1] > 0) {
				/* Fill vertical corrections for beam 2  */
				c[nc[i]].p_node_s2->cvkick += 0.001 * corvec[nx[i] - 1];
				pro_correct2_fill_corr_table(1, ip, c[nc[i]].p_node->name,
						c[nc[i]].val.before[ip - 1] * 0.001,
						c[nc[i]].p_node_s2->cvkick);
/* ???                                    c[nc[i]].p_node_s2->other_bv*0.001*corvec[nx[i]-1]); */
				if (fcdata != NULL ) {
					fprintf(fcdata, "[2] %s = %e;\n", c[nc[i]].p_node->name,
							0.001 * corvec[nx[i] - 1]);
				}
			}
		}

	} /* end loop over correctors */
}

static void correct_correct1(struct in_cmd* cmd)
/* Steering routine for orbit corrections of one beam */
{
	const char *rout_name = "correct_correct";
	int ix, im, ip, idrop; // , it not used
	int j, nnnseq; // ,err not used
	int imon, icor;
	int ncorr, nmon;
	int niter;

	int twism;
	int dbg;
	int ifail, sflag; // , svdflg; // not used
	float rms;
	double sngcut, sngval;
	double tmp1, tmp2, tmp3, tmp4;
	double sigcut; /* number of sigmas (normalized) for filter cut */
	char *clist, *mlist; /* file names for monitor and corrector output */
	double *dmat = { NULL }; /* response matrix, double precision */
	double *corvec, *monvec; /* vectors to hold measured orbit and correctors */
	double *resvec; /* vector to hold corrected orbit */
	char *conm; /* vector to hold corrector names (for MICADO) */
	int *sing; /* array to store pointer to singular correctors */
	static int *nm, *nx, *nc;
	struct id_mic *corl;

	int debug;

	debug = get_option("debug");


	/* If only Twiss summary is required prepare and write it */
	// Jun 26, 2013 8:06:33 PM ghislain : moved up from **twiss summary**
	if ((twism = command_par_value("twissum", cmd->clone)) > 0) {
	  if (ftdata == NULL ) {
	    if ((ftdata = fopen("twiss.summ", "w")) == NULL )
	      exit(99);
	  }
	  j = 1;
	  if ((nnnseq = get_variable("n")) == 0) {
	    nnnseq = twism;
	  }
	  double_from_table_row("summ", "xcomax", &j, &tmp1); // err = not used
	  double_from_table_row("summ", "xcorms", &j, &tmp2); // err = not used
	  double_from_table_row("summ", "ycomax", &j, &tmp3); // err = not used
	  double_from_table_row("summ", "ycorms", &j, &tmp4); // err = not used
	  fprintf(ftdata, " T: %d %e %e %e %e\n", nnnseq, tmp1, tmp2, tmp3, tmp4);
	  printf("TWISSUM: Data from twiss summary written to twiss.summ; aborting correction\n");
	  return; // abort the correction here
	}

	ip = pro_correct_getcommands(cmd);
	im = pro_correct_gettables(ip, cmd);
	ncorr = im % 10000;
	nmon = im / 10000;
	printf("%d monitors and %d correctors found in input\n", nmon, ncorr);

	if (nmon == 0) {
	  printf("No monitor found in input, no correction done\n");
	  return;
	}

	if (ncorr == 0) {
	  printf("No corrector found in input, no correction done\n");
	  return;
	}

	/* For debugging set output buffer to zero */
	if (debug) setbuf(stdout, NULL );

	/* Prepare file descriptors for the output */
	if (command_par_value("resout", cmd->clone) > 0) {
	  if (fddata == NULL ) {
	    if ((fddata = fopen("corr.out", "w")) == NULL )
	      exit(99);
	  }
	  if (fcdata == NULL ) {
	    if ((fcdata = fopen("stren.out", "w")) == NULL )
	      exit(99);
	  }
	}

	// Jun 26, 2013 8:07:01 PM ghislain : **twiss summary** was here

	/* allocate vectors used by correction algorithms */
	nx = mycalloc("correct_correct_nx",ncorr,sizeof(int));
	nc = mycalloc("correct_correct_nc",ncorr,sizeof(int));
	nm = mycalloc("correct_correct_nm",nmon,sizeof(int));
	sing = mycalloc("correct_correct_sing",ncorr*2,sizeof(int));
	corvec = mycalloc("correct_correct_corvec",ncorr,sizeof(double));
	monvec = mycalloc("correct_correct_monvec",nmon,sizeof(double));
	resvec = mycalloc("correct_correct_resvec",nmon,sizeof(double));
	conm = mycalloc("correct_correct_conm",ncorr*16,sizeof(char));

	/* get original settings of correctors from input Twiss-table */
	pro_correct_getcorrs(cmd); // it = not used

	/* get input orbit, default is from input Twiss-table */
	/* if flag "extern" is true: can be from external table */
	if (command_par_value("extern", cmd->clone))
	  pro_correct_getorbit_ext(cmd); // it = not used
	else
	  pro_correct_getorbit(cmd); // it = not used

	/* find and prepare enabled correctors and monitors, may be repeated */
	ix = pro_correct_getactive(ip, nm, nx, nc, corvec, monvec, conm);
	icor = ix % 10000;
	imon = ix / 10000;
	printf("%d monitors and %d correctors enabled\n", imon, icor);

	/* normalized cut on beam position, if requested */
	if ((sigcut = command_par_value("moncut", cmd->clone)) > 0) {
	  idrop = pro_correct_filter(ip, sigcut);
	  printf("Disabled %d monitors with %-2.2f sigma cut\n", idrop, sigcut);
	  ix = pro_correct_getactive(ip, nm, nx, nc, corvec, monvec, conm);
	  icor = ix % 10000;
	  imon = ix / 10000;
	  printf("After filter of %-2.2f sigma:\n", sigcut);
	  printf("%d monitors and %d correctors enabled\n", imon, icor);
	}

	/* set up response matrix for ring or line */
	corl = correct_orbit->cor_table;

	if (strcmp("ring", command_par_string("flag", cmd->clone)) == 0) {
	  if (dmat != NULL ) myfree(rout_name, dmat);
	  /* icor and imon used to set up correct matrix size !! */
	  dmat = pro_correct_response_ring(ip, icor, imon);
	  
	  if (command_par_value("cond", cmd->clone) == 1) { // (svdflg = not used
	    sngcut = command_par_value("sngcut", cmd->clone);
	    sngval = command_par_value("sngval", cmd->clone);
	    printf("SVD conditioning requested ...\n");
	    if (debug) printf("Conditioning parameters: %e %e\n", sngcut, sngval);
	    
	    /* printf("Time before svd-comd:  %-6.3f\n",fextim());    */
	    sflag = c_svddec(dmat, imon, icor, sing, &sngcut, &sngval);
	    printf("Initially found %d singular values\n", sflag);
	    /* printf("Time after svd-cond:  %-6.3f\n",fextim());     */
	    
	    for (ix = 0; ix < sflag; ix++) {
	      corl[nx[sing[2 * ix + 0]]].enable = 0;
	      if (dbg == 1)
		printf("Removed:   %d %s\n", nx[sing[2 * ix + 0]],
		       corl[nx[sing[2 * ix + 0]]].p_node->name);
	    }
	    
	    ix = pro_correct_getactive(ip, nm, nx, nc, corvec, monvec, conm);
	    icor = ix % 10000;
	    imon = ix / 10000;
	    
	    printf("After SVD conditioning:             \n");
	    printf("%d monitors and %d correctors enabled\n\n", imon, icor);
	    
	    if (dmat != NULL ) myfree(rout_name, dmat);
	    
	    /* icor and imon used to set up correct matrix size !! */
	    dmat = pro_correct_response_ring(ip, icor, imon);
	    sflag = c_svddec(dmat, imon, icor, sing, &sngcut, &sngval);
	    printf("Finally found %d singular values\n", sflag);
	  }

	} else if (strcmp("line", command_par_string("flag", cmd->clone)) == 0) {
	  if (dmat != NULL ) myfree(rout_name, dmat);
	  printf("make response for line\n");
	  dmat = pro_correct_response_line(ip, icor, imon);
	  
	  if (command_par_value("cond", cmd->clone) == 1) { // (svdflg = not used
	    sngcut = command_par_value("sngcut", cmd->clone);
	    sngval = command_par_value("sngval", cmd->clone);
	    printf("SVD conditioning requested ...\n");
	    if (debug)
	      printf("Conditioning parameters: %e %e\n", sngcut, sngval);
	    
	    /* printf("Time before svd-comd:  %-6.3f\n",fextim());    */
	    sflag = c_svddec(dmat, imon, icor, sing, &sngcut, &sngval);
	    printf("Initially found %d singular values\n", sflag);
	    /* printf("Time after svd-cond:  %-6.3f\n",fextim());     */
	    /* printf("sflag: %d\n",sflag); */
	    for (ix = 0; ix < sflag; ix++) {
	      corl[nx[sing[2 * ix + 0]]].enable = 0;
	      if (dbg == 1)
		printf("Removed:   %d %s\n", nx[sing[2 * ix + 0]],
		       corl[nx[sing[2 * ix + 0]]].p_node->name);
	      
	    }
	    ix = pro_correct_getactive(ip, nm, nx, nc, corvec, monvec, conm);
	    icor = ix % 10000;
	    imon = ix / 10000;
	    printf("After SVD conditioning:             \n");
	    printf("%d monitors and %d correctors enabled\n\n", imon, icor);
	    if (dmat != NULL )
	      myfree(rout_name, dmat);
	    /* icor and imon used to set up correct matrix size !! */
	    dmat = pro_correct_response_ring(ip, icor, imon);
	    sflag = c_svddec(dmat, imon, icor, sing, &sngcut, &sngval);
	    printf("Finally found %d singular values\n", sflag);
	  }
	}

	else { // neither ring nor line
	   printf("INVALID MACHINE TYPE\n");
	   exit(-1);
	}

	if (debug) {
	   pro_correct_prtwiss();
	   pro_correct_write_cocu_table();
	}


	/*  2013-Jul-17  19:41:55  ghislain: moved up from in between the SVD and MICADO corrections */
	corrl = command_par_value("corrlim", cmd->clone);
	set_variable("corrlim", &corrl);


	/* Switch block between LSQ, SVD and MICADO correction methods... */

	/* LSQ correction, use all available correctors */
	if (strcmp("lsq", command_par_string("mode", cmd->clone)) == 0) {
	   /*frs haveit_(dmat,monvec,corvec,resvec,nx,&imon,&icor); */
	   // if (debug) printf("Time before lsq:  %-6.3f\n",fextim());
	   c_haveit(dmat, monvec, corvec, resvec, nx, imon, icor);
	   // if (debug) printf("Time after lsq:  %-6.3f\n",fextim());
	   pro_correct_write_results(monvec, resvec, corvec, nx, nc, nm, imon,
				     icor, ip);
	}

	/* SVD correction, use all available correctors */
	else if (strcmp("svd", command_par_string("mode", cmd->clone)) == 0) {
	   /*frs haveit_(dmat,monvec,corvec,resvec,nx,&imon,&icor); */
	   // if (debug) printf("Time before svd-corr:  %-6.3f\n",fextim());
	   sflag = c_svdcorr(dmat, monvec, corvec, resvec, nx, imon, icor);
	   // if (debug) printf("Time after svd-corr:  %-6.3f\n",fextim());
	   pro_correct_write_results(monvec, resvec, corvec, nx, nc, nm, imon,
				     icor, ip);
	}

	/*  2013-Jul-17  19:41:55  ghislain: why is this appearing here ? it should be above 
	    before LSQ/SVD/MICADO selection block. Moved... */
	/* corrl = command_par_value("corrlim", cmd->clone); */
	/* set_variable("corrlim", &corrl); */

	/* MICADO correction, get desired number of correctors from command */
	else if (strcmp("micado", command_par_string("mode", cmd->clone)) == 0) {
	   printf("enter MICADO correction ...\n");

	   niter = command_par_value("ncorr", cmd->clone);
	   if (niter == 0) {
	      printf("Requested %d correctors (\?\?\?) set to %d\n", niter, icor);
	      niter = icor;
	   } else if (niter < 0) {
	      printf("Requested %d correctors (\?\?\?) set to 0\n", niter);
	      niter = 0;
	   } else if (niter > icor) {
	      printf("Fewer correctors available than requested by ncorr\n");
	      printf("you want %d,  you get %d\n", niter, icor);
	      printf("ncorr reset to %d\n", icor);
	      niter = icor;
	   }

	   // 2013-Jun-24  10:57:43  ghislain: why the multiplication by 1000 ?
	   rms = 1000.0 * command_par_value("error", cmd->clone);

	   /*frs       micit_(dmat,monvec,corvec,resvec,nx,&rms,&imon,&icor,&niter); */
	   // if (debug) printf("Time before micado:  %-6.3f\n",fextim());
	   ifail = c_micit(dmat, conm, monvec, corvec, resvec, nx, rms, imon, icor, niter);
	   // if (debug) printf("Time after micado:  %-6.3f\n",fextim());

	   if (ifail == 0)
	      pro_correct_write_results(monvec, resvec, corvec, nx, nc, nm, imon, icor, ip);
	   else {
	      printf("MICADO correction completed with error code %d\n\n", ifail);
	      warning("MICADO back with error", ", no correction done");
	   }
	}
	else { // neither ring nor line
	  printf("INVALID CORRECTION TYPE\n");
	  exit(-1);
	}

	/* write corrector output to tfs table */
	if ((clist = command_par_string("clist", cmd->clone)) != NULL )
		out_table("corr", corr_table, clist);

	/* write monitor output to tfs table */
	if ((mlist = command_par_string("mlist", cmd->clone)) != NULL )
		out_table("mon", mon_table, mlist);

	/* Clean up at the end of the module */
	myfree(rout_name, nm);
	myfree(rout_name, dmat);
	myfree(rout_name, nx);
	myfree(rout_name, nc);
	myfree(rout_name, corvec);
	myfree(rout_name, monvec);
	myfree(rout_name, resvec);
	myfree(rout_name, conm);
	return;
}

static void correct_correct(struct in_cmd* cmd)
/* Steering routine for orbit corrections */
{
	/*
	 const char *rout_name = "correct_correct";
	 */
	char *orbtab1, *orbtab2;

	/* Call for one or two ring orbit correction */
	// Jun 25, 2013 2:51:50 PM ghislain : FIXME - This option is not documented
	if (command_par_value("tworing", cmd->clone)) {
		printf("Want to correct orbit for two rings\n");

		// the following tests only whether a parameter was supplied;
	    // the validity of the parameter is tested in pro_correct2_gettables
		if ((orbtab1 = command_par_string("beam1tab", cmd->clone)) == NULL )
			fatal_error("Two beam correction requested but no table supplied for beam 1", orbtab1);
		if ((orbtab2 = command_par_string("beam2tab", cmd->clone)) == NULL )
			fatal_error("Two beam correction requested but no table supplied for beam 2", orbtab2);

		printf("Want to use orbits from: %s and : %s\n", orbtab1, orbtab2);

		correct_correct2(cmd);

	} else {
		printf("Want to correct orbit of a single ring\n");

		if ((orbtab1 = command_par_string("beam1tab", cmd->clone)) != NULL ) {
			warning(" ", " ");
			warning("Single beam correction requested but beam 1 table supplied:", orbtab1);
			warning("Specified table ignored:", orbtab1);
			warning(" ", " ");
		}

		if ((orbtab2 = command_par_string("beam2tab", cmd->clone)) != NULL ) {
			warning(" ", " ");
			warning("Single beam correction requested but beam 2 table supplied:", orbtab2);
			warning("Specified table ignored:", orbtab2);
			warning(" ", " ");
		}

		correct_correct1(cmd);
	}
}

#if 0 // not used...
static void
pro_correct_option(struct in_cmd* cmd)
{
	struct name_list* nl = cmd->clone->par_names;
	int i, debug;
	int val, pos, seed;

	if ((debug=get_option("debug"))) {
		fprintf(prt_file, "in coption routine\n");
		for(i=0;i<cmd->tok_list->curr;i++) {
			fprintf(prt_file, "command(s): %s\n",cmd->tok_list->p[i]);
		}
	}
	if ((pos = name_list_pos("seed", nl)) > -1)
	{
		if (nl->inform[pos])
		{
			seed = command_par_value("seed", cmd->clone);
			init55(seed);
		}
	}
	val = command_par_value("print", cmd->clone);
	if(val == 0) {
		if (debug) fprintf(prt_file, "print option not set\n");
		print_correct_opt = 0;
	} else {
		if (debug) fprintf(prt_file, "print option set\n");
		print_correct_opt = val;
	}
}
#endif

static int pro_correct_getcommands(struct in_cmd* cmd) {

	static char att[10][8] = { "iterate", "plane", "ncorr", "error", "clist",
			"mlist", "flag", "mode", "", "" };

	static int iplane = 1;
	char plane[20];

	if (get_option("debug"))
		printf("enter CORRECT module\n");

	if (current_sequ == NULL || current_sequ->ex_start == NULL ) {
		warning("CORRECT, but no active sequence:", "ignored");
		return (-1);
	}

	// Jun 25, 2013 3:29:19 PM ghislain : FIXME - documentation only mentions x and y; add h and v
	strcpy(plane, command_par_string(att[1], cmd->clone));
	if (strcmp("x", plane) == 0) {
		iplane = 1;
	} else if (strcmp("y", plane) == 0) {
		iplane = 2;
	} else if (strcmp("h", plane) == 0) {
		iplane = 1;
	} else if (strcmp("v", plane) == 0) {
		iplane = 2;
	} else {
		printf("No valid plane specified, x plane used \n");
		iplane = 1;
	}

	return (iplane);
}

static int pro_correct_gettables(int iplane, struct in_cmd* cmd) {

	const char *rout_name = "pro_correct_gettables";

	struct id_mic *cor_l;
	struct id_mic *mon_l;

	struct table *ttb;

	char* orbtab;
	char* tartab;
	char* modtab;

	int j;
	int pps, ppt;

	int cntm = { 0 };
	int cntc = { 0 };

	double ounits;

	/*
	 static char atm[6][4] = {"hmon","vmon","moni","hkic","vkic","kick"};
	 */

	int corzero;

	int debug;

	debug = get_option("debug");

	/* Get access to tables, for orbit and model the default is twiss_table */

	if ((orbtab = command_par_string("orbit", cmd->clone)) != NULL ) {
		printf("Want to use orbit from: %s\n", orbtab);
		if ((pps = name_list_pos(orbtab, table_register->names)) > -1) {
			orbin_table = table_register->tables[pps];
		} else {
			fatal_error(
					"ORBIT table for correction requested, but not provided:",
					orbtab);
		}
	} else { // the orbit table is the twiss table
		if ((orbin_table = twiss_table) == NULL ) {
			printf("FATAL ERROR:\n");
			printf("You request the ORBIT from a non-existing TWISS table\n");
			printf("You MUST run TWISS before trying to correct the orbit\n");
			printf("MAD-X stops\n");
			exit(81);
		} else {
			if (debug)
				printf("orbit from TWISS table at address: %p\n",
						(void*) twiss_table);
		}
		// 2013-Jun-24  12:00:18  ghislain: commented out; never used.
		// pps = -1;
	}

	if ((tartab = command_par_string("target", cmd->clone)) != NULL ) {
		printf("Want to use target orbit from: %s\n", tartab);
		if ((ppt = name_list_pos(tartab, table_register->names)) > -1) {
			target_table = table_register->tables[ppt];
		} else {
			fatal_error(
					"TARGET table for correction requested, but not provided:",
					tartab);
		}
	} else {
		if (debug)
			printf("No target orbit requested\n");
	}
	// 2013-Jun-24  12:00:18  ghislain: commented out; never used.
	// ppt = -1;

	if ((modtab = command_par_string("model", cmd->clone)) != NULL ) {
		printf("Want to use model orbit from: %s\n", modtab);
		if ((ppt = name_list_pos(modtab, table_register->names)) > -1) {
			model_table = table_register->tables[ppt];
		} else {
			fatal_error(
					"MODEL table for correction requested, but not provided:",
					modtab);
		}
	} else {
		if ((model_table = twiss_table) == NULL ) {
			printf("FATAL ERROR:\n");
			printf("You request the MODEL from a non-existing TWISS table\n");
			printf("You MUST run TWISS before trying to correct the orbit\n");
			printf("MAD-X stops\n");
			exit(81);
		} else {
			if (debug)
				printf("model from TWISS table at address: %p\n",
						(void*) twiss_table);
		}
		// 2013-Jun-24  12:00:18  ghislain: commented out; never used.
		// ppt = -1;
	}

	if (debug)
		printf(
				"The orbit, twiss, target and model tables are at addresses: %p %p %p %p\n",
				(void*) orbin_table, (void*) twiss_table, (void*) target_table,
				(void*) model_table);

	if (correct_orbit == NULL )
		correct_orbit =
				mycalloc("pro_correct_gettables", 1, sizeof *correct_orbit);

	if (debug)
		printf("-0-\n");
	/*    if(corr_table == NULL) {   */
	corr_table = make_table("corr", "corr", corr_table_cols, corr_table_types,
			5000);
	add_to_table_list(corr_table, table_register);
	pro_correct_make_corr_table();
	/* } */

	if (debug)
		printf("-1-\n");
	/*    if(mon_table == NULL) { */
	mon_table = make_table("mon", "mon", mon_table_cols, mon_table_types, 5000);
	add_to_table_list(mon_table, table_register);
	pro_correct_make_mon_table();
	/*    }                       */

	if (debug)
		printf("-2-\n");

	if (correct_orbit->cor_table != NULL )
		myfree(rout_name, correct_orbit->cor_table);
	if (correct_orbit->mon_table != NULL )
		myfree(rout_name, correct_orbit->mon_table);
	correct_orbit->cor_table =
			mycalloc("pro_correct_gettables_cor", 5200, sizeof *correct_orbit->cor_table);
	correct_orbit->mon_table =
			mycalloc("pro_correct_gettables_mon", 5200, sizeof *correct_orbit->mon_table);

	/* orbit table available, get units, if defined */
	//  2013-Jun-24  12:06:03  ghislain: FIXME - units option of correct command is not documented!!!
	if ((ounits = command_par_value("units", cmd->clone)) > 0) {
		correct_orbit->units = ounits;
	} else {
		correct_orbit->units = 1.0;
	}

	ttb = model_table;
	correct_orbit->mon_table->previous = NULL;
	correct_orbit->mon_table->next = NULL;
	correct_orbit->cor_table->previous = NULL;
	correct_orbit->cor_table->next = NULL;

	if (debug)
		printf("-3-\n");

	mon_l = correct_orbit->mon_table;
	cor_l = correct_orbit->cor_table;

	// 2013-Jun-24  12:09:17  ghislain: FIXME corzero option of correct command is not documented!!!
	corzero = command_par_value("corzero", cmd->clone);

	// go through the model table and build chained lists of monitors and correctors
	for (j = 0; j < ttb->curr; j++) {
		if ((strncmp(atm[iplane - 1], ttb->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atm[2], ttb->p_nodes[j]->base_name, 4) == 0)) {
			mon_l->id_ttb = j;
			mon_l->enable = ttb->p_nodes[j]->enable;
			mon_l->p_node = ttb->p_nodes[j];
			mon_l->next = mon_l;
			mon_l->next++;
			mon_l++;
			cntm++;
		}
		if ((strncmp(atc[iplane - 1], ttb->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atc[2], ttb->p_nodes[j]->base_name, 4) == 0)) {
			cor_l->id_ttb = j;
			cor_l->enable = ttb->p_nodes[j]->enable;
			cor_l->p_node = ttb->p_nodes[j];

			if (corzero > 0) {
				if (iplane == 1)
					cor_l->p_node->chkick = 0.0;
				if (iplane == 2)
					cor_l->p_node->cvkick = 0.0;
			}

			cor_l->next = cor_l;
			cor_l->next++;
			cor_l++;
			cntc++;
		}
	}

	if (debug)
		printf("-4-\n");

	mon_l--;
	mon_l->next = NULL;
	cor_l--;
	cor_l->next = NULL;

	if (debug)
		printf("done: %d %d\n", cntm, cntc);

	// 2013-Jun-24  12:21:49  ghislain: following is a kludge to return a single value but has to be decoded on other side.  // this should at the very least check that cntc < 10000

	/* if(cntc >= 10000) { */
	/*     printf("FATAL ERROR:\n"); */
	/*     printf("found more than 10000 correctors; decoding in mad_orbit.c will fail\n"); */
	/*     printf("report this issue to MAD developpers\n"); */
	/*     printf("MAD-X stops\n"); */
	/*     exit(81); */
	/* } */

	return (10000 * cntm + cntc);
}

static int pro_correct_getorbit(struct in_cmd* cmd) {
	struct name_list* nl;
	int i;

	int pos;
	struct id_mic *m; /* access to tables for monitors and correctors */
	struct table *ttb;
	struct table *tar = NULL;
	double **da1;
	double **da2 = NULL;
	double xlimit;
	double rx, ry, dpsi;

	char strx[40];
	char stry[40];

	int posx, posy, pospx, pospy;
	int tosx = -1;
	int tosy = -1;
	int tospx, tospy;

	int debug;

	debug = get_option("debug");

	ttb = orbin_table;
	da1 = ttb->d_cols;

	if (target_table != NULL ) {
		tar = target_table;
		da2 = tar->d_cols;
	}

	nl = cmd->clone->par_names;

	m = correct_orbit->mon_table;

	strcpy(strx, "x");
	strcpy(stry, "y");

	if ((posx = name_list_pos(strx, ttb->columns)) < 0) {
		fatal_error("orbit x not found in input table", ", MAD-X terminates ");
	}
	if ((posy = name_list_pos(stry, ttb->columns)) < 0) {
		fatal_error("orbit y not found in input table", ", MAD-X terminates ");
	}
	if (debug) {
		if ((pospx = name_list_pos("px", ttb->columns)) < 0) {
			fatal_error("orbit px not found in input table",
					", MAD-X terminates ");
		}
		if ((pospy = name_list_pos("py", ttb->columns)) < 0) {
			fatal_error("orbit py not found in input table",
					", MAD-X terminates ");
		}
		printf("====c1===>  %d %d %d %d \n", posx, posy, pospx, pospy);
	}

	if (command_par_string("target", cmd->clone) != NULL ) {
		if ((tosx = name_list_pos("x", tar->columns)) < 0) {
			fatal_error("target orbit x not found in table",
					", MAD-X terminates ");
		}
		if ((tosy = name_list_pos("y", tar->columns)) < 0) {
			fatal_error("target orbit y not found in table",
					", MAD-X terminates ");
		}
		if (get_option("debug")) {
			if ((tospx = name_list_pos("px", tar->columns)) < 0) {
				fatal_error("target orbit px not found in table",
						", MAD-X terminates ");
			}
			if ((tospy = name_list_pos("py", tar->columns)) < 0) {
				fatal_error("target orbit px not found in table",
						", MAD-X terminates ");
			}
			printf("====c1===>  %d %d %d %d \n", tosx, tosy, tospx, tospy);
		}
	}

	while (m) {

		/* If correction to target orbit, subtract the wanted orbit ... */
		if (command_par_string("target", cmd->clone) != NULL ) {
			m->val.before[0] = da1[posx][m->id_ttb] - da2[tosx][m->id_ttb];
			m->val.before[1] = da1[posy][m->id_ttb] - da2[tosy][m->id_ttb];
			m->val.before[0] = (da1[posx][m->id_ttb] - da2[tosx][m->id_ttb])
					* 1000. * correct_orbit->units;
			m->val.before[1] = (da1[posy][m->id_ttb] - da2[tosy][m->id_ttb])
					* 1000. * correct_orbit->units;
		} else {
			m->val.before[0] = da1[posx][m->id_ttb];
			m->val.before[1] = da1[posy][m->id_ttb];
			m->val.before[0] = da1[posx][m->id_ttb] * 1000.
					* correct_orbit->units;
			m->val.before[1] = da1[posy][m->id_ttb] * 1000.
					* correct_orbit->units;
		}

		pos = name_list_pos("monon", nl);
		if (nl->inform[pos] > 0) {
			xlimit = command_par_value("monon", cmd->clone);
			if (frndm() > xlimit) {
				m->enable = 0;
				printf("Monitor %s disabled\n", m->p_node->name);
			}
		}

		/* scaling error should come first, monitor alignment not scaled ... */
		pos = name_list_pos("monscale", nl);
		if (nl->inform[pos] > 0) {
			if ((command_par_value("monscale", cmd->clone)) == 1) {
				if (m->p_node->p_al_err != NULL ) {
					if (get_option("debug")) {
						printf("m-list: %d %s %s\n", m->id_ttb, m->p_node->name,
								m->p_node->base_name);
						printf("scales: %e %e\n", m->p_node->p_al_err->a[12],
								m->p_node->p_al_err->a[13]);
					}
					m->val.before[0] = m->val.before[0]
							* (1.0 + m->p_node->p_al_err->a[12]);
					m->val.before[1] = m->val.before[1]
							* (1.0 + m->p_node->p_al_err->a[13]);
				}
			}
		}

		/* monitor misalignment after all other reading manipulations ! */
		pos = name_list_pos("monerror", nl);
		if (nl->inform[pos] > 0) {
			if ((command_par_value("monerror", cmd->clone)) == 1) {
				if (m->p_node->p_al_err != NULL ) {
					if (get_option("debug")) {
						printf("m-list: %d %s %s\n", m->id_ttb, m->p_node->name,
								m->p_node->base_name);
						printf("errors: %e %e \n", m->p_node->p_al_err->a[6],
								m->p_node->p_al_err->a[7]);
					}
					dpsi = m->p_node->p_al_err->a[5];
					rx = m->val.before[0];
					ry = m->val.before[1];
					printf("\nA: %e %e %e\n", m->val.before[0],
							m->val.before[1], dpsi);
					m->val.before[0] = rx * cos(dpsi) + ry * sin(dpsi);
					m->val.before[1] = -rx * sin(dpsi) + ry * cos(dpsi);
					printf("B: %e %e %e\n", m->val.before[0], m->val.before[1],
							dpsi);
					m->val.before[0] += m->p_node->p_al_err->a[6] * 1000.;
					m->val.before[1] += m->p_node->p_al_err->a[7] * 1000.;
					printf("C: %e %e %e\n", m->val.before[0], m->val.before[1],
							dpsi);
				}
			}
		}
		m = m->next;
	};
	i = 0;
	return (i);
}

static int pro_correct_getorbit_ext(struct in_cmd* cmd) {
	struct name_list* nl;
	int i;
	int j;

	int pos;
	struct id_mic *m; /* access to tables for monitors and correctors */
	struct table *ttb;
	struct table *tar = NULL;
	double **da1;
	double **da2 = NULL;
	double xlimit;
	char name[NAME_L];
	char l1name[NAME_L];
	char l2name[NAME_L];
	char l3name[NAME_L];
	char l4name[NAME_L];
	double rx, ry, dpsi;

	char *nam_col;
	char *x_col;
	char *y_col;

	char strx[40];
	char stry[40];
	char strn[40];

	int posx, posy, pospx, pospy;
	int tosx = -1;
	int tosy = -1;
	int tospx, tospy;

	int yok;

	int jjx, jjy, jj;

	int debug;

	debug = get_option("debug");

	ttb = orbin_table;
	da1 = ttb->d_cols;

	if (target_table != NULL ) {
		tar = target_table;
		da2 = tar->d_cols;
	}

	nl = cmd->clone->par_names;

	m = correct_orbit->mon_table;

	if ((x_col = command_par_string("x_col", cmd->clone)) != NULL ) {
		printf("X orbit in column: %s\n", x_col);
		strcpy(strx, x_col);
	} else {
		strcpy(strx, "x");
	}
	if ((y_col = command_par_string("y_col", cmd->clone)) != NULL ) {
		printf("y orbit in column: %s\n", y_col);
		strcpy(stry, y_col);
	} else {
		strcpy(stry, "y");
	}
	if ((nam_col = command_par_string("name_col", cmd->clone)) != NULL ) {
		printf("names in column: %s\n", nam_col);
		strcpy(strn, "name");
	} else {
		strcpy(strn, "name");
	}

	if ((posx = name_list_pos(strx, ttb->columns)) < 0) {
		fatal_error("orbit x not found in input table", ", MAD-X terminates ");
	}
	if ((posy = name_list_pos(stry, ttb->columns)) < 0) {
		fatal_error("orbit y not found in input table", ", MAD-X terminates ");
	}
	if (debug) {
		if ((pospx = name_list_pos("px", ttb->columns)) < 0) {
			warning("orbit px not found in input table", ", MAD-X continues ");
		}
		if ((pospy = name_list_pos("py", ttb->columns)) < 0) {
			warning("orbit py not found in input table", ", MAD-X continues ");
		}
		printf("====c1===>  %d %d %d %d \n", posx, posy, pospx, pospy);
	}

	if (command_par_string("target", cmd->clone) != NULL ) {
		if ((tosx = name_list_pos("x", tar->columns)) < 0) {
			fatal_error("target orbit x not found in table",
					", MAD-X terminates ");
		}
		if ((tosy = name_list_pos("y", tar->columns)) < 0) {
			fatal_error("target orbit y not found in table",
					", MAD-X terminates ");
		}
		if (debug) {
			if ((tospx = name_list_pos("px", tar->columns)) < 0) {
				warning("target orbit px not found in table",
						", MAD-X continues ");
			}
			if ((tospy = name_list_pos("py", tar->columns)) < 0) {
				warning("target orbit px not found in table",
						", MAD-X continues ");
			}
			printf("====c1===>  %d %d %d %d \n", tosx, tosy, tospx, tospy);
		}
	}

	if (debug) {
		printf("Number in table: %d\n", ttb->curr);

		// 2013-Jun-24  13:42:16  ghislain: ????
		// for (j=1; j < (ttb->curr)+1; j++) {
		//  i = string_from_table_row(ttb->name, "name", &j, name);
		// }

	}

	jj = 0;

	while (m) {
		strcpy(l1name, m->p_node->name);
		stolower(l1name);
		strcpy(l2name, strip(l1name));
		supp_tb(l2name);

		if (debug)
			printf("monitor name: %s\n", l2name);

		jjx = -1;
		jjy = -1;
		jj++;
		yok = 0;

		for (j = 1; j < (ttb->curr) + 1; j++) {
			i = string_from_table_row(ttb->name, "name", &j, name);
			strcpy(l3name, name);
			stolower(l3name);
			strcpy(l4name, strip(l3name));
			supp_tb(l4name);
			if (strlen(l4name) == strlen(l2name)) {
				if (strncmp(l4name, l2name, strlen(l2name)) == 0) {
					jjx = j - 1;
					jjy = jj - 1;
					yok = 1;
					if (debug)
						printf("monitor names found: %s %s %d\n", l2name,
								l4name, yok);
				}
			}
		}
		if (debug)
			printf("jjx,jjy %d %d\n", jjx, jjy);

		/* If correction to target orbit, subtract the wanted orbit ... */

		if ((jjy >= 0) && (yok == 1)) {
			/*  if(jjx >= 0)  {  */
			if (command_par_string("target", cmd->clone) != NULL ) {

				if (debug) {
					printf("x ==> %d %d %e %e\n", jjx, m->id_ttb,
							da1[posx][jjx], da2[tosx][jjy]);
					printf("y ==> %e %e\n", da1[posy][jjx], da2[tosy][jjy]);
				}

				m->val.before[0] = da1[posx][jjx] - da2[tosx][jjy];
				m->val.before[1] = da1[posy][jjx] - da2[tosy][jjy];
				m->val.before[0] = (da1[posx][jjx] - da2[tosx][jjy]) * 1000.
						* correct_orbit->units;
				m->val.before[1] = (da1[posy][jjx] - da2[tosy][jjy]) * 1000.
						* correct_orbit->units;

				if (debug) {
					printf("bxy ==> %s %d %e %e\n", m->p_node->name, jjx,
							m->val.before[0], m->val.before[1]);
				}

			} else {

				if (debug) {
					printf("x ==> %e %e\n", da1[posx][jjx], da2[tosx][jjx]);
					printf("y ==> %e %e\n", da1[posy][jjx], da2[tosy][jjx]);
				}

				m->val.before[0] = da1[posx][jjx];
				m->val.before[1] = da1[posy][jjx];
				m->val.before[0] = da1[posx][jjx] * 1000.
						* correct_orbit->units;
				m->val.before[1] = da1[posy][jjx] * 1000.
						* correct_orbit->units;

				if (debug) {
					printf("bxy ==> %s %d %e %e\n", m->p_node->name, jjx,
							m->val.before[0], m->val.before[1]);
				}

			}

			pos = name_list_pos("monon", nl);
			if (nl->inform[pos] > 0) {
				xlimit = command_par_value("monon", cmd->clone);
				if (frndm() > xlimit) {
					m->enable = 0;
					printf("Monitor %s disabled\n", m->p_node->name);
				}
			}

			/* scaling error should come first, monitor alignment not scaled ... */
			pos = name_list_pos("monscale", nl);
			if (nl->inform[pos] > 0) {
				if ((command_par_value("monscale", cmd->clone)) == 1) {
					if (m->p_node->p_al_err != NULL ) {

						if (debug) {
							printf("m-list: %d %s %s\n", m->id_ttb,
									m->p_node->name, m->p_node->base_name);
							printf("scales: %e %e\n",
									m->p_node->p_al_err->a[12],
									m->p_node->p_al_err->a[13]);
						}

						m->val.before[0] = m->val.before[0]
								* (1.0 + m->p_node->p_al_err->a[12]);
						m->val.before[1] = m->val.before[1]
								* (1.0 + m->p_node->p_al_err->a[13]);
					}
				}
			}

			/* monitor misalignment after all other reading manipulations ! */
			pos = name_list_pos("monerror", nl);
			if (nl->inform[pos] > 0) {
				if ((command_par_value("monerror", cmd->clone)) == 1) {
					if (m->p_node->p_al_err != NULL ) {

						if (debug) {
							printf("m-list: %d %s %s\n", m->id_ttb,
									m->p_node->name, m->p_node->base_name);
							printf("errors: %e %e \n",
									m->p_node->p_al_err->a[6],
									m->p_node->p_al_err->a[7]);
						}

						dpsi = m->p_node->p_al_err->a[5];
						rx = m->val.before[0];
						ry = m->val.before[1];

						if (debug)
							printf("\nA: %e %e %e\n", m->val.before[0],
									m->val.before[1], dpsi);

						m->val.before[0] = rx * cos(dpsi) + ry * sin(dpsi);
						m->val.before[1] = -rx * sin(dpsi) + ry * cos(dpsi);

						if (debug)
							printf("B: %e %e %e\n", m->val.before[0],
									m->val.before[1], dpsi);

						m->val.before[0] += m->p_node->p_al_err->a[6] * 1000.;
						m->val.before[1] += m->p_node->p_al_err->a[7] * 1000.;

						if (debug)
							printf("C: %e %e %e\n", m->val.before[0],
									m->val.before[1], dpsi);
					}
				}
			}
		} else {
			m->enable = 0; /* Only enable monitors found in input */
		}

		m = m->next;
	}

	return 0;
}

static int pro_correct_getcorrs(struct in_cmd* cmd) {
	int debug;
	struct id_mic *c; /* access to tables for monitors and correctors */

	(void) cmd;

	debug = get_option("debug");

	c = correct_orbit->cor_table;
	while (c) {
		c->val.before[0] = c->p_node->chkick * 1000.;
		c->val.before[1] = c->p_node->cvkick * 1000.;

		if (debug) {
			printf("c-list: %d %s %s\n", c->id_ttb, c->p_node->name,
					c->p_node->base_name);
			printf("initial strengths: %e %e\n", c->val.before[0],
					c->val.before[1]);
		}

		c = c->next;
	};

	return (0);
}

static void pro_correct_prtwiss(void) {
	int i, j;
	int pr_cols;
	struct table *ttb;
	double **da1;

	ttb = model_table;

	printf(" %d %d\n", ttb->curr, ttb->num_cols);
	for (i = 0; i < ttb->curr; i++) {

		printf(" %s %s\n", ttb->s_cols[0][i], ttb->s_cols[1][i]);
	}

	da1 = ttb->d_cols;
	for (j = 0; j < ttb->curr; j++) {
		printf("\n\n");
		printf("from table: %s \n", ttb->node_nm->p[j]);
		printf("from node:  %s \n", ttb->p_nodes[j]->name);
		printf(" %s %s\n", ttb->s_cols[0][j], ttb->s_cols[1][j]);

		pr_cols = ttb->num_cols;
		pr_cols = 19; /* print only for 20 columns */
		for (i = 0; i < pr_cols; i++) {
			if (&da1[i][0] != NULL ) {
				printf("%-8s %f\n", twiss_table_cols[i], da1[i][j]);
			}
		}
	}
	return;
}

static void pro_correct_write_cocu_table(void) {
	int i, j;
	int pr_cols;
	int cp[13] = { 1, 0, 2, 9, 11, 3, 6, 4, 7, 5, 8, 15, 17 };
	struct table *ttb;
	double **da1;
	FILE *fp1;

	fp1 = fopen("cocu_in.opt", "w");
	ttb = model_table;

	pr_cols = ttb->num_cols;
	pr_cols = 13; /* print only for 19 columns */
	fprintf(fp1, "*");
	for (i = 0; i < pr_cols; i++) {
		fprintf(fp1, "%-8s ", twiss_table_cols[cp[i]]);
	}

	da1 = ttb->d_cols;
	for (j = 0; j < ttb->curr; j++) {
		fprintf(fp1, "\n%s %s ", ttb->s_cols[1][j], ttb->s_cols[0][j]);
		for (i = 2; i < pr_cols; i++) {
			if (&da1[cp[i]][0] != NULL ) {
				fprintf(fp1, " %f", da1[cp[i]][j]);
			}
		}
	}
	return;
}

static int pro_correct_filter(int iplane, double sigcut) {
	int im, ip, icnt; // ic, no used
	struct id_mic *m; /* access to tables for monitors */

	struct table *ttb;
	static char pl[2] = "xy";
	double **da1;
	double bx_m = -9999.;
	double xsig;
	double xmea; //, ymea; not used
	double xn;

	ttb = model_table;
	da1 = ttb->d_cols;
	im = 0;
	icnt = 0; // ic = 0; // not used
	ip = iplane - 1;

	printf("A (normalized) cut of %-2.2f is requested\n", sigcut);

	m = correct_orbit->mon_table;
	xmea = 0.0; // ymea = 0.0; // not used
	while (m) {
		if (get_option("debug")) {
			printf("monitor flag: %d\n", m->enable);
		}
		if (m->enable == 1) {
			if (ip == 0) {
				bx_m = da1[3][m->id_ttb];
			} else if (ip == 1) {
				bx_m = da1[6][m->id_ttb];
			}
			xn = m->val.before[ip] / sqrt(bx_m);
			xmea += xn;
			if (get_option("debug")) {
				printf("==> %s %-4.3f %-4.3f \n", m->p_node->name, bx_m,
						m->val.before[ip]);
				printf("==> %-4.3f \n", xn);
			}
			im++;
		}
		m = m->next;
	};
	xmea = xmea / im;
	if (get_option("debug")) {
		printf("Mean values: %-4.3f \n", xmea);
	}
	m = correct_orbit->mon_table;
	im = 0;
	xsig = 0.0;
	while (m) {
		if (m->enable == 1) {
			if (ip == 0) {
				bx_m = da1[3][m->id_ttb];
			} else if (ip == 1) {
				bx_m = da1[6][m->id_ttb];
			}
			xn = m->val.before[ip] / sqrt(bx_m);
			xsig += (xmea - xn) * (xmea - xn);
			im++;
		}
		m = m->next;
	};
	xsig = sqrt(xsig / im);
	if (get_option("debug")) {
		printf("Sigma values: %-4.3f \n", xsig);
	}

	m = correct_orbit->mon_table;
	while (m) {
		if (m->enable == 1) {
			if (ip == 0) {
				bx_m = da1[3][m->id_ttb];
			} else if (ip == 1) {
				bx_m = da1[6][m->id_ttb];
			}
			xn = (m->val.before[ip] / sqrt(bx_m)) - xmea;
			if (fabs(xn) > (sigcut * xsig)) {
				printf(
						"disabled %s %c = %-4.3f (%-4.3f), limit is %-2.2f*%-4.3f\n",
						m->p_node->name, pl[ip], xn, m->val.before[ip], sigcut,
						xsig);
				m->enable = 0;
				icnt++;
			}
		}
		m = m->next;
	};

	return (icnt);
}

static double*
pro_correct_response_ring(int ip, int nc, int nm) {
	int ic, im;
	struct id_mic *m, *c; /* access to tables for monitors and correctors */

	struct table *ttb;
	double **da1;
	double bx_c, by_c, pix_c, piy_c;
	double bx_m, by_m, pix_m, piy_m;
	double qx0, qy0;
	double respx1, respy1;
	double respx, respy;
	double *dmat;

	int debug;

	debug = get_option("debug");

	ttb = model_table;
	da1 = ttb->d_cols;
	ic = 0;
	im = 0;

	dmat = mycalloc_atomic("pro_correct_response_ring", nc*nm, sizeof *dmat);

	correct_orbit->qx0 = da1[5][ttb->curr - 1];
	correct_orbit->qy0 = da1[8][ttb->curr - 1];
	qx0 = correct_orbit->qx0;
	qy0 = correct_orbit->qy0;

	c = correct_orbit->cor_table;
	ic = 0;

	while (c) {
		if (debug)
			printf("corrector flag: %d\n", c->enable);

		if (c->enable == 1) {
			bx_c = da1[3][c->id_ttb];
			by_c = da1[6][c->id_ttb];
			pix_c = da1[5][c->id_ttb];
			piy_c = da1[8][c->id_ttb];
			m = correct_orbit->mon_table;
			im = 0;
			while (m) {
				if (debug)
					printf("monitor flag: %d\n", m->enable);
				if (m->enable == 1) {
					bx_m = da1[3][m->id_ttb];
					by_m = da1[6][m->id_ttb];
					pix_m = da1[5][m->id_ttb];
					piy_m = da1[8][m->id_ttb];
					respx = 0.0;
					respy = 0.0;
					if (ip == 1) {
						respx1 = cos((fabs(pix_m - pix_c) * twopi) - qx0 * pi);
						respx = respx1 * sqrt(bx_m * bx_c)
								/ (2.0 * sin(pi * qx0));
						setup_(&respx, dmat, &im, &ic, &nm, &nc);
					} else if (ip == 2) {
						respy1 = cos((fabs(piy_m - piy_c) * twopi) - qy0 * pi);
						respy = respy1 * sqrt(by_m * by_c)
								/ (2.0 * sin(pi * qy0));
						setup_(&respy, dmat, &im, &ic, &nm, &nc);
					}
					im++;
				}
				m = m->next;
			};
			ic++;
		}
		c = c->next;
	}

	return dmat;
}

static double*
pro_correct_response_line(int ip, int nc, int nm) {
	int ic, im;
	struct id_mic *m, *c; /* access to tables for monitors and correctors */

	struct table *ttb;
	double **da1;
	double bx_c, by_c, pix_c, piy_c;
	double bx_m, by_m, pix_m, piy_m;
	double respx1, respy1;
	double respx, respy;
	double *dmat;

	ttb = model_table;
	da1 = ttb->d_cols;
	ic = 0;
	im = 0;

	dmat = mycalloc_atomic("pro_correct_response_ring", nc*nm, sizeof *dmat);

	correct_orbit->qx0 = da1[5][ttb->curr - 1];
	correct_orbit->qy0 = da1[8][ttb->curr - 1];

	c = correct_orbit->cor_table;
	ic = 0;
	while (c) {
		if (c->enable == 1) {
			bx_c = da1[3][c->id_ttb];
			by_c = da1[6][c->id_ttb];
			pix_c = da1[5][c->id_ttb];
			piy_c = da1[8][c->id_ttb];
			m = correct_orbit->mon_table;
			im = 0;
			while (m) {
				if (m->enable == 1) {
					bx_m = da1[3][m->id_ttb];
					by_m = da1[6][m->id_ttb];
					pix_m = da1[5][m->id_ttb];
					piy_m = da1[8][m->id_ttb];
					respx = 0.0;
					respy = 0.0;
					if (ip == 1) {
						if (pix_m > pix_c) {
							respx1 = sin((pix_m - pix_c) * twopi);
							respx = respx1 * sqrt(bx_m * bx_c);
						} else {
							respx = 0.0;
						}
						setup_(&respx, dmat, &im, &ic, &nm, &nc);
					} else if (ip == 2) {
						if (piy_m > piy_c) {
							respy1 = sin((piy_m - piy_c) * twopi);
							respy = respy1 * sqrt(by_m * by_c);
						} else {
							respy = 0.0;
						}
						setup_(&respy, dmat, &im, &ic, &nm, &nc);
					}
					im++;
				}
				m = m->next;
			};
			ic++;
		}
		c = c->next;
	}

	return dmat;
}

static void pro_correct_make_corr_table(void) {
	struct table *ttb;
	int j;

	/*
	 static char atm[5][4] = {"hmon","vmon","hkic","vkic","kick"};
	 */

	/*
	 ttb = orbin_table;
	 */
	ttb = model_table;

	for (j = 0; j < ttb->curr; j++) {
		if ((strncmp(atc[0], ttb->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atc[1], ttb->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atc[2], ttb->p_nodes[j]->base_name, 4) == 0)) {
			string_to_table_curr("corr", "name", ttb->p_nodes[j]->name);
			augment_count("corr");
		}
	}
}

static void pro_correct2_make_corr_table(void) {
	struct id_mic2 *ttb;

	/*
	 static char atm[5][4] = {"hmon","vmon","hkic","vkic","kick"};
	 */

	ttb = correct_orbit12->cor_table;

	while (ttb != NULL ) {
		if ((strncmp(atc[0], ttb->p_node->base_name, 4) == 0)
				|| (strncmp(atc[1], ttb->p_node->base_name, 4) == 0)
				|| (strncmp(atc[2], ttb->p_node->base_name, 4) == 0)) {
			if (ttb->id_ttb[0] > 0) {
				string_to_table_curr("corr1", "name", ttb->p_node->name);
				augment_count("corr1");
			}

			if (ttb->id_ttb[1] > 0) {
				string_to_table_curr("corr2", "name", ttb->p_node->name);
				augment_count("corr2");
			}
		}
		ttb = ttb->next;
	}
}

static void pro_correct_make_mon_table(void) {
	struct table *ttb;
	int j;

	/*
	 static char atm[3][4] = {"hmon","vmon","moni"};
	 */

	ttb = model_table;

	for (j = 0; j < ttb->curr; j++) {
		if ((strncmp(atm[0], ttb->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atm[1], ttb->p_nodes[j]->base_name, 4) == 0)
				|| (strncmp(atm[2], ttb->p_nodes[j]->base_name, 4) == 0)) {
			string_to_table_curr("mon", "name", ttb->p_nodes[j]->name);
			augment_count("mon");
		}
	}
}

static void pro_correct2_make_mon_table(void) {
	struct id_mic2 *ttb;
	/*
	 static char atm[3][4] = {"hmon","vmon","moni"};
	 */

	ttb = correct_orbit12->mon_table;

	while (ttb != NULL ) {
		if ((strncmp(atm[0], ttb->p_node->base_name, 4) == 0)
				|| (strncmp(atm[1], ttb->p_node->base_name, 4) == 0)
				|| (strncmp(atm[2], ttb->p_node->base_name, 4) == 0)) {
			string_to_table_curr("mon", "name", ttb->p_node->name);
			augment_count("mon");
		}
		ttb = ttb->next;
	}
}

static void pro_correct_fill_corr_table(int ip, char *name, double old,
		double new) {
	struct table *cor;

	int j;

	cor = corr_table;

	for (j = 0; j < cor->curr; j++) {
		if (strcmp(name, cor->s_cols[0][j]) == 0) {
			cor->d_cols[ip][j] = old;
			cor->d_cols[ip + 2][j] = new;
		}
	}
}

static void pro_correct2_fill_corr_table(int b, int ip, char *name, double old,
		double new) {
	struct table *cor = NULL;

	int j;

	if ((b != 1) && (b != 0)) {
		char buf[64];
		sprintf(buf, "%d", b);
		fatal_error("Invalid beam requested:", buf);
	}

	if (b == 0)
		cor = corr_table1;
	if (b == 1)
		cor = corr_table2;

	for (j = 0; j < cor->curr; j++) {
		if (strcmp(name, cor->s_cols[0][j]) == 0) {
			cor->d_cols[ip][j] = old;
			cor->d_cols[ip + 2][j] = new;
		}
	}
}

static void pro_correct_fill_mon_table(int ip, char *name, double old,
		double new) {
	struct table *mon;

	int j;

	mon = mon_table;

	for (j = 0; j < mon->curr; j++) {
		if (strcmp(name, mon->s_cols[0][j]) == 0) {
			mon->d_cols[ip][j] = old * 0.001;
			mon->d_cols[ip + 2][j] = new * 0.001;
		}
	}
}

static void pro_correct2_fill_mon_table(int ip, char *name, double old,
		double new) {
	struct table *mon;

	int j;

	mon = mon_table;

	for (j = 0; j < mon->curr; j++) {
		if (strcmp(name, mon->s_cols[0][j]) == 0) {
			mon->d_cols[ip][j] = old * 0.001;
			mon->d_cols[ip + 2][j] = new * 0.001;
		}
	}
}

static void pro_correct_write_results(double *monvec, double *resvec,
		double *corvec, int *nx, int *nc, int *nm, int imon, int icor, int ip)
/*                                              */
/* Writes a summary of the correction           */
/* Writes correctors strengths into sequences   */
/* Fills TFS tables for correctors and monitors */
/* Fills the 'stren.out' output                 */
/* Makes various prints on request              */
/*                                              */
{
	int i;
	int rst;
	double corrm;
	struct id_mic *m, *c; /* access to tables for monitors and correctors */

	m = correct_orbit->mon_table;
	c = correct_orbit->cor_table;

	if (fddata != NULL ) {
	   rst = get_variable("n");
	   fprintf(fddata, "%d %d %e %e %e %e %e %e\n", ip, rst,
		   cprp(monvec, imon), cprp(resvec, imon), crms(monvec, imon),
		   crms(resvec, imon), copk(monvec, imon), copk(resvec, imon));
	}

	if (print_correct_opt > 0) {
	   printf("CORRECTION SUMMARY:   \n\n");
	   printf("rms before correction: %f mm\nrms after correction:  %f mm\n\n",
		  crms(monvec, imon), crms(resvec, imon));
	   printf("ptp before correction: %f mm\nptp after correction:  %f mm\n\n",
		  cprp(monvec, imon), cprp(resvec, imon));
	}

	if (print_correct_opt > 1) {
	   printf("Monitor:  Before:     After:    Difference:\n");
	   printf("           (mm)        (mm)         (mm)   \n");
	}

	for (i = 0; i < imon; i++) {
	   if (print_correct_opt > 1) {
	     printf("%s   %-4.3f     %-4.3f     %-4.3f\n", m[nm[i]].p_node->name,
		    monvec[i], resvec[i], resvec[i] - monvec[i]);
	   }
	   m[nm[i]].val.after[ip - 1] = resvec[i];
	   pro_correct_fill_mon_table(ip, m[nm[i]].p_node->name, monvec[i],
				      resvec[i]);
	}

	corrm = copk(corvec, icor);

	if (corrm > corrl) {
	   printf("Max strength: %e should be less than corrector strength limit: %e\n", corrm, corrl);
	   printf("++++++ warning: maximum corrector strength larger than limit\n");
	} else {
	   printf("Max strength: %e is below corrector strength limit: %e\n", corrm, corrl);
	}

	set_variable("corrmax", &corrm);
	if (print_correct_opt > 1) {
	   printf("Max strength: %e\n", copk(corvec, icor));
	   printf("Corrector:  Before:     After:    Difference:\n");
	   printf("             (mrad)     (mrad)       (mrad)  \n");
	}

	for (i = 0; i < icor; i++) { /* loop over all correctors */

	   if (print_correct_opt > 1) {
	      printf("%s %-3.6f %-3.6f %-3.6f\n", c[nc[i]].p_node->name,
		     c[nc[i]].val.before[ip - 1],
		     corvec[nx[i] - 1] + c[nc[i]].val.before[ip - 1],
		     corvec[nx[i] - 1]);
	   }

	   c[nc[i]].val.after[ip - 1] = corvec[nx[i] - 1];
	   if (ip == 1) {
	      c[nc[i]].p_node->chkick += c[nc[i]].p_node->other_bv * 0.001
		* corvec[nx[i] - 1];
	      pro_correct_fill_corr_table(ip, c[nc[i]].p_node->name,
					  c[nc[i]].val.before[ip - 1] * 0.001,
					  c[nc[i]].p_node->chkick);
	      /*                          c[nc[i]].p_node->other_bv*0.001*corvec[nx[i]-1]); */
	      if (fcdata != NULL ) {
	 	 fprintf(fcdata, "%s = %e;\n", c[nc[i]].p_node->name,
			 c[nc[i]].p_node->other_bv * 0.001 * corvec[nx[i] - 1]);
	      }
	   } else if (ip == 2) {
	      c[nc[i]].p_node->cvkick += c[nc[i]].p_node->other_bv * 0.001
		* corvec[nx[i] - 1];
	      pro_correct_fill_corr_table(ip, c[nc[i]].p_node->name,
					  c[nc[i]].val.before[ip - 1] * 0.001,
					  c[nc[i]].p_node->cvkick);
	      /*                          c[nc[i]].p_node->other_bv*0.001*corvec[nx[i]-1]); */
	      if (fcdata != NULL ) {
		 fprintf(fcdata, "%s = %e;\n", c[nc[i]].p_node->name,
			 c[nc[i]].p_node->other_bv * 0.001 * corvec[nx[i] - 1]);
	      }
	   }
	} /* end of loop ove correctors */
}

static int pro_correct_getactive(int ip, int *nm, int *nx, int *nc,
		double *corvec, double *monvec, char *conm) {
	int imon, icor;
	int imona, icora;
	struct id_mic *m, *c;

	m = correct_orbit->mon_table;
	imon = 0;
	imona = 0;
	while (m) {
		if (get_option("debug")) {
			printf("from list: %d %s %s\n", m->id_ttb, m->p_node->name,
					m->p_node->base_name);
			printf("orbit readings: %d %f %f\n", ip, m->val.before[0],
					m->val.before[1]);
		}
		if (m->enable == 1) {
			monvec[imon] = m->val.before[ip - 1];
			nm[imon] = imona;
			imon++;
		}
		imona++;
		m = m->next;
	};

	c = correct_orbit->cor_table;
	icor = 0;
	icora = 0;
	while (c) {
		if (get_option("debug")) {
			printf("from list: %d %d %s %s\n", c->enable, c->id_ttb,
					c->p_node->name, c->p_node->base_name);
			printf("kicker readings: %f %f\n", c->val.before[0],
					c->val.before[1]);
		}
		if (c->enable == 1) {
			corvec[icor] = c->val.before[ip - 1];
			nx[icor] = icora;
			nc[icor] = icora;
			strcpy(conm, c->p_node->name);
			conm += 16;
			/*          printf("nc: %d %d \n",icor,nc[icor]); */
			icor++;
		}
		icora++;
		c = c->next;
	};
	return (10000 * imon + icor);
}

static void correct_option(struct in_cmd* cmd) {
	struct name_list* nl = cmd->clone->par_names;
	int i, debug;
	int pos, seed;

	debug = get_option("debug");

	if (debug) {
		fprintf(prt_file, "in coption routine\n");
		for (i = 0; i < cmd->tok_list->curr; i++) {
			fprintf(prt_file, "command(s): %s\n", cmd->tok_list->p[i]);
		}
	}

	if ((pos = name_list_pos("seed", nl)) > -1) {
		if (nl->inform[pos]) {
			seed = command_par_value("seed", cmd->clone);
			init55(seed);
		}
	}

	print_correct_opt = command_par_value("print", cmd->clone);

	if (debug) {
		if (print_correct_opt == 0)
			fprintf(prt_file, "print option not set\n");
		else
			fprintf(prt_file, "print option set\n");
	}

	debug_correct_opt = command_par_value("debug", cmd->clone);

	if (debug) {
		if (debug_correct_opt == 0)
			fprintf(prt_file, "debug option not set\n");
		else
			fprintf(prt_file, "debug option set\n");
	}

}

static void correct_getorbit(struct in_cmd* cmd) {
	(void) cmd;
}

static void correct_putorbit(struct in_cmd* cmd) {
// Jun 25, 2013 3:33:03 PM ghislain : DOC - this option is documented as deprecated but still alive
	int i;
	struct name_list* nl;
	char* filename = command_par_string("file", cmd->clone);
	char* table_name;
	current_twiss = clone_command(find_command("twiss", defined_commands));
	nl = current_twiss->par_names;
	for (i = 0; i < nl->curr; i++)
		nl->inform[i] = 0;
	pro_twiss();
	table_name = permbuff("orbit");
	orbit_table = make_table(table_name, "orbit", orbit_table_cols,
			orbit_table_types, current_sequ->n_nodes);
	add_to_table_list(orbit_table, table_register);
	fill_orbit_table(orbit_table, orbin_table);
	out_table("orbit", orbit_table, filename);
	current_twiss = delete_command(current_twiss);
}

static void correct_usekick(struct in_cmd* cmd) {
	char temp[12];
	int count = set_enable("kicker", cmd);
	sprintf(temp, "%d", count);
	put_info(temp, "corrector(s) affected");
}

static void correct_usemonitor(struct in_cmd* cmd) {
	char temp[12];
	int count = set_enable("monitor", cmd);
	sprintf(temp, "%d", count);
	put_info(temp, "monitor(s) affected");
}

// public interface

void store_orbit(struct command* comm, double* orbit) {
	struct name_list* nl = comm->par_names;
	if (nl->inform[name_list_pos("x", nl)])
		orbit[0] = command_par_value("x", comm);
	if (nl->inform[name_list_pos("px", nl)])
		orbit[1] = command_par_value("px", comm);
	if (nl->inform[name_list_pos("y", nl)])
		orbit[2] = command_par_value("y", comm);
	if (nl->inform[name_list_pos("py", nl)])
		orbit[3] = command_par_value("py", comm);
	if (nl->inform[name_list_pos("t", nl)])
		orbit[4] = command_par_value("t", comm);
	if (nl->inform[name_list_pos("pt", nl)])
		orbit[5] = command_par_value("pt", comm);
}

void pro_correct(struct in_cmd* cmd) {
	if (strcmp(cmd->tok_list->p[0], "correct") == 0) {
		correct_correct(cmd);
	} else if (strcmp(cmd->tok_list->p[0], "usekick") == 0) {
		correct_usekick(cmd);
	} else if (strcmp(cmd->tok_list->p[0], "usemonitor") == 0) {
		correct_usemonitor(cmd);
	} else if (strcmp(cmd->tok_list->p[0], "getorbit") == 0) {
		correct_getorbit(cmd); // FIXME obsolete command; should be flagged and not call anything...
	} else if (strcmp(cmd->tok_list->p[0], "putorbit") == 0) {
		correct_putorbit(cmd); // FIXME obsolete command; should be flagged and not call anything...
	} else if (strcmp(cmd->tok_list->p[0], "readmytable") == 0) {
		read_my_table(cmd);
	} else if (strcmp(cmd->tok_list->p[0], "readcorr") == 0) {
		correct_readcorr(cmd);
	} else if (strcmp(cmd->tok_list->p[0], "setcorr") == 0) {
		correct_setcorr(cmd);
	} else if (strcmp(cmd->tok_list->p[0], "coption") == 0) {
		correct_option(cmd);
	}
}

int locf_(char *iadr)
#define NADUPW 4   /* Number of ADdress Units Per Word */
#define LADUPW 2   /* Logarithm base 2 of ADdress Units Per Word */
{
	return (uintptr_t) iadr >> LADUPW;
}

void f_ctof(int *j, char *string, int *nel) {
	long i, flg = 0;

	for (i = 0; i < *nel; i++) {
		if (flg == 1) {
			string[i] = ' ';
			continue;
		}

		if (string[i] == '\0') {
			string[i] = ' ';
			flg = 1;
			continue;
		}
	}
	*j = i;
}

